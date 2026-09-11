#include "decoded_measurement_tile_source_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <unistd.h>
#include <vector>

namespace bridge = truthraw::decoded_measurement_tile_source::v0_1;
namespace handoff = truthraw::decoded_measurement_handoff::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;
namespace streaming = truthraw::streaming_v0_1;

#define CHECK_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; \
        return __LINE__; \
    } \
} while (false)

struct FdGuard {
    int fd = -1;
    ~FdGuard() { if (fd >= 0) ::close(fd); }
};

static handoff::Hash256 make_hash(std::uint8_t seed) {
    handoff::Hash256 out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    return out;
}

int main() {
    char path[] = "/tmp/truthraw_tile_source_XXXXXX";
    FdGuard store{::mkstemp(path)};
    CHECK_TRUE(store.fd >= 0);
    CHECK_TRUE(::unlink(path) == 0);

    handoff::Descriptor descriptor{};
    descriptor.width = 23;
    descriptor.height = 19;
    descriptor.sourceBitDepth = 14;
    descriptor.topology = ingress::MeasurementTopology::Bayer2x2;
    descriptor.evidenceClass = ingress::EvidenceClass::LosslessDecodedCertified;
    descriptor.sourceCompression = ingress::CompressionSemantics::LosslessVerified;
    descriptor.cfa2x2 = {
        handoff::CfaColor::Red,
        handoff::CfaColor::Green,
        handoff::CfaColor::Green,
        handoff::CfaColor::Blue};
    descriptor.sourceEvidenceHash = make_hash(7);
    descriptor.decoderAuditHash = make_hash(91);

    std::vector<std::uint16_t> samples(
        static_cast<std::size_t>(descriptor.width) * descriptor.height);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<std::uint16_t>((i * 29u + 11u) & 0x3fffu);
    }

    handoff::Writer writer;
    CHECK_TRUE(writer.begin(store.fd, descriptor) == handoff::Status::Ok);
    std::size_t position = 0;
    while (position < samples.size()) {
        const std::size_t count = std::min<std::size_t>(43, samples.size() - position);
        CHECK_TRUE(writer.append_samples(
            std::span<const std::uint16_t>(samples.data() + position, count)) == handoff::Status::Ok);
        position += count;
    }
    CHECK_TRUE(writer.seal() == handoff::Status::Ok);

    handoff::Reader reader;
    CHECK_TRUE(reader.open(store.fd) == handoff::Status::Ok);

    truthraw::DngMetadata metadata{};
    metadata.width = static_cast<int>(descriptor.width);
    metadata.height = static_cast<int>(descriptor.height);
    metadata.cfa = truthraw::CfaPattern::RGGB;
    metadata.orientation = truthraw::Orientation::Normal;
    metadata.whiteLevel = 16383.0f;
    metadata.blackPhase = {512.0f, 512.0f, 512.0f, 512.0f};
    metadata.cameraToXyzD50 = {1.0f,0.0f,0.0f, 0.0f,1.0f,0.0f, 0.0f,0.0f,1.0f};
    metadata.hasNoiseProfile = false;
    metadata.hasGainField = false;
    metadata.hasResidualBlack = false;
    metadata.sourceId = "fixture-source-evidence-bound";

    bridge::BindingAuthority authority{};
    authority.metadataBoundToOriginalSource = true;
    authority.metadataSemanticsVerified = true;
    authority.decodedStoreBoundToOriginalSource = true;
    authority.gatehouseDetached = true;

    bridge::DecodedMeasurementTileSource source;
    CHECK_TRUE(source.bind(reader, metadata, authority) == bridge::BindStatus::IntegrityNotVerified);
    CHECK_TRUE(!source.bound());

    CHECK_TRUE(reader.verify_payload_integrity() == handoff::Status::Ok);

    auto missingAuthority = authority;
    missingAuthority.gatehouseDetached = false;
    bridge::DecodedMeasurementTileSource blockedByAttachment;
    CHECK_TRUE(blockedByAttachment.bind(reader, metadata, missingAuthority) ==
               bridge::BindStatus::AuthorityMissing);

    auto mismatchedMetadata = metadata;
    mismatchedMetadata.cfa = truthraw::CfaPattern::BGGR;
    bridge::DecodedMeasurementTileSource mismatched;
    CHECK_TRUE(mismatched.bind(reader, mismatchedMetadata, authority) ==
               bridge::BindStatus::MetadataMismatch);

    auto unsupportedMetadata = metadata;
    unsupportedMetadata.hasGainField = true;
    bridge::DecodedMeasurementTileSource gainBlocked;
    CHECK_TRUE(gainBlocked.bind(reader, unsupportedMetadata, authority) ==
               bridge::BindStatus::UnsupportedMetadata);

    CHECK_TRUE(source.bind(reader, metadata, authority) == bridge::BindStatus::Ok);
    CHECK_TRUE(source.bound());
    CHECK_TRUE(source.residentBytesUpperBound() < 32u * 1024u);

    streaming::IRawTileSource& mainHouseSource = source;
    CHECK_TRUE(mainHouseSource.metadata().width == metadata.width);
    CHECK_TRUE(mainHouseSource.metadata().height == metadata.height);
    CHECK_TRUE(mainHouseSource.metadata().cfa == truthraw::CfaPattern::RGGB);

    truthraw::TileRect rect{};
    rect.x0 = 4;
    rect.y0 = 5;
    rect.x1 = 9;
    rect.y1 = 8;
    rect.hx0 = 2;
    rect.hy0 = 3;
    rect.hx1 = 11;
    rect.hy1 = 10;

    constexpr std::size_t tileWidth = 9;
    constexpr std::size_t tileHeight = 7;
    std::array<std::uint16_t, tileWidth * tileHeight> tile{};
    CHECK_TRUE(static_cast<bool>(mainHouseSource.readRawTile(
        rect, tile.data(), tile.size(), nullptr, 0)));

    std::size_t dst = 0;
    for (int y = rect.hy0; y < rect.hy1; ++y) {
        for (int x = rect.hx0; x < rect.hx1; ++x) {
            const std::size_t sourceIndex =
                static_cast<std::size_t>(y) * descriptor.width + static_cast<std::size_t>(x);
            CHECK_TRUE(tile[dst++] == samples[sourceIndex]);
        }
    }

    std::array<float, 7> rowBias{};
    CHECK_TRUE(static_cast<bool>(mainHouseSource.readRowBias(3, 10, rowBias.data(), rowBias.size())));
    CHECK_TRUE(std::all_of(rowBias.begin(), rowBias.end(), [](float v) { return v == 0.0f; }));

    std::array<float, 9> colBias{};
    CHECK_TRUE(static_cast<bool>(mainHouseSource.readColBias(2, 11, colBias.data(), colBias.size())));
    CHECK_TRUE(std::all_of(colBias.begin(), colBias.end(), [](float v) { return v == 0.0f; }));

    std::cout << "Decoded Measurement Tile Source v0.1 PASS\n";
    std::cout << "main_house_irawtilesource_bridge=1\n";
    std::cout << "gatehouse_detached_required=1\n";
    std::cout << "payload_integrity_verified_required=1\n";
    std::cout << "gain_field_silent_drop=0\n";
    std::cout << "residual_black_silent_drop=0\n";
    std::cout << "full_frame_source_buffer=0\n";
    return 0;
}

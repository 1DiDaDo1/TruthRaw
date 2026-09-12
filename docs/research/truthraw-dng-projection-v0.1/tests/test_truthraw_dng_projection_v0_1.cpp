#include "truthraw_dng_projection_v0_1.h"

#include "scientific_master_streaming_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace truthraw;
namespace projection = truthraw::dng_projection::v0_1;
namespace binding1 = truthraw::scientific_preview_binding_v0_1;
namespace binding2 = truthraw::scientific_preview_binding_v0_2;
namespace master2 = truthraw::scientific_master_streaming_binding::v0_2;
namespace phase2 = truthraw::technical_backplane_phase2::v0_1;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(std::string("REQUIRE failed: ") + #x); } while (0)

namespace {

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    if (b.size() < o + 2u) b.resize(o + 2u, 0u);
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    if (b.size() < o + 4u) b.resize(o + 4u, 0u);
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    b[o + 2u] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    b[o + 3u] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::uint16_t get16(const std::vector<std::uint8_t>& b, std::size_t o) {
    REQUIRE(o + 2u <= b.size());
    return static_cast<std::uint16_t>(b[o]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o + 1u]) << 8u);
}

std::uint32_t get32(const std::vector<std::uint8_t>& b, std::size_t o) {
    REQUIRE(o + 4u <= b.size());
    return static_cast<std::uint32_t>(b[o]) |
           (static_cast<std::uint32_t>(b[o + 1u]) << 8u) |
           (static_cast<std::uint32_t>(b[o + 2u]) << 16u) |
           (static_cast<std::uint32_t>(b[o + 3u]) << 24u);
}

std::vector<std::uint8_t> rational3() {
    std::vector<std::uint8_t> b(24u, 0u);
    for (std::size_t i = 0u; i < 3u; ++i) {
        put32(b, i * 8u, 1u);
        put32(b, i * 8u + 4u, 1u);
    }
    return b;
}

std::vector<std::uint8_t> identity_srational() {
    std::vector<std::uint8_t> b(72u, 0u);
    for (std::size_t i = 0u; i < 9u; ++i) {
        const std::int32_t n = (i == 0u || i == 4u || i == 8u) ? 1 : 0;
        put32(b, i * 8u, static_cast<std::uint32_t>(n));
        put32(b, i * 8u + 4u, 1u);
    }
    return b;
}

struct FixtureEntry {
    std::uint16_t tag;
    std::uint16_t type;
    std::uint32_t count;
    std::vector<std::uint8_t> data;
};

std::vector<std::uint8_t> make_color_tiff() {
    std::vector<FixtureEntry> entries;
    entries.push_back({50721u, 10u, 9u, identity_srational()});
    entries.push_back({50728u, 5u, 3u, rational3()});
    entries.push_back({50778u, 3u, 1u, {21u, 0u}});
    std::sort(entries.begin(), entries.end(),
              [](const FixtureEntry& a, const FixtureEntry& b) { return a.tag < b.tag; });

    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::vector<std::uint8_t> out(8u + ifdBytes, 0u);
    out[0] = 'I'; out[1] = 'I';
    put16(out, 2u, 42u);
    put32(out, 4u, 8u);
    put16(out, 8u, static_cast<std::uint16_t>(entries.size()));
    std::size_t external = out.size();
    for (std::size_t i = 0u; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const std::size_t p = 10u + i * 12u;
        put16(out, p, e.tag);
        put16(out, p + 2u, e.type);
        put32(out, p + 4u, e.count);
        if (e.data.size() <= 4u) {
            std::copy(e.data.begin(), e.data.end(),
                      out.begin() + static_cast<std::ptrdiff_t>(p + 8u));
        } else {
            put32(out, p + 8u, static_cast<std::uint32_t>(external));
            out.insert(out.end(), e.data.begin(), e.data.end());
            external = out.size();
        }
    }
    return out;
}

struct MemBytes final : tile_dng_v0_1::IRandomAccessByteSource {
    std::vector<std::uint8_t> bytes;
    explicit MemBytes(std::vector<std::uint8_t> in) : bytes(std::move(in)) {}
    std::uint64_t sizeBytes() const override { return bytes.size(); }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this) + bytes.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes.size() || count > bytes.size() - offset) return false;
        std::memcpy(dst, bytes.data() + offset, count);
        return true;
    }
};

struct SyntheticTileSource final : streaming_v0_1::IRawTileSource {
    DngMetadata meta;
    const DngMetadata& metadata() const override { return meta; }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }

    static std::uint16_t raw_value(int x, int y) {
        return static_cast<std::uint16_t>(96 + ((x * 17 + y * 13) % 800));
    }

    streaming_v0_1::StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const int w = rect.hx1 - rect.hx0;
        const int h = rect.hy1 - rect.hy0;
        const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        if (rawOut == nullptr || rawCount != n || gainOut != nullptr || gainCount != 0u) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument, "bad synthetic tile request");
        }
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                rawOut[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) +
                       static_cast<std::size_t>(xx)] = raw_value(rect.hx0 + xx, rect.hy0 + yy);
            }
        }
        return streaming_v0_1::StreamStatus::ok();
    }
    streaming_v0_1::StreamStatus readRowBias(int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::ok();
    }
    streaming_v0_1::StreamStatus readColBias(int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::ok();
    }
};

struct VectorSink final : projection::ISequentialByteSink {
    std::vector<std::uint8_t> bytes;
    bool write(const void* data, std::size_t count) override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(), p, p + count);
        return true;
    }
};

struct TagView {
    std::uint16_t type = 0u;
    std::uint32_t count = 0u;
    std::uint32_t dataOffset = 0u;
    std::size_t dataBytes = 0u;
};

std::size_t type_size(std::uint16_t type) {
    switch (type) {
        case 1: case 2: case 6: case 7: return 1u;
        case 3: case 8: return 2u;
        case 4: case 9: case 11: return 4u;
        case 5: case 10: case 12: return 8u;
        default: return 0u;
    }
}

TagView find_tag(const std::vector<std::uint8_t>& dng, std::uint16_t wanted) {
    REQUIRE(dng.size() >= 10u);
    REQUIRE(dng[0] == 'I' && dng[1] == 'I');
    REQUIRE(get16(dng, 2u) == 42u);
    const std::uint32_t ifd = get32(dng, 4u);
    const std::uint16_t count = get16(dng, ifd);
    for (std::uint16_t i = 0u; i < count; ++i) {
        const std::size_t p = static_cast<std::size_t>(ifd) + 2u + static_cast<std::size_t>(i) * 12u;
        const std::uint16_t tag = get16(dng, p);
        if (tag != wanted) continue;
        const std::uint16_t type = get16(dng, p + 2u);
        const std::uint32_t items = get32(dng, p + 4u);
        const std::size_t bytes = type_size(type) * static_cast<std::size_t>(items);
        REQUIRE(bytes > 0u);
        const std::uint32_t offset = bytes <= 4u
            ? static_cast<std::uint32_t>(p + 8u)
            : get32(dng, p + 8u);
        REQUIRE(static_cast<std::size_t>(offset) + bytes <= dng.size());
        return TagView{type, items, offset, bytes};
    }
    throw std::runtime_error("tag missing: " + std::to_string(wanted));
}

bool has_tag(const std::vector<std::uint8_t>& dng, std::uint16_t wanted) {
    try { (void)find_tag(dng, wanted); return true; }
    catch (...) { return false; }
}

std::string ascii_tag(const std::vector<std::uint8_t>& dng, std::uint16_t tag) {
    const auto v = find_tag(dng, tag);
    REQUIRE(v.type == 2u && v.dataBytes >= 1u);
    const char* p = reinterpret_cast<const char*>(dng.data() + v.dataOffset);
    return std::string(p, p + v.dataBytes - 1u);
}

float float_at(const std::vector<std::uint8_t>& dng, std::size_t offset) {
    const std::uint32_t bits = get32(dng, offset);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

struct Fixture final {
    MemBytes bytes{make_color_tiff()};
    binding1::SourceSeal seal{};
    binding2::PreparedScientificPreviewSource prepared{};
    SyntheticTileSource source{};
    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction{};
    master2::Result identity{};
    phase2::Phase2Result finalized{};

    Fixture() {
        REQUIRE(binding1::seal_source_sha256(bytes, seal, 64u));

        binding1::ScientificColorBindingRecord color;
        color.authority = binding1::ColorBindingAuthority::SourceMetadataBound;
        color.sourceEvidenceId = seal.sourceEvidenceId;
        color.bindingId = "test-source-color";
        color.cameraToXyzD50 = {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f};
        color.normalized = true;
        color.validated = true;
        color.physicalFrameCount = 1u;
        color.independentEvidenceCount = 1u;
        REQUIRE(binding2::prepare_scientific_color_source(seal, color, prepared));

        source.meta.width = 130;
        source.meta.height = 70;
        source.meta.cfa = CfaPattern::BGGR;
        source.meta.orientation = Orientation::Normal;
        source.meta.whiteLevel = 1023.0f;
        source.meta.blackPhase = {64.f,64.f,64.f,64.f};
        source.meta.hasNoiseProfile = false;
        source.meta.hasGainField = false;
        source.meta.hasResidualBlack = false;
        source.meta.sourceId = seal.sourceEvidenceId;
        source.meta.cameraToXyzD50 = color.cameraToXyzD50;

        master2::Options options;
        REQUIRE(master2::bind_scientific_master_streaming(source, reconstruction, options, identity));
        finalized = finalize(identity);
    }

    phase2::Phase2Result finalize(const master2::Result& scientific) const {
        phase2::Phase2Input input;
        input.prepared = prepared;
        input.scientificMasterHash = scientific.scientificMasterHash;
        input.zeroLineGauge = scientific.zeroLineGauge;
        input.sceneBinding = scientific.sceneBinding;
        input.roomStatus.fill(technical_backplane::v0_1::RoomStatus::ResearchOnly);
        input.claimStatus = technical_backplane::v0_1::ClaimStatus::Candidate;
        phase2::Phase2Result result;
        REQUIRE(phase2::finalize_phase2(input, result));
        return result;
    }
};

void verify_common_dng(const Fixture& fixture,
                       const VectorSink& sink,
                       const projection::Result& result,
                       std::uint16_t expectedPhotometric,
                       std::uint16_t expectedSamplesPerPixel) {
    REQUIRE(result.sourceVerifiedBefore);
    REQUIRE(result.sourceVerifiedAfter);
    REQUIRE(result.scientificMasterHashMatched);
    REQUIRE(result.recomputedScientificMasterHash == fixture.identity.scientificMasterHash);
    REQUIRE(!result.fullFrameMaterialized);
    REQUIRE(!result.createsEvidence);
    REQUIRE(result.physicalFrameCount == 1u);
    REQUIRE(result.independentEvidenceCount == 1u);
    REQUIRE(result.canonicalTilesProcessed == 6u);
    REQUIRE(result.stripsWritten == 2u);
    REQUIRE(result.bytesWritten == sink.bytes.size());

    const auto photometric = find_tag(sink.bytes, 262u);
    REQUIRE(photometric.type == 3u && photometric.count == 1u);
    REQUIRE(get16(sink.bytes, photometric.dataOffset) == expectedPhotometric);
    const auto spp = find_tag(sink.bytes, 277u);
    REQUIRE(get16(sink.bytes, spp.dataOffset) == expectedSamplesPerPixel);

    const auto sampleFormat = find_tag(sink.bytes, 339u);
    REQUIRE(sampleFormat.type == 3u);
    REQUIRE(get16(sink.bytes, sampleFormat.dataOffset) == 3u);

    const auto dngVersion = find_tag(sink.bytes, 50706u);
    REQUIRE(dngVersion.type == 1u && dngVersion.count == 4u);
    REQUIRE(sink.bytes[dngVersion.dataOffset] == 1u);
    REQUIRE(sink.bytes[dngVersion.dataOffset + 1u] == 4u);
    REQUIRE(sink.bytes[dngVersion.dataOffset + 2u] == 0u);
    REQUIRE(sink.bytes[dngVersion.dataOffset + 3u] == 0u);
    const auto backward = find_tag(sink.bytes, 50707u);
    REQUIRE(backward.type == 1u && backward.count == 4u);
    REQUIRE(sink.bytes[backward.dataOffset] == 1u);
    REQUIRE(sink.bytes[backward.dataOffset + 1u] == 4u);

    REQUIRE(has_tag(sink.bytes, 50721u));
    REQUIRE(has_tag(sink.bytes, 50728u));
    const std::string description = ascii_tag(sink.bytes, 270u);
    REQUIRE(description.find(fixture.seal.sourceEvidenceId) != std::string::npos);
    REQUIRE(description.find(scientific_master_digest::v0_1::to_hex(
        fixture.identity.scientificMasterHash)) != std::string::npos);

    const auto offsets = find_tag(sink.bytes, 273u);
    const auto counts = find_tag(sink.bytes, 279u);
    REQUIRE(offsets.type == 4u && offsets.count == 2u);
    REQUIRE(counts.type == 4u && counts.count == 2u);
    const std::uint32_t off0 = get32(sink.bytes, offsets.dataOffset);
    const std::uint32_t off1 = get32(sink.bytes, offsets.dataOffset + 4u);
    const std::uint32_t bytes0 = get32(sink.bytes, counts.dataOffset);
    const std::uint32_t bytes1 = get32(sink.bytes, counts.dataOffset + 4u);
    REQUIRE(off0 < off1);
    REQUIRE(static_cast<std::uint64_t>(off0) + bytes0 == off1);
    REQUIRE(static_cast<std::uint64_t>(off1) + bytes1 == sink.bytes.size());
}

void test_stage2_cfa_projection() {
    Fixture fixture;
    VectorSink sink;
    projection::Options options;
    options.kind = projection::ProjectionKind::Stage2CfaFloat32;
    projection::Result result;
    const auto status = projection::export_projection(
        fixture.prepared, fixture.identity, fixture.finalized,
        fixture.bytes, fixture.source, fixture.reconstruction, sink, options, result);
    if (!status) std::cerr << projection::status_name(status.code) << ": " << status.message << '\n';
    REQUIRE(status);
    verify_common_dng(fixture, sink, result, 32803u, 1u);
    REQUIRE(has_tag(sink.bytes, 33421u));
    REQUIRE(has_tag(sink.bytes, 33422u));
    REQUIRE(has_tag(sink.bytes, 50710u));
    REQUIRE(has_tag(sink.bytes, 50711u));
    REQUIRE(ascii_tag(sink.bytes, 50708u) == "TruthRaw Stage2 CFA Projection v0.1");
    REQUIRE(ascii_tag(sink.bytes, 270u).find("DERIVED_MEASUREMENT") != std::string::npos);

    const auto offsets = find_tag(sink.bytes, 273u);
    const std::size_t pixel0 = get32(sink.bytes, offsets.dataOffset);
    const float expected =
        (static_cast<float>(SyntheticTileSource::raw_value(0, 0)) - 64.0f) / (1023.0f - 64.0f);
    REQUIRE(float_at(sink.bytes, pixel0) == expected);
}

void test_linear_raw_projection() {
    Fixture fixture;
    VectorSink sink;
    projection::Options options;
    options.kind = projection::ProjectionKind::LinearRawCameraRgbFloat32;
    projection::Result result;
    const auto status = projection::export_projection(
        fixture.prepared, fixture.identity, fixture.finalized,
        fixture.bytes, fixture.source, fixture.reconstruction, sink, options, result);
    if (!status) std::cerr << projection::status_name(status.code) << ": " << status.message << '\n';
    REQUIRE(status);
    verify_common_dng(fixture, sink, result, 34892u, 3u);
    REQUIRE(!has_tag(sink.bytes, 33421u));
    REQUIRE(!has_tag(sink.bytes, 33422u));
    REQUIRE(ascii_tag(sink.bytes, 50708u) == "TruthRaw LinearRaw Projection v0.1");
    REQUIRE(ascii_tag(sink.bytes, 270u).find("RECONSTRUCTED_COMPATIBILITY_PROJECTION") != std::string::npos);

    const auto offsets = find_tag(sink.bytes, 273u);
    const std::size_t pixel0 = get32(sink.bytes, offsets.dataOffset);
    const float measuredBlue =
        (static_cast<float>(SyntheticTileSource::raw_value(0, 0)) - 64.0f) / (1023.0f - 64.0f);
    REQUIRE(float_at(sink.bytes, pixel0 + 8u) == measuredBlue);
}

void test_missing_final_admission_is_rejected_without_output() {
    Fixture fixture;
    fixture.finalized.admission.claimScope = binding1::ColorClaimScope::None;
    VectorSink sink;
    projection::Result result;
    const auto status = projection::export_projection(
        fixture.prepared, fixture.identity, fixture.finalized,
        fixture.bytes, fixture.source, fixture.reconstruction, sink, {}, result);
    REQUIRE(!status);
    REQUIRE(status.code == projection::StatusCode::AdmissionRejected);
    REQUIRE(sink.bytes.empty());
}

void test_source_mutation_is_rejected_without_output() {
    Fixture fixture;
    fixture.bytes.bytes.back() ^= 0x01u;
    VectorSink sink;
    projection::Result result;
    const auto status = projection::export_projection(
        fixture.prepared, fixture.identity, fixture.finalized,
        fixture.bytes, fixture.source, fixture.reconstruction, sink, {}, result);
    REQUIRE(!status);
    REQUIRE(status.code == projection::StatusCode::SourceSealMismatch);
    REQUIRE(sink.bytes.empty());
}

void test_digest_mismatch_fails_closed() {
    Fixture fixture;
    auto falseIdentity = fixture.identity;
    falseIdentity.scientificMasterHash[0] ^= 0x01u;
    auto falseFinalized = fixture.finalize(falseIdentity);
    VectorSink sink;
    projection::Result result;
    const auto status = projection::export_projection(
        fixture.prepared, falseIdentity, falseFinalized,
        fixture.bytes, fixture.source, fixture.reconstruction, sink, {}, result);
    REQUIRE(!status);
    REQUIRE(status.code == projection::StatusCode::DigestMismatch);
    REQUIRE(!result.scientificMasterHashMatched);
    REQUIRE(!sink.bytes.empty());
}

} // namespace

int main() {
    try {
        test_stage2_cfa_projection();
        test_linear_raw_projection();
        test_missing_final_admission_is_rejected_without_output();
        test_source_mutation_is_rejected_without_output();
        test_digest_mismatch_fails_closed();
        std::cout << "TRUTHRAW_DNG_PROJECTION_V0_1_PASS\n";
        std::cout << "stage2_role=DERIVED_MEASUREMENT_NOT_EVIDENCE\n";
        std::cout << "linear_role=RECONSTRUCTED_COMPATIBILITY_NOT_EVIDENCE\n";
        std::cout << "master_digest_reverified=1\n";
        std::cout << "full_frame_materialized=0\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "TRUTHRAW_DNG_PROJECTION_V0_1_FAIL: " << error.what() << '\n';
        return 1;
    }
}

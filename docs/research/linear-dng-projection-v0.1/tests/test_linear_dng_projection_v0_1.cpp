#include "linear_dng_projection_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " #expr " at line " << __LINE__ << '\n'; return 1; } } while (0)

using truthraw::CfaPattern;
using truthraw::DngMetadata;
using truthraw::IReconstructionBackend;
using truthraw::ReconstructionQuality;
using truthraw::Status;
using truthraw::StatusCode;
using truthraw::TileRect;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::linear_dng_projection::v0_1::IRandomAccessByteSink;
using truthraw::linear_dng_projection::v0_1::Options;
using truthraw::linear_dng_projection::v0_1::Result;
using truthraw::streaming_v0_1::IRawTileSource;
using truthraw::streaming_v0_1::StreamStatus;
using truthraw::tile_dng_v0_1::IRandomAccessByteSource;

void put16(std::vector<std::uint8_t>& b, std::size_t o, std::uint16_t v) {
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o+1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put32(std::vector<std::uint8_t>& b, std::size_t o, std::uint32_t v) {
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o+1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    b[o+2] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    b[o+3] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::uint16_t get16(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint16_t>(b[o]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o+1]) << 8u);
}

std::uint32_t get32(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint32_t>(b[o]) |
           (static_cast<std::uint32_t>(b[o+1]) << 8u) |
           (static_cast<std::uint32_t>(b[o+2]) << 16u) |
           (static_cast<std::uint32_t>(b[o+3]) << 24u);
}

std::vector<std::uint8_t> make_source_metadata_tiff() {
    constexpr std::size_t ifd = 8;
    constexpr std::size_t count = 2;
    constexpr std::size_t ifdBytes = 2 + count * 12 + 4;
    constexpr std::size_t matrixOffset = ifd + ifdBytes;
    constexpr std::size_t matrixBytes = 9 * 8;
    constexpr std::size_t neutralOffset = matrixOffset + matrixBytes;
    constexpr std::size_t neutralBytes = 3 * 8;
    std::vector<std::uint8_t> b(neutralOffset + neutralBytes, 0u);
    b[0] = 'I'; b[1] = 'I';
    put16(b, 2, 42);
    put32(b, 4, static_cast<std::uint32_t>(ifd));
    put16(b, ifd, static_cast<std::uint16_t>(count));

    std::size_t e = ifd + 2;
    put16(b, e, 50721); put16(b, e+2, 10); put32(b, e+4, 9); put32(b, e+8, static_cast<std::uint32_t>(matrixOffset));
    e += 12;
    put16(b, e, 50728); put16(b, e+2, 5); put32(b, e+4, 3); put32(b, e+8, static_cast<std::uint32_t>(neutralOffset));
    put32(b, ifd + 2 + count * 12, 0);

    for (std::size_t i = 0; i < 9; ++i) {
        const std::int32_t n = (i == 0 || i == 4 || i == 8) ? 1 : 0;
        put32(b, matrixOffset + 8*i, static_cast<std::uint32_t>(n));
        put32(b, matrixOffset + 8*i + 4, 1);
    }
    for (std::size_t i = 0; i < 3; ++i) {
        put32(b, neutralOffset + 8*i, 1);
        put32(b, neutralOffset + 8*i + 4, 1);
    }
    return b;
}

class MemorySource final : public IRandomAccessByteSource {
public:
    explicit MemorySource(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}
    std::uint64_t sizeBytes() const override { return bytes_.size(); }
    std::size_t residentBytesUpperBound() const override { return bytes_.capacity(); }
    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        if (offset > bytes_.size() || count > bytes_.size() - static_cast<std::size_t>(offset)) return false;
        std::memcpy(dst, bytes_.data() + static_cast<std::size_t>(offset), count);
        return true;
    }
    const std::vector<std::uint8_t>& bytes() const { return bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
};

class MemorySink final : public IRandomAccessByteSink {
public:
    bool resize(std::uint64_t bytes) override {
        if (bytes > std::numeric_limits<std::size_t>::max()) return false;
        bytes_.assign(static_cast<std::size_t>(bytes), 0u);
        return true;
    }
    bool writeExact(std::uint64_t offset, const void* src, std::size_t bytes) override {
        if (offset > bytes_.size() || bytes > bytes_.size() - static_cast<std::size_t>(offset)) return false;
        std::memcpy(bytes_.data() + static_cast<std::size_t>(offset), src, bytes);
        return true;
    }
    std::size_t residentBytesUpperBound() const override { return 0u; }
    const std::vector<std::uint8_t>& bytes() const { return bytes_; }
private:
    std::vector<std::uint8_t> bytes_;
};

class SyntheticRawSource final : public IRawTileSource {
public:
    SyntheticRawSource() {
        metadata_.width = 70;
        metadata_.height = 50;
        metadata_.cfa = CfaPattern::BGGR;
        metadata_.orientation = truthraw::Orientation::Normal;
        metadata_.whiteLevel = 1023.0f;
        metadata_.blackPhase = {0.f,0.f,0.f,0.f};
        metadata_.sourceId = "sha256:test-linear-dng-projection";
    }
    const DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }
    StreamStatus readRawTile(const TileRect& rect,
                             std::uint16_t* rawOut,
                             std::size_t rawCount,
                             float* gainOut,
                             std::size_t gainCount) override {
        (void)gainOut; (void)gainCount;
        const int w = rect.hx1 - rect.hx0;
        const int h = rect.hy1 - rect.hy0;
        const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        if (rawOut == nullptr || rawCount != expected) {
            return StreamStatus::error(truthraw::streaming_v0_1::StreamStatusCode::InvalidArgument, "bad synthetic raw tile buffer");
        }
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const int gx = rect.hx0 + x;
                const int gy = rect.hy0 + y;
                rawOut[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] =
                    static_cast<std::uint16_t>(128 + ((gx * 7 + gy * 11) % 700));
            }
        }
        return StreamStatus::ok();
    }
    StreamStatus readRowBias(int, int, float*, std::size_t) override { return StreamStatus::ok(); }
    StreamStatus readColBias(int, int, float*, std::size_t) override { return StreamStatus::ok(); }
private:
    DngMetadata metadata_{};
};

class SyntheticReconstruction final : public IReconstructionBackend {
public:
    ReconstructionQuality quality() const override { return ReconstructionQuality::ResearchBackend; }
    const char* name() const override { return "synthetic_camera_native_rgb"; }
    int requiredHalo() const override { return 0; }
    Status reconstructTile(const float* stage2FullTile,
                           int tileW,
                           int tileH,
                           int globalHx0,
                           int globalHy0,
                           int coreX0,
                           int coreY0,
                           int coreW,
                           int coreH,
                           CfaPattern,
                           float* coreCameraRgb) override {
        if (stage2FullTile == nullptr || coreCameraRgb == nullptr || tileW <= 0 || tileH <= 0) {
            return Status::error(StatusCode::InvalidArgument, "bad synthetic reconstruction args");
        }
        for (int y = 0; y < coreH; ++y) {
            for (int x = 0; x < coreW; ++x) {
                const int lx = coreX0 + x - globalHx0;
                const int ly = coreY0 + y - globalHy0;
                if (lx < 0 || ly < 0 || lx >= tileW || ly >= tileH) {
                    return Status::error(StatusCode::InvalidArgument, "synthetic core outside tile");
                }
                const float v = stage2FullTile[static_cast<std::size_t>(ly) * static_cast<std::size_t>(tileW) + static_cast<std::size_t>(lx)];
                const std::size_t p = static_cast<std::size_t>(y) * static_cast<std::size_t>(coreW) + static_cast<std::size_t>(x);
                coreCameraRgb[3*p] = v;
                coreCameraRgb[3*p+1] = std::min(1.0f, v * 0.9f + 0.03f);
                coreCameraRgb[3*p+2] = std::min(1.0f, v * 0.8f + 0.05f);
            }
        }
        return Status::ok();
    }
};

struct ParsedEntry {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t valueOrOffset = 0;
    std::size_t entryOffset = 0;
};

bool find_entry(const std::vector<std::uint8_t>& b, std::uint16_t tag, ParsedEntry& out) {
    if (b.size() < 10 || b[0] != 'I' || b[1] != 'I' || get16(b,2) != 42) return false;
    const std::uint32_t ifd = get32(b,4);
    if (ifd > b.size() || b.size() - ifd < 2) return false;
    const std::uint16_t count = get16(b,ifd);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t e = static_cast<std::size_t>(ifd) + 2u + 12u * i;
        if (e + 12u > b.size()) return false;
        if (get16(b,e) == tag) {
            out.type = get16(b,e+2);
            out.count = get32(b,e+4);
            out.valueOrOffset = get32(b,e+8);
            out.entryOffset = e;
            return true;
        }
    }
    return false;
}

ReleaseResult make_release() {
    ReleaseResult release;
    release.authority = PreviewAuthority::FinalizedSourceBoundScientificPreview;
    release.scientificIdentity.scientificMasterHash[0] = 0x42u;
    release.scientificIdentity.physicalFrameCount = 1u;
    release.scientificIdentity.independentEvidenceCount = 1u;
    release.streaming.provenance.physicalFrameCount = 1u;
    release.streaming.provenance.independentEvidenceCount = 1u;
    release.streaming.provenance.scientificMasterModifiedByAppearance = false;
    release.streaming.provenance.counterfactualObservationCreated = false;
    return release;
}

} // namespace

int main() {
    MemorySource sourceBytes(make_source_metadata_tiff());
    SyntheticRawSource rawSource;
    SyntheticReconstruction reconstruction;
    MemorySink sink;
    auto release = make_release();

    Result result;
    Options options;
    options.memoryBudgetBytes = 4u * 1024u * 1024u;
    const auto status = truthraw::linear_dng_projection::v0_1::write_finalized_linear_dng(
        release, sourceBytes, rawSource, reconstruction, sink, options, result);
    CHECK(static_cast<bool>(status));
    CHECK(result.width == 70u);
    CHECK(result.height == 50u);
    CHECK(result.pixelPayloadBytes == 70u * 50u * 3u * 2u);
    CHECK(result.outputBytes == sink.bytes().size());
    CHECK(result.tilesWritten == 2u);
    CHECK(result.linearRawPhotometric);
    CHECK(result.boundedUnsigned16Projection);
    CHECK(result.sourceColorMetadataCopied);
    CHECK(!result.fullScientificMasterMaterialized);
    CHECK(result.physicalFrameCount == 1u);
    CHECK(result.independentEvidenceCount == 1u);

    ParsedEntry photometric{};
    CHECK(find_entry(sink.bytes(), 262, photometric));
    CHECK(photometric.type == 3u && photometric.count == 1u);
    CHECK(static_cast<std::uint16_t>(photometric.valueOrOffset & 0xffffu) == 34892u);

    ParsedEntry samplesPerPixel{};
    CHECK(find_entry(sink.bytes(), 277, samplesPerPixel));
    CHECK(static_cast<std::uint16_t>(samplesPerPixel.valueOrOffset & 0xffffu) == 3u);

    ParsedEntry dngVersion{};
    CHECK(find_entry(sink.bytes(), 50706, dngVersion));
    CHECK(dngVersion.type == 1u && dngVersion.count == 4u);
    const auto& outBytes = sink.bytes();
    CHECK(outBytes[dngVersion.entryOffset + 8u] == 1u);
    CHECK(outBytes[dngVersion.entryOffset + 9u] == 4u);

    ParsedEntry colorMatrix{};
    CHECK(find_entry(outBytes, 50721, colorMatrix));
    CHECK(colorMatrix.type == 10u && colorMatrix.count == 9u);
    CHECK(colorMatrix.valueOrOffset + 72u <= outBytes.size());
    const auto& inputBytes = sourceBytes.bytes();
    constexpr std::size_t inputMatrixOffset = 38u;
    CHECK(std::equal(inputBytes.begin() + inputMatrixOffset,
                     inputBytes.begin() + inputMatrixOffset + 72u,
                     outBytes.begin() + colorMatrix.valueOrOffset));

    ParsedEntry neutral{};
    CHECK(find_entry(outBytes, 50728, neutral));
    CHECK(neutral.type == 5u && neutral.count == 3u);

    ParsedEntry stripOffset{};
    ParsedEntry stripByteCounts{};
    CHECK(find_entry(outBytes, 273, stripOffset));
    CHECK(find_entry(outBytes, 279, stripByteCounts));
    CHECK(stripByteCounts.valueOrOffset == result.pixelPayloadBytes);
    CHECK(static_cast<std::uint64_t>(stripOffset.valueOrOffset) + result.pixelPayloadBytes == outBytes.size());
    CHECK(get16(outBytes, stripOffset.valueOrOffset) != 0u);

    auto rejectedRelease = release;
    rejectedRelease.authority = PreviewAuthority::None;
    MemorySink rejectedSink;
    Result rejectedResult;
    const auto rejected = truthraw::linear_dng_projection::v0_1::write_finalized_linear_dng(
        rejectedRelease, sourceBytes, rawSource, reconstruction, rejectedSink, options, rejectedResult);
    CHECK(!static_cast<bool>(rejected));
    CHECK(rejected.code == truthraw::linear_dng_projection::v0_1::StatusCode::FinalizedReleaseRequired);

    std::cout << "LINEAR_DNG_PROJECTION_V0_1_PASS\n";
    std::cout << "output_bytes=" << result.outputBytes << '\n';
    std::cout << "pixel_bytes=" << result.pixelPayloadBytes << '\n';
    std::cout << "tiles=" << result.tilesWritten << '\n';
    std::cout << "full_master_materialized=" << result.fullScientificMasterMaterialized << '\n';
    return 0;
}

#include "linear_dng_projection_v0_2.h"

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

struct SourceEntry {
    std::uint16_t tag;
    std::uint16_t type;
    std::uint32_t count;
    std::vector<std::uint8_t> payload;
};

std::vector<std::uint8_t> ascii(const std::string& s) {
    std::vector<std::uint8_t> out(s.begin(), s.end());
    out.push_back(0u);
    return out;
}

std::vector<std::uint8_t> make_source_metadata_tiff() {
    std::vector<SourceEntry> entries;
    entries.push_back({271u, 2u, 6u, ascii("HONOR")});
    entries.push_back({272u, 2u, 8u, ascii("BKQ-N49")});
    entries.push_back({50708u, 2u, 20u, ascii("BKQ-N49-HONOR-HONOR")});

    std::vector<std::uint8_t> matrix(9u * 8u, 0u);
    for (std::size_t i = 0; i < 9u; ++i) {
        const std::int32_t n = (i == 0u || i == 4u || i == 8u) ? 1 : 0;
        put32(matrix, i * 8u, static_cast<std::uint32_t>(n));
        put32(matrix, i * 8u + 4u, 1u);
    }
    entries.push_back({50721u, 10u, 9u, matrix});

    std::vector<std::uint8_t> neutral(3u * 8u, 0u);
    for (std::size_t i = 0; i < 3u; ++i) {
        put32(neutral, i * 8u, 1u);
        put32(neutral, i * 8u + 4u, 1u);
    }
    entries.push_back({50728u, 5u, 3u, neutral});

    const std::size_t ifd = 8u;
    const std::size_t ifdBytes = 2u + entries.size() * 12u + 4u;
    std::size_t cursor = ifd + ifdBytes;
    for (const auto& e : entries) if (e.payload.size() > 4u) cursor += e.payload.size();
    std::vector<std::uint8_t> b(cursor, 0u);
    b[0] = 'I'; b[1] = 'I';
    put16(b, 2u, 42u);
    put32(b, 4u, static_cast<std::uint32_t>(ifd));
    put16(b, ifd, static_cast<std::uint16_t>(entries.size()));

    std::size_t payloadOffset = ifd + ifdBytes;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const std::size_t e = ifd + 2u + i * 12u;
        put16(b, e, entry.tag);
        put16(b, e + 2u, entry.type);
        put32(b, e + 4u, entry.count);
        if (entry.payload.size() <= 4u) {
            std::copy(entry.payload.begin(), entry.payload.end(), b.begin() + static_cast<std::ptrdiff_t>(e + 8u));
        } else {
            put32(b, e + 8u, static_cast<std::uint32_t>(payloadOffset));
            std::copy(entry.payload.begin(), entry.payload.end(), b.begin() + static_cast<std::ptrdiff_t>(payloadOffset));
            payloadOffset += entry.payload.size();
        }
    }
    put32(b, ifd + 2u + entries.size() * 12u, 0u);
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
        metadata_.orientation = truthraw::Orientation::Rotate180;
        metadata_.whiteLevel = 1023.0f;
        metadata_.blackPhase = {0.f,0.f,0.f,0.f};
        metadata_.sourceId = "sha256:test-linear-dng-v0.2";
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
        for (std::size_t i = 0; i < expected; ++i) rawOut[i] = 512u;
        return StreamStatus::ok();
    }
    StreamStatus readRowBias(int, int, float*, std::size_t) override { return StreamStatus::ok(); }
    StreamStatus readColBias(int, int, float*, std::size_t) override { return StreamStatus::ok(); }
private:
    DngMetadata metadata_{};
};

class OverOneReconstruction final : public IReconstructionBackend {
public:
    explicit OverOneReconstruction(float value) : value_(value) {}
    ReconstructionQuality quality() const override { return ReconstructionQuality::ResearchBackend; }
    const char* name() const override { return "synthetic_over_one_rgb"; }
    int requiredHalo() const override { return 0; }
    Status reconstructTile(const float*, int, int, int, int, int, int,
                           int coreW, int coreH, CfaPattern, float* out) override {
        if (out == nullptr || coreW <= 0 || coreH <= 0) {
            return Status::error(StatusCode::InvalidArgument, "bad synthetic reconstruction args");
        }
        const std::size_t n = static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH) * 3u;
        for (std::size_t i = 0; i < n; ++i) out[i] = value_;
        return Status::ok();
    }
private:
    float value_ = 1.5f;
};

struct ParsedEntry {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::uint32_t valueOrOffset = 0;
};

bool find_entry(const std::vector<std::uint8_t>& b, std::uint16_t tag, ParsedEntry& out) {
    if (b.size() < 10u || b[0] != 'I' || b[1] != 'I' || get16(b,2u) != 42u) return false;
    const std::uint32_t ifd = get32(b,4u);
    if (ifd > b.size() || b.size() - ifd < 2u) return false;
    const std::uint16_t count = get16(b,ifd);
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::size_t e = static_cast<std::size_t>(ifd) + 2u + 12u * i;
        if (e + 12u > b.size()) return false;
        if (get16(b,e) == tag) {
            out.type = get16(b,e+2u);
            out.count = get32(b,e+4u);
            out.valueOrOffset = get32(b,e+8u);
            return true;
        }
    }
    return false;
}

std::string read_ascii(const std::vector<std::uint8_t>& b, std::uint16_t tag) {
    ParsedEntry e{};
    if (!find_entry(b, tag, e) || e.type != 2u || e.count == 0u) return {};
    std::vector<std::uint8_t> p(e.count, 0u);
    if (e.count <= 4u) {
        for (std::size_t i = 0; i < e.count; ++i) p[i] = static_cast<std::uint8_t>((e.valueOrOffset >> (8u*i)) & 0xffu);
    } else {
        if (e.valueOrOffset > b.size() || e.count > b.size() - e.valueOrOffset) return {};
        std::copy(b.begin() + e.valueOrOffset, b.begin() + e.valueOrOffset + e.count, p.begin());
    }
    const auto zero = std::find(p.begin(), p.end(), static_cast<std::uint8_t>(0u));
    return std::string(p.begin(), zero);
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
    auto release = make_release();

    OverOneReconstruction reconstruction(1.5f);
    MemorySink sink;
    truthraw::linear_dng_projection::v0_2::Options options;
    options.memoryBudgetBytes = 4u * 1024u * 1024u;
    truthraw::linear_dng_projection::v0_2::Result result;
    const auto status = truthraw::linear_dng_projection::v0_2::write_finalized_linear_dng(
        release, sourceBytes, rawSource, reconstruction, sink, options, result);
    CHECK(static_cast<bool>(status));
    CHECK(result.compatibilityWindow == 2.0);
    CHECK(result.baselineExposureEv == 1.0);
    CHECK(result.sourceCameraIdentityPreserved);
    CHECK(!result.overWindowRejected);
    CHECK(result.base.samplesClippedHigh == 0u);
    CHECK(result.base.samplesClippedLow == 0u);

    const auto& bytes = sink.bytes();
    CHECK(!bytes.empty());
    CHECK(read_ascii(bytes, 271u) == "HONOR");
    CHECK(read_ascii(bytes, 272u) == "BKQ-N49");
    CHECK(read_ascii(bytes, 50708u) == "BKQ-N49-HONOR-HONOR");

    ParsedEntry baseline{};
    CHECK(find_entry(bytes, 50730u, baseline));
    CHECK(baseline.type == 10u && baseline.count == 1u);
    CHECK(baseline.valueOrOffset + 8u <= bytes.size());
    CHECK(get32(bytes, baseline.valueOrOffset) == 1u);
    CHECK(get32(bytes, baseline.valueOrOffset + 4u) == 1u);

    ParsedEntry strip{};
    CHECK(find_entry(bytes, 273u, strip));
    CHECK(strip.valueOrOffset + 2u <= bytes.size());
    const std::uint16_t encoded = get16(bytes, strip.valueOrOffset);
    const double decodedScene = (static_cast<double>(encoded) / 65535.0) * 2.0;
    CHECK(decodedScene > 1.4999 && decodedScene < 1.5001);
    CHECK(encoded < 65535u);

    OverOneReconstruction tooBright(2.1f);
    MemorySink rejectedSink;
    truthraw::linear_dng_projection::v0_2::Result rejectedResult;
    const auto rejected = truthraw::linear_dng_projection::v0_2::write_finalized_linear_dng(
        release, sourceBytes, rawSource, tooBright, rejectedSink, options, rejectedResult);
    CHECK(!static_cast<bool>(rejected));
    CHECK(rejected.code == truthraw::linear_dng_projection::v0_2::StatusCode::SceneExceedsValidatedWindow);
    CHECK(rejectedResult.overWindowRejected);
    CHECK(rejectedSink.bytes().empty());

    std::cout << "LINEAR_DNG_PROJECTION_V0_2_PASS\n";
    std::cout << "window=2.0\n";
    std::cout << "baseline_exposure_ev=1.0\n";
    std::cout << "source_camera_identity_preserved=1\n";
    std::cout << "scene_1_5_preserved=1\n";
    std::cout << "scene_over_2_fail_closed=1\n";
    return 0;
}

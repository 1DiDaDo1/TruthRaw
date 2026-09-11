#include "dng_compatibility_projection_v0_1.h"
#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace truthraw;
namespace proj = truthraw::dng_compatibility_projection::v0_1;
namespace digest_v01 = truthraw::scientific_master_digest::v0_1;

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; \
} } while (0)

namespace {

int measured_channel(CfaPattern cfa, int x, int y) {
    const int p = ((y & 1) << 1) | (x & 1);
    switch (cfa) {
        case CfaPattern::BGGR: { constexpr int v[4] = {2,1,1,0}; return v[p]; }
        case CfaPattern::RGGB: { constexpr int v[4] = {0,1,1,2}; return v[p]; }
        case CfaPattern::GRBG: { constexpr int v[4] = {1,0,2,1}; return v[p]; }
        case CfaPattern::GBRG: { constexpr int v[4] = {1,2,0,1}; return v[p]; }
    }
    return 1;
}

std::uint16_t raw_value(int width, int x, int y) {
    return static_cast<std::uint16_t>(1000 + ((y * width + x) % 50000));
}

float stage2_value(int width, int x, int y) {
    return static_cast<float>(raw_value(width, x, y)) / 65535.0f;
}

class FakeSource final : public streaming_v0_1::IRawTileSource {
public:
    explicit FakeSource(CfaPattern cfa) {
        metadata_.width = 66;
        metadata_.height = 50;
        metadata_.cfa = cfa;
        metadata_.orientation = Orientation::Normal;
        metadata_.whiteLevel = 65535.0f;
        metadata_.blackPhase = {0.f,0.f,0.f,0.f};
        metadata_.hasGainField = false;
        metadata_.hasResidualBlack = false;
        metadata_.sourceId = "sha256:test-source-evidence";
    }

    const DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }

    streaming_v0_1::StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const int w = rect.hx1 - rect.hx0;
        const int h = rect.hy1 - rect.hy0;
        const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        if (!rawOut || rawCount != expected || gainOut != nullptr || gainCount != 0u) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument, "bad fake read buffer");
        }
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                const int x = rect.hx0 + xx;
                const int y = rect.hy0 + yy;
                rawOut[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) +
                       static_cast<std::size_t>(xx)] = raw_value(metadata_.width, x, y);
            }
        }
        ++readCalls;
        return streaming_v0_1::StreamStatus::ok();
    }

    streaming_v0_1::StreamStatus readRowBias(
        int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::error(
            streaming_v0_1::StreamStatusCode::InvalidArgument, "row bias not expected");
    }
    streaming_v0_1::StreamStatus readColBias(
        int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::error(
            streaming_v0_1::StreamStatusCode::InvalidArgument, "col bias not expected");
    }

    std::size_t readCalls = 0u;

private:
    DngMetadata metadata_{};
};

class MeasuredPreservingBackend final : public IReconstructionBackend {
public:
    ReconstructionQuality quality() const override {
        return ReconstructionQuality::ResearchBackend;
    }
    const char* name() const override { return "test_measured_preserving"; }
    int requiredHalo() const override { return 0; }

    Status reconstructTile(
        const float* stage2FullTile, int tileW, int tileH,
        int globalHx0, int globalHy0,
        int coreX0, int coreY0, int coreW, int coreH,
        CfaPattern cfa, float* coreCameraRgb) override {
        if (!stage2FullTile || !coreCameraRgb || tileW <= 0 || tileH <= 0) {
            return Status::error(StatusCode::InvalidArgument, "bad test reconstruction input");
        }
        for (int yy = 0; yy < coreH; ++yy) {
            for (int xx = 0; xx < coreW; ++xx) {
                const int gx = coreX0 + xx;
                const int gy = coreY0 + yy;
                const int lx = gx - globalHx0;
                const int ly = gy - globalHy0;
                const float measured = stage2FullTile[
                    static_cast<std::size_t>(ly) * static_cast<std::size_t>(tileW) +
                    static_cast<std::size_t>(lx)];
                const int mc = measured_channel(cfa, gx, gy);
                const std::size_t i =
                    static_cast<std::size_t>(yy) * static_cast<std::size_t>(coreW) +
                    static_cast<std::size_t>(xx);
                for (int c = 0; c < 3; ++c) {
                    coreCameraRgb[3u*i + static_cast<std::size_t>(c)] =
                        c == mc ? measured : measured + 0.01f * static_cast<float>(c + 1);
                }
            }
        }
        return Status::ok();
    }
};

class MemorySink final : public proj::ISequentialByteSink {
public:
    bool write(const void* data, std::size_t count) override {
        if (!data && count != 0u) return false;
        const auto* p = static_cast<const std::uint8_t*>(data);
        bytes.insert(bytes.end(), p, p + count);
        return true;
    }
    std::size_t residentBytesUpperBound() const override { return bytes.capacity(); }
    std::vector<std::uint8_t> bytes;
};

std::uint16_t u16(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint16_t>(b[o]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o+1]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint32_t>(b[o]) |
        (static_cast<std::uint32_t>(b[o+1]) << 8u) |
        (static_cast<std::uint32_t>(b[o+2]) << 16u) |
        (static_cast<std::uint32_t>(b[o+3]) << 24u);
}

struct TagView {
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::size_t data = 0;
};

std::size_t type_size(std::uint16_t type) {
    switch (type) {
        case 1: case 2: return 1u;
        case 3: return 2u;
        case 4: return 4u;
        case 5: case 10: return 8u;
        default: return 0u;
    }
}

bool tag(const std::vector<std::uint8_t>& b, std::uint16_t wanted, TagView& out) {
    if (b.size() < 10u || b[0] != 'I' || b[1] != 'I' || u16(b, 2u) != 42u) return false;
    const std::size_t ifd = u32(b, 4u);
    if (ifd + 2u > b.size()) return false;
    const std::uint16_t n = u16(b, ifd);
    for (std::uint16_t i = 0; i < n; ++i) {
        const std::size_t e = ifd + 2u + static_cast<std::size_t>(i) * 12u;
        if (e + 12u > b.size()) return false;
        if (u16(b, e) != wanted) continue;
        out.type = u16(b, e + 2u);
        out.count = u32(b, e + 4u);
        const std::size_t bytes = type_size(out.type) * static_cast<std::size_t>(out.count);
        out.data = bytes <= 4u ? e + 8u : static_cast<std::size_t>(u32(b, e + 8u));
        return out.data + bytes <= b.size();
    }
    return false;
}

float f32(const std::vector<std::uint8_t>& b, std::size_t o) {
    const std::uint32_t bits = u32(b, o);
    float value = 0.f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

float expected_camera_sample(CfaPattern cfa, int width, int x, int y, int channel) {
    const float measured = stage2_value(width, x, y);
    return channel == measured_channel(cfa, x, y) ?
        measured : measured + 0.01f * static_cast<float>(channel + 1);
}

bool compute_master_hash(FakeSource& source,
                         MeasuredPreservingBackend& backend,
                         digest_v01::Sha256& out) {
    const auto& m = source.metadata();
    digest_v01::ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(m.width), static_cast<std::uint32_t>(m.height));
    if (!digest.valid()) return false;
    for (int y0 = 0; y0 < m.height; y0 += 64) {
        const int y1 = std::min(m.height, y0 + 64);
        for (int x0 = 0; x0 < m.width; x0 += 64) {
            const int x1 = std::min(m.width, x0 + 64);
            TileRect rect{x0,y0,x1,y1,x0,y0,x1,y1};
            const int w = x1 - x0;
            const int h = y1 - y0;
            const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
            std::vector<std::uint16_t> raw(n);
            if (!source.readRawTile(rect, raw.data(), n, nullptr, 0u)) return false;
            std::vector<float> stage2(n);
            for (std::size_t i = 0; i < n; ++i) stage2[i] = static_cast<float>(raw[i]) / 65535.0f;
            std::vector<float> cam(3u*n);
            if (!backend.reconstructTile(stage2.data(), w, h, x0, y0,
                                         x0, y0, w, h, m.cfa, cam.data())) return false;
            digest_v01::TileView tile{};
            tile.x = static_cast<std::uint32_t>(x0);
            tile.y = static_cast<std::uint32_t>(y0);
            tile.width = static_cast<std::uint32_t>(w);
            tile.height = static_cast<std::uint32_t>(h);
            tile.rgb = cam.data();
            tile.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
            if (!digest.add_tile(tile)) return false;
        }
    }
    return digest.finalize(out);
}

proj::ProjectionMetadata metadata_for(const digest_v01::Sha256& hash) {
    proj::ProjectionMetadata p{};
    p.sourceSeal.sha256[0] = 0x42u;
    p.sourceSeal.byteLength = 123456u;
    p.sourceSeal.sourceEvidenceId = "sha256:test-source-evidence";
    p.color.authority = scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    p.color.sourceEvidenceId = p.sourceSeal.sourceEvidenceId;
    p.color.bindingId = "source_metadata_bound:test-dual-resolved";
    p.color.cameraToXyzD50 = {0.5f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,2.f};
    p.color.normalized = true;
    p.color.validated = true;
    p.color.physicalFrameCount = 1u;
    p.color.independentEvidenceCount = 1u;
    p.expectedScientificMasterHash = hash;
    p.asShotWhiteX = 0.3127;
    p.asShotWhiteY = 0.3290;
    p.sourceDisplayName = "synthetic.dng";
    return p;
}

int verify_common_dng(const MemorySink& sink, std::uint16_t photometric,
                      std::uint16_t samples, std::uint32_t expectedTiles) {
    CHECK(sink.bytes.size() > 8u);
    CHECK(sink.bytes[0] == 'I' && sink.bytes[1] == 'I');
    CHECK(u16(sink.bytes, 2u) == 42u);
    TagView t{};
    CHECK(tag(sink.bytes, 262u, t));
    CHECK(u16(sink.bytes, t.data) == photometric);
    CHECK(tag(sink.bytes, 277u, t));
    CHECK(u16(sink.bytes, t.data) == samples);
    CHECK(tag(sink.bytes, 324u, t));
    CHECK(t.count == expectedTiles);
    CHECK(tag(sink.bytes, 325u, t));
    CHECK(t.count == expectedTiles);
    CHECK(tag(sink.bytes, 50706u, t));
    CHECK(t.count == 4u);
    CHECK(sink.bytes[t.data] == 1u && sink.bytes[t.data+1] == 4u);
    CHECK(tag(sink.bytes, 50707u, t));
    CHECK(sink.bytes[t.data] == 1u && sink.bytes[t.data+1] == 4u);
    CHECK(tag(sink.bytes, 50721u, t));
    CHECK(t.type == 10u && t.count == 9u);
    CHECK(tag(sink.bytes, 50729u, t));
    CHECK(t.type == 5u && t.count == 2u);
    CHECK(tag(sink.bytes, 50740u, t));
    CHECK(t.count > 32u);
    const std::string needle = "projection_is_evidence=false";
    CHECK(std::search(sink.bytes.begin() + static_cast<std::ptrdiff_t>(t.data),
                      sink.bytes.begin() + static_cast<std::ptrdiff_t>(t.data + t.count),
                      needle.begin(), needle.end()) !=
          sink.bytes.begin() + static_cast<std::ptrdiff_t>(t.data + t.count));
    return 0;
}

} // namespace

int main() {
    MeasuredPreservingBackend backend;

    // LinearRaw: exact camera-native float32 Scientific Master samples.
    FakeSource linearSource(CfaPattern::BGGR);
    digest_v01::Sha256 linearHash{};
    CHECK(compute_master_hash(linearSource, backend, linearHash));
    auto linearMeta = metadata_for(linearHash);
    MemorySink linearSink;
    proj::Result linearResult{};
    const auto linearStatus = proj::write_linear_scientific_master_dng(
        linearSource, backend, linearMeta, linearSink, linearResult);
    CHECK(static_cast<bool>(linearStatus));
    CHECK(linearResult.scientificMasterMatched);
    CHECK(linearResult.observedScientificMasterHash == linearHash);
    CHECK(!linearResult.fullFrameMaterialized);
    CHECK(!linearResult.projectionIsEvidence);
    CHECK(!linearResult.colorAuthorityPromoted);
    CHECK(linearResult.physicalFrameCount == 1u);
    CHECK(linearResult.independentEvidenceCount == 1u);
    CHECK(linearResult.tilesWritten == 2u);
    CHECK(verify_common_dng(linearSink, 34892u, 3u, 2u) == 0);

    TagView offsets{};
    CHECK(tag(linearSink.bytes, 324u, offsets));
    const std::uint32_t off0 = u32(linearSink.bytes, offsets.data);
    const std::uint32_t off1 = u32(linearSink.bytes, offsets.data + 4u);
    CHECK(off1 - off0 == 64u * 64u * 3u * 4u);
    for (int y = 0; y < 50; ++y) {
        for (int x = 0; x < 66; ++x) {
            const int tileX = x / 64;
            const int localX = x & 63;
            const int localY = y & 63;
            const std::uint32_t base = tileX == 0 ? off0 : off1;
            for (int c = 0; c < 3; ++c) {
                const std::size_t sample =
                    (static_cast<std::size_t>(localY) * 64u +
                     static_cast<std::size_t>(localX)) * 3u +
                    static_cast<std::size_t>(c);
                const float actual = f32(linearSink.bytes,
                    static_cast<std::size_t>(base) + sample * 4u);
                const float expected = expected_camera_sample(
                    CfaPattern::BGGR, 66, x, y, c);
                CHECK(std::memcmp(&actual, &expected, sizeof(float)) == 0);
            }
        }
    }

    // CFA projection: verify all Bayer phases and exact measured component.
    const std::array<CfaPattern,4> patterns = {
        CfaPattern::BGGR, CfaPattern::RGGB, CfaPattern::GRBG, CfaPattern::GBRG};
    const std::array<std::array<std::uint8_t,4>,4> expectedPatterns = {{
        {{2u,1u,1u,0u}}, {{0u,1u,1u,2u}},
        {{1u,0u,2u,1u}}, {{1u,2u,0u,1u}}
    }};

    for (std::size_t pi = 0; pi < patterns.size(); ++pi) {
        FakeSource cfaSource(patterns[pi]);
        digest_v01::Sha256 cfaHash{};
        CHECK(compute_master_hash(cfaSource, backend, cfaHash));
        auto cfaMeta = metadata_for(cfaHash);
        MemorySink cfaSink;
        proj::Result cfaResult{};
        const auto cfaStatus = proj::write_measured_preserving_cfa_dng(
            cfaSource, backend, cfaMeta, cfaSink, cfaResult);
        CHECK(static_cast<bool>(cfaStatus));
        CHECK(cfaResult.scientificMasterMatched);
        CHECK(cfaResult.observedScientificMasterHash == cfaHash);
        CHECK(!cfaResult.projectionIsEvidence);
        CHECK(verify_common_dng(cfaSink, 32803u, 1u, 2u) == 0);

        TagView repeat{};
        TagView pattern{};
        TagView plane{};
        CHECK(tag(cfaSink.bytes, 33421u, repeat));
        CHECK(repeat.count == 2u && u16(cfaSink.bytes, repeat.data) == 2u &&
              u16(cfaSink.bytes, repeat.data + 2u) == 2u);
        CHECK(tag(cfaSink.bytes, 33422u, pattern));
        CHECK(pattern.count == 4u);
        for (std::size_t i = 0; i < 4u; ++i) {
            CHECK(cfaSink.bytes[pattern.data + i] == expectedPatterns[pi][i]);
        }
        CHECK(tag(cfaSink.bytes, 50710u, plane));
        CHECK(plane.count == 3u && cfaSink.bytes[plane.data] == 0u &&
              cfaSink.bytes[plane.data+1u] == 1u && cfaSink.bytes[plane.data+2u] == 2u);

        TagView cfaOffsets{};
        CHECK(tag(cfaSink.bytes, 324u, cfaOffsets));
        const std::uint32_t cfaOff0 = u32(cfaSink.bytes, cfaOffsets.data);
        const std::uint32_t cfaOff1 = u32(cfaSink.bytes, cfaOffsets.data + 4u);
        CHECK(cfaOff1 - cfaOff0 == 64u * 64u * 4u);
        for (int y = 0; y < 50; ++y) {
            for (int x = 0; x < 66; ++x) {
                const std::uint32_t base = x < 64 ? cfaOff0 : cfaOff1;
                const std::size_t sample =
                    static_cast<std::size_t>(y) * 64u + static_cast<std::size_t>(x & 63);
                const float actual = f32(cfaSink.bytes,
                    static_cast<std::size_t>(base) + sample * 4u);
                const float expected = stage2_value(66, x, y);
                CHECK(std::memcmp(&actual, &expected, sizeof(float)) == 0);
            }
        }
    }

    // A wrong finalized master identity must fail closed after the streamed
    // bytes are produced; the Android integration must delete/abandon such a
    // destination rather than surface it as a successful export.
    FakeSource mismatchSource(CfaPattern::BGGR);
    digest_v01::Sha256 mismatchHash{};
    CHECK(compute_master_hash(mismatchSource, backend, mismatchHash));
    auto mismatchMeta = metadata_for(mismatchHash);
    mismatchMeta.expectedScientificMasterHash[0] ^= 0x01u;
    MemorySink mismatchSink;
    proj::Result mismatchResult{};
    const auto mismatchStatus = proj::write_linear_scientific_master_dng(
        mismatchSource, backend, mismatchMeta, mismatchSink, mismatchResult);
    CHECK(!static_cast<bool>(mismatchStatus));
    CHECK(mismatchStatus.code == proj::StatusCode::ScientificMasterMismatch);
    CHECK(!mismatchResult.scientificMasterMatched);

    // Authority cannot be created by the exporter.
    FakeSource unauthorizedSource(CfaPattern::BGGR);
    digest_v01::Sha256 unauthorizedHash{};
    CHECK(compute_master_hash(unauthorizedSource, backend, unauthorizedHash));
    auto unauthorizedMeta = metadata_for(unauthorizedHash);
    unauthorizedMeta.color.authority =
        scientific_preview_binding_v0_1::ColorBindingAuthority::PreviewSentinel;
    MemorySink unauthorizedSink;
    proj::Result unauthorizedResult{};
    const auto unauthorizedStatus = proj::write_linear_scientific_master_dng(
        unauthorizedSource, backend, unauthorizedMeta, unauthorizedSink, unauthorizedResult);
    CHECK(!static_cast<bool>(unauthorizedStatus));
    CHECK(unauthorizedStatus.code == proj::StatusCode::InvalidAuthority);
    CHECK(unauthorizedSink.bytes.empty());

    std::cout << "DNG_COMPATIBILITY_PROJECTION_V0_1_PASS\n";
    std::cout << "linear_master_exact=1\n";
    std::cout << "cfa_measured_component_exact=1\n";
    std::cout << "bayer_patterns_tested=4\n";
    std::cout << "master_mismatch_rejected=1\n";
    std::cout << "authority_promotion=0\n";
    std::cout << "full_frame_materialized=0\n";
    return 0;
}

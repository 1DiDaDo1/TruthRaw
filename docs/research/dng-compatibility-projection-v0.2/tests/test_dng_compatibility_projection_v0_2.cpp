#include "dng_compatibility_projection_v0_2.h"
#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using namespace truthraw;
namespace proj1 = truthraw::dng_compatibility_projection::v0_1;
namespace proj2 = truthraw::dng_compatibility_projection::v0_2;
namespace digest_v01 = truthraw::scientific_master_digest::v0_1;

#define CHECK(expr) do { if (!(expr)) { \
    std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; \
} } while (0)

namespace {

int measured_channel(CfaPattern cfa, int x, int y) {
    const int p = ((y & 1) << 1) | (x & 1);
    constexpr int bggr[4] = {2,1,1,0};
    constexpr int rggb[4] = {0,1,1,2};
    constexpr int grbg[4] = {1,0,2,1};
    constexpr int gbrg[4] = {1,2,0,1};
    switch (cfa) {
        case CfaPattern::BGGR: return bggr[p];
        case CfaPattern::RGGB: return rggb[p];
        case CfaPattern::GRBG: return grbg[p];
        case CfaPattern::GBRG: return gbrg[p];
    }
    return 1;
}

std::uint16_t raw_value(int x, int y) {
    return static_cast<std::uint16_t>(1000 + y * 31 + x * 7);
}

class FakeSource final : public streaming_v0_1::IRawTileSource {
public:
    FakeSource() {
        meta_.width = 8;
        meta_.height = 8;
        meta_.cfa = CfaPattern::BGGR;
        meta_.orientation = Orientation::Normal;
        meta_.whiteLevel = 65535.0f;
        meta_.blackPhase = {0.f,0.f,0.f,0.f};
        meta_.hasGainField = false;
        meta_.hasResidualBlack = false;
        meta_.sourceId = "sha256:v02-fixed-d50-test";
    }

    const DngMetadata& metadata() const override { return meta_; }
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }

    streaming_v0_1::StreamStatus readRawTile(
        const TileRect& rect, std::uint16_t* rawOut, std::size_t rawCount,
        float* gainOut, std::size_t gainCount) override {
        const int w = rect.hx1 - rect.hx0;
        const int h = rect.hy1 - rect.hy0;
        const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        if (!rawOut || rawCount != expected || gainOut != nullptr || gainCount != 0u) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument, "bad fake read buffer");
        }
        for (int yy = 0; yy < h; ++yy) {
            for (int xx = 0; xx < w; ++xx) {
                rawOut[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) +
                       static_cast<std::size_t>(xx)] =
                    raw_value(rect.hx0 + xx, rect.hy0 + yy);
            }
        }
        return streaming_v0_1::StreamStatus::ok();
    }

    streaming_v0_1::StreamStatus readRowBias(int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::error(
            streaming_v0_1::StreamStatusCode::InvalidArgument, "row bias not expected");
    }
    streaming_v0_1::StreamStatus readColBias(int, int, float*, std::size_t) override {
        return streaming_v0_1::StreamStatus::error(
            streaming_v0_1::StreamStatusCode::InvalidArgument, "col bias not expected");
    }

private:
    DngMetadata meta_{};
};

class MemorySink final : public proj2::ISequentialByteSink {
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
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o + 1]) << 8u);
}

std::uint32_t u32(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint32_t>(b[o]) |
        (static_cast<std::uint32_t>(b[o + 1]) << 8u) |
        (static_cast<std::uint32_t>(b[o + 2]) << 16u) |
        (static_cast<std::uint32_t>(b[o + 3]) << 24u);
}

std::int32_t i32(const std::vector<std::uint8_t>& b, std::size_t o) {
    const std::uint32_t bits = u32(b, o);
    std::int32_t value = 0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
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

double rational(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<double>(u32(b, o)) / static_cast<double>(u32(b, o + 4u));
}

double srational(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<double>(i32(b, o)) / static_cast<double>(i32(b, o + 4u));
}

bool compute_master_hash(FakeSource& source,
                         ResearchEdgeAwareMeasuredPreservingReconstruction& backend,
                         digest_v01::Sha256& out) {
    constexpr int w = 8;
    constexpr int h = 8;
    std::vector<std::uint16_t> raw(static_cast<std::size_t>(w * h));
    TileRect rect{0,0,w,h,0,0,w,h};
    if (!source.readRawTile(rect, raw.data(), raw.size(), nullptr, 0u)) return false;
    std::vector<float> stage2(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        stage2[i] = static_cast<float>(raw[i]) / 65535.0f;
    }
    std::vector<float> cam(3u * raw.size());
    if (!backend.reconstructTile(stage2.data(), w, h, 0, 0,
                                 0, 0, w, h, CfaPattern::BGGR, cam.data())) return false;

    digest_v01::ScientificMasterDigestAccumulator digest(w, h);
    if (!digest.valid()) return false;
    digest_v01::TileView tile{};
    tile.x = 0; tile.y = 0; tile.width = w; tile.height = h;
    tile.rgb = cam.data();
    tile.rowStrideSamples = static_cast<std::size_t>(w) * 3u;
    return digest.add_tile(tile) && digest.finalize(out);
}

proj2::ProjectionMetadata metadata_for(const digest_v01::Sha256& hash) {
    proj2::ProjectionMetadata p{};
    p.sourceSeal.sha256[0] = 0x42u;
    p.sourceSeal.byteLength = 4096u;
    p.sourceSeal.sourceEvidenceId = "sha256:v02-fixed-d50-test";
    p.color.authority = scientific_preview_binding_v0_1::ColorBindingAuthority::SourceMetadataBound;
    p.color.sourceEvidenceId = p.sourceSeal.sourceEvidenceId;
    p.color.bindingId = "source_metadata_bound:v02-fixed-d50-test";
    p.color.cameraToXyzD50 = {0.5f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,2.f};
    p.color.normalized = true;
    p.color.validated = true;
    p.color.physicalFrameCount = 1u;
    p.color.independentEvidenceCount = 1u;
    p.expectedScientificMasterHash = hash;
    p.sourceResolvedWhiteX = 0.3127; // deliberately not D50
    p.sourceResolvedWhiteY = 0.3290;
    p.sourceResolvedWhiteTemperatureK = 6500.0;
    p.sourceDisplayName = "synthetic.dng";
    return p;
}

} // namespace

int main() {
    FakeSource source;
    ResearchEdgeAwareMeasuredPreservingReconstruction backend;
    digest_v01::Sha256 master{};
    CHECK(compute_master_hash(source, backend, master));
    auto meta = metadata_for(master);

    MemorySink linearSink;
    proj2::Result linear{};
    const auto linearStatus = proj2::write_linear_scientific_master_dng(
        source, backend, meta, linearSink, linear);
    CHECK(static_cast<bool>(linearStatus));
    CHECK(linear.writer.scientificMasterMatched);
    CHECK(linear.writer.observedScientificMasterHash == master);
    CHECK(linear.fixedD50CompatibilityWhite);
    CHECK(!linear.sourceResolvedWhiteRetainedAsProvenance);
    CHECK(!linear.projectionIsEvidence);
    CHECK(!linear.colorAuthorityPromoted);

    // The source white was D65-ish, but the compatibility profile MUST advertise
    // D50 because cameraToXyzD50 is already the finalized as-shot D50 transform.
    TagView white{};
    CHECK(tag(linearSink.bytes, 50729u, white));
    CHECK(white.type == 5u && white.count == 2u);
    const double outX = rational(linearSink.bytes, white.data);
    const double outY = rational(linearSink.bytes, white.data + 8u);
    CHECK(std::abs(outX - proj2::kCompatibilityD50X) < 1e-9);
    CHECK(std::abs(outY - proj2::kCompatibilityD50Y) < 1e-9);
    CHECK(std::abs(outX - meta.sourceResolvedWhiteX) > 1e-3);

    // ColorMatrix1 is XYZ->camera. With D50 advertised as the as-shot white,
    // the DNG no-ForwardMatrix chromatic-adaptation leg is D50->D50 identity.
    // Therefore Inverse(ColorMatrix1) must recover the exact finalized
    // cameraToXyzD50 transform up to the writer's 1e-6 SRATIONAL quantization.
    TagView cm{};
    CHECK(tag(linearSink.bytes, 50721u, cm));
    CHECK(cm.type == 10u && cm.count == 9u);
    CHECK(std::abs(srational(linearSink.bytes, cm.data + 0u * 8u) - 2.0) < 1e-6);
    CHECK(std::abs(srational(linearSink.bytes, cm.data + 4u * 8u) - 1.0) < 1e-6);
    CHECK(std::abs(srational(linearSink.bytes, cm.data + 8u * 8u) - 0.5) < 1e-6);

    // The derived CFA route is only exposed with the concrete validated
    // measured-preserving backend and is still explicitly non-evidence.
    FakeSource cfaSource;
    MemorySink cfaSink;
    proj2::Result cfa{};
    const auto cfaStatus = proj2::write_reconstructed_cfa_dng(
        cfaSource, backend, meta, cfaSink, cfa);
    CHECK(static_cast<bool>(cfaStatus));
    CHECK(cfa.role == proj2::ProjectionRole::ReconstructedCfa);
    CHECK(cfa.writer.scientificMasterMatched);
    CHECK(!cfa.projectionIsEvidence);
    CHECK(!cfa.colorAuthorityPromoted);

    // Invalid source-white provenance is rejected before a DNG is emitted.
    auto bad = meta;
    bad.sourceResolvedWhiteX = 0.0;
    FakeSource badSource;
    MemorySink badSink;
    proj2::Result badResult{};
    const auto badStatus = proj2::write_linear_scientific_master_dng(
        badSource, backend, bad, badSink, badResult);
    CHECK(!static_cast<bool>(badStatus));
    CHECK(badStatus.code == proj2::StatusCode::InvalidWhitePoint);
    CHECK(badSink.bytes.empty());

    std::cout << "DNG_COMPATIBILITY_PROJECTION_V0_2_PASS\n";
    std::cout << "fixed_d50_as_shot_profile=1\n";
    std::cout << "double_chromatic_adaptation_prevented=1\n";
    std::cout << "linear_master_digest_match=1\n";
    std::cout << "reconstructed_cfa_is_evidence=0\n";
    std::cout << "color_authority_promoted=0\n";
    return 0;
}

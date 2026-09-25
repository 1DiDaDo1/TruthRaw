#include "free_world_scientific_open_scene_binding_v0_3.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace binding = truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
namespace master = truthraw::scientific_master_linear_dng_projection::v0_1;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

class SyntheticMaster final : public master::IScientificMasterTileSource {
public:
    SyntheticMaster(std::uint32_t w, std::uint32_t h)
        : width_(w), height_(h) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    static std::array<float,3> pixel(std::uint32_t x, std::uint32_t y) {
        return {
            -0.2f + 0.01f * static_cast<float>(x) +
                0.005f * static_cast<float>(y),
            0.3f + 0.008f * static_cast<float>(x) +
                0.003f * static_cast<float>(y),
            1.1f + 0.006f * static_cast<float>(x) +
                0.004f * static_cast<float>(y),
        };
    }

    master::Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override {
        if (!rgb || x >= width_ || y >= height_ ||
            (x % master::kCanonicalTileEdge) != 0u ||
            (y % master::kCanonicalTileEdge) != 0u) {
            return master::Status::error(
                master::StatusCode::InvalidArgument, "bad master tile");
        }
        const auto expectedWidth =
            std::min(master::kCanonicalTileEdge, width_ - x);
        const auto expectedHeight =
            std::min(master::kCanonicalTileEdge, height_ - y);
        if (width != expectedWidth || height != expectedHeight ||
            floatCount != static_cast<std::size_t>(width) * height * 3u) {
            return master::Status::error(
                master::StatusCode::InvalidArgument, "bad master extent");
        }

        for (std::uint32_t yy = 0u; yy < height; ++yy) {
            for (std::uint32_t xx = 0u; xx < width; ++xx) {
                const auto p = pixel(x + xx, y + yy);
                const std::size_t i =
                    (static_cast<std::size_t>(yy) * width + xx) * 3u;
                rgb[i + 0u] = p[0];
                rgb[i + 1u] = p[1];
                rgb[i + 2u] = p[2];
            }
        }
        return master::Status::ok();
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
};

class SyntheticOpenScene final : public local::IFieldTileSource {
public:
    SyntheticOpenScene(std::uint32_t w, std::uint32_t h, bool mutateValue)
        : width_(w), height_(h), mutateValue_(mutateValue) {}

    local::Geometry geometry() const noexcept override {
        return {width_, height_, width_ * 4u, height_ * 4u};
    }

    bool readSourceTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        field::ChannelRecord* out,
        std::size_t recordCount) noexcept override {
        if (!out || x >= width_ || y >= height_ ||
            (x % field::kCanonicalTileEdge) != 0u ||
            (y % field::kCanonicalTileEdge) != 0u) {
            return false;
        }
        const auto expectedWidth =
            std::min(field::kCanonicalTileEdge, width_ - x);
        const auto expectedHeight =
            std::min(field::kCanonicalTileEdge, height_ - y);
        if (width != expectedWidth || height != expectedHeight) return false;

        const std::size_t pixels =
            static_cast<std::size_t>(width) * height;
        if (recordCount != pixels * 3u) return false;

        std::vector<std::uint16_t> raw(pixels, 100u);
        if (x == 0u && y == 0u && !raw.empty()) raw[0] = 1023u;

        std::vector<float> rgb(pixels * 3u);
        for (std::uint32_t yy = 0u; yy < height; ++yy) {
            for (std::uint32_t xx = 0u; xx < width; ++xx) {
                const auto p = SyntheticMaster::pixel(x + xx, y + yy);
                const std::size_t i =
                    (static_cast<std::size_t>(yy) * width + xx) * 3u;
                rgb[i + 0u] = p[0];
                rgb[i + 1u] = p[1];
                rgb[i + 2u] = p[2];
            }
        }

        std::vector<field::ChannelRecord> records;
        if (!field::build_source_tile_records(
                truthraw::CfaPattern::BGGR,
                x, y, width, height,
                raw, 1023.0f, rgb, records)) {
            return false;
        }
        if (mutateValue_ && !records.empty()) {
            records[0].value =
                std::bit_cast<float>(
                    std::bit_cast<std::uint32_t>(records[0].value) ^ 0x1u);
        }
        std::copy(records.begin(), records.end(), out);
        return true;
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
    bool mutateValue_;
};

void test_exact_master_open_scene_binding() {
    constexpr std::uint32_t w = 8u;
    constexpr std::uint32_t h = 6u;
    SyntheticMaster masterSource(w, h);
    SyntheticOpenScene fieldSource(w, h, false);

    binding::BoundScenePlane scene(masterSource, fieldSource, w, h);
    REQUIRE(scene.valid());

    fw::SourcePixel p{};
    REQUIRE(scene.readPixel(0u, 0u, p));

    REQUIRE(p.channel[2].role == fw::SourceCreationRole::SourceMeasuredCfa);
    REQUIRE(p.channel[2].authority == fw::SourceAuthority::Censored);
    REQUIRE(p.channel[2].boundKnown);
    REQUIRE(p.channel[2].boundDomain == fw::BoundDomain::SourceRawCode);
    REQUIRE((p.channel[2].contributionMask & field::ContributionMeasured) != 0u);
    REQUIRE((p.channel[2].contributionMask & field::ContributionCensored) != 0u);

    REQUIRE(p.channel[0].role ==
        fw::SourceCreationRole::ScientificReconstruction);
    REQUIRE(p.channel[0].authority == fw::SourceAuthority::Unknown);
    REQUIRE(p.channel[1].authority == fw::SourceAuthority::Unknown);

    const auto report = scene.report();
    REQUIRE(report.tileLoads == 1u);
    REQUIRE(report.channelRecordsValidated ==
        static_cast<std::uint64_t>(w) * h * 3u);
    REQUIRE(report.masterFieldValueBitMatches ==
        report.channelRecordsValidated);
    REQUIRE(report.masterFieldValueBitMismatches == 0u);
    REQUIRE(report.fieldSchemaValidated);
    REQUIRE(report.valueIdentityVerifiedForLoadedRecords);
    REQUIRE(!report.createsNewEvidence);
    REQUIRE(!report.scientificWritebackAllowed);
    REQUIRE(report.physicalFrameCount == 1u);
    REQUIRE(report.independentEvidenceCount == 1u);
}

void test_free_world_resolver_consumes_bound_scene() {
    constexpr std::uint32_t w = 8u;
    constexpr std::uint32_t h = 6u;
    SyntheticMaster masterSource(w, h);
    SyntheticOpenScene fieldSource(w, h, false);
    binding::BoundScenePlane scene(masterSource, fieldSource, w, h);
    REQUIRE(scene.valid());

    fw::Resolver resolver(scene, w * 2u, h * 2u);
    REQUIRE(resolver.valid());
    fw::ResolvedPixel p{};
    REQUIRE(resolver.resolvePixel(1u, 1u, p));

    REQUIRE(p.measuredTargetClaimCount == 0u);
    REQUIRE(!p.createsNewEvidence);
    REQUIRE(p.physicalFrameCount == 1u);
    REQUIRE(p.independentEvidenceCount == 1u);

    // Current Open Scene v0.85 source policy leaves reconstructed RGB
    // channels UNKNOWN without admitted reconstruction uncertainty. Area
    // integration must not promote them.
    REQUIRE(p.support[0].authority == fw::ResolvedAuthority::Unknown);
    REQUIRE(p.support[1].authority == fw::ResolvedAuthority::Unknown);
    REQUIRE(p.support[2].authority == fw::ResolvedAuthority::Unknown);
    REQUIRE(p.support[2].sourceMeasuredCfaWeight > 0.0);
    REQUIRE(p.support[2].scientificReconstructionWeight > 0.0);
    REQUIRE((p.support[2].contributionMask &
             field::ContributionCensored) != 0u);
    REQUIRE(!p.support[2].boundKnown);
}

void test_value_identity_mismatch_fails_closed() {
    constexpr std::uint32_t w = 8u;
    constexpr std::uint32_t h = 6u;
    SyntheticMaster masterSource(w, h);
    SyntheticOpenScene fieldSource(w, h, true);
    binding::BoundScenePlane scene(masterSource, fieldSource, w, h);
    REQUIRE(scene.valid());

    fw::SourcePixel p{};
    REQUIRE(!scene.readPixel(0u, 0u, p));
    const auto report = scene.report();
    REQUIRE(report.masterFieldValueBitMismatches == 1u);
    REQUIRE(!report.valueIdentityVerifiedForLoadedRecords);
}

void test_geometry_mismatch_fails_closed() {
    SyntheticMaster masterSource(8u, 6u);
    SyntheticOpenScene fieldSource(8u, 6u, false);
    binding::BoundScenePlane bad(masterSource, fieldSource, 7u, 6u);
    REQUIRE(!bad.valid());
}

}  // namespace

int main() {
    test_exact_master_open_scene_binding();
    test_free_world_resolver_consumes_bound_scene();
    test_value_identity_mismatch_fails_closed();
    test_geometry_mismatch_fails_closed();

    std::cout << "FreeWorldScientificOpenSceneBinding/0.3 PASS\n";
    std::cout << "binding_id=" << binding::kBindingId << "\n";
    std::cout << "scientific_writeback_allowed=0\n";
    std::cout << "creates_new_evidence=0\n";
    return 0;
}

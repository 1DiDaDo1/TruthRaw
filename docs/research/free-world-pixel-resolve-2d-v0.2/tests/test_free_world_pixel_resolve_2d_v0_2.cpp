#include "free_world_pixel_resolve_2d_v0_2.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

bool near(double a, double b, double eps = 1e-12) {
    return std::abs(a - b) <= eps;
}

class SyntheticScene final : public fw::IScenePlaneSource {
public:
    SyntheticScene(std::uint32_t width, std::uint32_t height)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * height) {
        for (std::uint32_t y = 0u; y < height_; ++y) {
            for (std::uint32_t x = 0u; x < width_; ++x) {
                auto& p = at(x, y);
                p.channel[0].value =
                    -0.25 + 0.05 * static_cast<double>(x) +
                    0.03 * static_cast<double>(y);
                p.channel[1].value =
                    0.20 + 0.04 * static_cast<double>(x) +
                    0.02 * static_cast<double>(y);
                p.channel[2].value =
                    0.80 + 0.07 * static_cast<double>(x) +
                    0.01 * static_cast<double>(y);
                for (auto& c : p.channel) {
                    c.role = fw::SourceCreationRole::SourceMeasuredCfa;
                    c.authority = fw::SourceAuthority::CalibratedEstimate;
                    c.uncertaintyKnown = true;
                    c.p95Uncertainty = 0.01;
                }
            }
        }
    }

    std::uint32_t width() const noexcept override { return width_; }
    std::uint32_t height() const noexcept override { return height_; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= width_ || y >= height_) return false;
        out = pixels_[static_cast<std::size_t>(y) * width_ + x];
        return true;
    }

    fw::SourcePixel& at(std::uint32_t x, std::uint32_t y) {
        return pixels_[static_cast<std::size_t>(y) * width_ + x];
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
    std::vector<fw::SourcePixel> pixels_;
};

class ConstantScene final : public fw::IScenePlaneSource {
public:
    ConstantScene(std::uint32_t w, std::uint32_t h)
        : width_(w), height_(h) {}

    std::uint32_t width() const noexcept override { return width_; }
    std::uint32_t height() const noexcept override { return height_; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= width_ || y >= height_) return false;
        const std::array<double, 3u> values = {-0.5, 0.5, 1.5};
        for (std::size_t c = 0; c < 3u; ++c) {
            out.channel[c].value = values[c];
            out.channel[c].role =
                fw::SourceCreationRole::SourceMeasuredCfa;
            out.channel[c].authority =
                fw::SourceAuthority::CalibratedEstimate;
            out.channel[c].uncertaintyKnown = true;
            out.channel[c].p95Uncertainty = 0.125;
        }
        return true;
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
};

void test_axis_weights_normalized_and_bounded() {
    for (std::uint32_t targetCount : {2u, 4u, 7u, 16u}) {
        for (std::uint32_t target = 0u; target < targetCount; ++target) {
            const auto weights = fw::axisAreaWeights(5u, target, targetCount);
            REQUIRE(!weights.empty());
            double sum = 0.0;
            for (const auto& item : weights) {
                REQUIRE(item.index < 5u);
                REQUIRE(item.weight > 0.0);
                REQUIRE(std::isfinite(item.weight));
                sum += item.weight;
            }
            REQUIRE(near(sum, 1.0));
        }
    }
}

void test_constant_field_is_resolution_independent() {
    ConstantScene source(5u, 4u);
    for (const auto dims : {
            std::array<std::uint32_t, 2u>{1u, 1u},
            std::array<std::uint32_t, 2u>{3u, 2u},
            std::array<std::uint32_t, 2u>{5u, 4u},
            std::array<std::uint32_t, 2u>{20u, 16u}}) {
        fw::Resolver resolver(source, dims[0], dims[1]);
        REQUIRE(resolver.valid());
        std::vector<fw::ResolvedPixel> raster;
        REQUIRE(resolver.resolveRaster(raster, false));
        REQUIRE(raster.size() ==
            static_cast<std::size_t>(dims[0]) * dims[1]);
        for (const auto& p : raster) {
            REQUIRE(near(p.sceneLinear[0], -0.5));
            REQUIRE(near(p.sceneLinear[1], 0.5));
            REQUIRE(near(p.sceneLinear[2], 1.5));
            REQUIRE(p.measuredTargetClaimCount == 0u);
            REQUIRE(!p.createsNewEvidence);
            REQUIRE(p.physicalFrameCount == 1u);
            REQUIRE(p.independentEvidenceCount == 1u);
            for (const auto& s : p.support) {
                REQUIRE(s.authority == fw::ResolvedAuthority::Reconstructed);
                REQUIRE(near(s.calibratedEstimateWeight, 1.0));
                REQUIRE(near(s.p95Uncertainty, 0.125));
            }
        }
    }
}

void test_area_integration_matches_affine_interior_average() {
    SyntheticScene source(7u, 7u);
    fw::Resolver resolver(source, 7u, 7u);
    REQUIRE(resolver.valid());

    fw::ResolvedPixel p{};
    REQUIRE(resolver.resolvePixel(3u, 3u, p));

    // For an affine field and a symmetric unit-width footprint around an
    // interior source centre, the bilinear continuous field average equals
    // the value at that centre exactly.
    fw::SourcePixel centre{};
    REQUIRE(source.readPixel(3u, 3u, centre));
    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(near(p.sceneLinear[c], centre.channel[c].value));
    }

    double footprintSum = 0.0;
    for (const auto& f : p.footprint) {
        REQUIRE(f.weight > 0.0);
        footprintSum += f.weight;
    }
    REQUIRE(near(footprintSum, 1.0));
    REQUIRE(p.footprint.size() == 9u);
}

void test_unknown_and_censored_authority_propagate_fail_closed() {
    SyntheticScene source(4u, 4u);
    // Central target pixel at this geometry overlaps neighbours with
    // positive tent-basis support.
    source.at(1u, 1u).channel[0].authority =
        fw::SourceAuthority::Censored;
    source.at(2u, 1u).channel[1].authority =
        fw::SourceAuthority::Unknown;

    fw::Resolver resolver(source, 4u, 4u);
    fw::ResolvedPixel p{};
    REQUIRE(resolver.resolvePixel(1u, 1u, p));

    REQUIRE(p.support[0].authority == fw::ResolvedAuthority::Censored);
    REQUIRE(p.support[0].censoredWeight > 0.0);
    REQUIRE(p.support[1].authority == fw::ResolvedAuthority::Unknown);
    REQUIRE(p.support[1].unknownWeight > 0.0);
    REQUIRE(p.support[2].authority == fw::ResolvedAuthority::Reconstructed);
}

void test_uncertainty_is_weighted_without_promotion() {
    SyntheticScene source(3u, 3u);
    source.at(1u, 1u).channel[0].p95Uncertainty = 0.50;
    source.at(0u, 0u).channel[1].uncertaintyKnown = false;

    fw::Resolver resolver(source, 3u, 3u);
    fw::ResolvedPixel centre{};
    REQUIRE(resolver.resolvePixel(1u, 1u, centre));
    REQUIRE(centre.support[0].uncertaintyKnown);
    REQUIRE(centre.support[0].p95Uncertainty > 0.01);
    REQUIRE(centre.support[0].p95Uncertainty < 0.50);

    fw::ResolvedPixel corner{};
    REQUIRE(resolver.resolvePixel(0u, 0u, corner));
    REQUIRE(!corner.support[1].uncertaintyKnown);
}

void test_pixel_density_does_not_upgrade_authority() {
    ConstantScene source(6u, 5u);
    for (const auto dims : {
            std::array<std::uint32_t, 2u>{3u, 2u},
            std::array<std::uint32_t, 2u>{6u, 5u},
            std::array<std::uint32_t, 2u>{24u, 20u}}) {
        fw::Resolver resolver(source, dims[0], dims[1]);
        std::vector<fw::ResolvedPixel> pixels;
        REQUIRE(resolver.resolveRaster(pixels, false));
        for (const auto& p : pixels) {
            for (const auto& s : p.support) {
                REQUIRE(s.authority == fw::ResolvedAuthority::Reconstructed);
            }
            REQUIRE(p.measuredTargetClaimCount == 0u);
        }
    }
}

class SceneLinearCensoredScene final : public fw::IScenePlaneSource {
public:
    std::uint32_t width() const noexcept override { return 3u; }
    std::uint32_t height() const noexcept override { return 3u; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= 3u || y >= 3u) return false;
        for (std::size_t c = 0u; c < 3u; ++c) {
            auto& s = out.channel[c];
            s.value = 2.0 + static_cast<double>(c);
            s.role = fw::SourceCreationRole::SourceMeasuredCfa;
            s.authority = fw::SourceAuthority::Censored;
            s.boundKnown = true;
            s.lowerBound = 1.0 + 0.1 * static_cast<double>(x + y + c);
            s.boundDomain = fw::BoundDomain::SceneLinear;
            s.contributionMask = 0x05u;
        }
        return true;
    }
};

class RawCodeCensoredScene final : public fw::IScenePlaneSource {
public:
    std::uint32_t width() const noexcept override { return 2u; }
    std::uint32_t height() const noexcept override { return 2u; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= 2u || y >= 2u) return false;
        for (auto& s : out.channel) {
            s.value = 1.0;
            s.role = fw::SourceCreationRole::SourceMeasuredCfa;
            s.authority = fw::SourceAuthority::Censored;
            s.boundKnown = true;
            s.lowerBound = 1023.0;
            s.boundDomain = fw::BoundDomain::SourceRawCode;
            s.contributionMask = 0x05u;
        }
        return true;
    }
};

void test_scene_linear_censor_bound_projects_only_in_same_domain() {
    SceneLinearCensoredScene scene;
    fw::Resolver resolver(scene, 6u, 6u);
    fw::ResolvedPixel p{};
    REQUIRE(resolver.resolvePixel(2u, 2u, p));
    for (const auto& s : p.support) {
        REQUIRE(s.authority == fw::ResolvedAuthority::Censored);
        REQUIRE(s.boundKnown);
        REQUIRE(s.boundDomain == fw::BoundDomain::SceneLinear);
        REQUIRE(std::isfinite(s.lowerBound));
        REQUIRE(s.censoredWeight > 0.999999999999);
        REQUIRE((s.contributionMask & 0x04u) != 0u);
    }

    RawCodeCensoredScene rawScene;
    fw::Resolver rawResolver(rawScene, 4u, 4u);
    fw::ResolvedPixel raw{};
    REQUIRE(rawResolver.resolvePixel(1u, 1u, raw));
    for (const auto& s : raw.support) {
        REQUIRE(s.authority == fw::ResolvedAuthority::Censored);
        REQUIRE(!s.boundKnown);
        REQUIRE(s.boundDomain == fw::BoundDomain::None);
    }
}

void test_display_intent_is_separate_from_scene_resolve() {
    ConstantScene source(4u, 4u);
    fw::Resolver resolver(source, 8u, 8u);
    fw::ResolvedPixel before{};
    REQUIRE(resolver.resolvePixel(3u, 3u, before));

    const fw::ViewDisplayPolicy sdr{
        "CAM16_RESEARCH_VIEW_V0",
        "SDR_100_NIT_SRGB"};
    const fw::ViewDisplayPolicy hdr{
        "CAM16_RESEARCH_VIEW_V0",
        "HDR_1000_NIT_BT2100_PQ"};

    const auto a = fw::bindOutputIntent("SCENE_SHA256_TEST", sdr);
    const auto b = fw::bindOutputIntent("SCENE_SHA256_TEST", hdr);
    REQUIRE(a.sceneStateId == b.sceneStateId);
    REQUIRE(a.displayTargetId != b.displayTargetId);
    REQUIRE(!a.scientificSceneMutationAllowed);
    REQUIRE(!b.scientificSceneMutationAllowed);

    fw::ResolvedPixel after{};
    REQUIRE(resolver.resolvePixel(3u, 3u, after));
    for (std::size_t c = 0u; c < 3u; ++c) {
        REQUIRE(near(before.sceneLinear[c], after.sceneLinear[c]));
        REQUIRE(before.support[c].authority == after.support[c].authority);
    }
}

}  // namespace

int main() {
    test_axis_weights_normalized_and_bounded();
    test_constant_field_is_resolution_independent();
    test_area_integration_matches_affine_interior_average();
    test_unknown_and_censored_authority_propagate_fail_closed();
    test_uncertainty_is_weighted_without_promotion();
    test_pixel_density_does_not_upgrade_authority();
    test_scene_linear_censor_bound_projects_only_in_same_domain();
    test_display_intent_is_separate_from_scene_resolve();

    std::cout << "FreeWorldPixelResolve2D/0.2 PASS\n";
    std::cout << "method=" << fw::kMethodId << "\n";
    std::cout << "creation_role=" << fw::kCreationRole << "\n";
    std::cout << "measured_target_claim_count=0\n";
    return 0;
}

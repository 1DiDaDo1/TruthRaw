#include "truthnegative_continuous_v0_5.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

tn::Digest digest(std::uint8_t seed) {
    tn::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

class SyntheticField final : public local::IFieldTileSource {
public:
    SyntheticField(
        std::uint32_t width,
        std::uint32_t height,
        bool mutateAuthority = false)
        : width_(width),
          height_(height),
          mutateAuthority_(mutateAuthority) {}

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
        if (!out ||
            x + width > width_ ||
            y + height > height_ ||
            width == 0u ||
            height == 0u ||
            recordCount !=
                static_cast<std::size_t>(width) * height * 3u) {
            return false;
        }

        for (std::uint32_t yy = 0u; yy < height; ++yy) {
            for (std::uint32_t xx = 0u; xx < width; ++xx) {
                for (std::size_t c = 0u; c < 3u; ++c) {
                    const std::size_t i =
                        (static_cast<std::size_t>(yy) * width + xx) * 3u + c;
                    auto& r = out[i];
                    r.value =
                        static_cast<float>(
                            0.1 * static_cast<double>(c + 1u) +
                            0.01 * static_cast<double>(x + xx) +
                            0.02 * static_cast<double>(y + yy));
                    r.valuePresent = true;
                    r.role =
                        c == 1u
                            ? field::CreationRole::SourceMeasuredCfa
                            : field::CreationRole::ScientificReconstruction;
                    r.authority =
                        c == 1u
                            ? field::Authority::CalibratedEstimate
                            : field::Authority::Unknown;
                    if (c == 1u) {
                        r.supportKnown = true;
                        r.support = 1.0f;
                    }
                    r.uncertainty =
                        field::UncertaintyKnowledge::Unresolved;
                    r.contributionMask =
                        c == 1u
                            ? field::ContributionMeasured
                            : field::ContributionReconstructed |
                                  field::ContributionUnknown;
                }
            }
        }

        if (mutateAuthority_ && x == 0u && y == 0u && recordCount > 0u) {
            out[0].authority = field::Authority::Censored;
            out[0].boundKnown = true;
            out[0].bound = 1.0f;
            out[0].boundDomain = field::BoundDomain::SourceRawCode;
            out[0].contributionMask =
                field::ContributionMeasured | field::ContributionCensored;
        }
        return true;
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
    bool mutateAuthority_;
};

class SyntheticScene final : public fw::IScenePlaneSource {
public:
    SyntheticScene(std::uint32_t width, std::uint32_t height)
        : width_(width), height_(height) {}

    std::uint32_t width() const noexcept override { return width_; }
    std::uint32_t height() const noexcept override { return height_; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= width_ || y >= height_) return false;

        const std::array<double, 3u> base{-0.25, 0.5, 1.25};
        for (std::size_t c = 0u; c < 3u; ++c) {
            auto& s = out.channel[c];
            s.value =
                base[c] +
                0.01 * static_cast<double>(x) +
                0.02 * static_cast<double>(y);
            s.role =
                c == 1u
                    ? fw::SourceCreationRole::SourceMeasuredCfa
                    : fw::SourceCreationRole::ScientificReconstruction;
            s.authority =
                c == 1u
                    ? fw::SourceAuthority::CalibratedEstimate
                    : fw::SourceAuthority::Unknown;
            s.contributionMask =
                c == 1u ? 0x01u : 0x0au;
        }
        return true;
    }

private:
    std::uint32_t width_;
    std::uint32_t height_;
};

tn::State stateFor(
    std::uint32_t width,
    std::uint32_t height,
    const tn::Digest& fieldDigest) {
    tn::StateInput in{};
    in.sourceEvidenceSha256 = digest(1u);
    in.scientificMasterSha256 = digest(40u);
    in.authorityFieldSha256 = fieldDigest;
    in.width = width;
    in.height = height;
    in.reconstructionBackendId =
        "F64_MEASURED_PRESERVING_REFERENCE";
    in.colourBindingId = "SOURCE_METADATA_BOUND_TEST";
    tn::State out{};
    REQUIRE(tn::finalizeState(in, out));
    return out;
}

void test_authority_field_digest_is_deterministic_and_sensitive() {
    SyntheticField a(9u, 7u, false);
    SyntheticField b(9u, 7u, false);
    SyntheticField changed(9u, 7u, true);

    tn::AuthorityFieldSummary sa{};
    tn::AuthorityFieldSummary sb{};
    tn::AuthorityFieldSummary sc{};
    REQUIRE(tn::summarizeAuthorityField(a, sa));
    REQUIRE(tn::summarizeAuthorityField(b, sb));
    REQUIRE(tn::summarizeAuthorityField(changed, sc));

    REQUIRE(sa.contentSha256 == sb.contentSha256);
    REQUIRE(sa.contentSha256 != sc.contentSha256);
    REQUIRE(sa.recordCount == 9u * 7u * 3u);
    REQUIRE(sa.authorityCounts[
        static_cast<std::size_t>(
            static_cast<std::uint8_t>(field::Authority::CalibratedEstimate) - 1u)]
        == 9u * 7u);
    REQUIRE(sa.authorityCounts[
        static_cast<std::size_t>(
            static_cast<std::uint8_t>(field::Authority::Unknown) - 1u)]
        == 9u * 7u * 2u);
    REQUIRE(!sa.createsNewEvidence);
    REQUIRE(!sa.scientificWritebackAllowed);
}

void test_state_binds_source_master_authority_but_not_output_raster() {
    SyntheticField fieldSource(8u, 6u, false);
    tn::AuthorityFieldSummary summary{};
    REQUIRE(tn::summarizeAuthorityField(fieldSource, summary));

    const auto state = stateFor(8u, 6u, summary.contentSha256);
    REQUIRE(state.finalized);
    REQUIRE(state.isRasterIndependent);
    REQUIRE(!state.createsNewEvidence);
    REQUIRE(!state.scientificWritebackAllowed);

    auto changedInput = state.input;
    changedInput.scientificMasterSha256[0] ^= 0x01u;
    tn::State changed{};
    REQUIRE(tn::finalizeState(changedInput, changed));
    REQUIRE(state.stateSha256 != changed.stateSha256);
}

void test_queries_keep_one_state_across_multiple_rasters() {
    SyntheticField fieldSource(8u, 6u, false);
    tn::AuthorityFieldSummary summary{};
    REQUIRE(tn::summarizeAuthorityField(fieldSource, summary));
    const auto state = stateFor(8u, 6u, summary.contentSha256);
    SyntheticScene scene(8u, 6u);

    tn::QueryResult sourceSized{};
    tn::QueryResult dense{};
    tn::QueryResult small{};
    REQUIRE(tn::resolvePixel(scene, state, 8u, 6u, 3u, 2u, sourceSized));
    REQUIRE(tn::resolvePixel(scene, state, 32u, 24u, 12u, 8u, dense));
    REQUIRE(tn::resolvePixel(scene, state, 4u, 3u, 1u, 1u, small));

    REQUIRE(sourceSized.stateSha256 == state.stateSha256);
    REQUIRE(dense.stateSha256 == state.stateSha256);
    REQUIRE(small.stateSha256 == state.stateSha256);
    REQUIRE(!sourceSized.stateIdentityChangedByTargetRaster);
    REQUIRE(!dense.stateIdentityChangedByTargetRaster);
    REQUIRE(!small.stateIdentityChangedByTargetRaster);

    for (const auto* q : {&sourceSized, &dense, &small}) {
        REQUIRE(q->pixel.measuredTargetClaimCount == 0u);
        REQUIRE(!q->pixel.createsNewEvidence);
        REQUIRE(q->pixel.physicalFrameCount == 1u);
        REQUIRE(q->pixel.independentEvidenceCount == 1u);
        REQUIRE(!q->createsNewEvidence);
        REQUIRE(!q->scientificWritebackAllowed);
        REQUIRE(q->pixel.support[0].authority ==
            fw::ResolvedAuthority::Unknown);
        REQUIRE(q->pixel.support[1].authority ==
            fw::ResolvedAuthority::Reconstructed);
        REQUIRE(q->pixel.support[2].authority ==
            fw::ResolvedAuthority::Unknown);
    }

    REQUIRE(dense.querySha256 != sourceSized.querySha256);
    REQUIRE(small.querySha256 != sourceSized.querySha256);
}

void test_extended_scene_values_survive_until_display_boundary() {
    SyntheticField fieldSource(5u, 4u, false);
    tn::AuthorityFieldSummary summary{};
    REQUIRE(tn::summarizeAuthorityField(fieldSource, summary));
    const auto state = stateFor(5u, 4u, summary.contentSha256);
    SyntheticScene scene(5u, 4u);

    tn::QueryResult q{};
    REQUIRE(tn::resolvePixel(scene, state, 10u, 8u, 0u, 0u, q));
    REQUIRE(q.pixel.sceneLinear[0] < 0.0);
    REQUIRE(q.pixel.sceneLinear[2] > 1.0);

    double footprintSum = 0.0;
    for (const auto& f : q.pixel.footprint) footprintSum += f.weight;
    REQUIRE(std::abs(footprintSum - 1.0) < 1e-12);
}

void test_geometry_or_state_mismatch_fails_closed() {
    SyntheticField fieldSource(8u, 6u, false);
    tn::AuthorityFieldSummary summary{};
    REQUIRE(tn::summarizeAuthorityField(fieldSource, summary));
    const auto state = stateFor(8u, 6u, summary.contentSha256);
    SyntheticScene wrongScene(7u, 6u);

    tn::QueryResult q{};
    REQUIRE(!tn::resolvePixel(
        wrongScene, state, 8u, 6u, 1u, 1u, q));
    REQUIRE(!tn::resolvePixel(
        wrongScene, state, 0u, 6u, 0u, 0u, q));
}

}  // namespace

int main() {
    test_authority_field_digest_is_deterministic_and_sensitive();
    test_state_binds_source_master_authority_but_not_output_raster();
    test_queries_keep_one_state_across_multiple_rasters();
    test_extended_scene_values_survive_until_display_boundary();
    test_geometry_or_state_mismatch_fails_closed();

    std::cout << "TruthNegativeContinuousScientificNegative/0.5 PASS\n";
    std::cout << "raster_independent_state=1\n";
    std::cout << "measured_target_claim_count=0\n";
    std::cout << "scientific_writeback_allowed=0\n";
    return 0;
}

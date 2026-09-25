#include "truthnegative_roundtrip_oracle_v0_6.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace oracle = truthraw::truthnegative_roundtrip_oracle::v0_6;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

oracle::Digest digest(std::uint8_t seed) {
    oracle::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

class Scene final : public fw::IScenePlaneSource {
public:
    std::uint32_t width() const noexcept override { return 7u; }
    std::uint32_t height() const noexcept override { return 5u; }

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        fw::SourcePixel& out) const noexcept override {
        if (x >= width() || y >= height()) return false;
        for (std::size_t c = 0u; c < 3u; ++c) {
            auto& s = out.channel[c];
            s.value =
                -0.35 +
                0.22 * static_cast<double>(c) +
                0.03 * static_cast<double>(x) +
                0.05 * static_cast<double>(y) +
                0.004 * static_cast<double>(x * y);
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
};

tn::State state() {
    tn::StateInput in{};
    in.sourceEvidenceSha256 = digest(1u);
    in.scientificMasterSha256 = digest(33u);
    in.authorityFieldSha256 = digest(65u);
    in.width = 7u;
    in.height = 5u;
    in.reconstructionBackendId = "TEST_F64";
    in.colourBindingId = "TEST_COLOR";
    tn::State out{};
    REQUIRE(tn::finalizeState(in, out));
    return out;
}

void test_area_conservation_across_rasters() {
    Scene scene;
    const auto s = state();

    oracle::Report report{};
    REQUIRE(oracle::run(
        scene,
        s,
        {
            {1u, 1u},
            {7u, 5u},
            {14u, 10u},
            {28u, 20u},
            {4u, 3u},
            {11u, 8u},
        },
        2e-12,
        report));

    REQUIRE(report.truthNegativeStateSha256 == s.stateSha256);
    REQUIRE(report.globalAreaConserved);
    REQUIRE(report.stateIdentityPreserved);
    REQUIRE(report.footprintsNormalized);
    REQUIRE(report.noEvidencePromotion);
    REQUIRE(!report.createsNewEvidence);
    REQUIRE(!report.scientificWritebackAllowed);
    REQUIRE(report.maximumAbsoluteMeanError <= report.tolerance);
    REQUIRE(report.rasters.size() == 6u);

    for (const auto& r : report.rasters) {
        REQUIRE(r.measuredTargetClaimCount == 0u);
        REQUIRE(r.stateIdentityPreserved);
        REQUIRE(r.footprintsNormalized);
        REQUIRE(r.noEvidencePromotion);
        REQUIRE(r.footprintLinkCount >= r.pixelCount);
        REQUIRE(r.queryChainSha256 != oracle::Digest{});
        REQUIRE(r.resolvedAuthorityCounts[2] > 0u);
    }
}

void test_oracle_hash_changes_with_raster_set() {
    Scene scene;
    const auto s = state();
    oracle::Report a{};
    oracle::Report b{};
    REQUIRE(oracle::run(scene, s, {{7u,5u},{14u,10u}}, 2e-12, a));
    REQUIRE(oracle::run(scene, s, {{7u,5u},{21u,15u}}, 2e-12, b));
    REQUIRE(a.oracleSha256 != b.oracleSha256);
    REQUIRE(a.truthNegativeStateSha256 == b.truthNegativeStateSha256);
}

void test_bad_state_fails_closed() {
    Scene scene;
    auto s = state();
    s.finalized = false;
    oracle::Report report{};
    REQUIRE(!oracle::run(scene, s, {{7u,5u}}, 1e-12, report));
}

}  // namespace

int main() {
    test_area_conservation_across_rasters();
    test_oracle_hash_changes_with_raster_set();
    test_bad_state_fails_closed();

    std::cout << "TruthNegativeRoundTripExplainabilityOracle/0.6 PASS\n";
    std::cout << "area_conservation=1\n";
    std::cout << "state_identity_preserved=1\n";
    std::cout << "no_evidence_promotion=1\n";
    return 0;
}

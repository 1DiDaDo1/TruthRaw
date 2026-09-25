#include "truthnegative_optics_support_v0_7.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace optics = truthraw::truthnegative_optics_support::v0_7;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

optics::Digest digest(std::uint8_t seed) {
    optics::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

optics::CalibrationInput calibrationInput(
    optics::CalibrationAuthority authority) {
    optics::CalibrationInput in{};
    in.sourceEvidenceSha256 = digest(1u);
    in.lensIdentitySha256 = digest(33u);
    in.sensorIdentitySha256 = digest(65u);
    in.calibrationEvidenceSha256 = digest(97u);
    in.authority = authority;
    in.kernelWidth = 3u;
    in.kernelHeight = 3u;
    in.psfKernel = {
        1.0, 2.0, 1.0,
        2.0, 4.0, 2.0,
        1.0, 2.0, 1.0,
    };
    in.mtf50XCyclesPerPixel = 0.22;
    in.mtf50YCyclesPerPixel = 0.20;
    return in;
}

fw::ResolvedPixel query() {
    fw::ResolvedPixel p{};
    p.sceneLinear = {-0.2, 0.4, 1.3};
    p.footprint = {
        {2u, 2u, 0.25},
        {3u, 2u, 0.25},
        {2u, 3u, 0.25},
        {3u, 3u, 0.25},
    };
    p.measuredTargetClaimCount = 0u;
    p.createsNewEvidence = false;
    p.physicalFrameCount = 1u;
    p.independentEvidenceCount = 1u;
    return p;
}

void test_measured_calibration_expands_support_only() {
    optics::CalibrationState state{};
    REQUIRE(optics::finalizeCalibration(
        calibrationInput(optics::CalibrationAuthority::Measured),
        state));
    REQUIRE(state.scientificSupportUseAllowed);
    REQUIRE(!state.deconvolutionAllowed);

    const auto q = query();
    optics::EffectiveSupport support{};
    REQUIRE(optics::propagateSupport(state, 8u, 8u, q, support));

    REQUIRE(support.footprint.size() > q.footprint.size());
    REQUIRE(support.calibrationStateSha256 == state.stateSha256);
    REQUIRE(support.supportSha256 != optics::Digest{});
    REQUIRE(!support.numericSceneValueChanged);
    REQUIRE(!support.authorityUpgraded);
    REQUIRE(!support.deconvolutionApplied);
    REQUIRE(!support.createsNewEvidence);
    REQUIRE(!support.scientificWritebackAllowed);
    REQUIRE(std::abs(support.weightSum - 1.0) <= 1e-12);
}

void test_calibrated_estimate_is_allowed_but_inferred_is_not() {
    optics::CalibrationState calibrated{};
    optics::CalibrationState inferred{};
    REQUIRE(optics::finalizeCalibration(
        calibrationInput(
            optics::CalibrationAuthority::CalibratedEstimate),
        calibrated));
    REQUIRE(optics::finalizeCalibration(
        calibrationInput(optics::CalibrationAuthority::Inferred),
        inferred));
    REQUIRE(calibrated.scientificSupportUseAllowed);
    REQUIRE(!inferred.scientificSupportUseAllowed);

    optics::EffectiveSupport support{};
    REQUIRE(optics::propagateSupport(
        calibrated, 8u, 8u, query(), support));
    REQUIRE(!optics::propagateSupport(
        inferred, 8u, 8u, query(), support));
}

void test_calibration_identity_is_sensitive_to_psf_and_mtf() {
    auto aIn = calibrationInput(
        optics::CalibrationAuthority::Measured);
    auto bIn = aIn;
    auto cIn = aIn;
    bIn.psfKernel[4] += 0.5;
    cIn.mtf50XCyclesPerPixel = 0.21;

    optics::CalibrationState a{};
    optics::CalibrationState b{};
    optics::CalibrationState c{};
    REQUIRE(optics::finalizeCalibration(aIn, a));
    REQUIRE(optics::finalizeCalibration(bIn, b));
    REQUIRE(optics::finalizeCalibration(cIn, c));
    REQUIRE(a.stateSha256 != b.stateSha256);
    REQUIRE(a.stateSha256 != c.stateSha256);
}

void test_invalid_psf_fails_closed() {
    auto bad = calibrationInput(
        optics::CalibrationAuthority::Measured);
    bad.kernelWidth = 2u;
    optics::CalibrationState state{};
    REQUIRE(!optics::finalizeCalibration(bad, state));

    bad = calibrationInput(
        optics::CalibrationAuthority::Measured);
    bad.psfKernel[0] = -1.0;
    REQUIRE(!optics::finalizeCalibration(bad, state));
}

}  // namespace

int main() {
    test_measured_calibration_expands_support_only();
    test_calibrated_estimate_is_allowed_but_inferred_is_not();
    test_calibration_identity_is_sensitive_to_psf_and_mtf();
    test_invalid_psf_fails_closed();

    std::cout << "TruthNegativeOpticsSupport/0.7 PASS\n";
    std::cout << "deconvolution_applied=0\n";
    std::cout << "numeric_scene_value_changed=0\n";
    std::cout << "inferred_calibration_scientific_use=0\n";
    return 0;
}

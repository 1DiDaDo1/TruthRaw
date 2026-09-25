#include "free_world_light_transport_state_v0_6.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace lt = truthraw::free_world_light_transport_state::v0_6;
namespace bind = truthraw::free_world_deep_scene_binding::v0_5;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace fw = truthraw::free_world_pixel_resolve_2d::v0_2;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

namespace {

lt::Digest digest(std::uint8_t seed) {
    lt::Digest d{};
    for (std::size_t i = 0u; i < d.size(); ++i) {
        d[i] = static_cast<std::uint8_t>(seed + i);
    }
    return d;
}

lt::SpectralHypothesisState inferredSpectrum(std::uint8_t seed) {
    lt::SpectralHypothesisState s{};
    s.identitySha256 = digest(seed);
    s.authority = lt::ParameterAuthority::Inferred;
    s.spectralMeasurementAdmitted = false;
    s.fullSpectrumRecovered = false;
    return s;
}

lt::LightTransportState inferredState() {
    lt::LightTransportState s{};
    s.parentBoundDeepPacketSha256 = digest(1u);
    s.incomingDirection = {0.0, 0.0, 2.0};
    s.outgoingDirection = {0.0, 0.0, 1.0};
    s.surface.normal = {0.0, 0.0, 1.0};
    s.surface.normalAuthority = bind::GeometryAuthority::Inferred;
    s.surface.regionId = 11u;
    s.surface.objectId = 22u;

    s.material.identitySha256 = digest(20u);
    s.material.model = lt::MaterialModel::LambertianDiffuse;
    s.material.authority = lt::ParameterAuthority::Inferred;
    s.material.diffuseReflectanceRgb = {0.8, 0.5, 0.25};
    s.material.spectral = inferredSpectrum(40u);

    s.illumination.identitySha256 = digest(60u);
    s.illumination.authority = lt::ParameterAuthority::Inferred;
    s.illumination.spectral = inferredSpectrum(80u);

    s.visibility = 0.75;
    s.contributionClass = deep::ContributionClass::InferredScene;
    s.provenanceId = 333u;
    return s;
}

void test_vectors_normalize_and_state_hashes() {
    auto s = inferredState();
    REQUIRE(lt::finalizeState(s));
    REQUIRE(s.finalized);
    REQUIRE(std::abs(s.incomingDirection.z - 1.0) < 1e-12);
    REQUIRE(std::abs(s.outgoingDirection.z - 1.0) < 1e-12);
    REQUIRE(std::abs(s.surface.normal.z - 1.0) < 1e-12);
    REQUIRE(s.stateSha256 != lt::Digest{});
    REQUIRE(!s.createsNewEvidence);
    REQUIRE(!s.scientificWritebackAllowed);
}

void test_state_identity_changes_with_material_or_illumination() {
    auto a = inferredState();
    auto b = inferredState();
    auto c = inferredState();

    b.material.identitySha256[0] ^= 0x1u;
    c.illumination.identitySha256[0] ^= 0x1u;

    REQUIRE(lt::finalizeState(a));
    REQUIRE(lt::finalizeState(b));
    REQUIRE(lt::finalizeState(c));

    REQUIRE(a.stateSha256 != b.stateSha256);
    REQUIRE(a.stateSha256 != c.stateSha256);
}

void test_counterfactual_requires_counterfactual_class() {
    auto bad = inferredState();
    bad.illumination.authority =
        lt::ParameterAuthority::Counterfactual;
    REQUIRE(!lt::finalizeState(bad));

    auto good = inferredState();
    good.illumination.authority =
        lt::ParameterAuthority::Counterfactual;
    good.contributionClass =
        deep::ContributionClass::CounterfactualScene;
    REQUIRE(lt::finalizeState(good));
}

void test_spectral_claim_gate() {
    auto invalid = inferredSpectrum(91u);
    invalid.spectralMeasurementAdmitted = true;
    REQUIRE(!lt::validateSpectralState(invalid));

    lt::SpectralHypothesisState calibrated{};
    calibrated.identitySha256 = digest(92u);
    calibrated.authority =
        lt::ParameterAuthority::CalibratedEstimate;
    calibrated.spectralMeasurementAdmitted = true;
    calibrated.fullSpectrumRecovered = false;
    REQUIRE(lt::validateSpectralState(calibrated));

    calibrated.fullSpectrumRecovered = true;
    REQUIRE(lt::validateSpectralState(calibrated));
}

void test_lambertian_integrand_numeric_reference() {
    auto state = inferredState();
    state.visibility = 0.5;
    REQUIRE(lt::finalizeState(state));

    lt::IntegrandInput input{};
    input.state = state;
    input.incidentSceneLinearRgb = {2.0, 1.0, 0.5};
    input.emittedSceneLinearRgb = {0.1, 0.2, 0.3};
    input.radiometricAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    input.zFront = 2.0;
    input.zBack = 2.0;
    input.opacity = 0.4;

    lt::IntegrandResult out{};
    REQUIRE(lt::evaluateLambertianIntegrand(input, out));

    const double pi =
        3.141592653589793238462643383279502884;
    REQUIRE(std::abs(out.cosineTerm - 1.0) < 1e-12);
    REQUIRE(std::abs(out.bsdfRgb[0] - 0.8 / pi) < 1e-12);
    REQUIRE(std::abs(
        out.reflectedIntegrandRgb[0] -
        2.0 * (0.8 / pi) * 0.5) < 1e-12);

    REQUIRE(std::abs(
        out.path.sample.sceneLinearRgb[0] -
        (0.1 + 2.0 * (0.8 / pi) * 0.5)) < 1e-12);

    REQUIRE(!out.renderingEquationIntegralSolved);
    REQUIRE(!out.multipleScatteringSolved);
    REQUIRE(!out.spectralMeasurementClaimed);
    REQUIRE(!out.createsNewEvidence);
    REQUIRE(!out.scientificWritebackAllowed);
    REQUIRE(out.pathSegmentAncestrySha256 != lt::Digest{});
}

void test_backfacing_light_gives_zero_reflected_integrand() {
    auto state = inferredState();
    state.incomingDirection = {0.0, 0.0, -1.0};
    REQUIRE(lt::finalizeState(state));

    lt::IntegrandInput input{};
    input.state = state;
    input.incidentSceneLinearRgb = {10.0, 10.0, 10.0};
    input.emittedSceneLinearRgb = {0.2, 0.3, 0.4};
    input.radiometricAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    input.zFront = 1.0;
    input.zBack = 1.0;
    input.opacity = 1.0;

    lt::IntegrandResult out{};
    REQUIRE(lt::evaluateLambertianIntegrand(input, out));
    REQUIRE(out.cosineTerm == 0.0);
    REQUIRE(out.reflectedIntegrandRgb[0] == 0.0);
    REQUIRE(std::abs(out.path.sample.sceneLinearRgb[0] - 0.2) < 1e-12);
}

void test_geometry_and_radiometry_remain_separate() {
    auto state = inferredState();
    state.surface.normalAuthority =
        bind::GeometryAuthority::Inferred;
    REQUIRE(lt::finalizeState(state));

    lt::IntegrandInput input{};
    input.state = state;
    input.incidentSceneLinearRgb = {1.0, 1.0, 1.0};
    input.emittedSceneLinearRgb = {0.0, 0.0, 0.0};
    input.radiometricAuthority = {
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown,
        fw::ResolvedAuthority::Unknown};
    input.zFront = 3.0;
    input.zBack = 3.0;
    input.opacity = 0.5;

    lt::IntegrandResult out{};
    REQUIRE(lt::evaluateLambertianIntegrand(input, out));
    REQUIRE(out.path.geometryAuthority ==
        bind::GeometryAuthority::Inferred);
    for (auto a : out.path.sample.channelAuthority) {
        REQUIRE(a == fw::ResolvedAuthority::Unknown);
    }
}

void test_evidence_constrained_requires_strong_parameter_authority() {
    auto bad = inferredState();
    bad.contributionClass =
        deep::ContributionClass::EvidenceConstrained;
    bad.surface.normalAuthority =
        bind::GeometryAuthority::ImagePlaneBound;
    REQUIRE(!lt::finalizeState(bad));

    auto good = inferredState();
    good.contributionClass =
        deep::ContributionClass::EvidenceConstrained;
    good.surface.normalAuthority =
        bind::GeometryAuthority::Calibrated3DEstimate;
    good.material.authority =
        lt::ParameterAuthority::CalibratedEstimate;
    good.illumination.authority =
        lt::ParameterAuthority::CalibratedEstimate;
    good.material.spectral.authority =
        lt::ParameterAuthority::CalibratedEstimate;
    good.illumination.spectral.authority =
        lt::ParameterAuthority::CalibratedEstimate;
    REQUIRE(lt::finalizeState(good));
}

}  // namespace

int main() {
    test_vectors_normalize_and_state_hashes();
    test_state_identity_changes_with_material_or_illumination();
    test_counterfactual_requires_counterfactual_class();
    test_spectral_claim_gate();
    test_lambertian_integrand_numeric_reference();
    test_backfacing_light_gives_zero_reflected_integrand();
    test_geometry_and_radiometry_remain_separate();
    test_evidence_constrained_requires_strong_parameter_authority();

    std::cout << "FreeWorldLightTransportState/0.6 PASS\n";
    std::cout << "integrand_method=" << lt::kIntegrandMethodId << "\n";
    std::cout << "rendering_equation_integral_solved=0\n";
    std::cout << "multiple_scattering_solved=0\n";
    std::cout << "scientific_writeback_allowed=0\n";
    return 0;
}

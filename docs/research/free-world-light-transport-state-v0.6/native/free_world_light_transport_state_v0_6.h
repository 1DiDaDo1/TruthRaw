#pragma once

#include "free_world_deep_scene_binding_v0_5.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::free_world_light_transport_state::v0_6 {

namespace binding = truthraw::free_world_deep_scene_binding::v0_5;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "FreeWorldLightTransportState/0.6";
inline constexpr const char* kIntegrandMethodId =
    "RGB_LAMBERTIAN_RENDERING_EQUATION_INTEGRAND_V0_6";

struct Vec3 final {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

enum class ParameterAuthority : std::uint8_t {
    Measured = 1u,
    CalibratedEstimate = 2u,
    Inferred = 3u,
    Hypothetical = 4u,
    Counterfactual = 5u,
    Unknown = 6u,
};

enum class MaterialModel : std::uint8_t {
    LambertianDiffuse = 1u,
};

struct SpectralHypothesisState final {
    Digest identitySha256{};
    ParameterAuthority authority = ParameterAuthority::Unknown;
    bool spectralMeasurementAdmitted = false;
    bool fullSpectrumRecovered = false;
};

struct IlluminationState final {
    Digest identitySha256{};
    ParameterAuthority authority = ParameterAuthority::Unknown;
    SpectralHypothesisState spectral{};
};

struct MaterialState final {
    Digest identitySha256{};
    MaterialModel model = MaterialModel::LambertianDiffuse;
    ParameterAuthority authority = ParameterAuthority::Unknown;

    std::array<double, 3u> diffuseReflectanceRgb{};
    SpectralHypothesisState spectral{};
};

struct SurfaceState final {
    Vec3 normal{};
    binding::GeometryAuthority normalAuthority =
        binding::GeometryAuthority::Unknown;
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;
};

struct LightTransportState final {
    Digest parentBoundDeepPacketSha256{};
    Digest stateSha256{};

    Vec3 incomingDirection{};
    Vec3 outgoingDirection{};

    SurfaceState surface{};
    MaterialState material{};
    IlluminationState illumination{};

    double visibility = 1.0;

    deep::ContributionClass contributionClass =
        deep::ContributionClass::InferredScene;
    std::uint64_t provenanceId = 0u;

    bool finalized = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

struct IntegrandInput final {
    LightTransportState state{};
    std::array<double, 3u> incidentSceneLinearRgb{};
    std::array<double, 3u> emittedSceneLinearRgb{};

    std::array<free_world::ResolvedAuthority, 3u> radiometricAuthority{
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown};
    std::array<bool, 3u> uncertaintyKnown{};
    std::array<double, 3u> p95Uncertainty{};

    double zFront = 0.0;
    double zBack = 0.0;
    double opacity = 1.0;
};

struct IntegrandResult final {
    binding::PathRadianceResult path{};
    Digest lightTransportStateSha256{};
    Digest pathSegmentAncestrySha256{};

    double cosineTerm = 0.0;
    std::array<double, 3u> bsdfRgb{};
    std::array<double, 3u> reflectedIntegrandRgb{};

    bool renderingEquationIntegralSolved = false;
    bool multipleScatteringSolved = false;
    bool spectralMeasurementClaimed = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kIntegrandMethodId;
};

bool normalize(Vec3 input, Vec3& output) noexcept;
double dot(Vec3 a, Vec3 b) noexcept;

bool validateSpectralState(
    const SpectralHypothesisState& spectral) noexcept;

bool finalizeState(LightTransportState& state) noexcept;

bool evaluateLambertianIntegrand(
    const IntegrandInput& input,
    IntegrandResult& out) noexcept;

const char* toString(ParameterAuthority authority) noexcept;
const char* toString(MaterialModel model) noexcept;

}  // namespace truthraw::free_world_light_transport_state::v0_6

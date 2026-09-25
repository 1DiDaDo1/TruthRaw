#include "free_world_light_transport_state_v0_6.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace truthraw::free_world_light_transport_state::v0_6 {
namespace {

constexpr double kPi =
    3.141592653589793238462643383279502884;

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

bool finite(double v) noexcept {
    return std::isfinite(v);
}

bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v) && v >= 0.0;
}

bool finiteUnit(double v) noexcept {
    return std::isfinite(v) && v >= 0.0 && v <= 1.0;
}

bool validParameterAuthority(ParameterAuthority a) noexcept {
    const auto v = static_cast<std::uint8_t>(a);
    return v >= 1u && v <= 6u;
}

bool validMaterialModel(MaterialModel m) noexcept {
    return m == MaterialModel::LambertianDiffuse;
}

bool strongPhysicalAuthority(ParameterAuthority a) noexcept {
    return a == ParameterAuthority::Measured ||
           a == ParameterAuthority::CalibratedEstimate;
}

bool classCompatible(const LightTransportState& s) noexcept {
    const bool anyCounterfactual =
        s.material.authority == ParameterAuthority::Counterfactual ||
        s.illumination.authority == ParameterAuthority::Counterfactual ||
        s.material.spectral.authority == ParameterAuthority::Counterfactual ||
        s.illumination.spectral.authority == ParameterAuthority::Counterfactual ||
        s.surface.normalAuthority ==
            binding::GeometryAuthority::Counterfactual;

    const bool anyHypothetical =
        s.material.authority == ParameterAuthority::Hypothetical ||
        s.illumination.authority == ParameterAuthority::Hypothetical ||
        s.material.spectral.authority == ParameterAuthority::Hypothetical ||
        s.illumination.spectral.authority == ParameterAuthority::Hypothetical ||
        s.surface.normalAuthority ==
            binding::GeometryAuthority::Hypothetical;

    switch (s.contributionClass) {
        case deep::ContributionClass::EvidenceConstrained:
            return !anyCounterfactual &&
                   !anyHypothetical &&
                   strongPhysicalAuthority(s.material.authority) &&
                   strongPhysicalAuthority(s.illumination.authority) &&
                   (s.surface.normalAuthority ==
                        binding::GeometryAuthority::ImagePlaneBound ||
                    s.surface.normalAuthority ==
                        binding::GeometryAuthority::Calibrated3DEstimate);
        case deep::ContributionClass::InferredScene:
            return !anyCounterfactual && !anyHypothetical;
        case deep::ContributionClass::RestorationHypothesis:
            return !anyCounterfactual;
        case deep::ContributionClass::CounterfactualScene:
            return anyCounterfactual;
    }
    return false;
}

bool authorityAllowedForClass(
    deep::ContributionClass c,
    free_world::ResolvedAuthority a) noexcept {
    if (c == deep::ContributionClass::EvidenceConstrained) {
        return a == free_world::ResolvedAuthority::Reconstructed ||
               a == free_world::ResolvedAuthority::Censored ||
               a == free_world::ResolvedAuthority::Unknown;
    }
    return a == free_world::ResolvedAuthority::Unknown;
}

void hashU64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hashF64(truthraw::sha256_v0_69::Hasher& h, double v) noexcept {
    hashU64(h, std::bit_cast<std::uint64_t>(v));
}

void hashVec3(truthraw::sha256_v0_69::Hasher& h, Vec3 v) noexcept {
    hashF64(h, v.x);
    hashF64(h, v.y);
    hashF64(h, v.z);
}

void hashSpectral(
    truthraw::sha256_v0_69::Hasher& h,
    const SpectralHypothesisState& s) noexcept {
    h.update(s.identitySha256);
    const std::uint8_t authority =
        static_cast<std::uint8_t>(s.authority);
    const std::uint8_t admitted =
        s.spectralMeasurementAdmitted ? 1u : 0u;
    const std::uint8_t recovered =
        s.fullSpectrumRecovered ? 1u : 0u;
    h.update(&authority, 1u);
    h.update(&admitted, 1u);
    h.update(&recovered, 1u);
}

Digest segmentAncestry(
    const LightTransportState& state,
    const binding::PathRadianceResult& path) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "D_RAW_FREE_WORLD_PATH_SEGMENT_ANCESTRY_V0_6";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    h.update(state.stateSha256);
    hashU64(h, state.provenanceId);
    hashU64(h, state.surface.regionId);
    hashU64(h, state.surface.objectId);
    const std::uint8_t kind =
        static_cast<std::uint8_t>(path.pathKind);
    h.update(&kind, 1u);
    return h.finalize();
}

}  // namespace

bool normalize(Vec3 input, Vec3& output) noexcept {
    output = {};
    if (!finite(input.x) || !finite(input.y) || !finite(input.z)) {
        return false;
    }
    const double length2 =
        input.x * input.x +
        input.y * input.y +
        input.z * input.z;
    if (!finite(length2) || length2 <= 0.0) return false;

    const double inv = 1.0 / std::sqrt(length2);
    output = {input.x * inv, input.y * inv, input.z * inv};
    return finite(output.x) && finite(output.y) && finite(output.z);
}

double dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool validateSpectralState(
    const SpectralHypothesisState& spectral) noexcept {
    if (!nonzero(spectral.identitySha256) ||
        !validParameterAuthority(spectral.authority)) {
        return false;
    }

    if (spectral.fullSpectrumRecovered &&
        !spectral.spectralMeasurementAdmitted) {
        return false;
    }

    if (spectral.spectralMeasurementAdmitted &&
        !strongPhysicalAuthority(spectral.authority)) {
        return false;
    }

    return true;
}

bool finalizeState(LightTransportState& state) noexcept {
    try {
        state.finalized = false;
        state.stateSha256 = {};

        if (!nonzero(state.parentBoundDeepPacketSha256) ||
            state.surface.regionId == 0u ||
            state.surface.objectId == 0u ||
            state.provenanceId == 0u ||
            !validParameterAuthority(state.material.authority) ||
            !validParameterAuthority(state.illumination.authority) ||
            !validMaterialModel(state.material.model) ||
            !binding::toString(state.surface.normalAuthority) ||
            !nonzero(state.material.identitySha256) ||
            !nonzero(state.illumination.identitySha256) ||
            !validateSpectralState(state.material.spectral) ||
            !validateSpectralState(state.illumination.spectral) ||
            !finiteUnit(state.visibility) ||
            state.createsNewEvidence ||
            state.scientificWritebackAllowed ||
            !classCompatible(state)) {
            return false;
        }

        Vec3 wi{};
        Vec3 wo{};
        Vec3 n{};
        if (!normalize(state.incomingDirection, wi) ||
            !normalize(state.outgoingDirection, wo) ||
            !normalize(state.surface.normal, n)) {
            return false;
        }
        state.incomingDirection = wi;
        state.outgoingDirection = wo;
        state.surface.normal = n;

        for (double r : state.material.diffuseReflectanceRgb) {
            if (!finiteUnit(r)) return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_FREE_WORLD_LIGHT_TRANSPORT_STATE_V0_6";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(state.parentBoundDeepPacketSha256);
        hashVec3(h, state.incomingDirection);
        hashVec3(h, state.outgoingDirection);
        hashVec3(h, state.surface.normal);
        hashU64(h, state.surface.regionId);
        hashU64(h, state.surface.objectId);
        hashU64(h, state.provenanceId);

        const std::uint8_t normalAuthority =
            static_cast<std::uint8_t>(state.surface.normalAuthority);
        const std::uint8_t materialModel =
            static_cast<std::uint8_t>(state.material.model);
        const std::uint8_t materialAuthority =
            static_cast<std::uint8_t>(state.material.authority);
        const std::uint8_t illuminationAuthority =
            static_cast<std::uint8_t>(state.illumination.authority);
        const std::uint8_t contributionClass =
            static_cast<std::uint8_t>(state.contributionClass);

        h.update(&normalAuthority, 1u);
        h.update(&materialModel, 1u);
        h.update(&materialAuthority, 1u);
        h.update(&illuminationAuthority, 1u);
        h.update(&contributionClass, 1u);

        h.update(state.material.identitySha256);
        h.update(state.illumination.identitySha256);
        for (double r : state.material.diffuseReflectanceRgb) {
            hashF64(h, r);
        }
        hashSpectral(h, state.material.spectral);
        hashSpectral(h, state.illumination.spectral);
        hashF64(h, state.visibility);

        state.stateSha256 = h.finalize();
        state.finalized = nonzero(state.stateSha256);
        return state.finalized;
    } catch (...) {
        state.finalized = false;
        state.stateSha256 = {};
        return false;
    }
}

bool evaluateLambertianIntegrand(
    const IntegrandInput& input,
    IntegrandResult& out) noexcept {
    out = IntegrandResult{};
    try {
        if (!input.state.finalized ||
            !nonzero(input.state.stateSha256) ||
            !finiteNonNegative(input.zFront) ||
            !finiteNonNegative(input.zBack) ||
            input.zBack < input.zFront ||
            !finiteUnit(input.opacity)) {
            return false;
        }

        Vec3 wi{};
        Vec3 n{};
        if (!normalize(input.state.incomingDirection, wi) ||
            !normalize(input.state.surface.normal, n)) {
            return false;
        }

        const double cosine =
            std::max(0.0, dot(n, wi));
        if (!finiteUnit(cosine)) return false;

        binding::PathRadianceInput path{};
        path.pathKind = binding::PathKind::SurfaceReflection;
        path.geometryAuthority = input.state.surface.normalAuthority;
        path.contributionClass = input.state.contributionClass;
        path.provenanceId = input.state.provenanceId;
        path.regionId = input.state.surface.regionId;
        path.objectId = input.state.surface.objectId;
        path.zFront = input.zFront;
        path.zBack = input.zBack;
        path.opacity = input.opacity;
        path.visibility = input.state.visibility;
        path.cosineTerm = cosine;

        for (std::size_t c = 0u; c < 3u; ++c) {
            if (!finite(input.incidentSceneLinearRgb[c]) ||
                !finite(input.emittedSceneLinearRgb[c]) ||
                !authorityAllowedForClass(
                    input.state.contributionClass,
                    input.radiometricAuthority[c])) {
                return false;
            }
            if (input.uncertaintyKnown[c]) {
                if (!finiteNonNegative(input.p95Uncertainty[c])) return false;
            } else if (input.p95Uncertainty[c] != 0.0) {
                return false;
            }

            const double bsdf =
                input.state.material.diffuseReflectanceRgb[c] / kPi;
            out.bsdfRgb[c] = bsdf;
            out.reflectedIntegrandRgb[c] =
                input.incidentSceneLinearRgb[c] *
                bsdf *
                input.state.visibility *
                cosine;

            path.incidentSceneLinearRgb[c] =
                input.incidentSceneLinearRgb[c];
            path.throughputRgb[c] = bsdf;
            path.emittedSceneLinearRgb[c] =
                input.emittedSceneLinearRgb[c];
            path.radiometricAuthority[c] =
                input.radiometricAuthority[c];
            path.uncertaintyKnown[c] =
                input.uncertaintyKnown[c];
            path.p95Uncertainty[c] =
                input.uncertaintyKnown[c]
                    ? input.p95Uncertainty[c]
                    : 0.0;
        }

        if (!binding::evaluatePathRadiance(path, out.path)) {
            return false;
        }

        out.lightTransportStateSha256 =
            input.state.stateSha256;
        out.pathSegmentAncestrySha256 =
            segmentAncestry(input.state, out.path);
        out.cosineTerm = cosine;
        out.renderingEquationIntegralSolved = false;
        out.multipleScatteringSolved = false;
        out.spectralMeasurementClaimed =
            input.state.material.spectral.spectralMeasurementAdmitted ||
            input.state.illumination.spectral.spectralMeasurementAdmitted;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return nonzero(out.pathSegmentAncestrySha256);
    } catch (...) {
        out = IntegrandResult{};
        return false;
    }
}

const char* toString(ParameterAuthority authority) noexcept {
    switch (authority) {
        case ParameterAuthority::Measured: return "MEASURED";
        case ParameterAuthority::CalibratedEstimate:
            return "CALIBRATED_ESTIMATE";
        case ParameterAuthority::Inferred: return "INFERRED";
        case ParameterAuthority::Hypothetical: return "HYPOTHETICAL";
        case ParameterAuthority::Counterfactual: return "COUNTERFACTUAL";
        case ParameterAuthority::Unknown: return "UNKNOWN";
    }
    return "INVALID";
}

const char* toString(MaterialModel model) noexcept {
    switch (model) {
        case MaterialModel::LambertianDiffuse:
            return "LAMBERTIAN_DIFFUSE";
    }
    return "INVALID";
}

}  // namespace truthraw::free_world_light_transport_state::v0_6

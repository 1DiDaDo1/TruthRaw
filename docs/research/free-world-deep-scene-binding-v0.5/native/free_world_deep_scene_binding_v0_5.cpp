#include "free_world_deep_scene_binding_v0_5.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace truthraw::free_world_deep_scene_binding::v0_5 {
namespace {

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

bool validGeometryAuthority(GeometryAuthority a) noexcept {
    const auto v = static_cast<std::uint8_t>(a);
    return v >= 1u && v <= 6u;
}

bool validPathKind(PathKind k) noexcept {
    const auto v = static_cast<std::uint8_t>(k);
    return v >= 1u && v <= 5u;
}

void hashU64(truthraw::sha256_v0_69::Hasher& h, std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

Digest contributionAncestry(
    const Digest& cameraPlaneSceneSha256,
    const Digest& parentDeepPacketSha256,
    const deep::DeepSample& sample,
    const ContributionMetadata& metadata) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "D_RAW_FREE_WORLD_CONTRIBUTION_ANCESTRY_V0_5";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    h.update(cameraPlaneSceneSha256);
    h.update(parentDeepPacketSha256);
    h.update(metadata.parentAncestrySha256);
    hashU64(h, sample.provenanceId);
    hashU64(h, metadata.regionId);
    hashU64(h, metadata.objectId);
    const std::uint8_t geometry =
        static_cast<std::uint8_t>(metadata.geometryAuthority);
    h.update(&geometry, 1u);
    return h.finalize();
}

bool classGeometryCompatible(
    deep::ContributionClass c,
    GeometryAuthority g) noexcept {
    switch (c) {
        case deep::ContributionClass::EvidenceConstrained:
            return g == GeometryAuthority::ImagePlaneBound ||
                   g == GeometryAuthority::Calibrated3DEstimate ||
                   g == GeometryAuthority::Inferred ||
                   g == GeometryAuthority::Unknown;
        case deep::ContributionClass::InferredScene:
            return g == GeometryAuthority::Inferred ||
                   g == GeometryAuthority::Calibrated3DEstimate ||
                   g == GeometryAuthority::Unknown;
        case deep::ContributionClass::RestorationHypothesis:
            return g == GeometryAuthority::Hypothetical ||
                   g == GeometryAuthority::Inferred ||
                   g == GeometryAuthority::Unknown;
        case deep::ContributionClass::CounterfactualScene:
            return g == GeometryAuthority::Counterfactual;
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

}  // namespace

bool validateMetadata(
    const deep::DeepSample& sample,
    const ContributionMetadata& metadata) noexcept {
    if (!deep::validateSample(sample) ||
        metadata.provenanceId == 0u ||
        metadata.provenanceId != sample.provenanceId ||
        metadata.regionId == 0u ||
        metadata.objectId == 0u ||
        !validGeometryAuthority(metadata.geometryAuthority) ||
        !classGeometryCompatible(
            sample.contributionClass, metadata.geometryAuthority)) {
        return false;
    }
    return true;
}

bool bindPacket(
    const deep::DeepPixelPacket& parent,
    const Digest& cameraPlaneSceneSha256,
    std::vector<ContributionMetadata> metadata,
    BoundDeepPacket& out) noexcept {
    out = BoundDeepPacket{};
    try {
        if (!parent.finalized ||
            parent.samples.empty() ||
            metadata.size() != parent.samples.size() ||
            !nonzero(parent.packetSha256) ||
            !nonzero(cameraPlaneSceneSha256) ||
            parent.createsNewEvidence ||
            parent.scientificWritebackAllowed ||
            parent.physicalFrameCount != 1u ||
            parent.independentEvidenceCount != 1u) {
            return false;
        }

        out.cameraPlaneSceneSha256 = cameraPlaneSceneSha256;
        out.parentDeepPacketSha256 = parent.packetSha256;
        out.contributions.reserve(parent.samples.size());

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_FREE_WORLD_BOUND_DEEP_PACKET_V0_5";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(cameraPlaneSceneSha256);
        h.update(parent.packetSha256);

        for (std::size_t i = 0u; i < parent.samples.size(); ++i) {
            const auto& sample = parent.samples[i];
            auto& meta = metadata[i];
            if (!validateMetadata(sample, meta)) return false;

            meta.contributionAncestrySha256 = contributionAncestry(
                cameraPlaneSceneSha256,
                parent.packetSha256,
                sample,
                meta);
            if (!nonzero(meta.contributionAncestrySha256)) return false;

            h.update(meta.contributionAncestrySha256);
            h.update(meta.parentAncestrySha256);
            hashU64(h, meta.provenanceId);
            hashU64(h, meta.regionId);
            hashU64(h, meta.objectId);
            const std::uint8_t geometry =
                static_cast<std::uint8_t>(meta.geometryAuthority);
            h.update(&geometry, 1u);

            out.contributions.push_back({sample, meta});
        }

        out.boundPacketSha256 = h.finalize();
        out.finalized = nonzero(out.boundPacketSha256);
        return out.finalized;
    } catch (...) {
        out = BoundDeepPacket{};
        return false;
    }
}

bool evaluatePathRadiance(
    const PathRadianceInput& input,
    PathRadianceResult& out) noexcept {
    out = PathRadianceResult{};
    try {
        if (!validPathKind(input.pathKind) ||
            !validGeometryAuthority(input.geometryAuthority) ||
            input.provenanceId == 0u ||
            input.regionId == 0u ||
            input.objectId == 0u ||
            !finiteNonNegative(input.zFront) ||
            !finiteNonNegative(input.zBack) ||
            input.zBack < input.zFront ||
            !finite(input.opacity) ||
            input.opacity < 0.0 || input.opacity > 1.0 ||
            !finite(input.visibility) ||
            input.visibility < 0.0 || input.visibility > 1.0 ||
            !finite(input.cosineTerm) ||
            input.cosineTerm < 0.0 || input.cosineTerm > 1.0 ||
            !classGeometryCompatible(
                input.contributionClass, input.geometryAuthority)) {
            return false;
        }

        deep::DeepSample sample{};
        sample.zFront = input.zFront;
        sample.zBack = input.zBack;
        sample.opacity = input.opacity;
        sample.contributionClass = input.contributionClass;
        sample.provenanceId = input.provenanceId;

        for (std::size_t c = 0u; c < 3u; ++c) {
            if (!finite(input.incidentSceneLinearRgb[c]) ||
                !finiteNonNegative(input.throughputRgb[c]) ||
                !finite(input.emittedSceneLinearRgb[c]) ||
                !authorityAllowedForClass(
                    input.contributionClass,
                    input.radiometricAuthority[c])) {
                return false;
            }

            if (input.uncertaintyKnown[c]) {
                if (!finiteNonNegative(input.p95Uncertainty[c])) return false;
            } else if (input.p95Uncertainty[c] != 0.0) {
                return false;
            }

            sample.sceneLinearRgb[c] =
                input.emittedSceneLinearRgb[c] +
                input.incidentSceneLinearRgb[c] *
                    input.throughputRgb[c] *
                    input.visibility *
                    input.cosineTerm;

            sample.channelAuthority[c] =
                input.radiometricAuthority[c];

            if (input.contributionClass ==
                deep::ContributionClass::EvidenceConstrained) {
                sample.uncertaintyKnown[c] = input.uncertaintyKnown[c];
                sample.p95Uncertainty[c] =
                    input.uncertaintyKnown[c]
                        ? input.p95Uncertainty[c]
                        : 0.0;
            } else {
                sample.uncertaintyKnown[c] = false;
                sample.p95Uncertainty[c] = 0.0;
            }
        }

        if (!deep::validateSample(sample)) return false;

        out.sample = sample;
        out.regionId = input.regionId;
        out.objectId = input.objectId;
        out.geometryAuthority = input.geometryAuthority;
        out.pathKind = input.pathKind;
        out.spectralMeasurementClaimed = false;
        out.fullPathTracingClaimed = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return true;
    } catch (...) {
        out = PathRadianceResult{};
        return false;
    }
}

const char* toString(GeometryAuthority authority) noexcept {
    switch (authority) {
        case GeometryAuthority::ImagePlaneBound:
            return "IMAGE_PLANE_BOUND";
        case GeometryAuthority::Calibrated3DEstimate:
            return "CALIBRATED_3D_ESTIMATE";
        case GeometryAuthority::Inferred:
            return "INFERRED";
        case GeometryAuthority::Hypothetical:
            return "HYPOTHETICAL";
        case GeometryAuthority::Counterfactual:
            return "COUNTERFACTUAL";
        case GeometryAuthority::Unknown:
            return "UNKNOWN";
    }
    return "INVALID";
}

const char* toString(PathKind kind) noexcept {
    switch (kind) {
        case PathKind::CameraPrimary: return "CAMERA_PRIMARY";
        case PathKind::SurfaceReflection: return "SURFACE_REFLECTION";
        case PathKind::Transmission: return "TRANSMISSION";
        case PathKind::Volume: return "VOLUME";
        case PathKind::Emission: return "EMISSION";
    }
    return "INVALID";
}

}  // namespace truthraw::free_world_deep_scene_binding::v0_5

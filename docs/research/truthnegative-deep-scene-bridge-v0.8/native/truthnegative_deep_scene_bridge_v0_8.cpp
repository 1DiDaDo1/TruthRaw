#include "truthnegative_deep_scene_bridge_v0_8.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace truthraw::truthnegative_deep_scene_bridge::v0_8 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

bool valid_geometry_for_evidence(
    binding::GeometryAuthority authority) noexcept {
    return authority == binding::GeometryAuthority::ImagePlaneBound ||
           authority == binding::GeometryAuthority::Calibrated3DEstimate ||
           authority == binding::GeometryAuthority::Inferred ||
           authority == binding::GeometryAuthority::Unknown;
}

Digest scene_packet_digest(
    const tn::State& state,
    const tn::QueryResult& query,
    const binding::BoundDeepPacket& bound) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "D_RAW_TRUTHNEGATIVE_DEEP_SCENE_PACKET_V0_8";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    h.update(state.stateSha256);
    h.update(query.querySha256);
    h.update(bound.boundPacketSha256);
    return h.finalize();
}

}  // namespace

bool buildCameraPlaneObject(
    const tn::State& state,
    const tn::QueryResult& query,
    const CameraPlaneObjectInput& object,
    ScenePacket& out) noexcept {
    out = ScenePacket{};
    try {
        if (!state.finalized ||
            !nonzero(state.stateSha256) ||
            !nonzero(query.querySha256) ||
            query.stateSha256 != state.stateSha256 ||
            query.stateIdentityChangedByTargetRaster ||
            query.createsNewEvidence ||
            query.scientificWritebackAllowed ||
            query.pixel.createsNewEvidence ||
            query.pixel.measuredTargetClaimCount != 0u ||
            query.pixel.physicalFrameCount != 1u ||
            query.pixel.independentEvidenceCount != 1u ||
            object.provenanceId == 0u ||
            object.regionId == 0u ||
            object.objectId == 0u ||
            !std::isfinite(object.depth) ||
            object.depth < 0.0 ||
            !valid_geometry_for_evidence(object.geometryAuthority)) {
            return false;
        }

        deep::DeepSample sample{};
        if (!deep::makeCameraPlaneContribution(
                query.pixel,
                object.depth,
                object.provenanceId,
                sample)) {
            return false;
        }

        deep::DeepPixelPacket deepPacket{};
        deepPacket.samples.push_back(sample);
        if (!deep::finalizePacket(deepPacket) ||
            deepPacket.createsNewEvidence ||
            deepPacket.scientificWritebackAllowed) {
            return false;
        }

        binding::ContributionMetadata metadata{};
        metadata.provenanceId = object.provenanceId;
        metadata.regionId = object.regionId;
        metadata.objectId = object.objectId;
        metadata.geometryAuthority = object.geometryAuthority;
        metadata.parentAncestrySha256 =
            nonzero(object.parentAncestrySha256)
                ? object.parentAncestrySha256
                : query.querySha256;

        binding::BoundDeepPacket bound{};
        if (!binding::bindPacket(
                deepPacket,
                state.stateSha256,
                {metadata},
                bound) ||
            !bound.finalized ||
            bound.createsNewEvidence ||
            bound.scientificWritebackAllowed ||
            bound.contributions.size() != 1u) {
            return false;
        }

        const auto& contribution = bound.contributions.front();
        if (contribution.sample.provenanceId != object.provenanceId ||
            contribution.metadata.regionId != object.regionId ||
            contribution.metadata.objectId != object.objectId ||
            contribution.metadata.geometryAuthority !=
                object.geometryAuthority) {
            return false;
        }

        for (std::size_t c = 0u; c < 3u; ++c) {
            if (contribution.sample.channelAuthority[c] !=
                query.pixel.support[c].authority) {
                return false;
            }
        }

        out.truthNegativeStateSha256 = state.stateSha256;
        out.truthNegativeQuerySha256 = query.querySha256;
        out.deepPacket = std::move(deepPacket);
        out.boundPacket = std::move(bound);
        out.scenePacketSha256 =
            scene_packet_digest(state, query, out.boundPacket);
        out.radiometryBoundToTruthNegative = true;
        out.geometryAuthoritySeparate = true;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.physicalFrameCount = 1u;
        out.independentEvidenceCount = 1u;

        return nonzero(out.scenePacketSha256);
    } catch (...) {
        out = ScenePacket{};
        return false;
    }
}

bool buildInferredLambertianSeed(
    const InferredLambertianSeedInput& input,
    InferredLambertianSeedResult& out) noexcept {
    out = InferredLambertianSeedResult{};
    try {
        if (!nonzero(input.scene.scenePacketSha256) ||
            !input.scene.boundPacket.finalized ||
            input.scene.createsNewEvidence ||
            input.scene.scientificWritebackAllowed ||
            input.provenanceId == 0u ||
            input.regionId == 0u ||
            input.objectId == 0u ||
            !nonzero(input.materialIdentitySha256) ||
            !nonzero(input.illuminationIdentitySha256) ||
            !nonzero(input.materialSpectralHypothesisSha256) ||
            !nonzero(input.illuminationSpectralHypothesisSha256) ||
            !std::isfinite(input.visibility) ||
            input.visibility < 0.0 ||
            input.visibility > 1.0) {
            return false;
        }

        for (double r : input.diffuseReflectanceRgb) {
            if (!std::isfinite(r) || r < 0.0 || r > 1.0) return false;
        }

        light::LightTransportState state{};
        state.parentBoundDeepPacketSha256 =
            input.scene.boundPacket.boundPacketSha256;
        state.incomingDirection = input.incomingDirection;
        state.outgoingDirection = input.outgoingDirection;
        state.surface.normal = input.surfaceNormal;
        state.surface.normalAuthority =
            binding::GeometryAuthority::Inferred;
        state.surface.regionId = input.regionId;
        state.surface.objectId = input.objectId;

        state.material.identitySha256 =
            input.materialIdentitySha256;
        state.material.model =
            light::MaterialModel::LambertianDiffuse;
        state.material.authority =
            light::ParameterAuthority::Inferred;
        state.material.diffuseReflectanceRgb =
            input.diffuseReflectanceRgb;
        state.material.spectral.identitySha256 =
            input.materialSpectralHypothesisSha256;
        state.material.spectral.authority =
            light::ParameterAuthority::Inferred;
        state.material.spectral.spectralMeasurementAdmitted = false;
        state.material.spectral.fullSpectrumRecovered = false;

        state.illumination.identitySha256 =
            input.illuminationIdentitySha256;
        state.illumination.authority =
            light::ParameterAuthority::Inferred;
        state.illumination.spectral.identitySha256 =
            input.illuminationSpectralHypothesisSha256;
        state.illumination.spectral.authority =
            light::ParameterAuthority::Inferred;
        state.illumination.spectral.spectralMeasurementAdmitted = false;
        state.illumination.spectral.fullSpectrumRecovered = false;

        state.visibility = input.visibility;
        state.contributionClass =
            deep::ContributionClass::InferredScene;
        state.provenanceId = input.provenanceId;

        if (!light::finalizeState(state) ||
            state.createsNewEvidence ||
            state.scientificWritebackAllowed) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_TRUTHNEGATIVE_INFERRED_LIGHT_TRANSPORT_SEED_V0_8";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(input.scene.scenePacketSha256);
        h.update(state.stateSha256);
        hash_u64(h, input.provenanceId);
        hash_u64(h, input.regionId);
        hash_u64(h, input.objectId);
        for (double r : input.diffuseReflectanceRgb) hash_f64(h, r);
        hash_f64(h, input.visibility);

        out.state = std::move(state);
        out.seedSha256 = h.finalize();
        out.inheritedScientificRadiometryAsMeasurement = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return nonzero(out.seedSha256);
    } catch (...) {
        out = InferredLambertianSeedResult{};
        return false;
    }
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::truthnegative_deep_scene_bridge::v0_8

#pragma once

#include "free_world_deep_scene_binding_v0_5.h"
#include "free_world_light_transport_state_v0_6.h"
#include "truthnegative_continuous_v0_5.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::truthnegative_deep_scene_bridge::v0_8 {

namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace binding = truthraw::free_world_deep_scene_binding::v0_5;
namespace light = truthraw::free_world_light_transport_state::v0_6;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeDeepSceneBridge/0.8";
inline constexpr const char* kMethodId =
    "TN_CONTINUOUS_TO_AUTHORITY_SEPARATED_DEEP_SCENE_V0_8";

struct CameraPlaneObjectInput final {
    std::uint64_t provenanceId = 0u;
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;
    double depth = 0.0;
    binding::GeometryAuthority geometryAuthority =
        binding::GeometryAuthority::ImagePlaneBound;
    Digest parentAncestrySha256{};
};

struct ScenePacket final {
    Digest truthNegativeStateSha256{};
    Digest truthNegativeQuerySha256{};
    Digest scenePacketSha256{};

    deep::DeepPixelPacket deepPacket{};
    binding::BoundDeepPacket boundPacket{};

    bool radiometryBoundToTruthNegative = false;
    bool geometryAuthoritySeparate = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    std::string methodId = kMethodId;
};

struct InferredLambertianSeedInput final {
    ScenePacket scene{};
    std::uint64_t provenanceId = 0u;
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;

    light::Vec3 incomingDirection{};
    light::Vec3 outgoingDirection{};
    light::Vec3 surfaceNormal{};

    Digest materialIdentitySha256{};
    Digest illuminationIdentitySha256{};
    Digest materialSpectralHypothesisSha256{};
    Digest illuminationSpectralHypothesisSha256{};

    std::array<double, 3u> diffuseReflectanceRgb{};
    double visibility = 1.0;
};

struct InferredLambertianSeedResult final {
    light::LightTransportState state{};
    Digest seedSha256{};
    bool inheritedScientificRadiometryAsMeasurement = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool buildCameraPlaneObject(
    const tn::State& state,
    const tn::QueryResult& query,
    const CameraPlaneObjectInput& object,
    ScenePacket& out) noexcept;

bool buildInferredLambertianSeed(
    const InferredLambertianSeedInput& input,
    InferredLambertianSeedResult& out) noexcept;

const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_deep_scene_bridge::v0_8

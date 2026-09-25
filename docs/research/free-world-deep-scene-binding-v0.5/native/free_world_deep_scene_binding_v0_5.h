#pragma once

#include "free_world_deep_scene_contribution_v0_4.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_deep_scene_binding::v0_5 {

namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "FreeWorldDeepSceneBinding/0.5";
inline constexpr const char* kPathMethodId =
    "RGB_FACTORISED_SINGLE_PATH_REFERENCE_V0_5";

enum class GeometryAuthority : std::uint8_t {
    ImagePlaneBound = 1u,
    Calibrated3DEstimate = 2u,
    Inferred = 3u,
    Hypothetical = 4u,
    Counterfactual = 5u,
    Unknown = 6u,
};

enum class PathKind : std::uint8_t {
    CameraPrimary = 1u,
    SurfaceReflection = 2u,
    Transmission = 3u,
    Volume = 4u,
    Emission = 5u,
};

struct ContributionMetadata final {
    std::uint64_t provenanceId = 0u;
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;
    GeometryAuthority geometryAuthority = GeometryAuthority::Unknown;

    Digest parentAncestrySha256{};
    Digest contributionAncestrySha256{};
};

struct BoundContribution final {
    deep::DeepSample sample{};
    ContributionMetadata metadata{};
};

struct BoundDeepPacket final {
    Digest cameraPlaneSceneSha256{};
    Digest parentDeepPacketSha256{};
    Digest boundPacketSha256{};
    std::vector<BoundContribution> contributions;

    bool finalized = false;
    bool geometryAndRadiometrySeparated = true;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct PathRadianceInput final {
    PathKind pathKind = PathKind::CameraPrimary;
    GeometryAuthority geometryAuthority = GeometryAuthority::Unknown;
    deep::ContributionClass contributionClass =
        deep::ContributionClass::InferredScene;

    std::uint64_t provenanceId = 0u;
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;

    double zFront = 0.0;
    double zBack = 0.0;
    double opacity = 0.0;

    std::array<double, 3u> incidentSceneLinearRgb{};
    std::array<double, 3u> throughputRgb{};
    std::array<double, 3u> emittedSceneLinearRgb{};

    double visibility = 1.0;
    double cosineTerm = 1.0;

    std::array<free_world::ResolvedAuthority, 3u> radiometricAuthority{
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown,
        free_world::ResolvedAuthority::Unknown};
    std::array<bool, 3u> uncertaintyKnown{};
    std::array<double, 3u> p95Uncertainty{};
};

struct PathRadianceResult final {
    deep::DeepSample sample{};
    std::uint64_t regionId = 0u;
    std::uint64_t objectId = 0u;
    GeometryAuthority geometryAuthority = GeometryAuthority::Unknown;
    PathKind pathKind = PathKind::CameraPrimary;

    bool spectralMeasurementClaimed = false;
    bool fullPathTracingClaimed = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kPathMethodId;
};

bool validateMetadata(
    const deep::DeepSample& sample,
    const ContributionMetadata& metadata) noexcept;

bool bindPacket(
    const deep::DeepPixelPacket& parent,
    const Digest& cameraPlaneSceneSha256,
    std::vector<ContributionMetadata> metadata,
    BoundDeepPacket& out) noexcept;

bool evaluatePathRadiance(
    const PathRadianceInput& input,
    PathRadianceResult& out) noexcept;

const char* toString(GeometryAuthority authority) noexcept;
const char* toString(PathKind kind) noexcept;

}  // namespace truthraw::free_world_deep_scene_binding::v0_5

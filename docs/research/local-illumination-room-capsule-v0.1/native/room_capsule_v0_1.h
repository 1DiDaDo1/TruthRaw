#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace truthraw::room_capsule::v0_1 {

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    BudgetTooSmall,
    NoAdmissiblePlan,
    OutsideRoom,
    PhysicalRelightBlocked
};

enum class LightKind : std::uint8_t {
    AmbientOnly = 0,
    Directional = 1,
    Point = 2
};

enum class RelightSemantics : std::uint8_t {
    RelativeAppearanceOnly = 0,
    CalibratedIntrinsicRelightReserved = 1
};

enum class ComputeBackend : std::uint8_t {
    CpuBaseline = 0,
    OptionalGpu = 1
};

struct RoomBounds {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

struct RoomEvidenceLedger {
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    std::uint32_t counterfactualLightStateCount = 0;
    bool lightStatesAreEvidence = false;
    bool scientificMasterModified = false;
    bool zeroLineModified = false;
};

struct MobileMemoryInput {
    std::uint64_t appMemoryClassMiB = 0;
    std::uint64_t explicitMaxWorkingSetBytes = 0;
    bool lowRamDevice = false;
    bool gpuAvailable = false;
};

struct RoomCapsuleRequest {
    std::uint32_t sourceWidth = 0;
    std::uint32_t sourceHeight = 0;
    RoomBounds room;
    std::uint32_t vectorBoundaryVertexCount = 0;
    MobileMemoryInput memory;
};

struct RoomCapsulePlan {
    bool valid = false;
    RoomBounds roomBounds;
    ComputeBackend baselineBackend = ComputeBackend::CpuBaseline;
    bool gpuAccelerationOptional = true;
    std::uint32_t geometryDownsample = 0;
    std::uint32_t geometryWidth = 0;
    std::uint32_t geometryHeight = 0;
    std::uint32_t tileSize = 0;
    std::uint32_t tileHalo = 0;
    std::uint64_t packedGeometryBytes = 0;
    std::uint64_t vectorBoundaryBytes = 0;
    std::uint64_t tileWorkspaceBytes = 0;
    std::uint64_t estimatedPeakBytes = 0;
    std::uint64_t budgetBytes = 0;
    std::uint32_t roomTileColumns = 0;
    std::uint32_t roomTileRows = 0;
    std::uint64_t roomTileCount = 0;
    RoomEvidenceLedger ledger;
};

// Persistent per-geometry-sample payload. 8 bytes exactly:
// depth(16), oct normal(16), visibility(8), confidence(8), roughness(8), flags(8).
struct PackedRoomSample {
    std::uint16_t depthQ = 0;
    std::uint8_t normalOctX = 128;
    std::uint8_t normalOctY = 128;
    std::uint8_t visibilityQ = 255;
    std::uint8_t confidenceQ = 0;
    std::uint8_t roughnessQ = 128;
    std::uint8_t flags = 0;
};
static_assert(sizeof(PackedRoomSample) == 8, "Room sample must stay compact");

struct BoundaryIlluminationEnvelope {
    std::array<float, 3> ambientRgb{1.0F, 1.0F, 1.0F};
    std::array<float, 3> dominantIncomingDirection{0.0F, 0.0F, 1.0F};
    float directionalWeight = 0.0F;
};

struct LightState {
    LightKind kind = LightKind::AmbientOnly;
    RelightSemantics semantics = RelightSemantics::RelativeAppearanceOnly;
    std::array<float, 3> position{0.0F, 0.0F, 1.0F};
    std::array<float, 3> direction{0.0F, 0.0F, -1.0F};
    std::array<float, 3> rgb{1.0F, 1.0F, 1.0F};
    float relativePower = 0.0F;
    float radius = 0.1F;
};
static_assert(sizeof(LightState) <= 64, "Light-state descriptor must stay tiny");

struct RoomSampleRuntime {
    std::array<float, 3> position{0.0F, 0.0F, 0.0F};
    std::array<float, 3> normal{0.0F, 0.0F, 1.0F};
    float visibility = 1.0F;
    float confidence = 0.0F;
    bool insideRoom = true;
};

struct RelativeLightResult {
    bool valid = false;
    bool physicalRelightClaimAllowed = false;
    std::array<float, 3> relativeMultiplier{1.0F, 1.0F, 1.0F};
    float directTerm = 0.0F;
    float ambientTerm = 1.0F;
    RoomEvidenceLedger ledger;
};

std::uint64_t derive_room_budget_bytes(const MobileMemoryInput& memory) noexcept;
Status plan_room_capsule(const RoomCapsuleRequest& request, RoomCapsulePlan& out) noexcept;
Status evaluate_relative_room_light(const RoomSampleRuntime& sample,
    const BoundaryIlluminationEnvelope& boundary,
    std::span<const LightState> lights,
    RelativeLightResult& out) noexcept;
std::array<float, 3> apply_relative_room_light(const std::array<float, 3>& sourceRgb,
    const RelativeLightResult& light) noexcept;
std::uint64_t light_state_storage_bytes(std::size_t count) noexcept;
const char* status_name(Status status) noexcept;

} // namespace truthraw::room_capsule::v0_1

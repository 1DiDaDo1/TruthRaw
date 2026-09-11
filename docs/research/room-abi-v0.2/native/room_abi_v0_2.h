#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "building_runtime_v0_1.h"
#include "full_frame_streaming_v0_1.h"
#include "illumination_room_v0_2.h"
#include "technical_backplane_v0_1.h"

namespace truthraw::room_abi::v0_2 {

using building_runtime::v0_1::ExecutionPlan;
using building_runtime::v0_1::ResourcePolicy;
using building_runtime::v0_1::RoomId;

constexpr std::size_t kRoomCount = static_cast<std::size_t>(RoomId::Count);
constexpr std::size_t kMaxWaves = kRoomCount + 1U;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidExecutionPlan,
    InvalidBackplane,
    InvalidEndpoint,
    InvalidRoomProfile,
    InvalidRoomDemand,
    IsoSceneViolation,
    RoomLeaseExceeded,
    ResidentBudgetExceeded,
    CapacityExceeded,
    UpstreamFailure
};

enum class CaptureIsoAccess : std::uint8_t {
    None = 0,
    CaptureProvenanceReadOnly,
    CalibratedSensorForwardOnly
};

enum class WorkspaceClass : std::uint8_t {
    None = 0,
    BoundedTransient,
    RebuildableCache,
    MixedBounded,
    ZeroHiddenAllocation
};

// Room-level capability declaration. ISO may be readable only as immutable
// capture provenance or in CICM's separately calibrated sensor-forward path.
// It can never become a scene coordinate or truth authority.
struct RoomBindingProfile {
    RoomId room = RoomId::Archivist;
    WorkspaceClass workspace = WorkspaceClass::None;
    CaptureIsoAccess captureIsoAccess = CaptureIsoAccess::None;
    bool mayRequireStreamingSource = false;
    bool mayRequireStreamingSink = false;
    bool mayWriteSceneIso = false;
    bool isoMayAffectSceneCoordinates = false;
    bool isoMayAffectResourcePolicy = false;
    bool resourceTierMayChangeTruthAuthority = false;
};

// Adapter-reported bounded demand for one room. This is resource information,
// never a scientific parameter. The room implementation remains responsible
// for asking Room ABI v0.1 / its successor for actual leases.
struct RoomResourceDemand {
    RoomId room = RoomId::Archivist;
    std::uint64_t transientPeakBytes = 0;
    std::uint64_t rebuildableCacheBytes = 0;
    bool valid = false;
};

// Borrowed handles. v0.2 does not own the source/sink and does not materialize
// their frames. Resident bounds come from the existing streaming interfaces.
struct StreamingEndpointBinding {
    streaming_v0_1::IRawTileSource* source = nullptr;
    streaming_v0_1::IStreamingSink* sink = nullptr;
    std::uint64_t sourceResidentUpperBound = 0;
    std::uint64_t sinkResidentUpperBound = 0;
    bool valid = false;
};

// Exactly one Technical Backplane state is shared by all room bindings for one
// lineage. This is a borrowed immutable identity handle, not a copied pixel or
// scene payload.
struct SharedLineageBinding {
    const technical_backplane::v0_1::State* backplane = nullptr;
    bool valid = false;
};

struct AllRoomIsoBoundary {
    bool sceneIsoAxisPresent = false;
    bool corridorMayCarrySceneIsoAuthority = false;
    bool resourcePolicyMayDependOnSceneIso = false;
    bool appearanceEvIsIso = false;
    bool counterfactualWorldScaleIsIso = false;
    bool calibratedSensorForwardMayCarryNominalIso = true;
    bool calibratedSensorForwardIsoPromotesToScene = false;
};

constexpr AllRoomIsoBoundary all_room_iso_boundary() noexcept { return {}; }

struct AdaptiveAllRoomRequest {
    ExecutionPlan execution{};
    SharedLineageBinding lineage{};
    StreamingEndpointBinding endpoints{};
    std::array<RoomBindingProfile, kRoomCount> profiles{};
    std::array<RoomResourceDemand, kRoomCount> demands{};
};

struct WaveMemoryInfo {
    std::uint8_t wave = 0;
    std::uint32_t activeHeavyRooms = 0;
    std::uint64_t activeTransientBytes = 0;
    std::uint64_t activeRebuildableBytes = 0;
    std::uint64_t retainedRebuildableBytes = 0;
    std::uint64_t totalResidentUpperBound = 0;
};

struct AdaptiveAllRoomPlan {
    bool valid = false;
    ResourcePolicy resources{};
    SharedLineageBinding lineage{};
    StreamingEndpointBinding endpoints{};
    std::array<WaveMemoryInfo, kMaxWaves> waves{};
    std::size_t waveCount = 0;
    std::uint64_t peakResidentUpperBound = 0;
    bool sourceSinkCountedOnce = false;
    bool oneSharedBackplaneForAllRooms = false;
    bool sceneIsoAxisPresent = false;
    bool resourceTierChangesTruthAuthority = false;
};

std::array<RoomBindingProfile, kRoomCount> default_room_profiles() noexcept;
Status validate_room_profiles(const std::array<RoomBindingProfile, kRoomCount>& profiles) noexcept;
Status validate_room_demands(const std::array<RoomResourceDemand, kRoomCount>& demands) noexcept;

Status bind_shared_lineage(const technical_backplane::v0_1::State& backplane,
                           SharedLineageBinding& out) noexcept;
Status bind_streaming_endpoints(streaming_v0_1::IRawTileSource& source,
                                streaming_v0_1::IStreamingSink& sink,
                                StreamingEndpointBinding& out) noexcept;

// Confirms that the already validated Illumination Room v0.2 dependency agrees
// with the all-room ISO-free scene contract.
Status validate_illumination_iso_boundary() noexcept;

// Admission over Building Runtime's existing execution waves. Source and sink
// resident bounds are counted once. Room demand is counted only while active;
// rebuildable cache retention follows ResourcePolicy::retainRebuildableCaches.
// No scheduling or scientific decision is recomputed here.
Status plan_adaptive_all_room_binding(const AdaptiveAllRoomRequest& request,
                                      AdaptiveAllRoomPlan& out) noexcept;

const char* status_name(Status status) noexcept;
const char* iso_access_name(CaptureIsoAccess access) noexcept;
const char* workspace_class_name(WorkspaceClass workspace) noexcept;

} // namespace truthraw::room_abi::v0_2

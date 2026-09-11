#include "room_abi_v0_2.h"

#include <algorithm>
#include <limits>

namespace truthraw::room_abi::v0_2 {
namespace {

constexpr std::size_t idx(RoomId room) noexcept {
    return static_cast<std::size_t>(room);
}

bool valid_room(RoomId room) noexcept {
    return idx(room) < kRoomCount;
}

bool valid_iso_access(CaptureIsoAccess access) noexcept {
    return static_cast<std::uint8_t>(access) <=
        static_cast<std::uint8_t>(CaptureIsoAccess::CalibratedSensorForwardOnly);
}

bool valid_workspace(WorkspaceClass workspace) noexcept {
    return static_cast<std::uint8_t>(workspace) <=
        static_cast<std::uint8_t>(WorkspaceClass::ZeroHiddenAllocation);
}

bool add_u64(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool room_total(const RoomResourceDemand& demand, std::uint64_t& out) noexcept {
    return add_u64(demand.transientPeakBytes, demand.rebuildableCacheBytes, out);
}

bool execution_contains_room(const ExecutionPlan& execution, RoomId room) noexcept {
    for (std::size_t i = 0; i < execution.placementCount; ++i) {
        if (execution.placements[i].room == room) return true;
    }
    return false;
}

Status validate_demand_against_profile(const RoomResourceDemand& demand,
                                       const RoomBindingProfile& profile) noexcept {
    std::uint64_t total = 0;
    if (!room_total(demand, total)) return Status::InvalidRoomDemand;
    switch (profile.workspace) {
        case WorkspaceClass::None:
        case WorkspaceClass::ZeroHiddenAllocation:
            return total == 0U ? Status::Ok : Status::InvalidRoomDemand;
        case WorkspaceClass::BoundedTransient:
            return demand.rebuildableCacheBytes == 0U ? Status::Ok : Status::InvalidRoomDemand;
        case WorkspaceClass::RebuildableCache:
            return demand.transientPeakBytes == 0U ? Status::Ok : Status::InvalidRoomDemand;
        case WorkspaceClass::MixedBounded:
            return Status::Ok;
    }
    return Status::InvalidRoomProfile;
}

} // namespace

std::array<RoomBindingProfile, kRoomCount> default_room_profiles() noexcept {
    using W = WorkspaceClass;
    using I = CaptureIsoAccess;
    std::array<RoomBindingProfile, kRoomCount> p{};

    p[0] = {RoomId::Archivist, W::None, I::CaptureProvenanceReadOnly,
            false, false, false, false, false, false};
    p[1] = {RoomId::MeasurementLab, W::None, I::CaptureProvenanceReadOnly,
            false, false, false, false, false, false};
    p[2] = {RoomId::Architect, W::MixedBounded, I::CaptureProvenanceReadOnly,
            true, false, false, false, false, false};
    p[3] = {RoomId::Restorer, W::MixedBounded, I::CaptureProvenanceReadOnly,
            false, false, false, false, false, false};
    p[4] = {RoomId::SceneRegistry, W::None, I::None,
            false, false, false, false, false, false};
    p[5] = {RoomId::Surveyor, W::MixedBounded, I::None,
            false, false, false, false, false, false};
    p[6] = {RoomId::ManifoldConditioning, W::ZeroHiddenAllocation, I::None,
            false, false, false, false, false, false};
    p[7] = {RoomId::LightingStudioCicm, W::BoundedTransient, I::CalibratedSensorForwardOnly,
            false, false, false, false, false, false};
    p[8] = {RoomId::RoomCapsule, W::MixedBounded, I::None,
            false, false, false, false, false, false};
    p[9] = {RoomId::Colorist, W::MixedBounded, I::None,
            false, false, false, false, false, false};
    p[10] = {RoomId::Finisher, W::MixedBounded, I::None,
             false, false, false, false, false, false};
    p[11] = {RoomId::Exporter, W::BoundedTransient, I::CaptureProvenanceReadOnly,
             false, true, false, false, false, false};
    return p;
}

Status validate_room_profiles(const std::array<RoomBindingProfile, kRoomCount>& profiles) noexcept {
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        const auto& p = profiles[i];
        if (!valid_room(p.room) || idx(p.room) != i || !valid_iso_access(p.captureIsoAccess) ||
            !valid_workspace(p.workspace)) {
            return Status::InvalidRoomProfile;
        }
        if (p.mayWriteSceneIso || p.isoMayAffectSceneCoordinates || p.isoMayAffectResourcePolicy ||
            p.resourceTierMayChangeTruthAuthority) {
            return Status::IsoSceneViolation;
        }
        if (p.captureIsoAccess == CaptureIsoAccess::CalibratedSensorForwardOnly &&
            p.room != RoomId::LightingStudioCicm) {
            return Status::InvalidRoomProfile;
        }
        if (p.workspace == WorkspaceClass::ZeroHiddenAllocation && p.room != RoomId::ManifoldConditioning) {
            return Status::InvalidRoomProfile;
        }
    }
    return Status::Ok;
}

Status validate_room_demands(const std::array<RoomResourceDemand, kRoomCount>& demands) noexcept {
    for (std::size_t i = 0; i < demands.size(); ++i) {
        const auto& d = demands[i];
        if (!d.valid || !valid_room(d.room) || idx(d.room) != i) return Status::InvalidRoomDemand;
        std::uint64_t total = 0;
        if (!room_total(d, total)) return Status::InvalidRoomDemand;
    }
    return Status::Ok;
}

Status bind_shared_lineage(const technical_backplane::v0_1::State& backplane,
                           SharedLineageBinding& out) noexcept {
    out = {};
    if (technical_backplane::v0_1::validate(backplane) != technical_backplane::v0_1::Status::Ok) {
        return Status::InvalidBackplane;
    }
    out.backplane = &backplane;
    out.valid = true;
    return Status::Ok;
}

Status bind_streaming_endpoints(streaming_v0_1::IRawTileSource& source,
                                streaming_v0_1::IStreamingSink& sink,
                                StreamingEndpointBinding& out) noexcept {
    out = {};
    static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t),
                  "Room ABI v0.2 requires size_t to fit in uint64_t accounting");
    const std::size_t sourceBytes = source.residentBytesUpperBound();
    const std::size_t sinkBytes = sink.residentBytesUpperBound();
    out.source = &source;
    out.sink = &sink;
    out.sourceResidentUpperBound = static_cast<std::uint64_t>(sourceBytes);
    out.sinkResidentUpperBound = static_cast<std::uint64_t>(sinkBytes);
    out.valid = true;
    return Status::Ok;
}

Status validate_illumination_iso_boundary() noexcept {
    constexpr auto illumination = illumination_room::v0_2::iso_boundary();
    constexpr auto allRooms = all_room_iso_boundary();
    if (illumination.sceneIsoAxisPresent || illumination.relativeScenarioHasIsoParameter ||
        illumination.relativeEvUsesIso || illumination.calibratedSensorForwardIsoPromotesToScene ||
        !illumination.calibratedSensorForwardMayCarryNominalIso) {
        return Status::IsoSceneViolation;
    }
    if (allRooms.sceneIsoAxisPresent || allRooms.corridorMayCarrySceneIsoAuthority ||
        allRooms.resourcePolicyMayDependOnSceneIso || allRooms.appearanceEvIsIso ||
        allRooms.counterfactualWorldScaleIsIso || allRooms.calibratedSensorForwardIsoPromotesToScene ||
        !allRooms.calibratedSensorForwardMayCarryNominalIso) {
        return Status::IsoSceneViolation;
    }
    return Status::Ok;
}

Status plan_adaptive_all_room_binding(const AdaptiveAllRoomRequest& request,
                                      AdaptiveAllRoomPlan& out) noexcept {
    out = {};
    if (request.execution.status != building_runtime::v0_1::Status::Ok ||
        !request.execution.resources.valid ||
        request.execution.placementCount > kRoomCount ||
        request.execution.waveCount > kMaxWaves) {
        return Status::InvalidExecutionPlan;
    }
    if (!request.lineage.valid || request.lineage.backplane == nullptr ||
        technical_backplane::v0_1::validate(*request.lineage.backplane) != technical_backplane::v0_1::Status::Ok) {
        return Status::InvalidBackplane;
    }
    if (!request.endpoints.valid || request.endpoints.source == nullptr || request.endpoints.sink == nullptr) {
        return Status::InvalidEndpoint;
    }
    const Status profileStatus = validate_room_profiles(request.profiles);
    if (profileStatus != Status::Ok) return profileStatus;
    const Status demandStatus = validate_room_demands(request.demands);
    if (demandStatus != Status::Ok) return demandStatus;
    const Status isoStatus = validate_illumination_iso_boundary();
    if (isoStatus != Status::Ok) return isoStatus;

    const ResourcePolicy& resources = request.execution.resources;
    if (resources.totalWorkingSetBudgetBytes == 0U || resources.maxConcurrentHeavyRooms == 0U) {
        return Status::InvalidExecutionPlan;
    }

    std::uint64_t endpointResident = 0;
    if (!add_u64(request.endpoints.sourceResidentUpperBound,
                 request.endpoints.sinkResidentUpperBound,
                 endpointResident)) {
        return Status::InvalidEndpoint;
    }
    if (endpointResident > resources.totalWorkingSetBudgetBytes) {
        return Status::ResidentBudgetExceeded;
    }

    // Room profiles can require endpoint roles, but v0.2 keeps handles borrowed
    // and shared. They are never multiplied by room count.
    for (std::size_t i = 0; i < kRoomCount; ++i) {
        if (!execution_contains_room(request.execution, static_cast<RoomId>(i))) continue;
        const auto& profile = request.profiles[i];
        if (profile.mayRequireStreamingSource && request.endpoints.source == nullptr) return Status::InvalidEndpoint;
        if (profile.mayRequireStreamingSink && request.endpoints.sink == nullptr) return Status::InvalidEndpoint;
    }

    std::uint64_t retainedRebuildable = 0;
    std::uint64_t peak = endpointResident;

    for (std::size_t wave = 0; wave < request.execution.waveCount; ++wave) {
        WaveMemoryInfo info{};
        info.wave = static_cast<std::uint8_t>(wave);
        info.retainedRebuildableBytes = retainedRebuildable;

        for (std::size_t pIndex = 0; pIndex < request.execution.placementCount; ++pIndex) {
            const auto& placement = request.execution.placements[pIndex];
            if (placement.wave != wave) continue;
            if (!valid_room(placement.room)) return Status::InvalidExecutionPlan;

            const auto& demand = request.demands[idx(placement.room)];
            const auto& profile = request.profiles[idx(placement.room)];
            const Status profileDemandStatus = validate_demand_against_profile(demand, profile);
            if (profileDemandStatus != Status::Ok) return profileDemandStatus;

            std::uint64_t demandTotal = 0;
            if (!room_total(demand, demandTotal)) return Status::InvalidRoomDemand;

            if (placement.heavyLease) {
                ++info.activeHeavyRooms;
                if (demandTotal > placement.memoryLeaseBytes) return Status::RoomLeaseExceeded;
            } else if (demandTotal != 0U) {
                // Building Runtime v0.1 grants no memory lease to non-heavy
                // rooms. v0.2 refuses to invent one behind the runtime's back.
                return Status::RoomLeaseExceeded;
            }

            if (!add_u64(info.activeTransientBytes, demand.transientPeakBytes, info.activeTransientBytes) ||
                !add_u64(info.activeRebuildableBytes, demand.rebuildableCacheBytes, info.activeRebuildableBytes)) {
                return Status::CapacityExceeded;
            }
        }

        if (info.activeHeavyRooms > resources.maxConcurrentHeavyRooms) {
            return Status::InvalidExecutionPlan;
        }

        std::uint64_t total = endpointResident;
        if (!add_u64(total, retainedRebuildable, total) ||
            !add_u64(total, info.activeTransientBytes, total) ||
            !add_u64(total, info.activeRebuildableBytes, total)) {
            return Status::CapacityExceeded;
        }
        info.totalResidentUpperBound = total;
        if (total > resources.totalWorkingSetBudgetBytes) return Status::ResidentBudgetExceeded;
        peak = std::max(peak, total);

        if (resources.retainRebuildableCaches) {
            if (!add_u64(retainedRebuildable, info.activeRebuildableBytes, retainedRebuildable)) {
                return Status::CapacityExceeded;
            }
        } else {
            retainedRebuildable = 0U;
        }

        out.waves[out.waveCount++] = info;
    }

    out.valid = true;
    out.resources = resources;
    out.lineage = request.lineage;
    out.endpoints = request.endpoints;
    out.peakResidentUpperBound = peak;
    out.sourceSinkCountedOnce = true;
    out.oneSharedBackplaneForAllRooms = true;
    out.sceneIsoAxisPresent = false;
    out.resourceTierChangesTruthAuthority = false;
    return Status::Ok;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::InvalidExecutionPlan: return "INVALID_EXECUTION_PLAN";
        case Status::InvalidBackplane: return "INVALID_BACKPLANE";
        case Status::InvalidEndpoint: return "INVALID_ENDPOINT";
        case Status::InvalidRoomProfile: return "INVALID_ROOM_PROFILE";
        case Status::InvalidRoomDemand: return "INVALID_ROOM_DEMAND";
        case Status::IsoSceneViolation: return "ISO_SCENE_VIOLATION";
        case Status::RoomLeaseExceeded: return "ROOM_LEASE_EXCEEDED";
        case Status::ResidentBudgetExceeded: return "RESIDENT_BUDGET_EXCEEDED";
        case Status::CapacityExceeded: return "CAPACITY_EXCEEDED";
        case Status::UpstreamFailure: return "UPSTREAM_FAILURE";
    }
    return "UNKNOWN";
}

const char* iso_access_name(CaptureIsoAccess access) noexcept {
    switch (access) {
        case CaptureIsoAccess::None: return "NONE";
        case CaptureIsoAccess::CaptureProvenanceReadOnly: return "CAPTURE_PROVENANCE_READ_ONLY";
        case CaptureIsoAccess::CalibratedSensorForwardOnly: return "CALIBRATED_SENSOR_FORWARD_ONLY";
    }
    return "UNKNOWN";
}

const char* workspace_class_name(WorkspaceClass workspace) noexcept {
    switch (workspace) {
        case WorkspaceClass::None: return "NONE";
        case WorkspaceClass::BoundedTransient: return "BOUNDED_TRANSIENT";
        case WorkspaceClass::RebuildableCache: return "REBUILDABLE_CACHE";
        case WorkspaceClass::MixedBounded: return "MIXED_BOUNDED";
        case WorkspaceClass::ZeroHiddenAllocation: return "ZERO_HIDDEN_ALLOCATION";
    }
    return "UNKNOWN";
}

} // namespace truthraw::room_abi::v0_2

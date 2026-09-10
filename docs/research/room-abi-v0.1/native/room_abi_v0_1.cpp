#include "room_abi_v0_1.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace truthraw::room_abi::v0_1 {
namespace {

bool valid_alignment(std::uint32_t a) noexcept {
    return a != 0U && (a & (a - 1U)) == 0U;
}

bool pointer_aligned(const void* p, std::uint32_t a) noexcept {
    if (!p || !valid_alignment(a)) return false;
    return (reinterpret_cast<std::uintptr_t>(p) & static_cast<std::uintptr_t>(a - 1U)) == 0U;
}

bool add_no_overflow(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

bool valid_room(RoomId room) noexcept {
    return static_cast<std::uint8_t>(room) < static_cast<std::uint8_t>(RoomId::Count);
}

bool valid_floor(TruthFloor floor) noexcept {
    return static_cast<std::uint8_t>(floor) <= static_cast<std::uint8_t>(TruthFloor::Projection);
}

bool valid_claim_status(ClaimStatus status) noexcept {
    return static_cast<std::uint8_t>(status) <= static_cast<std::uint8_t>(ClaimStatus::Promoted);
}

bool effective_room_budget(const ResourcePolicy& resources, std::uint64_t& out) noexcept {
    if (!resources.valid || resources.totalWorkingSetBudgetBytes == 0U ||
        resources.perHeavyRoomBudgetBytes == 0U) return false;
    out = std::min(resources.totalWorkingSetBudgetBytes, resources.perHeavyRoomBudgetBytes);
    return out != 0U;
}

LeaseRequest make_request(RoomId owner, BufferClass cls, std::uint64_t bytes) noexcept {
    LeaseRequest r{};
    r.owner = owner;
    r.bufferClass = cls;
    r.minimumBytes = bytes;
    r.preferredBytes = bytes;
    r.alignment = 64U;
    r.optional = bytes == 0U;
    return r;
}

} // namespace

Status validate_authority(const CorridorToken& token) noexcept {
    if (token.artifactHandle == 0U || token.provenanceFingerprint == 0U) return Status::InvalidAuthority;
    if (!valid_floor(token.floor) || !valid_claim_status(token.claimStatus)) return Status::InvalidAuthority;
    if (token.physicalFrameCount != 1U || token.independentEvidenceCount != 1U) return Status::InvalidAuthority;
    if (token.scientificMasterModified) return Status::InvalidAuthority;
    return Status::Ok;
}

Status acquire_lease(const RoomExecutionContext& context,
                     const LeaseRequest& request,
                     MemoryLease& out) noexcept {
    out = {};
    if (validate_authority(context.authority) != Status::Ok) return Status::InvalidAuthority;
    if (!valid_room(context.room) || !valid_room(request.owner) || request.owner != context.room) return Status::InvalidInput;
    if (!context.resources.valid) return Status::InvalidInput;
    if (!valid_alignment(request.alignment)) return Status::InvalidAlignment;
    if (request.preferredBytes < request.minimumBytes) return Status::InvalidInput;
    if (!context.memory.request || !context.memory.release) return Status::InvalidHost;
    if (request.minimumBytes == 0U && request.optional) return Status::Ok;
    if (request.minimumBytes == 0U) return Status::InvalidInput;

    std::uint64_t roomBudget = 0U;
    if (!effective_room_budget(context.resources, roomBudget)) return Status::LeaseDenied;
    if (request.minimumBytes > roomBudget) return Status::LeaseDenied;

    MemoryLease candidate{};
    if (!context.memory.request(context.memory.context, request, candidate)) return Status::LeaseDenied;

    const bool valid = candidate.active && candidate.data != nullptr && candidate.leaseId != 0U &&
        candidate.owner == request.owner && candidate.bufferClass == request.bufferClass &&
        candidate.capacityBytes >= request.minimumBytes &&
        candidate.capacityBytes <= roomBudget &&
        (request.preferredBytes == 0U || candidate.capacityBytes <= request.preferredBytes) &&
        candidate.alignment >= request.alignment &&
        valid_alignment(candidate.alignment) && pointer_aligned(candidate.data, candidate.alignment);
    if (!valid) {
        if (candidate.active) context.memory.release(context.memory.context, candidate);
        return Status::LeaseContractViolation;
    }
    out = candidate;
    return Status::Ok;
}

void release_lease(const RoomExecutionContext& context, MemoryLease& lease) noexcept {
    if (!lease.active) { lease = {}; return; }
    if (context.memory.release) context.memory.release(context.memory.context, lease);
    lease = {};
}

Status apply_runtime_resources_to_v47i(const ResourcePolicy& resources,
                                       const ::truthraw::ProcessOptions& base,
                                       ::truthraw::ProcessOptions& out) noexcept {
    if (!resources.valid || resources.tileSize == 0U || resources.cpuThreadsPerHeavyRoom == 0U) {
        return Status::InvalidInput;
    }
    if (resources.tileSize > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
        resources.cpuThreadsPerHeavyRoom > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return Status::InvalidInput;
    }
    out = base;
    out.tile.core = static_cast<int>(resources.tileSize);
    out.threads = static_cast<int>(resources.cpuThreadsPerHeavyRoom);
    return Status::Ok;
}

Status manifold_roundtrip_noalloc(const conditioning::v1::GaussianScalar& input,
                                  const conditioning::v1::Config& config,
                                  conditioning::v1::GaussianScalar& output,
                                  conditioning::v1::ConditioningGauge& gauge) noexcept {
    using conditioning::v1::ConditionedScalar;
    if (conditioning::v1::choose_best_conditioning_gauge(input, config, gauge) != conditioning::v1::Status::Ok) {
        return Status::UpstreamFailure;
    }
    ConditionedScalar conditioned{};
    if (conditioning::v1::condition_exact(input, gauge, conditioned) != conditioning::v1::Status::Ok) {
        return Status::UpstreamFailure;
    }
    if (conditioning::v1::decondition_exact(conditioned, output) != conditioning::v1::Status::Ok) {
        return Status::UpstreamFailure;
    }
    return Status::Ok;
}

Status acquire_room_capsule_leases(const RoomExecutionContext& context,
                                   const room_capsule::v0_1::RoomCapsulePlan& plan,
                                   RoomCapsuleLeaseSet& out) noexcept {
    out = {};
    if (context.room != RoomId::RoomCapsule || !plan.valid) return Status::InvalidInput;
    if (validate_authority(context.authority) != Status::Ok ||
        context.authority.floor != TruthFloor::Counterfactual) return Status::InvalidAuthority;
    if (plan.ledger.physicalFrameCount != 1U || plan.ledger.independentEvidenceCount != 1U ||
        plan.ledger.lightStatesAreEvidence || plan.ledger.scientificMasterModified || plan.ledger.zeroLineModified) {
        return Status::InvalidAuthority;
    }

    std::uint64_t sum = 0U;
    if (!add_no_overflow(plan.packedGeometryBytes, plan.vectorBoundaryBytes, sum) ||
        !add_no_overflow(sum, plan.tileWorkspaceBytes, sum)) return Status::InvalidInput;
    if (plan.estimatedPeakBytes < sum || plan.budgetBytes < plan.estimatedPeakBytes) {
        return Status::InvalidInput;
    }
    std::uint64_t roomBudget = 0U;
    if (!effective_room_budget(context.resources, roomBudget) || plan.estimatedPeakBytes > roomBudget) {
        return Status::LeaseDenied;
    }

    // Reserve the full conservative planner peak in room-level accounting.
    // Only the three concrete buffers below are handed to the MemoryHost; the
    // difference is planner metadata/headroom, not an invented pixel buffer.
    out.reservedPeakBytes = plan.estimatedPeakBytes;

    const LeaseRequest geometry = make_request(context.room, BufferClass::RebuildableCache, plan.packedGeometryBytes);
    const LeaseRequest boundary = make_request(context.room, BufferClass::TransientScratch, plan.vectorBoundaryBytes);
    const LeaseRequest workspace = make_request(context.room, BufferClass::TransientScratch, plan.tileWorkspaceBytes);

    Status s = acquire_lease(context, geometry, out.packedGeometry);
    if (s != Status::Ok) return s;
    s = acquire_lease(context, boundary, out.vectorBoundary);
    if (s != Status::Ok) { release_room_capsule_leases(context, out); return s; }
    s = acquire_lease(context, workspace, out.tileWorkspace);
    if (s != Status::Ok) { release_room_capsule_leases(context, out); return s; }

    out.totalGrantedBytes = out.packedGeometry.capacityBytes + out.vectorBoundary.capacityBytes + out.tileWorkspace.capacityBytes;
    return Status::Ok;
}

void release_room_capsule_leases(const RoomExecutionContext& context,
                                 RoomCapsuleLeaseSet& leases) noexcept {
    release_lease(context, leases.tileWorkspace);
    release_lease(context, leases.vectorBoundary);
    release_lease(context, leases.packedGeometry);
    leases.totalGrantedBytes = 0U;
    leases.reservedPeakBytes = 0U;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::InvalidAuthority: return "INVALID_AUTHORITY";
        case Status::InvalidHost: return "INVALID_HOST";
        case Status::InvalidAlignment: return "INVALID_ALIGNMENT";
        case Status::LeaseDenied: return "LEASE_DENIED";
        case Status::LeaseContractViolation: return "LEASE_CONTRACT_VIOLATION";
        case Status::UpstreamFailure: return "UPSTREAM_FAILURE";
    }
    return "UNKNOWN";
}

} // namespace truthraw::room_abi::v0_1

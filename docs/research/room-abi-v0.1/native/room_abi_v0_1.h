#pragma once

#include <cstddef>
#include <cstdint>

#include "building_runtime_v0_1.h"
#include "manifold_conditioning_v1.h"
#include "room_capsule_v0_1.h"
#include "truthraw/core.h"

namespace truthraw::room_abi::v0_1 {

using building_runtime::v0_1::ClaimStatus;
using building_runtime::v0_1::CorridorToken;
using building_runtime::v0_1::ResourcePolicy;
using building_runtime::v0_1::RoomId;
using building_runtime::v0_1::TruthFloor;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidAuthority,
    InvalidHost,
    InvalidAlignment,
    LeaseDenied,
    LeaseContractViolation,
    UpstreamFailure
};

enum class BufferClass : std::uint8_t {
    ImmutableInput = 0,
    DurableOutput,
    TransientScratch,
    RebuildableCache,
    Diagnostic
};

struct LeaseRequest {
    RoomId owner = RoomId::Archivist;
    BufferClass bufferClass = BufferClass::TransientScratch;
    std::uint64_t minimumBytes = 0;
    std::uint64_t preferredBytes = 0;
    std::uint32_t alignment = alignof(std::max_align_t);
    bool optional = false;
};

struct MemoryLease {
    void* data = nullptr;
    std::uint64_t capacityBytes = 0;
    std::uint64_t leaseId = 0;
    RoomId owner = RoomId::Archivist;
    BufferClass bufferClass = BufferClass::TransientScratch;
    std::uint32_t alignment = alignof(std::max_align_t);
    bool active = false;
};

using RequestLeaseFn = bool (*)(void* context, const LeaseRequest& request, MemoryLease& out) noexcept;
using ReleaseLeaseFn = void (*)(void* context, MemoryLease& lease) noexcept;

struct MemoryHost {
    void* context = nullptr;
    RequestLeaseFn request = nullptr;
    ReleaseLeaseFn release = nullptr;
};

struct RoomExecutionContext {
    RoomId room = RoomId::Archivist;
    CorridorToken authority{};
    ResourcePolicy resources{};
    MemoryHost memory{};
};

struct RoomCapsuleLeaseSet {
    MemoryLease packedGeometry{};
    MemoryLease vectorBoundary{};
    MemoryLease tileWorkspace{};
    std::uint64_t totalGrantedBytes = 0;
    // Full conservative peak reserved by the upstream Room Capsule planner.
    // This may exceed explicit lease bytes because the planner includes fixed
    // metadata/headroom that is not represented as a separate data buffer.
    std::uint64_t reservedPeakBytes = 0;
};

// Validate the authority independently from resource availability. A memory
// host is never permitted to upgrade floor, claim status, evidence count, or
// scientific-master authority.
Status validate_authority(const CorridorToken& token) noexcept;

Status acquire_lease(const RoomExecutionContext& context,
                     const LeaseRequest& request,
                     MemoryLease& out) noexcept;
void release_lease(const RoomExecutionContext& context, MemoryLease& lease) noexcept;

// Resource-only bridge: copies an existing v4.7i ProcessOptions and changes
// only tile core and thread count. Halo and all scientific/appearance options
// remain caller-owned and unchanged.
Status apply_runtime_resources_to_v47i(const ResourcePolicy& resources,
                                       const ::truthraw::ProcessOptions& base,
                                       ::truthraw::ProcessOptions& out) noexcept;

// Manifold Conditioning is a zero-allocation room in its current upstream
// contract. This adapter calls the real upstream functions and has no path to
// the memory host.
Status manifold_roundtrip_noalloc(const conditioning::v1::GaussianScalar& input,
                                  const conditioning::v1::Config& config,
                                  conditioning::v1::GaussianScalar& output,
                                  conditioning::v1::ConditioningGauge& gauge) noexcept;

// Acquire execution memory from an already validated upstream RoomCapsulePlan.
// Packed geometry is marked rebuildable cache; vector boundary and tile buffer
// are transient scratch. No sole copy of source evidence may be stored here.
Status acquire_room_capsule_leases(const RoomExecutionContext& context,
                                   const room_capsule::v0_1::RoomCapsulePlan& plan,
                                   RoomCapsuleLeaseSet& out) noexcept;
void release_room_capsule_leases(const RoomExecutionContext& context,
                                 RoomCapsuleLeaseSet& leases) noexcept;

const char* status_name(Status status) noexcept;

} // namespace truthraw::room_abi::v0_1

#include "room_abi_v0_1.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>

using namespace truthraw::room_abi::v0_1;
namespace br = truthraw::building_runtime::v0_1;
namespace rc = truthraw::room_capsule::v0_1;
namespace mc = truthraw::conditioning::v1;

#define CHECK(x) do { if (!(x)) { std::cerr << "CHECK failed: " #x << " line " << __LINE__ << '\n'; return 1; } } while (0)

struct alignas(64) Arena {
    std::array<std::byte, 8 * 1024 * 1024> bytes{};
    std::uint64_t offset = 0;
    std::uint64_t nextId = 1;
    std::uint32_t requests = 0;
    std::uint32_t releases = 0;
    std::uint32_t active = 0;
    bool corruptNext = false;
};

static std::uint64_t align_up(std::uint64_t v, std::uint32_t a) {
    return (v + a - 1U) & ~(static_cast<std::uint64_t>(a) - 1U);
}

static bool host_request(void* ctx, const LeaseRequest& req, MemoryLease& out) noexcept {
    auto& a = *static_cast<Arena*>(ctx);
    ++a.requests;
    const std::uint64_t start = align_up(a.offset, req.alignment);
    if (start > a.bytes.size() || req.preferredBytes > a.bytes.size() - start) return false;
    out.data = a.bytes.data() + start;
    out.capacityBytes = req.preferredBytes;
    out.leaseId = a.nextId++;
    out.owner = req.owner;
    out.bufferClass = req.bufferClass;
    out.alignment = req.alignment;
    out.active = true;
    a.offset = start + req.preferredBytes;
    ++a.active;
    if (a.corruptNext) { out.owner = br::RoomId::Archivist; a.corruptNext = false; }
    return true;
}

static void host_release(void* ctx, MemoryLease& lease) noexcept {
    auto& a = *static_cast<Arena*>(ctx);
    if (lease.active) {
        ++a.releases;
        if (a.active > 0) --a.active;
    }
    lease.active = false;
}

static br::CorridorToken valid_token(br::TruthFloor floor) {
    br::CorridorToken t{};
    t.artifactHandle = 77;
    t.provenanceFingerprint = 0x123456789abcdef0ULL;
    t.floor = floor;
    t.claimStatus = br::ClaimStatus::Candidate;
    return t;
}

int main() {
    // Derive real Building Runtime low/high policies, then bridge resources only.
    br::DeviceEnvelope low{};
    low.appMemoryClassMiB = 256;
    low.explicitMaxWorkingSetBytes = 32 * br::kMiB;
    low.currentlyAvailableBytes = 96 * br::kMiB;
    low.cpuThreadBudget = 4;
    low.lowRamDevice = true;
    br::ResourcePolicy lowPolicy{};
    CHECK(br::derive_resource_policy(low, lowPolicy) == br::Status::Ok);

    br::DeviceEnvelope high{};
    high.appMemoryClassMiB = 2048;
    high.currentlyAvailableBytes = 1024 * br::kMiB;
    high.cpuThreadBudget = 12;
    high.vulkanAvailable = true;
    br::ResourcePolicy highPolicy{};
    CHECK(br::derive_resource_policy(high, highPolicy) == br::Status::Ok);

    truthraw::ProcessOptions base{};
    base.tile.core = 384;
    base.tile.halo = 19;
    base.threads = 3;
    base.hdrEnabled = false;
    base.appearance = truthraw::AppearanceProfile::SkinSafeDetailedCrisp;
    base.keepScientificDiagnostics = true;
    base.sdrLutSize = 2048;
    truthraw::ProcessOptions lowOut{}, highOut{};
    CHECK(apply_runtime_resources_to_v47i(lowPolicy, base, lowOut) == Status::Ok);
    CHECK(apply_runtime_resources_to_v47i(highPolicy, base, highOut) == Status::Ok);
    CHECK(lowOut.tile.core == 128 && highOut.tile.core == 512);
    CHECK(lowOut.tile.halo == 19 && highOut.tile.halo == 19);
    CHECK(lowOut.hdrEnabled == base.hdrEnabled && highOut.hdrEnabled == base.hdrEnabled);
    CHECK(lowOut.appearance == base.appearance && highOut.appearance == base.appearance);
    CHECK(lowOut.keepScientificDiagnostics == base.keepScientificDiagnostics);
    CHECK(lowOut.sdrLutSize == base.sdrLutSize);

    // Authority fail-closed.
    auto token = valid_token(br::TruthFloor::Scene);
    CHECK(validate_authority(token) == Status::Ok);
    auto bad = token;
    bad.independentEvidenceCount = 2;
    CHECK(validate_authority(bad) == Status::InvalidAuthority);
    bad = token;
    bad.scientificMasterModified = true;
    CHECK(validate_authority(bad) == Status::InvalidAuthority);
    bad = token;
    bad.floor = static_cast<br::TruthFloor>(255);
    CHECK(validate_authority(bad) == Status::InvalidAuthority);
    bad = token;
    bad.claimStatus = static_cast<br::ClaimStatus>(255);
    CHECK(validate_authority(bad) == Status::InvalidAuthority);

    // Manifold adapter executes real upstream exact roundtrip with zero host calls.
    static Arena arena{};
    MemoryHost host{&arena, host_request, host_release};
    mc::GaussianScalar in{0.03125F, 0.0078125F}, out{};
    mc::ConditioningGauge gauge{};
    const auto beforeRequests = arena.requests;
    CHECK(manifold_roundtrip_noalloc(in, mc::Config{}, out, gauge) == Status::Ok);
    CHECK(std::bit_cast<std::uint32_t>(in.mean) == std::bit_cast<std::uint32_t>(out.mean));
    CHECK(std::bit_cast<std::uint32_t>(in.sigma) == std::bit_cast<std::uint32_t>(out.sigma));
    CHECK(arena.requests == beforeRequests);

    // Ask real Room Capsule planner for a heavy-room plan.
    rc::RoomCapsuleRequest request{};
    request.sourceWidth = 1920;
    request.sourceHeight = 1080;
    request.room = {128, 96, 640, 480};
    request.vectorBoundaryVertexCount = 64;
    request.memory.appMemoryClassMiB = 512;
    request.memory.explicitMaxWorkingSetBytes = 8 * br::kMiB;
    request.memory.gpuAvailable = true;
    rc::RoomCapsulePlan plan{};
    CHECK(rc::plan_room_capsule(request, plan) == rc::Status::Ok);
    CHECK(plan.valid);

    RoomExecutionContext context{};
    context.room = br::RoomId::RoomCapsule;
    context.authority = valid_token(br::TruthFloor::Counterfactual);
    context.resources = highPolicy;
    context.memory = host;
    RoomCapsuleLeaseSet leases{};
    CHECK(acquire_room_capsule_leases(context, plan, leases) == Status::Ok);
    CHECK(leases.packedGeometry.bufferClass == BufferClass::RebuildableCache || plan.packedGeometryBytes == 0);
    CHECK(leases.tileWorkspace.bufferClass == BufferClass::TransientScratch || plan.tileWorkspaceBytes == 0);
    CHECK(leases.reservedPeakBytes == plan.estimatedPeakBytes);
    CHECK(leases.totalGrantedBytes <= leases.reservedPeakBytes);
    CHECK(arena.active >= 1);
    release_room_capsule_leases(context, leases);
    CHECK(arena.active == 0);

    // Room Capsule execution authority is floor-bound. A Scene-floor token cannot
    // obtain Counterfactual-room memory even when every evidence count is otherwise valid.
    const auto floorRequestCount = arena.requests;
    RoomExecutionContext wrongFloor = context;
    wrongFloor.authority = valid_token(br::TruthFloor::Scene);
    CHECK(acquire_room_capsule_leases(wrongFloor, plan, leases) == Status::InvalidAuthority);
    CHECK(arena.requests == floorRequestCount);

    // The upstream plan includes conservative metadata/headroom in estimatedPeakBytes.
    // ABI budget admission must use that full peak, not only concrete lease bytes.
    const auto peakRequestCount = arena.requests;
    auto peakDeniedPlan = plan;
    RoomExecutionContext peakTiny = context;
    peakTiny.resources.perHeavyRoomBudgetBytes = plan.estimatedPeakBytes - 1U;
    CHECK(acquire_room_capsule_leases(peakTiny, peakDeniedPlan, leases) == Status::LeaseDenied);
    CHECK(arena.requests == peakRequestCount);
    auto malformedPeakPlan = plan;
    malformedPeakPlan.estimatedPeakBytes = 1U;
    CHECK(acquire_room_capsule_leases(context, malformedPeakPlan, leases) == Status::InvalidInput);
    CHECK(arena.requests == peakRequestCount);

    // Host contract corruption is detected and rolled back.
    LeaseRequest one{};
    one.owner = br::RoomId::RoomCapsule;
    one.bufferClass = BufferClass::TransientScratch;
    one.minimumBytes = 256;
    one.preferredBytes = 256;
    one.alignment = 64;
    arena.corruptNext = true;
    MemoryLease badLease{};
    CHECK(acquire_lease(context, one, badLease) == Status::LeaseContractViolation);
    CHECK(arena.active == 0);

    // Room mismatch and invalid alignment fail before host allocation.
    const auto requestCount = arena.requests;
    one.owner = br::RoomId::Architect;
    CHECK(acquire_lease(context, one, badLease) == Status::InvalidInput);
    CHECK(arena.requests == requestCount);
    one.owner = br::RoomId::RoomCapsule;
    one.alignment = 48;
    CHECK(acquire_lease(context, one, badLease) == Status::InvalidAlignment);
    CHECK(arena.requests == requestCount);

    // Budget exhaustion is explicit, never a silent algorithm downgrade.
    RoomExecutionContext tiny = context;
    tiny.resources.perHeavyRoomBudgetBytes = 128;
    one.alignment = 64;
    one.minimumBytes = 256;
    one.preferredBytes = 256;
    CHECK(acquire_lease(tiny, one, badLease) == Status::LeaseDenied);

    // A forged "valid" policy with zero positive-lease budget fails closed.
    RoomExecutionContext zeroBudget = context;
    zeroBudget.resources.totalWorkingSetBudgetBytes = 0;
    const auto zeroBudgetRequests = arena.requests;
    CHECK(acquire_lease(zeroBudget, one, badLease) == Status::LeaseDenied);
    CHECK(arena.requests == zeroBudgetRequests);

    // If minimum fits but preferred exceeds the runtime budget, an over-grant from
    // the host is rejected against the actual granted capacity.
    RoomExecutionContext capped = context;
    capped.resources.totalWorkingSetBudgetBytes = 128;
    capped.resources.perHeavyRoomBudgetBytes = 128;
    one.minimumBytes = 64;
    one.preferredBytes = 256;
    one.alignment = 64;
    const auto beforeOvergrant = arena.requests;
    CHECK(acquire_lease(capped, one, badLease) == Status::LeaseContractViolation);
    CHECK(arena.requests == beforeOvergrant + 1);
    CHECK(arena.active == 0);

    // Invalid room enum is rejected before the host is called.
    RoomExecutionContext invalidRoom = context;
    invalidRoom.room = static_cast<br::RoomId>(255);
    one.owner = static_cast<br::RoomId>(255);
    const auto invalidRoomRequests = arena.requests;
    CHECK(acquire_lease(invalidRoom, one, badLease) == Status::InvalidInput);
    CHECK(arena.requests == invalidRoomRequests);

    std::cout << "ROOM_ABI_V0_1_TEST_PASS\n";
    std::cout << "lowTile=" << lowOut.tile.core << '\n';
    std::cout << "highTile=" << highOut.tile.core << '\n';
    std::cout << "lowThreads=" << lowOut.threads << '\n';
    std::cout << "highThreads=" << highOut.threads << '\n';
    std::cout << "manifoldGaugeEv=" << gauge.ev << '\n';
    std::cout << "roomCapsulePeak=" << plan.estimatedPeakBytes << '\n';
    std::cout << "leaseRequests=" << arena.requests << '\n';
    std::cout << "leaseReleases=" << arena.releases << '\n';
    return 0;
}

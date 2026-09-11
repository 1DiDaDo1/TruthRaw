#include "illumination_room_v0_2.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace truthraw::illumination_room::v0_2;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(2);
    }
}

bool near(double a, double b, double eps = 1.0e-9) { return std::abs(a - b) <= eps; }
bool nearf(float a, float b, float eps = 1.0e-6F) { return std::abs(a - b) <= eps; }

struct HostState { std::uint64_t nextLeaseId = 1U; };

bool request_lease(void* opaque,
                   const truthraw::room_abi::v0_1::LeaseRequest& request,
                   truthraw::room_abi::v0_1::MemoryLease& out) noexcept {
    auto* host = static_cast<HostState*>(opaque);
    if (!host || request.preferredBytes == 0U) return false;
    void* p = nullptr;
    if (posix_memalign(&p, request.alignment, static_cast<std::size_t>(request.preferredBytes)) != 0 || !p) {
        return false;
    }
    out.data = p;
    out.capacityBytes = request.preferredBytes;
    out.leaseId = host->nextLeaseId++;
    out.owner = request.owner;
    out.bufferClass = request.bufferClass;
    out.alignment = request.alignment;
    out.active = true;
    return true;
}

void release_lease(void*, truthraw::room_abi::v0_1::MemoryLease& lease) noexcept {
    std::free(lease.data);
    lease = {};
}

truthraw::technical_backplane::v0_1::State valid_backplane() {
    truthraw::technical_backplane::v0_1::State state{};
    state.sourceEvidenceHash.fill(0x11U);
    state.scientificMasterHash.fill(0x22U);
    state.zeroLineHash.fill(0x33U);
    state.sceneScaleHash.fill(0x44U);
    state.physicalFrameCount = 1U;
    state.independentEvidenceCount = 1U;
    state.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    state.forbiddenFlags = 0U;
    return state;
}

} // namespace

int main() {
    using truthraw::building_runtime::v0_1::ClaimStatus;
    using truthraw::building_runtime::v0_1::ComputeBackend;
    using truthraw::building_runtime::v0_1::RoomId;
    using truthraw::building_runtime::v0_1::TruthFloor;
    using truthraw::room_capsule::v0_1::LightKind;
    using truthraw::room_capsule::v0_1::RelightSemantics;

    AdaptivePlanRequest low{};
    low.sourceWidth = 16320U;
    low.sourceHeight = 12288U;
    low.room = {0U, 0U, 16320U, 12288U};
    low.vectorBoundaryVertexCount = 64U;
    low.resources.valid = true;
    low.resources.totalWorkingSetBudgetBytes = 32ULL * 1024ULL * 1024ULL;
    low.resources.perHeavyRoomBudgetBytes = 8ULL * 1024ULL * 1024ULL;
    low.resources.tileSize = 128U;
    low.resources.cpuThreadsPerHeavyRoom = 1U;
    low.resources.preferredBackend = ComputeBackend::CpuBaseline;

    AdaptivePlan lowPlan{};
    require(plan_from_runtime(low, lowPlan) == Status::Ok, "low-tier runtime plan");
    require(lowPlan.valid && lowPlan.capsule.valid, "low-tier plan valid");
    require(lowPlan.capsule.tileSize == 128U, "runtime owns low-tier tile size");
    require(lowPlan.capsule.geometryDownsample >= 16U, "low-tier geometry degrades to fit lease");
    require(lowPlan.capsule.estimatedPeakBytes <= lowPlan.roomBudgetBytes, "low-tier peak within room budget");

    AdaptivePlanRequest high = low;
    high.resources.totalWorkingSetBudgetBytes = 256ULL * 1024ULL * 1024ULL;
    high.resources.perHeavyRoomBudgetBytes = 64ULL * 1024ULL * 1024ULL;
    high.resources.tileSize = 512U;
    high.resources.cpuThreadsPerHeavyRoom = 4U;
    high.resources.preferredBackend = ComputeBackend::OptionalVulkan;

    AdaptivePlan highPlan{};
    require(plan_from_runtime(high, highPlan) == Status::Ok, "high-tier runtime plan");
    require(highPlan.capsule.tileSize == 512U, "runtime owns high-tier tile size");
    require(highPlan.capsule.geometryDownsample <= lowPlan.capsule.geometryDownsample,
            "more resources never force coarser local geometry");
    require(highPlan.capsule.gpuAccelerationOptional, "GPU remains optional acceleration only");

    AdaptivePlanRequest tiny = low;
    tiny.resources.perHeavyRoomBudgetBytes = 1ULL * 1024ULL * 1024ULL;
    AdaptivePlan tinyPlan{};
    require(plan_from_runtime(tiny, tinyPlan) == Status::BudgetTooSmall,
            "runtime budget below scientific room minimum fails closed");

    auto backplane = valid_backplane();
    truthraw::counterfactual::v1::SceneBinding binding{};
    require(bind_scene_from_backplane(backplane, binding) == Status::Ok,
            "backplane identities bind into CICM");
    require(binding.sourceEvidenceId.size() == 64U && binding.zeroLineId.size() == 64U &&
            binding.sceneScaleId.size() == 64U && binding.scientificMasterSha256.size() == 64U,
            "all lineage identities are hash-bound");

    auto brokenBackplane = backplane;
    brokenBackplane.zeroLineHash.fill(0U);
    truthraw::counterfactual::v1::SceneBinding rejectedBinding{};
    require(bind_scene_from_backplane(brokenBackplane, rejectedBinding) == Status::BackplaneInvalid,
            "missing zero-line identity fails closed");

    RelativeScenario day{};
    day.kind = ScenarioKind::RelativeDaylightStudy;
    day.neutralIlluminationScale = 4.0;
    day.boundary.ambientRgb = {0.25F, 0.25F, 0.25F};
    day.boundary.dominantIncomingDirection = {0.0F, 0.0F, 1.0F};
    day.boundary.directionalWeight = 0.25F;

    truthraw::counterfactual::v1::RelativeWorldPrediction dayPrediction{};
    require(simulate_relative_scenario(1.0, 1.0, "relative-day-study", day, binding, dayPrediction) == Status::Ok,
            "relative daylight CICM prediction");
    require(near(dayPrediction.counterfactualDeltaEv, 2.0), "4x relative day scale is +2 EV");
    require(!dayPrediction.physicalSnrAvailable, "relative daylight cannot invent physical SNR");
    require(dayPrediction.ledger.physicalFrameCount == 1U && dayPrediction.ledger.independentEvidenceCount == 1U &&
            !dayPrediction.ledger.counterfactualObservationsAreEvidence &&
            !dayPrediction.ledger.scientificMasterModified && !dayPrediction.ledger.zeroLineModified,
            "day study preserves evidence and zero-line invariants");

    RelativeScenario night = day;
    night.kind = ScenarioKind::RelativeNightStudy;
    night.neutralIlluminationScale = 1.0 / 16.0;
    night.boundary.ambientRgb = {0.08F, 0.09F, 0.12F};
    night.boundary.directionalWeight = 0.0F;

    truthraw::counterfactual::v1::RelativeWorldPrediction nightPrediction{};
    require(simulate_relative_scenario(1.0, 1.0, "relative-night-study", night, binding, nightPrediction) == Status::Ok,
            "relative night CICM prediction");
    require(near(nightPrediction.counterfactualDeltaEv, -4.0), "1/16 relative night scale is -4 EV");
    require(!nightPrediction.physicalSnrAvailable, "relative night cannot invent physical SNR");

    truthraw::room_capsule::v0_1::RoomSampleRuntime sample{};
    sample.position = {0.0F, 0.0F, 0.0F};
    sample.normal = {0.0F, 0.0F, 1.0F};
    sample.visibility = 1.0F;
    sample.confidence = 0.8F;
    sample.insideRoom = true;

    truthraw::room_capsule::v0_1::LightState sunStudy{};
    sunStudy.kind = LightKind::Directional;
    sunStudy.semantics = RelightSemantics::RelativeAppearanceOnly;
    sunStudy.direction = {0.0F, 0.0F, -1.0F};
    sunStudy.rgb = {1.0F, 0.92F, 0.80F};
    sunStudy.relativePower = 1.25F;
    std::array<truthraw::room_capsule::v0_1::LightState, 1> lights{sunStudy};

    truthraw::room_capsule::v0_1::RelativeLightResult lowResult{};
    truthraw::room_capsule::v0_1::RelativeLightResult highResult{};
    require(evaluate_relative_sample(sample, day, lights, lowResult) == Status::Ok,
            "low-tier day sample evaluates");
    require(evaluate_relative_sample(sample, day, lights, highResult) == Status::Ok,
            "high-tier day sample evaluates");
    for (std::size_t c = 0; c < 3U; ++c) {
        require(nearf(lowResult.relativeMultiplier[c], highResult.relativeMultiplier[c]),
                "resource tier cannot change illumination math");
    }
    require(!lowResult.physicalRelightClaimAllowed && !lowResult.ledger.lightStatesAreEvidence &&
            !lowResult.ledger.scientificMasterModified && !lowResult.ledger.zeroLineModified,
            "local relight remains appearance-only counterfactual state");

    auto physical = sunStudy;
    physical.semantics = RelightSemantics::CalibratedIntrinsicRelightReserved;
    std::array<truthraw::room_capsule::v0_1::LightState, 1> physicalLights{physical};
    truthraw::room_capsule::v0_1::RelativeLightResult blocked{};
    require(evaluate_relative_sample(sample, day, physicalLights, blocked) == Status::PhysicalRelightBlocked,
            "geometry/BRDF/spectral physical relight stays blocked");
    require(!blocked.valid, "blocked physical path leaves no apply-able result");

    HostState host{};
    truthraw::room_abi::v0_1::RoomExecutionContext context{};
    context.room = RoomId::RoomCapsule;
    context.authority.artifactHandle = 0x1234U;
    context.authority.provenanceFingerprint = 0x5678U;
    context.authority.floor = TruthFloor::Counterfactual;
    context.authority.claimStatus = ClaimStatus::Candidate;
    context.authority.physicalFrameCount = 1U;
    context.authority.independentEvidenceCount = 1U;
    context.authority.scientificMasterModified = false;
    context.resources = low.resources;
    context.memory.context = &host;
    context.memory.request = request_lease;
    context.memory.release = release_lease;

    truthraw::room_abi::v0_1::RoomCapsuleLeaseSet leases{};
    require(acquire_runtime_leases(context, lowPlan, leases) == Status::Ok,
            "Illumination Room obtains memory only through Room ABI leases");
    require(leases.totalGrantedBytes > 0U && leases.reservedPeakBytes == lowPlan.capsule.estimatedPeakBytes,
            "lease accounting preserves conservative room peak");
    release_runtime_leases(context, leases);
    require(leases.totalGrantedBytes == 0U && leases.reservedPeakBytes == 0U,
            "all room leases released at room boundary");

    auto badAuthority = context;
    badAuthority.authority.artifactHandle = 0U;
    truthraw::room_abi::v0_1::RoomCapsuleLeaseSet denied{};
    require(acquire_runtime_leases(badAuthority, lowPlan, denied) == Status::InvalidAuthority,
            "memory availability cannot upgrade invalid truth authority");

    std::cout << "low_geometry_scale=1/" << lowPlan.capsule.geometryDownsample << '\n';
    std::cout << "high_geometry_scale=1/" << highPlan.capsule.geometryDownsample << '\n';
    std::cout << "low_tile=" << lowPlan.capsule.tileSize << '\n';
    std::cout << "high_tile=" << highPlan.capsule.tileSize << '\n';
    std::cout << "day_delta_ev=" << dayPrediction.counterfactualDeltaEv << '\n';
    std::cout << "night_delta_ev=" << nightPrediction.counterfactualDeltaEv << '\n';
    std::cout << "ILLUMINATION_ROOM_V0_2_PASS\n";
    return 0;
}

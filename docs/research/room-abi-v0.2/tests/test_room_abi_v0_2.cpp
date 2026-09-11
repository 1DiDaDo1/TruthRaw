#include "room_abi_v0_2.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace truthraw::room_abi::v0_2;

namespace {

constexpr std::uint64_t MiB = 1024ULL * 1024ULL;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(2);
    }
}

template <class T>
concept HasNominalIso = requires(T value) { value.nominalIso; };

static_assert(!HasNominalIso<RoomBindingProfile>, "room ABI profile must not contain nominal ISO");
static_assert(!HasNominalIso<RoomResourceDemand>, "resource demand must not contain nominal ISO");
static_assert(!HasNominalIso<AdaptiveAllRoomRequest>, "all-room request must not contain nominal ISO");
static_assert(!HasNominalIso<AdaptiveAllRoomPlan>, "all-room plan must not contain nominal ISO");
static_assert(!HasNominalIso<StreamingEndpointBinding>, "streaming binding must not contain nominal ISO");
static_assert(!HasNominalIso<SharedLineageBinding>, "lineage binding must not contain nominal ISO");

class DummySource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit DummySource(std::size_t resident) : resident_(resident) {}
    const truthraw::DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override { return resident_; }
    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect&, std::uint16_t*, std::size_t, float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus readRowBias(int, int, float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus readColBias(int, int, float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
private:
    truthraw::DngMetadata metadata_{};
    std::size_t resident_ = 0;
};

class DummySink final : public truthraw::streaming_v0_1::IStreamingSink {
public:
    explicit DummySink(std::size_t resident) : resident_(resident) {}
    std::size_t residentBytesUpperBound() const override { return resident_; }
    truthraw::streaming_v0_1::StreamStatus beginFrame(
        int, int, truthraw::Orientation, const truthraw::ExposurePlan&, bool, bool) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeSdrTile(
        const truthraw::TileRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeHalfLogGainBlock(
        const truthraw::streaming_v0_1::HalfStateRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus writeStage2DiagnosticTile(
        const truthraw::TileRect&, const float*, std::size_t) override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
    truthraw::streaming_v0_1::StreamStatus finishFrame() override {
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }
private:
    std::size_t resident_ = 0;
};

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

std::array<RoomResourceDemand, kRoomCount> default_demands() {
    using truthraw::building_runtime::v0_1::RoomId;
    std::array<RoomResourceDemand, kRoomCount> d{};
    for (std::size_t i = 0; i < d.size(); ++i) {
        d[i].room = static_cast<RoomId>(i);
        d[i].valid = true;
    }
    d[static_cast<std::size_t>(RoomId::Architect)].transientPeakBytes = 6U * MiB;
    d[static_cast<std::size_t>(RoomId::Architect)].rebuildableCacheBytes = 2U * MiB;
    d[static_cast<std::size_t>(RoomId::Restorer)].transientPeakBytes = 5U * MiB;
    d[static_cast<std::size_t>(RoomId::Restorer)].rebuildableCacheBytes = 2U * MiB;
    d[static_cast<std::size_t>(RoomId::Surveyor)].transientPeakBytes = 4U * MiB;
    d[static_cast<std::size_t>(RoomId::Surveyor)].rebuildableCacheBytes = 2U * MiB;
    // ManifoldConditioning is deliberately zero-hidden-allocation.
    d[static_cast<std::size_t>(RoomId::LightingStudioCicm)].transientPeakBytes = 2U * MiB;
    d[static_cast<std::size_t>(RoomId::RoomCapsule)].transientPeakBytes = 5U * MiB;
    d[static_cast<std::size_t>(RoomId::RoomCapsule)].rebuildableCacheBytes = 2U * MiB;
    d[static_cast<std::size_t>(RoomId::Colorist)].transientPeakBytes = 4U * MiB;
    d[static_cast<std::size_t>(RoomId::Colorist)].rebuildableCacheBytes = 1U * MiB;
    d[static_cast<std::size_t>(RoomId::Finisher)].transientPeakBytes = 4U * MiB;
    d[static_cast<std::size_t>(RoomId::Finisher)].rebuildableCacheBytes = 1U * MiB;
    d[static_cast<std::size_t>(RoomId::Exporter)].transientPeakBytes = 3U * MiB;
    return d;
}

truthraw::building_runtime::v0_1::RuntimeResult evaluate_all_rooms() {
    using namespace truthraw::building_runtime::v0_1;
    ScientificState scientific{};
    scientific.physicalFrameCount = 1U;
    scientific.independentEvidenceCount = 1U;
    scientific.scientificMasterModified = false;
    scientific.sigmaKnown = true;
    scientific.sigma = 1.0;
    scientific.physicalCaptureEv = 0.0;
    scientific.claimStatus = ClaimStatus::Candidate;

    AppearanceState appearance{};
    CaptureProvenance provenance{};
    provenance.immutableEvidenceFingerprint = 0x12345678ULL;
    provenance.physicalFrameCount = 1U;
    provenance.independentEvidenceCount = 1U;

    ActivationRequest request{};
    request.requested.fill(true);
    const auto graph = default_room_graph();
    RuntimeResult runtime{};
    require(evaluate(scientific, appearance, provenance, request, graph, runtime) ==
                truthraw::building_runtime::v0_1::Status::Ok,
            "Building Runtime admits full requested room graph");
    require(runtime.decisionCount == kRoomCount, "all 12 room decisions present");
    for (std::size_t i = 0; i < runtime.decisionCount; ++i) {
        require(runtime.decisions[i].execute, "all requested rooms execute when dependencies/evidence are satisfied");
    }
    return runtime;
}

truthraw::building_runtime::v0_1::ExecutionPlan make_execution(
    const truthraw::building_runtime::v0_1::RuntimeResult& runtime,
    bool highTier) {
    using namespace truthraw::building_runtime::v0_1;
    DeviceEnvelope device{};
    device.appMemoryClassMiB = highTier ? 2048U : 256U;
    device.cpuThreadBudget = highTier ? 12U : 4U;
    device.lowRamDevice = !highTier;
    device.vulkanAvailable = highTier;
    device.foreground = true;
    device.thermalState = ThermalState::Nominal;

    ResourcePolicy policy{};
    require(derive_resource_policy(device, policy) == truthraw::building_runtime::v0_1::Status::Ok,
            "resource policy derived");
    require(policy.valid, "resource policy valid");
    if (highTier) {
        require(policy.totalWorkingSetBudgetBytes == 256U * MiB, "high tier 256 MiB total budget");
        require(policy.maxConcurrentHeavyRooms == 4U, "high tier allows four heavy rooms");
        require(policy.tileSize == 512U, "high tier tile 512");
    } else {
        require(policy.totalWorkingSetBudgetBytes == 32U * MiB, "low tier 32 MiB total budget");
        require(policy.maxConcurrentHeavyRooms == 1U, "low tier serial heavy rooms");
        require(policy.tileSize == 128U, "low tier tile 128");
    }

    ExecutionPlan execution{};
    const auto graph = default_room_graph();
    require(plan_execution(runtime, graph, policy, execution) == truthraw::building_runtime::v0_1::Status::Ok,
            "Building Runtime execution plan");
    require(execution.status == truthraw::building_runtime::v0_1::Status::Ok,
            "execution plan status OK");
    return execution;
}

} // namespace

int main() {
    using truthraw::building_runtime::v0_1::RoomId;

    constexpr auto iso = all_room_iso_boundary();
    require(!iso.sceneIsoAxisPresent, "new house has no ISO scene axis");
    require(!iso.corridorMayCarrySceneIsoAuthority, "corridor cannot carry scene ISO authority");
    require(!iso.resourcePolicyMayDependOnSceneIso, "resource policy cannot depend on scene ISO");
    require(!iso.appearanceEvIsIso, "appearance EV is not ISO");
    require(!iso.counterfactualWorldScaleIsIso, "counterfactual illumination scale is not ISO");
    require(iso.calibratedSensorForwardMayCarryNominalIso, "separate calibrated sensor forward may carry nominal ISO");
    require(!iso.calibratedSensorForwardIsoPromotesToScene, "sensor-forward ISO never promotes into scene");
    require(validate_illumination_iso_boundary() == Status::Ok,
            "Illumination Room ISO-free contract agrees with all-room ABI");

    auto profiles = default_room_profiles();
    require(validate_room_profiles(profiles) == Status::Ok, "default profiles valid");
    require(profiles[static_cast<std::size_t>(RoomId::LightingStudioCicm)].captureIsoAccess ==
                CaptureIsoAccess::CalibratedSensorForwardOnly,
            "CICM ISO access is sensor-forward-only");
    require(profiles[static_cast<std::size_t>(RoomId::ManifoldConditioning)].workspace ==
                WorkspaceClass::ZeroHiddenAllocation,
            "Manifold preserves zero-hidden-allocation contract");

    auto backplane = valid_backplane();
    SharedLineageBinding lineage{};
    require(bind_shared_lineage(backplane, lineage) == Status::Ok, "one shared backplane binds");
    require(lineage.backplane == &backplane, "lineage is borrowed, not copied");

    DummySource source(1U * MiB);
    DummySink sink(1U * MiB);
    StreamingEndpointBinding endpoints{};
    require(bind_streaming_endpoints(source, sink, endpoints) == Status::Ok,
            "borrowed source/sink endpoints bind");
    require(endpoints.sourceResidentUpperBound == 1U * MiB && endpoints.sinkResidentUpperBound == 1U * MiB,
            "source/sink resident bounds preserved exactly");

    const auto runtime = evaluate_all_rooms();
    const auto lowExecution = make_execution(runtime, false);
    const auto highExecution = make_execution(runtime, true);
    const auto demands = default_demands();
    require(validate_room_demands(demands) == Status::Ok, "room demands valid");

    AdaptiveAllRoomRequest lowRequest{};
    lowRequest.execution = lowExecution;
    lowRequest.lineage = lineage;
    lowRequest.endpoints = endpoints;
    lowRequest.profiles = profiles;
    lowRequest.demands = demands;

    AdaptiveAllRoomPlan lowPlan{};
    require(plan_adaptive_all_room_binding(lowRequest, lowPlan) == Status::Ok,
            "low-tier all-room admission succeeds");
    require(lowPlan.valid, "low-tier all-room plan valid");
    require(lowPlan.peakResidentUpperBound <= lowExecution.resources.totalWorkingSetBudgetBytes,
            "low-tier resident peak stays within 32 MiB");
    require(lowPlan.sourceSinkCountedOnce && lowPlan.oneSharedBackplaneForAllRooms,
            "source/sink and lineage are shared once");
    require(!lowPlan.sceneIsoAxisPresent && !lowPlan.resourceTierChangesTruthAuthority,
            "low-tier resource plan cannot change truth or create ISO axis");

    AdaptiveAllRoomRequest highRequest = lowRequest;
    highRequest.execution = highExecution;
    AdaptiveAllRoomPlan highPlan{};
    require(plan_adaptive_all_room_binding(highRequest, highPlan) == Status::Ok,
            "high-tier all-room admission succeeds");
    require(highPlan.valid, "high-tier all-room plan valid");
    require(highPlan.peakResidentUpperBound <= highExecution.resources.totalWorkingSetBudgetBytes,
            "high-tier resident peak stays within 256 MiB");
    require(highPlan.lineage.backplane == lowPlan.lineage.backplane,
            "low/high tiers share exact same Backplane identity object");
    require(!highPlan.sceneIsoAxisPresent && !highPlan.resourceTierChangesTruthAuthority,
            "high-tier resources cannot upgrade scientific authority");
    require(lowPlan.resources.tileSize != highPlan.resources.tileSize,
            "resource tiers may change tile size");

    auto badIsoProfiles = profiles;
    badIsoProfiles[static_cast<std::size_t>(RoomId::Colorist)].mayWriteSceneIso = true;
    lowRequest.profiles = badIsoProfiles;
    AdaptiveAllRoomPlan rejectedIso{};
    require(plan_adaptive_all_room_binding(lowRequest, rejectedIso) == Status::IsoSceneViolation,
            "reintroduced scene ISO fails closed");

    lowRequest.profiles = profiles;
    auto hiddenManifold = demands;
    hiddenManifold[static_cast<std::size_t>(RoomId::ManifoldConditioning)].transientPeakBytes = 1U;
    lowRequest.demands = hiddenManifold;
    AdaptiveAllRoomPlan rejectedHidden{};
    require(plan_adaptive_all_room_binding(lowRequest, rejectedHidden) == Status::InvalidRoomDemand,
            "zero-hidden-allocation room cannot acquire hidden workspace");

    lowRequest.demands = demands;
    auto oversizedRoom = demands;
    oversizedRoom[static_cast<std::size_t>(RoomId::Architect)].transientPeakBytes = 40U * MiB;
    lowRequest.demands = oversizedRoom;
    AdaptiveAllRoomPlan rejectedLease{};
    require(plan_adaptive_all_room_binding(lowRequest, rejectedLease) == Status::RoomLeaseExceeded,
            "room demand above runtime lease fails closed");

    lowRequest.demands = demands;
    auto hugeEndpoints = endpoints;
    hugeEndpoints.sourceResidentUpperBound = 31U * MiB;
    hugeEndpoints.sinkResidentUpperBound = 2U * MiB;
    lowRequest.endpoints = hugeEndpoints;
    AdaptiveAllRoomPlan rejectedResident{};
    require(plan_adaptive_all_room_binding(lowRequest, rejectedResident) == Status::ResidentBudgetExceeded,
            "source plus sink can consume total budget only once and must fit");

    auto badBackplane = backplane;
    badBackplane.zeroLineHash.fill(0U);
    SharedLineageBinding badLineage{};
    require(bind_shared_lineage(badBackplane, badLineage) == Status::InvalidBackplane,
            "missing zero-line binding fails closed");

    std::cout << "rooms=" << kRoomCount << '\n';
    std::cout << "low_tile=" << lowPlan.resources.tileSize << '\n';
    std::cout << "high_tile=" << highPlan.resources.tileSize << '\n';
    std::cout << "low_peak_bytes=" << lowPlan.peakResidentUpperBound << '\n';
    std::cout << "high_peak_bytes=" << highPlan.peakResidentUpperBound << '\n';
    std::cout << "scene_iso_axis=ABSENT\n";
    std::cout << "shared_backplane=TRUE\n";
    std::cout << "source_sink_counted_once=TRUE\n";
    std::cout << "ROOM_ABI_V0_2_ADAPTIVE_ALL_ROOM_PASS\n";
    return 0;
}

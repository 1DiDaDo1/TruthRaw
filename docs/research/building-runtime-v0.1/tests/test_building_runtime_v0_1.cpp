#include "building_runtime_v0_1.h"

#include <cmath>
#include <iostream>

using namespace truthraw::building_runtime::v0_1;

#define CHECK(x) do { if (!(x)) { std::cerr << "CHECK failed: " #x << " line " << __LINE__ << '\n'; return 1; } } while (0)

static bool same_runtime_result(const RuntimeResult& a, const RuntimeResult& b) {
    if (a.status != b.status || a.decisionCount != b.decisionCount || a.ledgerCount != b.ledgerCount ||
        a.physicalFrameCount != b.physicalFrameCount || a.independentEvidenceCount != b.independentEvidenceCount ||
        a.scientificMasterModified != b.scientificMasterModified || a.physicalCaptureEv != b.physicalCaptureEv ||
        a.bestConditioningEvKnown != b.bestConditioningEvKnown || a.bestConditioningEv != b.bestConditioningEv ||
        a.appearanceEvKnown != b.appearanceEvKnown || a.appearanceEv != b.appearanceEv ||
        a.claimStatus != b.claimStatus || a.provenanceFingerprint != b.provenanceFingerprint) return false;
    for (std::size_t i = 0; i < a.decisionCount; ++i) {
        if (a.decisions[i].room != b.decisions[i].room || a.decisions[i].status != b.decisions[i].status ||
            a.decisions[i].reason != b.decisions[i].reason || a.decisions[i].execute != b.decisions[i].execute) return false;
    }
    for (std::size_t i = 0; i < a.ledgerCount; ++i) {
        if (a.ledger[i].room != b.ledger[i].room || a.ledger[i].status != b.ledger[i].status ||
            a.ledger[i].reason != b.ledger[i].reason) return false;
    }
    return true;
}

static ActivationRequest all_requested() {
    ActivationRequest r{};
    for (std::size_t i = 0; i < static_cast<std::size_t>(RoomId::Count); ++i) r.requested[i] = true;
    return r;
}

static const ExecutionPlacement* find_placement(const ExecutionPlan& p, RoomId id) {
    for (std::size_t i = 0; i < p.placementCount; ++i) if (p.placements[i].room == id) return &p.placements[i];
    return nullptr;
}

int main() {
    const auto graph = default_room_graph();
    CHECK(validate_graph(graph) == Status::Ok);

    ScientificState sci{};
    sci.sigmaKnown = true;
    sci.sigma = 0.02;
    sci.physicalCaptureEv = -1.25;
    sci.bestConditioningEvKnown = true;
    sci.bestConditioningEv = 3.0;
    sci.claimStatus = ClaimStatus::Candidate;
    AppearanceState app{};
    app.appearanceEvKnown = true;
    app.appearanceEv = 0.5;
    CaptureProvenance prov{};
    prov.immutableEvidenceFingerprint = 0x123456789abcdef0ULL;

    RuntimeResult a{}, b{};
    CHECK(evaluate(sci, app, prov, all_requested(), graph, a) == Status::Ok);
    CHECK(evaluate(sci, app, prov, all_requested(), graph, b) == Status::Ok);
    CHECK(a.physicalFrameCount == 1 && a.independentEvidenceCount == 1 && !a.scientificMasterModified);
    CHECK(same_runtime_result(a, b));

    // Appearance changes cannot alter scientific/evidence state or conditioning EV.
    AppearanceState extremeApp{};
    extremeApp.appearanceEvKnown = true;
    extremeApp.appearanceEv = 12.0;
    RuntimeResult c{};
    CHECK(evaluate(sci, extremeApp, prov, all_requested(), graph, c) == Status::Ok);
    CHECK(c.physicalFrameCount == a.physicalFrameCount);
    CHECK(c.independentEvidenceCount == a.independentEvidenceCount);
    CHECK(c.claimStatus == a.claimStatus);
    CHECK(c.bestConditioningEv == a.bestConditioningEv);

    // Three EV domains remain distinct.
    AppearanceState noApp{};
    RuntimeResult noAppOut{};
    CHECK(evaluate(sci, noApp, prov, all_requested(), graph, noAppOut) == Status::Ok);
    CHECK(noAppOut.physicalCaptureEv == -1.25);
    CHECK(noAppOut.bestConditioningEvKnown && noAppOut.bestConditioningEv == 3.0);
    CHECK(!noAppOut.appearanceEvKnown && noAppOut.appearanceEv == 0.0);

    // Unknown sigma fails closed for sigma-dependent rooms and blocks their dependent corridor.
    ScientificState noSigma = sci;
    noSigma.sigmaKnown = false;
    RuntimeResult blocked{};
    CHECK(evaluate(noSigma, app, prov, all_requested(), graph, blocked) == Status::Ok);
    CHECK(!blocked.decisions[static_cast<std::size_t>(RoomId::Restorer)].execute);
    CHECK(blocked.decisions[static_cast<std::size_t>(RoomId::Restorer)].status == RoomStatus::BlockedMissingEvidence);
    CHECK(!blocked.decisions[static_cast<std::size_t>(RoomId::Surveyor)].execute);
    CHECK(!blocked.decisions[static_cast<std::size_t>(RoomId::ManifoldConditioning)].execute);

    // Graph cycles and forbidden backward truth-floor edges are rejected.
    auto cyclic = graph;
    cyclic[0].dependencies[0] = RoomId::Exporter;
    cyclic[0].dependencyCount = 1;
    CHECK(validate_graph(cyclic) == Status::InvalidGraph);
    auto backward = graph;
    backward[2].dependencies[0] = RoomId::Finisher;
    CHECK(validate_graph(backward) == Status::InvalidGraph);

    // Runtime refuses pre-modified scientific master and impossible evidence multiplicity.
    ScientificState modified = sci;
    modified.scientificMasterModified = true;
    RuntimeResult modOut{};
    CHECK(evaluate(modified, app, prov, all_requested(), graph, modOut) == Status::ScientificMasterAlreadyModified);
    ScientificState fakeEvidence = sci;
    fakeEvidence.independentEvidenceCount = 2;
    RuntimeResult fakeOut{};
    CHECK(evaluate(fakeEvidence, app, prov, all_requested(), graph, fakeOut) == Status::EvidenceInvariantViolation);

    // Scientific rooms cannot consume/write downstream domains or claim evidence/master mutation.
    auto badPolicy = graph;
    badPolicy[0].mayReadAppearance = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);
    badPolicy = graph;
    badPolicy[0].mayReadCounterfactual = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);
    badPolicy = graph;
    badPolicy[0].mayIncreaseIndependentEvidence = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);
    badPolicy = graph;
    badPolicy[0].mayMutateInputScientificMaster = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);

    // Corridors pass handles/authority, not copied image payload; backward feedback is rejected.
    CorridorToken sceneToken{};
    sceneToken.artifactHandle = 42;
    sceneToken.provenanceFingerprint = prov.immutableEvidenceFingerprint;
    sceneToken.floor = TruthFloor::Scene;
    sceneToken.claimStatus = ClaimStatus::Candidate;
    CHECK(validate_corridor_transition(graph[static_cast<std::size_t>(RoomId::SceneRegistry)],
                                       graph[static_cast<std::size_t>(RoomId::Surveyor)], sceneToken) == Status::Ok);
    CorridorToken appearanceToken = sceneToken;
    appearanceToken.floor = TruthFloor::Appearance;
    CHECK(validate_corridor_transition(graph[static_cast<std::size_t>(RoomId::Finisher)],
                                       graph[static_cast<std::size_t>(RoomId::Surveyor)], appearanceToken) == Status::InvalidCorridor);
    sceneToken.independentEvidenceCount = 2;
    CHECK(validate_corridor_transition(graph[static_cast<std::size_t>(RoomId::SceneRegistry)],
                                       graph[static_cast<std::size_t>(RoomId::Surveyor)], sceneToken) == Status::EvidenceInvariantViolation);

    // Resource governor: low/mid/high change execution only, never truth authority.
    DeviceEnvelope low{};
    low.appMemoryClassMiB = 256;
    low.explicitMaxWorkingSetBytes = 32 * kMiB;
    low.currentlyAvailableBytes = 96 * kMiB;
    low.cpuThreadBudget = 4;
    low.lowRamDevice = true;
    ResourcePolicy lowPolicy{};
    CHECK(derive_resource_policy(low, lowPolicy) == Status::Ok);
    CHECK(lowPolicy.tier == ResourceTier::Low);
    CHECK(lowPolicy.maxConcurrentHeavyRooms == 1);
    CHECK(lowPolicy.tileSize == 128);
    CHECK(lowPolicy.totalWorkingSetBudgetBytes == 32 * kMiB);
    CHECK(lowPolicy.perHeavyRoomBudgetBytes == 32 * kMiB);
    CHECK(lowPolicy.correctnessBaseline == ComputeBackend::CpuBaseline);

    DeviceEnvelope high{};
    high.appMemoryClassMiB = 2048;
    high.currentlyAvailableBytes = 1024 * kMiB;
    high.cpuThreadBudget = 12;
    high.vulkanAvailable = true;
    ResourcePolicy highPolicy{};
    CHECK(derive_resource_policy(high, highPolicy) == Status::Ok);
    CHECK(highPolicy.tier == ResourceTier::High);
    CHECK(highPolicy.maxConcurrentHeavyRooms == 4);
    CHECK(highPolicy.tileSize == 512);
    CHECK(highPolicy.totalWorkingSetBudgetBytes == 256 * kMiB);
    CHECK(highPolicy.preferredBackend == ComputeBackend::OptionalVulkan);

    DeviceEnvelope hot = high;
    hot.thermalState = ThermalState::Severe;
    ResourcePolicy hotPolicy{};
    CHECK(derive_resource_policy(hot, hotPolicy) == Status::Ok);
    CHECK(hotPolicy.maxConcurrentHeavyRooms == 1);
    CHECK(hotPolicy.preferredBackend == ComputeBackend::CpuBaseline);
    CHECK(hotPolicy.tileSize == 128);

    DeviceEnvelope tiny{};
    tiny.appMemoryClassMiB = 64;
    tiny.explicitMaxWorkingSetBytes = 2 * kMiB;
    tiny.cpuThreadBudget = 2;
    ResourcePolicy tinyPolicy{};
    CHECK(derive_resource_policy(tiny, tinyPolicy) == Status::BudgetTooSmall);

    // Dynamic placement: same admissible rooms, different scheduling/resource lanes.
    ExecutionPlan lowPlan{}, highPlan{};
    CHECK(plan_execution(a, graph, lowPolicy, lowPlan) == Status::Ok);
    CHECK(plan_execution(a, graph, highPolicy, highPlan) == Status::Ok);
    CHECK(lowPlan.placementCount == highPlan.placementCount);
    CHECK(lowPlan.resources.maxConcurrentHeavyRooms == 1);
    CHECK(highPlan.resources.maxConcurrentHeavyRooms == 4);
    const auto* lowArchitect = find_placement(lowPlan, RoomId::Architect);
    const auto* highArchitect = find_placement(highPlan, RoomId::Architect);
    CHECK(lowArchitect && highArchitect && lowArchitect->heavyLease && highArchitect->heavyLease);
    CHECK(lowArchitect->memoryLeaseBytes == lowPolicy.perHeavyRoomBudgetBytes);
    CHECK(highArchitect->memoryLeaseBytes == highPolicy.perHeavyRoomBudgetBytes);
    CHECK(lowPlan.waveCount >= highPlan.waveCount);

    // Research-only status and claim status are preserved; scheduler cannot auto-promote.
    CHECK(a.decisions[static_cast<std::size_t>(RoomId::Restorer)].status == RoomStatus::ResearchOnly);
    CHECK(a.decisions[static_cast<std::size_t>(RoomId::LightingStudioCicm)].status == RoomStatus::ResearchOnly);
    CHECK(a.decisions[static_cast<std::size_t>(RoomId::RoomCapsule)].status == RoomStatus::ResearchOnly);
    CHECK(a.claimStatus == ClaimStatus::Candidate);

    // Fixed-size mobile hot path and finite-state guards.
    CHECK(sizeof(RuntimeResult) <= 2048);
    CHECK(sizeof(RoomDescriptor) <= 48);
    CHECK(sizeof(CorridorToken) <= 48);
    CHECK(sizeof(ResourcePolicy) <= 64);
    ScientificState nanEv = sci;
    nanEv.physicalCaptureEv = std::nan("");
    RuntimeResult nanOut{};
    CHECK(evaluate(nanEv, app, prov, all_requested(), graph, nanOut) == Status::EvidenceInvariantViolation);

    std::cout << "BUILDING_RUNTIME_V0_1_TEST_PASS\n";
    std::cout << "physicalFrameCount=" << a.physicalFrameCount << '\n';
    std::cout << "independentEvidenceCount=" << a.independentEvidenceCount << '\n';
    std::cout << "scientificMasterModified=" << a.scientificMasterModified << '\n';
    std::cout << "decisionCount=" << a.decisionCount << '\n';
    std::cout << "ledgerCount=" << a.ledgerCount << '\n';
    std::cout << "sizeofRuntimeResult=" << sizeof(RuntimeResult) << '\n';
    std::cout << "lowBudgetBytes=" << lowPolicy.totalWorkingSetBudgetBytes << '\n';
    std::cout << "lowConcurrency=" << lowPolicy.maxConcurrentHeavyRooms << '\n';
    std::cout << "lowTile=" << lowPolicy.tileSize << '\n';
    std::cout << "highBudgetBytes=" << highPolicy.totalWorkingSetBudgetBytes << '\n';
    std::cout << "highConcurrency=" << highPolicy.maxConcurrentHeavyRooms << '\n';
    std::cout << "highTile=" << highPolicy.tileSize << '\n';
    std::cout << "lowWaves=" << static_cast<unsigned>(lowPlan.waveCount) << '\n';
    std::cout << "highWaves=" << static_cast<unsigned>(highPlan.waveCount) << '\n';
    return 0;
}

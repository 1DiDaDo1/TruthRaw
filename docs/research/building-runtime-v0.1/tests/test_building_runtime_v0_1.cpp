#include "building_runtime_v0_1.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace truthraw::building_runtime::v0_1;

#define CHECK(x) do { if (!(x)) { std::cerr << "CHECK failed: " #x << " line " << __LINE__ << '\n'; return 1; } } while (0)


static bool same_runtime_result(const RuntimeResult& a, const RuntimeResult& b) {
    if (a.status != b.status || a.decisionCount != b.decisionCount || a.ledgerCount != b.ledgerCount ||
        a.physicalFrameCount != b.physicalFrameCount ||
        a.independentEvidenceCount != b.independentEvidenceCount ||
        a.scientificMasterModified != b.scientificMasterModified ||
        a.physicalCaptureEv != b.physicalCaptureEv ||
        a.bestConditioningEvKnown != b.bestConditioningEvKnown ||
        a.bestConditioningEv != b.bestConditioningEv ||
        a.appearanceEvKnown != b.appearanceEvKnown || a.appearanceEv != b.appearanceEv ||
        a.claimStatus != b.claimStatus || a.provenanceFingerprint != b.provenanceFingerprint) return false;
    for (std::size_t i = 0; i < a.decisionCount; ++i) {
        if (a.decisions[i].room != b.decisions[i].room ||
            a.decisions[i].status != b.decisions[i].status ||
            a.decisions[i].reason != b.decisions[i].reason ||
            a.decisions[i].execute != b.decisions[i].execute) return false;
    }
    for (std::size_t i = 0; i < a.ledgerCount; ++i) {
        if (a.ledger[i].room != b.ledger[i].room ||
            a.ledger[i].status != b.ledger[i].status ||
            a.ledger[i].reason != b.ledger[i].reason) return false;
    }
    return true;
}

static ActivationRequest all_requested() {
    ActivationRequest r{};
    for (std::size_t i = 0; i < static_cast<std::size_t>(RoomId::Count); ++i) r.requested[i] = true;
    return r;
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

    // 1) no virtual room/view can multiply evidence.
    CHECK(a.physicalFrameCount == 1);
    CHECK(a.independentEvidenceCount == 1);
    CHECK(!a.scientificMasterModified);

    // 2) appearance cannot feed scientific confidence/evidence back upstream.
    AppearanceState extremeApp{};
    extremeApp.appearanceEvKnown = true;
    extremeApp.appearanceEv = 12.0;
    RuntimeResult c{};
    CHECK(evaluate(sci, extremeApp, prov, all_requested(), graph, c) == Status::Ok);
    CHECK(c.physicalFrameCount == a.physicalFrameCount);
    CHECK(c.independentEvidenceCount == a.independentEvidenceCount);
    CHECK(c.claimStatus == a.claimStatus);
    CHECK(c.bestConditioningEv == a.bestConditioningEv);

    // 3) three EV domains stay distinct; conditioning never becomes appearance EV implicitly.
    AppearanceState noApp{};
    RuntimeResult noAppOut{};
    CHECK(evaluate(sci, noApp, prov, all_requested(), graph, noAppOut) == Status::Ok);
    CHECK(noAppOut.physicalCaptureEv == -1.25);
    CHECK(noAppOut.bestConditioningEvKnown && noAppOut.bestConditioningEv == 3.0);
    CHECK(!noAppOut.appearanceEvKnown);
    CHECK(noAppOut.appearanceEv == 0.0);

    // 4) unknown sigma blocks only rooms that require it; it never becomes zero-certainty.
    ScientificState noSigma = sci;
    noSigma.sigmaKnown = false;
    RuntimeResult blocked{};
    CHECK(evaluate(noSigma, app, prov, all_requested(), graph, blocked) == Status::Ok);
    CHECK(!blocked.decisions[static_cast<std::size_t>(RoomId::ManifoldConditioning)].execute);
    CHECK(blocked.decisions[static_cast<std::size_t>(RoomId::ManifoldConditioning)].status == RoomStatus::BlockedMissingEvidence);
    CHECK(blocked.decisions[static_cast<std::size_t>(RoomId::ManifoldConditioning)].reason == Reason::RequiredSigmaUnknown);

    // 5) deterministic semantic output for identical inputs. Padding bytes are not part of the contract.
    CHECK(same_runtime_result(a, b));

    // 6) dependency graph cycles are rejected.
    auto cyclic = graph;
    cyclic[0].dependencies[0] = RoomId::OutputProjection;
    cyclic[0].dependencyCount = 1;
    CHECK(validate_graph(cyclic) == Status::InvalidGraph);

    // 7) runtime refuses a pre-modified scientific master.
    ScientificState modified = sci;
    modified.scientificMasterModified = true;
    RuntimeResult modOut{};
    CHECK(evaluate(modified, app, prov, all_requested(), graph, modOut) == Status::ScientificMasterAlreadyModified);

    // 8) statuses are preserved: research-only remains research-only and claim status never auto-promotes.
    CHECK(a.decisions[static_cast<std::size_t>(RoomId::CounterfactualIllumination)].status == RoomStatus::ResearchOnly);
    CHECK(a.decisions[static_cast<std::size_t>(RoomId::RoomCapsule)].status == RoomStatus::ResearchOnly);
    CHECK(a.claimStatus == ClaimStatus::Candidate);

    // 9) bounded fixed-size mobile contract.
    CHECK(sizeof(RuntimeResult) <= 1024);
    CHECK(sizeof(RoomDescriptor) <= 32);

    // 10) provenance is copied as identity only and not mutated.
    CHECK(a.provenanceFingerprint == prov.immutableEvidenceFingerprint);
    CHECK(prov.physicalFrameCount == 1 && prov.independentEvidenceCount == 1);

    // 11) impossible evidence counts fail closed.
    ScientificState fakeEvidence = sci;
    fakeEvidence.independentEvidenceCount = 2;
    RuntimeResult fakeOut{};
    CHECK(evaluate(fakeEvidence, app, prov, all_requested(), graph, fakeOut) == Status::EvidenceInvariantViolation);

    // Additional graph policy gates: scientific rooms cannot read/write appearance or claim evidence/master writes.
    auto badPolicy = graph;
    badPolicy[0].mayReadAppearance = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);
    badPolicy = graph;
    badPolicy[0].mayIncreaseIndependentEvidence = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);
    badPolicy = graph;
    badPolicy[0].mayModifyScientificMaster = true;
    CHECK(validate_graph(badPolicy) == Status::InvalidGraph);

    // Non-finite critical values fail closed.
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
    return 0;
}

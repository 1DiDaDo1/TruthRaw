#include "drawnegative_v0_1.h"

#include <iostream>
#include <stdexcept>

namespace dn = truthraw::drawnegative::v0_1;
namespace tn = truthraw::truthnegative_continuous::v0_5;

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error(#x); } while (0)

static tn::State makeLegacy() {
    tn::StateInput in{};
    in.sourceEvidenceSha256[0] = 1u;
    in.scientificMasterSha256[0] = 2u;
    in.authorityFieldSha256[0] = 3u;
    in.width = 4080u;
    in.height = 3072u;
    in.reconstructionBackendId = "test-backend";
    in.colourBindingId = "test-colour";
    in.physicalFrameCount = 1u;
    in.independentEvidenceCount = 1u;

    tn::State s{};
    REQUIRE(tn::finalizeState(in, s));
    return s;
}

int main() {
    const auto parent = makeLegacy();

    dn::Input local{};
    local.truthNegativeState = parent;
    local.observationId = "DRAW_OBS_TEST_1";
    local.scaleGaugeId = "DRAW_LOCAL_GAUGE_TEST_1";
    local.gaugeRelation = dn::GaugeRelation::SourceLocalOnly;

    dn::State a{};
    REQUIRE(dn::finalize(local, a));
    REQUIRE(a.finalized);
    REQUIRE(a.isRasterIndependent);
    REQUIRE(a.isPerObservationLineage);
    REQUIRE(!a.commonGaugeAdmitted);
    REQUIRE(!a.crossObservationRadiometricEqualityAllowed);
    REQUIRE(!a.crossObservationRadiometricFusionAllowed);
    REQUIRE(a.parentTruthNegativeStateSha256 == parent.stateSha256);
    REQUIRE(a.branchSensitiveComputeFloat64);
    REQUIRE(!a.precisionUpgradesAuthority);
    REQUIRE(!a.createsNewEvidence);
    REQUIRE(!a.scientificWritebackAllowed);

    auto shared = local;
    shared.sharedFreeWorldGaugeId = "DRAW_SHARED_GAUGE_1";
    shared.gaugeRelation = dn::GaugeRelation::SharedRelative;
    dn::State b{};
    REQUIRE(dn::finalize(shared, b));
    REQUIRE(b.commonGaugeAdmitted);
    REQUIRE(b.crossObservationRadiometricEqualityAllowed);
    REQUIRE(b.crossObservationRadiometricFusionAllowed);
    REQUIRE(b.stateSha256 != a.stateSha256);

    auto bad = local;
    bad.sharedFreeWorldGaugeId = "ILLEGAL_SHARED_WITH_LOCAL";
    dn::State fail{};
    REQUIRE(!dn::finalize(bad, fail));

    bad = local;
    bad.gaugeRelation = dn::GaugeRelation::SharedRelative;
    bad.sharedFreeWorldGaugeId.clear();
    REQUIRE(!dn::finalize(bad, fail));

    bad = local;
    bad.observationId.clear();
    REQUIRE(!dn::finalize(bad, fail));

    bad = local;
    bad.truthRangeCoordinateFamilyDeclared = false;
    REQUIRE(!dn::finalize(bad, fail));

    bad = local;
    bad.truthNegativeState.createsNewEvidence = true;
    REQUIRE(!dn::finalize(bad, fail));

    std::cout << "D.RAWnegative/0.1 PASS\n";
    return 0;
}

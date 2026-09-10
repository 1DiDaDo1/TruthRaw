#include "manifold_conditioning_v1.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#define REQUIRE(x) do { if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " #x "\n"; return 1; } } while (0)

namespace {
bool near(double a, double b, double eps=1e-6) { return std::abs(a-b) <= eps; }

truthraw_v09::VirtualObservationSpecV09 ev_view(double ev) {
    truthraw_v09::VirtualObservationSpecV09 v;
    v.viewId = "ev";
    v.exposureEv = ev;
    return v;
}

truthraw_mc_v1::HeldoutSceneEvidenceV1 evidence(
    const std::string& id, bool frozen,
    double maeRatio, double p95Ratio, double ordDelta, double curvDelta) {
    truthraw_mc_v1::HeldoutSceneEvidenceV1 e;
    e.evidenceId=id; e.sourceSceneId=id; e.channel="G"; e.regimeId="dog-tele-general";
    e.candidateFrozenBeforeEvidence=frozen;
    e.metrics.samples=400000;
    e.metrics.baselineMae=0.01; e.metrics.candidateMae=0.01*maeRatio;
    e.metrics.baselineP95Abs=0.03; e.metrics.candidateP95Abs=0.03*p95Ratio;
    e.metrics.baselineOrdering=0.95; e.metrics.candidateOrdering=0.95+ordDelta;
    e.metrics.baselineCurvature=0.85; e.metrics.candidateCurvature=0.85+curvDelta;
    return e;
}
}

int main() {
    using namespace truthraw_mc_v1;

    // Exact scalar round-trip across extreme but finite EV coordinates.
    for (double ev : {-20.0,-10.0,-4.0,0.0,4.0,10.0,20.0}) {
        ExactScalarV1 c;
        REQUIRE(exact_condition_scalar_v1(-0.00325, 0.0075, ev_view(ev), c));
        double x=0.0,s=0.0;
        REQUIRE(exact_decondition_scalar_v1(c,x,s));
        REQUIRE(near(x,-0.00325,1e-12));
        REQUIRE(near(s,0.0075,1e-12));
    }

    // Unresolved sigma remains unresolved instead of being invented.
    ExactScalarV1 nanSigma;
    REQUIRE(exact_condition_scalar_v1(0.1, std::numeric_limits<double>::quiet_NaN(), ev_view(8.0), nanSigma));
    double xv=0.0, sv=0.0;
    REQUIRE(exact_decondition_scalar_v1(nanSigma,xv,sv));
    REQUIRE(std::isnan(sv));

    // Gain/ISO cannot silently become an exact optimizer coordinate.
    auto bad = ev_view(2.0); bad.gainEv=1.0;
    ExactScalarV1 tmp;
    REQUIRE(!exact_condition_scalar_v1(0.1,0.01,bad,tmp));

    // Full/partial covariance exact round-trip through the v0.9/v0.6 chain.
    std::array<float,3> rgb {0.1f,-0.003f,1.25f};
    truthraw_v06::PixelCameraRgbCovarianceV06 cov;
    cov.variance = {1e-4f,2e-4f,4e-4f};
    cov.varianceKnownMask = 0x7u;
    cov.covariance = {2e-5f,std::numeric_limits<float>::quiet_NaN(),3e-5f};
    cov.covarianceKnownMask = 0x5u;
    ExactRgbV1 crgb;
    REQUIRE(exact_condition_rgb_v1(rgb,cov,ev_view(10.0),crgb));
    std::array<float,3> back{}; truthraw_v06::PixelCameraRgbCovarianceV06 backCov;
    REQUIRE(exact_decondition_rgb_v1(crgb,back,backCov));
    for (int i=0;i<3;++i) REQUIRE(near(back[static_cast<std::size_t>(i)],rgb[static_cast<std::size_t>(i)],2e-7));
    REQUIRE(std::isnan(backCov.covariance[1]));
    REQUIRE(near(backCov.covariance[0],cov.covariance[0],2e-10));
    REQUIRE(near(backCov.variance[2],cov.variance[2],2e-10));

    // Manifold evidence ledger is forced to one physical frame / one evidence root.
    truthraw_v09::VirtualObservationManifoldV09 m;
    m.sourceEvidenceId="dog094423"; m.views={ev_view(-4),ev_view(0),ev_view(8)};
    ConditioningLedgerV1 ledger;
    REQUIRE(validate_manifold_for_conditioning_v1(m,ledger));
    REQUIRE(ledger.physicalFrameCount==1 && ledger.independentEvidenceCount==1);
    m.independentEvidenceCount=3;
    REQUIRE(!validate_manifold_for_conditioning_v1(m,ledger));

    // Historical dog candidate: development evidence is excluded and only 2 frozen scenes remain -> insufficient.
    PromotionPolicyV1 p;
    p.minIndependentScenes=3; p.minSamplesPerScene=100000; p.maxMaeRatio=0.999; p.maxP95Ratio=1.0;
    p.minOrderingDelta=0.0; p.minCurvatureDelta=0.0;
    std::vector<HeldoutSceneEvidenceV1> dog;
    dog.push_back(evidence("094423-development",false,0.99305,0.98941,+0.00064,+0.00114));
    dog.push_back(evidence("094414-frozen",true,0.99642,0.99654,-0.00195,-0.00544));
    dog.push_back(evidence("094416-frozen",true,0.99961,0.99991,-0.00201,-0.00445));
    SelectionDecisionV1 d;
    REQUIRE(evaluate_model_selection_v1("historical-multi-ev-robust","G","dog-tele-general",p,dog,d));
    REQUIRE(d.status==SelectionStatusV1::InsufficientIndependentEvidence);
    double selected=0.0;
    REQUIRE(apply_selected_estimate_v1(0.25,0.30,d,selected));
    REQUIRE(selected==0.25);

    // A genuinely frozen 3-scene candidate can become eligible only when every configured gate passes.
    std::vector<HeldoutSceneEvidenceV1> good;
    good.push_back(evidence("s1",true,0.990,0.995,+0.001,+0.001));
    good.push_back(evidence("s2",true,0.995,0.998,+0.002,+0.001));
    good.push_back(evidence("s3",true,0.997,0.999,+0.001,+0.002));
    REQUIRE(evaluate_model_selection_v1("future-frozen-candidate","G","dog-tele-general",p,good,d));
    REQUIRE(d.status==SelectionStatusV1::Eligible);
    REQUIRE(apply_selected_estimate_v1(0.25,0.30,d,selected));
    REQUIRE(selected==0.30);

    // One topology regression rejects even when scalar error improves.
    good[1].metrics.candidateOrdering=good[1].metrics.baselineOrdering-0.0001;
    REQUIRE(evaluate_model_selection_v1("future-frozen-candidate","G","dog-tele-general",p,good,d));
    REQUIRE(d.status==SelectionStatusV1::RejectedMetricRegression);
    REQUIRE(apply_selected_estimate_v1(0.25,0.30,d,selected));
    REQUIRE(selected==0.25);

    std::cout << "TRUTHRAW_MANIFOLD_CONDITIONING_V1_TESTS_PASS\n";
    return 0;
}

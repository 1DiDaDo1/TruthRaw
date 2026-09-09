#include "virtual_observation_manifold_v0_9.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

[[noreturn]] void fail(const char* expr, int line) {
    std::cerr << "CHECK failed at line " << line << ": " << expr << "\n";
    std::exit(1);
}
#define CHECK(x) do { if (!(x)) fail(#x, __LINE__); } while (0)

bool near(double a, double b, double eps = 1e-6) {
    return std::abs(a-b) <= eps * std::max({1.0, std::abs(a), std::abs(b)});
}

truthraw_v06::PixelCameraRgbCovarianceV06 full_cov() {
    truthraw_v06::PixelCameraRgbCovarianceV06 c;
    c.variance = {1.f, 4.f, 9.f};
    c.varianceKnownMask = 0x7u;
    c.covariance = {0.2f, 0.1f, 0.3f};
    c.covarianceKnownMask = 0x7u;
    c.fullPsdCertified = true;
    return c;
}

truthraw::LatentCameraSceneV02 tiny_scene() {
    truthraw::LatentCameraSceneV02 s;
    s.width = 2; s.height = 2;
    s.stage2Cfa = {0.1f,0.2f,0.3f,0.4f};
    s.cameraRgb.assign(12, 0.25f);
    s.measuredChannel = {0,1,1,2};
    s.sourceHighCensor = {0,0,0,0};
    s.sourceHighCensorLower = {0,0,0,0};
    s.binding.sceneScaleId = "scene-scale-test";
    return s;
}

} // namespace

int main() {
    using namespace truthraw_v09;

    const VirtualObservationSpecV09 e4{"ev+4",4.0,0.0,std::numeric_limits<double>::quiet_NaN(),VirtualIsoSemanticsV09::None};
    const VirtualObservationSpecV09 gm2{"iso800-view",0.0,3.0,800.0,VirtualIsoSemanticsV09::GainEncodingOnly};
    CHECK(validate_virtual_observation_spec_v0_9(e4));
    CHECK(validate_virtual_observation_spec_v0_9(gm2));

    auto badIso = gm2; badIso.isoSemantics = VirtualIsoSemanticsV09::None;
    CHECK(!validate_virtual_observation_spec_v0_9(badIso));

    VirtualObservationManifoldV09 manifold;
    const auto scene = tiny_scene();
    CHECK(build_virtual_observation_manifold_v0_9(scene,"sha256:source",{e4,gm2},manifold));
    CHECK(manifold.physicalFrameCount == 1u);
    CHECK(manifold.independentEvidenceCount == 1u);
    CHECK(!manifold.viewsAreIndependentMeasurements);
    CHECK(manifold.evidenceConfidenceMultiplier == 1.0);
    CHECK(manifold.views.size() == 2u);
    auto duplicate = e4; duplicate.viewId = gm2.viewId;
    CHECK(!build_virtual_observation_manifold_v0_9(scene,"sha256:source",{gm2,duplicate},manifold));

    truthraw::TruthRangeSampleV02 tr;
    tr.muLinearSigned = -0.125f;
    tr.hasEstimate = true;
    tr.estimateEv = -3.0;
    tr.p50LowerEv = -3.5; tr.p50UpperEv = -2.5;
    tr.p95LowerEv = -4.0; tr.p95UpperEv = -2.0;
    tr.evidenceLowerEv = -5.0;
    tr.evidenceUpperEv = std::numeric_limits<double>::infinity();
    tr.gaugeId = "gauge-A";
    tr.uncertaintySource = "u";
    tr.support = truthraw::TruthRangeSupportV02::MeasuredCensoredLowerBound;
    tr.censor = truthraw::TruthRangeCensorV02::HighClipped;

    truthraw::TruthRangeSampleV02 trOut;
    CHECK(project_truthrange_sample_v0_9(tr,e4,trOut));
    CHECK(near(trOut.muLinearSigned,-2.0));
    CHECK(near(trOut.estimateEv,1.0));
    CHECK(near(trOut.p95LowerEv,0.0));
    CHECK(std::isinf(trOut.evidenceUpperEv) && trOut.evidenceUpperEv > 0);
    CHECK(trOut.support == tr.support && trOut.censor == tr.censor);
    CHECK(trOut.gaugeId == tr.gaugeId && trOut.uncertaintySource == tr.uncertaintySource);

    // Gain encoding must not move TruthRange, even though encoded camera values scale.
    CHECK(project_truthrange_sample_v0_9(tr,gm2,trOut));
    CHECK(near(trOut.muLinearSigned,-0.125));
    CHECK(near(trOut.estimateEv,-3.0));

    VirtualCameraRgbPixelV09 p;
    CHECK(project_camera_rgb_pixel_v0_9({1.f,2.f,-0.5f},full_cov(),"sha256:source",e4,p));
    CHECK(near(p.encodedCameraRgb[0],16.0));
    CHECK(near(p.encodedCameraRgb[1],32.0));
    CHECK(near(p.encodedCameraRgb[2],-8.0));
    CHECK(near(p.encodedCovariance.variance[0],256.0));
    CHECK(near(p.encodedCovariance.variance[1],1024.0));
    CHECK(near(p.encodedCovariance.covariance[0],51.2,1e-5));
    CHECK(p.encodedCovariance.fullPsdCertified);

    CHECK(project_camera_rgb_pixel_v0_9({1.f,2.f,-0.5f},full_cov(),"sha256:source",gm2,p));
    CHECK(near(p.totalEncodingScale,8.0));
    CHECK(near(p.encodedCameraRgb[0],8.0));
    CHECK(near(p.encodedCovariance.variance[2],576.0));

    std::vector<VirtualObservationSpecV09> nodes;
    for (int ev : {-6,-4,-2,0,2,4,6,8,10})
        nodes.push_back({"ev"+std::to_string(ev),static_cast<double>(ev),0.0,
                         std::numeric_limits<double>::quiet_NaN(),VirtualIsoSemanticsV09::None});
    std::vector<double> w;
    CHECK(normalized_virtual_ev_weights_v0_9(0.01,nodes,w));
    double sum = 0.0; for (double x : w) { CHECK(x >= 0.0); sum += x; }
    CHECK(near(sum,1.0,1e-12));
    const auto best = static_cast<int>(std::distance(w.begin(), std::max_element(w.begin(),w.end())));
    CHECK(nodes[static_cast<std::size_t>(best)].exposureEv >= 4.0 && nodes[static_cast<std::size_t>(best)].exposureEv <= 6.0);

    VirtualSensorForwardModelV09 unresolved;
    VirtualSensorPredictionV09 pred;
    auto physicalSpec = gm2; physicalSpec.isoSemantics = VirtualIsoSemanticsV09::CalibratedForwardModel;
    CHECK(!predict_virtual_sensor_observation_v0_9(0.2,physicalSpec,unresolved,pred));

    VirtualSensorForwardModelV09 model;
    model.authority = SensorModelAuthorityV09::ExplicitResearchFixture;
    model.modelId = "fixture:v1";
    model.sourceClassId = "synthetic";
    model.referenceIso = 100.0;
    model.sceneToPregainSignal = 100.0;
    model.shotVarianceSlopePregain = 1.0;
    model.readVariancePregain = 4.0;
    model.saturationPregain = 50.0;
    CHECK(validate_virtual_sensor_forward_model_v0_9(model));
    CHECK(predict_virtual_sensor_observation_v0_9(0.2,physicalSpec,model,pred));
    CHECK(pred.valid);
    CHECK(near(pred.expectedPregainSignal,20.0));
    CHECK(near(pred.expectedEncodedSignal,160.0));
    CHECK(near(pred.conditionalVarianceEncoded,(20.0+4.0)*64.0));
    CHECK(!pred.highCensored);

    auto brightSpec = physicalSpec; brightSpec.exposureEv = 2.0; brightSpec.viewId = "bright";
    CHECK(predict_virtual_sensor_observation_v0_9(0.2,brightSpec,model,pred));
    CHECK(pred.highCensored);
    CHECK(near(pred.censorLowerBoundEncoded,400.0));

    std::cout << "TRUTHRAW_VIRTUAL_OBSERVATION_MANIFOLD_V0_9_TESTS_PASS\n";
    std::cout << "physical_frame_count=" << 1 << " independent_evidence_count=" << 1 << "\n";
    return 0;
}

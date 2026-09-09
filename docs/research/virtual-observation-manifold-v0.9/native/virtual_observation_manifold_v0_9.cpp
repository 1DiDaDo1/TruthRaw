#include "virtual_observation_manifold_v0_9.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace truthraw_v09 {
namespace {

bool finite(double x) { return std::isfinite(x); }
bool positive_finite(double x) { return finite(x) && x > 0.0; }
bool nonnegative_finite(double x) { return finite(x) && x >= 0.0; }

StatusV09 scale_from_ev(double ev, double& out) {
    if (!finite(ev)) return StatusV09::error("EV must be finite");
    out = std::exp2(ev);
    if (!positive_finite(out) || out > static_cast<double>(std::numeric_limits<float>::max()))
        return StatusV09::error("EV scale is outside supported finite float range");
    return StatusV09::success();
}

double shift_finite_or_infinite(double x, double delta) {
    if (std::isinf(x)) return x;
    if (std::isnan(x)) return x;
    return x + delta;
}

StatusV09 scale_covariance_copy(
    const truthraw_v06::PixelCameraRgbCovarianceV06& in,
    float scalar,
    truthraw_v06::PixelCameraRgbCovarianceV06& out) {
    out = in;
    const auto s = truthraw_v06::scale_pixel_covariance_v0_6(out, scalar);
    if (!s) return StatusV09::error(std::string("v0.6 covariance scaling rejected: ") + s.message);
    return StatusV09::success();
}

} // namespace

const char* virtual_iso_semantics_name_v0_9(VirtualIsoSemanticsV09 s) {
    switch (s) {
        case VirtualIsoSemanticsV09::None: return "NONE";
        case VirtualIsoSemanticsV09::GainEncodingOnly: return "GAIN_ENCODING_ONLY";
        case VirtualIsoSemanticsV09::CalibratedForwardModel: return "CALIBRATED_FORWARD_MODEL";
    }
    return "UNKNOWN";
}

const char* sensor_model_authority_name_v0_9(SensorModelAuthorityV09 a) {
    switch (a) {
        case SensorModelAuthorityV09::Unresolved: return "UNRESOLVED";
        case SensorModelAuthorityV09::ExplicitResearchFixture: return "EXPLICIT_RESEARCH_FIXTURE";
        case SensorModelAuthorityV09::IndependentSensorCalibration: return "INDEPENDENT_SENSOR_CALIBRATION";
    }
    return "UNKNOWN";
}

StatusV09 validate_virtual_observation_spec_v0_9(const VirtualObservationSpecV09& spec) {
    if (spec.viewId.empty()) return StatusV09::error("viewId must be non-empty");
    double se = 0.0, sg = 0.0, st = 0.0;
    auto s = scale_from_ev(spec.exposureEv, se); if (!s) return s;
    s = scale_from_ev(spec.gainEv, sg); if (!s) return s;
    s = scale_from_ev(spec.exposureEv + spec.gainEv, st); if (!s) return s;
    (void)se; (void)sg; (void)st;

    const bool isoFinite = finite(spec.nominalVirtualIso);
    if (spec.isoSemantics == VirtualIsoSemanticsV09::None) {
        if (isoFinite) return StatusV09::error("nominalVirtualIso requires explicit ISO semantics");
    } else {
        if (!positive_finite(spec.nominalVirtualIso))
            return StatusV09::error("virtual ISO must be positive and finite when ISO semantics are enabled");
    }
    return StatusV09::success();
}

StatusV09 build_virtual_observation_manifold_v0_9(
    const truthraw::LatentCameraSceneV02& scene,
    const std::string& sourceEvidenceId,
    const std::vector<VirtualObservationSpecV09>& views,
    VirtualObservationManifoldV09& out) {

    if (scene.width <= 0 || scene.height <= 0) return StatusV09::error("latent scene geometry invalid");
    const std::size_t n = static_cast<std::size_t>(scene.width) * static_cast<std::size_t>(scene.height);
    if (scene.cameraRgb.size() != 3u*n || scene.stage2Cfa.size() != n || scene.measuredChannel.size() != n)
        return StatusV09::error("latent scene buffers do not match geometry");
    if (sourceEvidenceId.empty()) return StatusV09::error("sourceEvidenceId must be non-empty");
    if (views.empty()) return StatusV09::error("at least one virtual observation is required");

    std::set<std::string> ids;
    for (const auto& v : views) {
        const auto s = validate_virtual_observation_spec_v0_9(v);
        if (!s) return s;
        if (!ids.insert(v.viewId).second) return StatusV09::error("viewId values must be unique");
    }

    out = VirtualObservationManifoldV09{};
    out.sourceEvidenceId = sourceEvidenceId;
    out.sceneScaleId = scene.binding.sceneScaleId;
    out.views = views;
    // Hard no-double-counting invariant. These fields are intentionally not derived from views.size().
    out.physicalFrameCount = 1;
    out.independentEvidenceCount = 1;
    out.viewsAreIndependentMeasurements = false;
    out.evidenceConfidenceMultiplier = 1.0;
    return StatusV09::success();
}

StatusV09 project_truthrange_sample_v0_9(
    const truthraw::TruthRangeSampleV02& in,
    const VirtualObservationSpecV09& spec,
    truthraw::TruthRangeSampleV02& out) {

    auto s = validate_virtual_observation_spec_v0_9(spec);
    if (!s) return s;
    double exposureScale = 0.0;
    s = scale_from_ev(spec.exposureEv, exposureScale); if (!s) return s;
    if (!std::isfinite(static_cast<double>(in.muLinearSigned) * exposureScale))
        return StatusV09::error("scaled signed scene estimate overflow");

    out = in;
    out.muLinearSigned = static_cast<float>(static_cast<double>(in.muLinearSigned) * exposureScale);
    // Gain/ISO encoding does not move physical scene light on TruthRange. Exposure reparameterization does.
    if (in.hasEstimate) out.estimateEv = shift_finite_or_infinite(in.estimateEv, spec.exposureEv);
    out.p50LowerEv = shift_finite_or_infinite(in.p50LowerEv, spec.exposureEv);
    out.p50UpperEv = shift_finite_or_infinite(in.p50UpperEv, spec.exposureEv);
    out.p95LowerEv = shift_finite_or_infinite(in.p95LowerEv, spec.exposureEv);
    out.p95UpperEv = shift_finite_or_infinite(in.p95UpperEv, spec.exposureEv);
    out.evidenceLowerEv = shift_finite_or_infinite(in.evidenceLowerEv, spec.exposureEv);
    out.evidenceUpperEv = shift_finite_or_infinite(in.evidenceUpperEv, spec.exposureEv);
    // support/censor/gauge/uncertainty source are inherited exactly from the single evidence root.
    return StatusV09::success();
}

StatusV09 project_camera_rgb_pixel_v0_9(
    const std::array<float, 3>& cameraRgb,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    const std::string& sourceEvidenceId,
    const VirtualObservationSpecV09& spec,
    VirtualCameraRgbPixelV09& out) {

    if (sourceEvidenceId.empty()) return StatusV09::error("sourceEvidenceId must be non-empty");
    auto s = validate_virtual_observation_spec_v0_9(spec); if (!s) return s;
    double exposureScale = 0.0, gainScale = 0.0, totalScale = 0.0;
    s = scale_from_ev(spec.exposureEv, exposureScale); if (!s) return s;
    s = scale_from_ev(spec.gainEv, gainScale); if (!s) return s;
    s = scale_from_ev(spec.exposureEv + spec.gainEv, totalScale); if (!s) return s;

    out = VirtualCameraRgbPixelV09{};
    for (int c = 0; c < 3; ++c) {
        if (!std::isfinite(cameraRgb[static_cast<std::size_t>(c)]))
            return StatusV09::error("camera RGB input must be finite");
        const double y = static_cast<double>(cameraRgb[static_cast<std::size_t>(c)]) * totalScale;
        if (!std::isfinite(y) || std::abs(y) > static_cast<double>(std::numeric_limits<float>::max()))
            return StatusV09::error("encoded camera RGB overflow");
        out.encodedCameraRgb[static_cast<std::size_t>(c)] = static_cast<float>(y);
    }
    s = scale_covariance_copy(covariance, static_cast<float>(totalScale), out.encodedCovariance);
    if (!s) return s;
    out.exposureScale = exposureScale;
    out.gainScale = gainScale;
    out.totalEncodingScale = totalScale;
    out.sourceEvidenceId = sourceEvidenceId;
    out.viewId = spec.viewId;
    return StatusV09::success();
}

StatusV09 normalized_virtual_ev_weights_v0_9(
    double positiveSceneSignal,
    const std::vector<VirtualObservationSpecV09>& views,
    std::vector<double>& weights,
    double target,
    double widthStops) {

    if (!positive_finite(positiveSceneSignal)) return StatusV09::error("positiveSceneSignal must be > 0");
    if (!positive_finite(target)) return StatusV09::error("target must be > 0");
    if (!positive_finite(widthStops)) return StatusV09::error("widthStops must be > 0");
    if (views.empty()) return StatusV09::error("views cannot be empty");

    weights.assign(views.size(), 0.0);
    double sum = 0.0;
    for (std::size_t i = 0; i < views.size(); ++i) {
        auto s = validate_virtual_observation_spec_v0_9(views[i]); if (!s) return s;
        double exposureScale = 0.0;
        s = scale_from_ev(views[i].exposureEv, exposureScale); if (!s) return s;
        const double mapped = positiveSceneSignal * exposureScale;
        const double d = std::abs(std::log2(mapped / target));
        const double q = d / widthStops;
        const double w = std::exp(-0.5*q*q);
        if (!nonnegative_finite(w)) return StatusV09::error("weight calculation failed");
        weights[i] = w;
        sum += w;
    }
    if (!positive_finite(sum)) return StatusV09::error("weight normalization sum invalid");
    for (double& w : weights) w /= sum;
    return StatusV09::success();
}

StatusV09 validate_virtual_sensor_forward_model_v0_9(const VirtualSensorForwardModelV09& model) {
    if (model.authority == SensorModelAuthorityV09::Unresolved)
        return StatusV09::error("virtual sensor forward model authority is unresolved");
    if (model.modelId.empty() || model.sourceClassId.empty())
        return StatusV09::error("virtual sensor model requires modelId and sourceClassId");
    if (!positive_finite(model.referenceIso)) return StatusV09::error("referenceIso must be positive");
    if (!positive_finite(model.sceneToPregainSignal)) return StatusV09::error("sceneToPregainSignal must be positive");
    if (!nonnegative_finite(model.shotVarianceSlopePregain)) return StatusV09::error("shot variance slope invalid");
    if (!nonnegative_finite(model.readVariancePregain)) return StatusV09::error("read variance invalid");
    if (!positive_finite(model.saturationPregain)) return StatusV09::error("saturationPregain must be positive");
    return StatusV09::success();
}

StatusV09 predict_virtual_sensor_observation_v0_9(
    double positiveSceneSignal,
    const VirtualObservationSpecV09& spec,
    const VirtualSensorForwardModelV09& model,
    VirtualSensorPredictionV09& out) {

    out = VirtualSensorPredictionV09{};
    auto s = validate_virtual_observation_spec_v0_9(spec); if (!s) return s;
    s = validate_virtual_sensor_forward_model_v0_9(model); if (!s) return s;
    if (spec.isoSemantics != VirtualIsoSemanticsV09::CalibratedForwardModel)
        return StatusV09::error("physical/noise forward prediction requires CALIBRATED_FORWARD_MODEL ISO semantics");
    if (!nonnegative_finite(positiveSceneSignal))
        return StatusV09::error("physical virtual-sensor prediction requires nonnegative scene light");

    double exposureScale = 0.0, gainScale = 0.0;
    s = scale_from_ev(spec.exposureEv, exposureScale); if (!s) return s;
    s = scale_from_ev(spec.gainEv, gainScale); if (!s) return s;

    const double pregain = positiveSceneSignal * exposureScale * model.sceneToPregainSignal;
    const double varPregain = model.shotVarianceSlopePregain * pregain + model.readVariancePregain;
    const double meanEncoded = pregain * gainScale;
    const double varEncoded = varPregain * gainScale * gainScale;
    if (!nonnegative_finite(pregain) || !nonnegative_finite(varPregain) ||
        !nonnegative_finite(meanEncoded) || !nonnegative_finite(varEncoded))
        return StatusV09::error("virtual sensor prediction overflow/invalid");

    out.valid = true;
    out.expectedPregainSignal = pregain;
    out.expectedEncodedSignal = meanEncoded;
    out.conditionalVarianceEncoded = varEncoded;
    out.highCensored = pregain >= model.saturationPregain;
    if (out.highCensored) out.censorLowerBoundEncoded = model.saturationPregain * gainScale;
    out.modelId = model.modelId;
    return StatusV09::success();
}

} // namespace truthraw_v09

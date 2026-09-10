#include "manifold_conditioning_v1.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace truthraw_mc_v1 {
namespace {

bool finite(double x) { return std::isfinite(x); }
bool positive_finite(double x) { return finite(x) && x > 0.0; }
bool nonnegative_finite(double x) { return finite(x) && x >= 0.0; }

StatusV1 validate_exact_view(const truthraw_v09::VirtualObservationSpecV09& view) {
    const auto s = truthraw_v09::validate_virtual_observation_spec_v0_9(view);
    if (!s) return StatusV1::error(std::string("v0.9 rejected conditioning view: ") + s.message);
    if (view.gainEv != 0.0)
        return StatusV1::error("EXACT_REPARAMETERIZATION accepts exposure EV only; gain/ISO encoding is not an optimizer coordinate");
    if (view.isoSemantics != truthraw_v09::VirtualIsoSemanticsV09::None)
        return StatusV1::error("EXACT_REPARAMETERIZATION rejects ISO semantics");
    if (!finite(view.exposureEv)) return StatusV1::error("conditioning EV must be finite");
    const double scale = std::exp2(view.exposureEv);
    if (!positive_finite(scale)) return StatusV1::error("conditioning scale invalid");
    return StatusV1::success();
}

bool valid_metric(const HeldoutMetricV1& m) {
    return m.samples > 0 &&
           positive_finite(m.baselineMae) && nonnegative_finite(m.candidateMae) &&
           positive_finite(m.baselineP95Abs) && nonnegative_finite(m.candidateP95Abs) &&
           finite(m.baselineOrdering) && finite(m.candidateOrdering) &&
           finite(m.baselineCurvature) && finite(m.candidateCurvature) &&
           m.baselineOrdering >= 0.0 && m.baselineOrdering <= 1.0 &&
           m.candidateOrdering >= 0.0 && m.candidateOrdering <= 1.0 &&
           m.baselineCurvature >= 0.0 && m.baselineCurvature <= 1.0 &&
           m.candidateCurvature >= 0.0 && m.candidateCurvature <= 1.0;
}

} // namespace

const char* selection_status_name_v1(SelectionStatusV1 status) {
    switch (status) {
        case SelectionStatusV1::InvalidEvidence: return "INVALID_EVIDENCE";
        case SelectionStatusV1::InsufficientIndependentEvidence: return "INSUFFICIENT_INDEPENDENT_EVIDENCE";
        case SelectionStatusV1::RejectedMetricRegression: return "REJECTED_METRIC_REGRESSION";
        case SelectionStatusV1::Eligible: return "ELIGIBLE";
    }
    return "UNKNOWN";
}

StatusV1 validate_manifold_for_conditioning_v1(
    const truthraw_v09::VirtualObservationManifoldV09& manifold,
    ConditioningLedgerV1& ledger) {

    if (manifold.sourceEvidenceId.empty()) return StatusV1::error("sourceEvidenceId must be non-empty");
    if (manifold.views.empty()) return StatusV1::error("conditioning requires at least one virtual view");
    if (manifold.physicalFrameCount != 1 || manifold.independentEvidenceCount != 1 ||
        manifold.viewsAreIndependentMeasurements || manifold.evidenceConfidenceMultiplier != 1.0)
        return StatusV1::error("virtual manifold violates the one-frame/one-evidence-root invariant");

    ledger = ConditioningLedgerV1{};
    ledger.sourceEvidenceId = manifold.sourceEvidenceId;
    return StatusV1::success();
}

StatusV1 exact_condition_scalar_v1(
    double value,
    double sigma,
    const truthraw_v09::VirtualObservationSpecV09& view,
    ExactScalarV1& out) {

    const auto v = validate_exact_view(view); if (!v) return v;
    if (!finite(value)) return StatusV1::error("value must be finite");
    if (!(std::isnan(sigma) || nonnegative_finite(sigma))) return StatusV1::error("sigma must be nonnegative finite or NaN/unresolved");
    const double scale = std::exp2(view.exposureEv);
    const double y = value * scale;
    const double sy = std::isnan(sigma) ? sigma : sigma * scale;
    if (!finite(y) || (!std::isnan(sy) && !finite(sy))) return StatusV1::error("conditioning overflow");
    out = ExactScalarV1{y, sy, scale, view.exposureEv};
    return StatusV1::success();
}

StatusV1 exact_decondition_scalar_v1(
    const ExactScalarV1& conditioned,
    double& value,
    double& sigma) {

    if (!positive_finite(conditioned.scale)) return StatusV1::error("conditioned scale invalid");
    if (!finite(conditioned.conditionedValue)) return StatusV1::error("conditioned value invalid");
    if (!(std::isnan(conditioned.conditionedSigma) || nonnegative_finite(conditioned.conditionedSigma)))
        return StatusV1::error("conditioned sigma invalid");
    value = conditioned.conditionedValue / conditioned.scale;
    sigma = std::isnan(conditioned.conditionedSigma)
        ? conditioned.conditionedSigma : conditioned.conditionedSigma / conditioned.scale;
    if (!finite(value) || (!std::isnan(sigma) && !finite(sigma))) return StatusV1::error("deconditioning failed");
    return StatusV1::success();
}

StatusV1 exact_condition_rgb_v1(
    const std::array<float, 3>& cameraRgb,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    const truthraw_v09::VirtualObservationSpecV09& view,
    ExactRgbV1& out) {

    const auto v = validate_exact_view(view); if (!v) return v;
    truthraw_v09::VirtualCameraRgbPixelV09 tmp;
    const auto s = truthraw_v09::project_camera_rgb_pixel_v0_9(cameraRgb, covariance, "conditioning-shared-evidence", view, tmp);
    if (!s) return StatusV1::error(std::string("v0.9 RGB projection failed: ") + s.message);
    out = ExactRgbV1{};
    out.conditionedRgb = tmp.encodedCameraRgb;
    out.conditionedCovariance = tmp.encodedCovariance;
    out.scale = tmp.exposureScale;
    out.exposureEv = view.exposureEv;
    return StatusV1::success();
}

StatusV1 exact_decondition_rgb_v1(
    const ExactRgbV1& conditioned,
    std::array<float, 3>& cameraRgb,
    truthraw_v06::PixelCameraRgbCovarianceV06& covariance) {

    if (!positive_finite(conditioned.scale)) return StatusV1::error("conditioned RGB scale invalid");
    const float inv = static_cast<float>(1.0 / conditioned.scale);
    cameraRgb = conditioned.conditionedRgb;
    for (float& x : cameraRgb) x *= inv;
    covariance = conditioned.conditionedCovariance;
    const auto s = truthraw_v06::scale_pixel_covariance_v0_6(covariance, inv);
    if (!s) return StatusV1::error(std::string("v0.6 covariance inverse scaling failed: ") + s.message);
    return StatusV1::success();
}

StatusV1 evaluate_model_selection_v1(
    const std::string& candidateModelId,
    const std::string& channel,
    const std::string& regimeId,
    const PromotionPolicyV1& policy,
    const std::vector<HeldoutSceneEvidenceV1>& evidence,
    SelectionDecisionV1& out) {

    out = SelectionDecisionV1{};
    out.candidateModelId = candidateModelId;
    out.channel = channel;
    out.regimeId = regimeId;

    if (candidateModelId.empty() || channel.empty() || regimeId.empty())
        return StatusV1::error("candidateModelId/channel/regimeId must be non-empty");
    if (policy.minIndependentScenes == 0 || policy.minSamplesPerScene == 0 ||
        !positive_finite(policy.maxMaeRatio) || !positive_finite(policy.maxP95Ratio) ||
        !finite(policy.minOrderingDelta) || !finite(policy.minCurvatureDelta))
        return StatusV1::error("promotion policy invalid");

    std::set<std::string> seenRoots;
    bool metricRegression = false;
    double worstMae = 0.0, worstP95 = 0.0;
    double worstOrd = std::numeric_limits<double>::infinity();
    double worstCurv = std::numeric_limits<double>::infinity();

    for (const auto& e : evidence) {
        if (e.channel != channel || e.regimeId != regimeId) continue;
        if (!e.candidateFrozenBeforeEvidence) {
            out.reasons.push_back("excluded tuning/development evidence: " + e.evidenceId);
            continue;
        }
        if (!e.independentEvidenceRoot) {
            out.reasons.push_back("excluded non-independent evidence root: " + e.evidenceId);
            continue;
        }
        if (e.evidenceId.empty() || e.sourceSceneId.empty() || !valid_metric(e.metrics)) {
            out.status = SelectionStatusV1::InvalidEvidence;
            return StatusV1::error("invalid held-out evidence record");
        }
        if (!seenRoots.insert(e.sourceSceneId).second) {
            out.reasons.push_back("duplicate source scene/evidence root excluded: " + e.sourceSceneId);
            continue;
        }
        if (e.metrics.samples < policy.minSamplesPerScene) {
            out.reasons.push_back("insufficient samples in held-out scene: " + e.sourceSceneId);
            continue;
        }

        ++out.qualifyingSceneCount;
        out.qualifyingSampleCount += e.metrics.samples;
        const double maeRatio = e.metrics.candidateMae / e.metrics.baselineMae;
        const double p95Ratio = e.metrics.candidateP95Abs / e.metrics.baselineP95Abs;
        const double ordDelta = e.metrics.candidateOrdering - e.metrics.baselineOrdering;
        const double curvDelta = e.metrics.candidateCurvature - e.metrics.baselineCurvature;
        worstMae = std::max(worstMae, maeRatio);
        worstP95 = std::max(worstP95, p95Ratio);
        worstOrd = std::min(worstOrd, ordDelta);
        worstCurv = std::min(worstCurv, curvDelta);

        if (maeRatio > policy.maxMaeRatio || p95Ratio > policy.maxP95Ratio ||
            ordDelta < policy.minOrderingDelta || curvDelta < policy.minCurvatureDelta) {
            metricRegression = true;
            out.reasons.push_back("held-out metric gate failed: " + e.sourceSceneId);
        }
    }

    out.worstMaeRatio = out.qualifyingSceneCount ? worstMae : std::numeric_limits<double>::quiet_NaN();
    out.worstP95Ratio = out.qualifyingSceneCount ? worstP95 : std::numeric_limits<double>::quiet_NaN();
    out.worstOrderingDelta = out.qualifyingSceneCount ? worstOrd : std::numeric_limits<double>::quiet_NaN();
    out.worstCurvatureDelta = out.qualifyingSceneCount ? worstCurv : std::numeric_limits<double>::quiet_NaN();

    if (out.qualifyingSceneCount < policy.minIndependentScenes) {
        out.status = SelectionStatusV1::InsufficientIndependentEvidence;
        out.reasons.push_back("not enough qualifying independent held-out scenes");
    } else if (metricRegression) {
        out.status = SelectionStatusV1::RejectedMetricRegression;
    } else {
        out.status = SelectionStatusV1::Eligible;
    }
    return StatusV1::success();
}

StatusV1 apply_selected_estimate_v1(
    double canonicalBaseline,
    double candidateEstimate,
    const SelectionDecisionV1& decision,
    double& out) {

    if (!finite(canonicalBaseline) || !finite(candidateEstimate))
        return StatusV1::error("estimates must be finite");
    out = (decision.status == SelectionStatusV1::Eligible) ? candidateEstimate : canonicalBaseline;
    return StatusV1::success();
}

} // namespace truthraw_mc_v1

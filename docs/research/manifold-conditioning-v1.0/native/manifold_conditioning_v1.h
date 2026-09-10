#pragma once

#include "virtual_observation_manifold_v0_9.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace truthraw_mc_v1 {

enum class ConditioningModeV1 : std::uint8_t {
    ExactReparameterization = 0,
    RobustModelSelection = 1,
};

enum class SelectionStatusV1 : std::uint8_t {
    InvalidEvidence = 0,
    InsufficientIndependentEvidence = 1,
    RejectedMetricRegression = 2,
    Eligible = 3,
};

struct StatusV1 {
    bool ok = true;
    std::string message;
    explicit operator bool() const { return ok; }
    static StatusV1 success() { return {}; }
    static StatusV1 error(std::string m) { return {false, std::move(m)}; }
};

struct ConditioningLedgerV1 {
    std::string sourceEvidenceId;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    bool virtualViewsAreIndependentMeasurements = false;
    double evidenceConfidenceMultiplier = 1.0;
};

struct ExactScalarV1 {
    double conditionedValue = 0.0;
    double conditionedSigma = std::numeric_limits<double>::quiet_NaN();
    double scale = 1.0;
    double exposureEv = 0.0;
};

struct ExactRgbV1 {
    std::array<float, 3> conditionedRgb {0.f, 0.f, 0.f};
    truthraw_v06::PixelCameraRgbCovarianceV06 conditionedCovariance;
    double scale = 1.0;
    double exposureEv = 0.0;
};

struct HeldoutMetricV1 {
    std::uint64_t samples = 0;
    double baselineMae = std::numeric_limits<double>::quiet_NaN();
    double candidateMae = std::numeric_limits<double>::quiet_NaN();
    double baselineP95Abs = std::numeric_limits<double>::quiet_NaN();
    double candidateP95Abs = std::numeric_limits<double>::quiet_NaN();
    double baselineOrdering = std::numeric_limits<double>::quiet_NaN();
    double candidateOrdering = std::numeric_limits<double>::quiet_NaN();
    double baselineCurvature = std::numeric_limits<double>::quiet_NaN();
    double candidateCurvature = std::numeric_limits<double>::quiet_NaN();
};

struct HeldoutSceneEvidenceV1 {
    std::string evidenceId;
    std::string sourceSceneId;
    std::string channel;
    std::string regimeId;
    bool candidateFrozenBeforeEvidence = false;
    bool independentEvidenceRoot = true;
    HeldoutMetricV1 metrics;
};

struct PromotionPolicyV1 {
    std::size_t minIndependentScenes = 3;
    std::uint64_t minSamplesPerScene = 10000;
    double maxMaeRatio = 0.999;
    double maxP95Ratio = 1.0;
    double minOrderingDelta = 0.0;
    double minCurvatureDelta = 0.0;
};

struct SelectionDecisionV1 {
    SelectionStatusV1 status = SelectionStatusV1::InvalidEvidence;
    std::string candidateModelId;
    std::string channel;
    std::string regimeId;
    std::size_t qualifyingSceneCount = 0;
    std::uint64_t qualifyingSampleCount = 0;
    double worstMaeRatio = std::numeric_limits<double>::quiet_NaN();
    double worstP95Ratio = std::numeric_limits<double>::quiet_NaN();
    double worstOrderingDelta = std::numeric_limits<double>::quiet_NaN();
    double worstCurvatureDelta = std::numeric_limits<double>::quiet_NaN();
    std::vector<std::string> reasons;
    std::string claimBoundary =
        "Eligibility is a held-out model-selection permission only. It does not create sensor evidence, "
        "does not certify co-sited missing colours, and does not alter the sealed source likelihood.";
};

StatusV1 validate_manifold_for_conditioning_v1(
    const truthraw_v09::VirtualObservationManifoldV09& manifold,
    ConditioningLedgerV1& ledger);

StatusV1 exact_condition_scalar_v1(
    double value,
    double sigma,
    const truthraw_v09::VirtualObservationSpecV09& view,
    ExactScalarV1& out);

StatusV1 exact_decondition_scalar_v1(
    const ExactScalarV1& conditioned,
    double& value,
    double& sigma);

StatusV1 exact_condition_rgb_v1(
    const std::array<float, 3>& cameraRgb,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    const truthraw_v09::VirtualObservationSpecV09& view,
    ExactRgbV1& out);

StatusV1 exact_decondition_rgb_v1(
    const ExactRgbV1& conditioned,
    std::array<float, 3>& cameraRgb,
    truthraw_v06::PixelCameraRgbCovarianceV06& covariance);

StatusV1 evaluate_model_selection_v1(
    const std::string& candidateModelId,
    const std::string& channel,
    const std::string& regimeId,
    const PromotionPolicyV1& policy,
    const std::vector<HeldoutSceneEvidenceV1>& evidence,
    SelectionDecisionV1& out);

StatusV1 apply_selected_estimate_v1(
    double canonicalBaseline,
    double candidateEstimate,
    const SelectionDecisionV1& decision,
    double& out);

const char* selection_status_name_v1(SelectionStatusV1 status);

} // namespace truthraw_mc_v1

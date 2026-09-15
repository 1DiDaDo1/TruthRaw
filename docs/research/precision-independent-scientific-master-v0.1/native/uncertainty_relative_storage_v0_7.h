#pragma once

#include "precision_policy_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace truthraw_precision_v07 {

enum class UncertaintySemanticsV07 : std::uint8_t {
    Unknown = 0,
    GaussianEquivalentSigma = 1,
    ErrorQuantileP50 = 2,
    ErrorQuantileP95 = 3,
};

struct UncertaintyAnchorV07 {
    UncertaintySemanticsV07 semantics = UncertaintySemanticsV07::Unknown;
    double value = std::numeric_limits<double>::quiet_NaN();
    bool known = false;
};

struct StorageRelativeAssessmentV07 {
    double f64Reference = std::numeric_limits<double>::quiet_NaN();
    float f32Stored = std::numeric_limits<float>::quiet_NaN();
    double absStorageError = std::numeric_limits<double>::quiet_NaN();
    UncertaintySemanticsV07 semantics = UncertaintySemanticsV07::Unknown;
    double anchorValue = std::numeric_limits<double>::quiet_NaN();
    double errorOverAnchor = std::numeric_limits<double>::quiet_NaN();
    bool comparable = false;
    bool withinRequestedRatio = false;
    bool gaussianEquivalentComparison = false;
};

struct StorageRelativeBatchStatsV07 {
    std::uint64_t samples = 0;
    std::uint64_t unknownOrInvalidAnchors = 0;
    std::uint64_t gaussianComparable = 0;
    std::uint64_t quantileComparable = 0;
    std::uint64_t withinRequestedRatio = 0;
    std::uint64_t outsideRequestedRatio = 0;
    double maxGaussianErrorOverSigma = 0.0;
    double maxQuantileErrorOverAnchor = 0.0;
    double maxAbsStorageError = 0.0;
};

// A real Gaussian-equivalent sigma may be used only when that sigma is already
// an admitted uncertainty quantity. This helper does not derive sigma from a
// percentile, confidence score, or arbitrary error band.
UncertaintyAnchorV07 gaussian_sigma_anchor_v0_7(double sigmaEquivalent);

// Quantile anchors remain quantile anchors. They are useful for scale
// comparison, but they never become variance/covariance or Gaussian sigma.
UncertaintyAnchorV07 p50_error_anchor_v0_7(double p50Error);
UncertaintyAnchorV07 p95_error_anchor_v0_7(double p95Error);

// Extract one marginal sigma from a known diagonal covariance term. A full RGB
// covariance is not required for this componentwise comparison, but an unknown
// diagonal stays unknown/NaN. Off-diagonals are never assumed zero.
UncertaintyAnchorV07 covariance_diagonal_sigma_anchor_v0_7(
    const truthraw_precision_v01::Covariance3dV01& covariance,
    int channel);

// Compare one already-computed F64 scientific value with its F32 storage value.
// maxErrorOverAnchor is a numerical policy threshold, not an evidence threshold.
StorageRelativeAssessmentV07 assess_f64_to_f32_storage_v0_7(
    double f64Reference,
    const UncertaintyAnchorV07& anchor,
    double maxErrorOverAnchor);

StorageRelativeBatchStatsV07 assess_f64_to_f32_storage_batch_v0_7(
    const double* f64Reference,
    const UncertaintyAnchorV07* anchors,
    std::size_t count,
    double maxErrorOverAnchor);

} // namespace truthraw_precision_v07

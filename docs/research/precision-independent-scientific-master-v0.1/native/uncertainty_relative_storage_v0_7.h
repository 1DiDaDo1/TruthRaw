#pragma once

#include "precision_policy_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace truthraw_precision_v07 {

// Exact identity declared by canonical/uncertainty/v5.0g-p1/UNCERTAINTY_MODEL_v5_0g.json.
// The precision layer never recomputes or weakens this identity.
constexpr const char* kV5gP1UncertaintyBindingSha256V07 =
    "61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";
constexpr const char* kV5gP1FeatureSchemaSha256V07 =
    "8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3";

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

struct V5gP1QuantileBindingV07 {
    UncertaintyAnchorV07 p50{};
    UncertaintyAnchorV07 p95{};
    bool bindingAccepted = false;
    bool censored = false;
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
    bool storageNonFinite = false;
};

struct StorageRelativeBatchStatsV07 {
    std::uint64_t samples = 0;
    std::uint64_t unknownOrInvalidAnchors = 0;
    std::uint64_t storageNonFinite = 0;
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

// Generic quantile anchors preserve semantics but do NOT, by themselves, prove
// that the values came from canonical v5.0g-p1.
UncertaintyAnchorV07 p50_error_anchor_v0_7(double p50Error);
UncertaintyAnchorV07 p95_error_anchor_v0_7(double p95Error);

// Exact identity check for canonical v5.0g-p1. A model-binding mismatch, feature-
// schema mismatch, censored sample, invalid p50/p95, or p95<p50 returns unknown
// anchors. Missing/invalid uncertainty is never replaced with zero.
bool v5g_p1_binding_exact_v0_7(const std::string& uncertaintyBindingSha256,
                               const std::string& featureSchemaSha256);
V5gP1QuantileBindingV07 bind_v5g_p1_quantiles_v0_7(
    double p50Error,
    double p95Error,
    bool censored,
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256);

// Extract one marginal sigma from a known diagonal covariance term. A full RGB
// covariance is not required for this componentwise comparison, but an unknown
// diagonal stays unknown/NaN. Off-diagonals are never assumed zero.
UncertaintyAnchorV07 covariance_diagonal_sigma_anchor_v0_7(
    const truthraw_precision_v01::Covariance3dV01& covariance,
    int channel);

// Compare one already-computed F64 scientific value with its F32 storage value.
// maxErrorOverAnchor is a numerical policy threshold, not an evidence threshold.
// If a finite Free-Scientific-Space F64 value overflows Float32, the comparison
// fails closed with storageNonFinite=true.
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

#include "uncertainty_relative_storage_v0_7.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v07 {
namespace {

bool finite_nonnegative(double x) {
    return std::isfinite(x) && x >= 0.0;
}

UncertaintyAnchorV07 make_anchor(UncertaintySemanticsV07 semantics, double value) {
    UncertaintyAnchorV07 a;
    if (!finite_nonnegative(value)) return a;
    a.semantics = semantics;
    a.value = value;
    a.known = true;
    return a;
}

bool is_quantile(UncertaintySemanticsV07 s) {
    return s == UncertaintySemanticsV07::ErrorQuantileP50 ||
           s == UncertaintySemanticsV07::ErrorQuantileP95;
}

} // namespace

UncertaintyAnchorV07 gaussian_sigma_anchor_v0_7(double sigmaEquivalent) {
    return make_anchor(UncertaintySemanticsV07::GaussianEquivalentSigma, sigmaEquivalent);
}

UncertaintyAnchorV07 p50_error_anchor_v0_7(double p50Error) {
    return make_anchor(UncertaintySemanticsV07::ErrorQuantileP50, p50Error);
}

UncertaintyAnchorV07 p95_error_anchor_v0_7(double p95Error) {
    return make_anchor(UncertaintySemanticsV07::ErrorQuantileP95, p95Error);
}

UncertaintyAnchorV07 covariance_diagonal_sigma_anchor_v0_7(
    const truthraw_precision_v01::Covariance3dV01& covariance,
    int channel) {
    UncertaintyAnchorV07 unknown;
    if (channel < 0 || channel > 2) return unknown;
    const int bitIndex = channel * 3 + channel;
    const std::uint16_t bit = static_cast<std::uint16_t>(1u << bitIndex);
    if ((covariance.knownMask & bit) == 0u) return unknown;
    const double variance = covariance.at(channel, channel);
    if (!finite_nonnegative(variance)) return unknown;
    return gaussian_sigma_anchor_v0_7(std::sqrt(variance));
}

StorageRelativeAssessmentV07 assess_f64_to_f32_storage_v0_7(
    double f64Reference,
    const UncertaintyAnchorV07& anchor,
    double maxErrorOverAnchor) {
    StorageRelativeAssessmentV07 out;
    out.f64Reference = f64Reference;
    out.semantics = anchor.semantics;
    out.anchorValue = anchor.value;

    if (!std::isfinite(f64Reference) || !anchor.known ||
        anchor.semantics == UncertaintySemanticsV07::Unknown ||
        !finite_nonnegative(anchor.value) ||
        !finite_nonnegative(maxErrorOverAnchor)) {
        return out;
    }

    out.f32Stored = static_cast<float>(f64Reference);
    if (!std::isfinite(out.f32Stored)) return out;
    out.absStorageError = std::abs(static_cast<double>(out.f32Stored) - f64Reference);

    if (anchor.value == 0.0) {
        out.errorOverAnchor = out.absStorageError == 0.0
            ? 0.0
            : std::numeric_limits<double>::infinity();
    } else {
        out.errorOverAnchor = out.absStorageError / anchor.value;
    }

    out.comparable = true;
    out.gaussianEquivalentComparison =
        anchor.semantics == UncertaintySemanticsV07::GaussianEquivalentSigma;
    out.withinRequestedRatio = out.errorOverAnchor <= maxErrorOverAnchor;
    return out;
}

StorageRelativeBatchStatsV07 assess_f64_to_f32_storage_batch_v0_7(
    const double* f64Reference,
    const UncertaintyAnchorV07* anchors,
    std::size_t count,
    double maxErrorOverAnchor) {
    StorageRelativeBatchStatsV07 stats;
    stats.samples = static_cast<std::uint64_t>(count);
    if (!f64Reference || !anchors) {
        stats.unknownOrInvalidAnchors = stats.samples;
        return stats;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const auto a = assess_f64_to_f32_storage_v0_7(
            f64Reference[i], anchors[i], maxErrorOverAnchor);
        if (!a.comparable) {
            ++stats.unknownOrInvalidAnchors;
            continue;
        }
        stats.maxAbsStorageError = std::max(stats.maxAbsStorageError, a.absStorageError);
        if (a.gaussianEquivalentComparison) {
            ++stats.gaussianComparable;
            stats.maxGaussianErrorOverSigma =
                std::max(stats.maxGaussianErrorOverSigma, a.errorOverAnchor);
        } else if (is_quantile(a.semantics)) {
            ++stats.quantileComparable;
            stats.maxQuantileErrorOverAnchor =
                std::max(stats.maxQuantileErrorOverAnchor, a.errorOverAnchor);
        }
        if (a.withinRequestedRatio) ++stats.withinRequestedRatio;
        else ++stats.outsideRequestedRatio;
    }
    return stats;
}

} // namespace truthraw_precision_v07

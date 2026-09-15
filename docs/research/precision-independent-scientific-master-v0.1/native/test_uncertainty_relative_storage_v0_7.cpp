#include "uncertainty_relative_storage_v0_7.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace truthraw_precision_v07;

int main() {
    // Unknown stays unknown. No numerical storage decision may manufacture an
    // uncertainty authority that the upstream model did not provide.
    const UncertaintyAnchorV07 unknown;
    const auto u = assess_f64_to_f32_storage_v0_7(0.123456789, unknown, 1e-3);
    assert(!u.comparable);
    assert(!u.withinRequestedRatio);
    assert(std::isnan(u.errorOverAnchor));

    // A known marginal variance may provide a componentwise sigma without
    // inventing any unknown RGB off-diagonal covariance.
    truthraw_precision_v01::Covariance3dV01 cov;
    cov.at(0,0) = 4.0e-4; // sigma = 0.02
    cov.knownMask |= static_cast<std::uint16_t>(1u << 0);
    const auto sigma = covariance_diagonal_sigma_anchor_v0_7(cov, 0);
    assert(sigma.known);
    assert(sigma.semantics == UncertaintySemanticsV07::GaussianEquivalentSigma);
    assert(std::abs(sigma.value - 0.02) < 1e-15);
    // Unknown off-diagonals and other diagonals remain unknown.
    assert(std::isnan(cov.at(0,1)));
    assert(!covariance_diagonal_sigma_anchor_v0_7(cov, 1).known);

    const auto s = assess_f64_to_f32_storage_v0_7(0.123456789, sigma, 1e-3);
    assert(s.comparable);
    assert(s.gaussianEquivalentComparison);
    assert(s.withinRequestedRatio);
    assert(s.errorOverAnchor < 1e-5);

    // Canonical v5.0g-p1 reconstructed-channel anchors are quantile-only. They
    // may be used for a scale comparison but MUST NOT become Gaussian sigma.
    const auto p50 = p50_error_anchor_v0_7(0.008743);
    const auto p95 = p95_error_anchor_v0_7(0.036181);
    assert(p50.known && p95.known);
    assert(p50.semantics == UncertaintySemanticsV07::ErrorQuantileP50);
    assert(p95.semantics == UncertaintySemanticsV07::ErrorQuantileP95);
    const auto q50 = assess_f64_to_f32_storage_v0_7(0.123456789, p50, 1e-3);
    const auto q95 = assess_f64_to_f32_storage_v0_7(0.123456789, p95, 1e-3);
    assert(q50.comparable && q95.comparable);
    assert(!q50.gaussianEquivalentComparison);
    assert(!q95.gaussianEquivalentComparison);

    // Zero uncertainty is fail-hard unless storage is exactly lossless.
    const auto zeroSigma = gaussian_sigma_anchor_v0_7(0.0);
    const auto exact = assess_f64_to_f32_storage_v0_7(0.5, zeroSigma, 0.0);
    assert(exact.comparable && exact.absStorageError == 0.0);
    assert(exact.errorOverAnchor == 0.0 && exact.withinRequestedRatio);
    const auto nonExact = assess_f64_to_f32_storage_v0_7(0.1, zeroSigma, 1.0);
    assert(nonExact.comparable);
    assert(std::isinf(nonExact.errorOverAnchor));
    assert(!nonExact.withinRequestedRatio);

    // Invalid variances do not become uncertainty.
    truthraw_precision_v01::Covariance3dV01 invalid;
    invalid.at(2,2) = -1.0;
    invalid.knownMask |= static_cast<std::uint16_t>(1u << 8);
    assert(!covariance_diagonal_sigma_anchor_v0_7(invalid, 2).known);

    // Batch accounting preserves semantic classes and unresolved entries.
    const double values[4] = {0.123456789, 0.25, 0.75, 0.333333333333};
    const UncertaintyAnchorV07 anchors[4] = {
        sigma,
        p50,
        unknown,
        p95
    };
    const auto batch = assess_f64_to_f32_storage_batch_v0_7(values, anchors, 4, 1e-3);
    assert(batch.samples == 4);
    assert(batch.gaussianComparable == 1);
    assert(batch.quantileComparable == 2);
    assert(batch.unknownOrInvalidAnchors == 1);
    assert(batch.outsideRequestedRatio == 0);
    assert(batch.withinRequestedRatio == 3);

    std::cout << "test_uncertainty_relative_storage_v0_7 PASS"
              << " max_sigma_ratio=" << batch.maxGaussianErrorOverSigma
              << " max_quantile_ratio=" << batch.maxQuantileErrorOverAnchor
              << "\n";
    return 0;
}

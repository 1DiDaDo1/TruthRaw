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

    // Generic quantiles retain quantile semantics but do not prove a model identity.
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

    // Canonical v5.0g-p1 quantiles require exact model + feature-schema binding.
    assert(v5g_p1_binding_exact_v0_7(
        kV5gP1UncertaintyBindingSha256V07,
        kV5gP1FeatureSchemaSha256V07));
    assert(!v5g_p1_binding_exact_v0_7(
        "wrong-binding",
        kV5gP1FeatureSchemaSha256V07));

    const auto bound = bind_v5g_p1_quantiles_v0_7(
        0.008743, 0.036181, false,
        kV5gP1UncertaintyBindingSha256V07,
        kV5gP1FeatureSchemaSha256V07);
    assert(bound.bindingAccepted && !bound.censored);
    assert(bound.p50.known && bound.p95.known);

    const auto mismatched = bind_v5g_p1_quantiles_v0_7(
        0.008743, 0.036181, false,
        "wrong-binding",
        kV5gP1FeatureSchemaSha256V07);
    assert(!mismatched.bindingAccepted);
    assert(!mismatched.p50.known && !mismatched.p95.known);

    const auto censored = bind_v5g_p1_quantiles_v0_7(
        0.008743, 0.036181, true,
        kV5gP1UncertaintyBindingSha256V07,
        kV5gP1FeatureSchemaSha256V07);
    assert(censored.bindingAccepted && censored.censored);
    assert(!censored.p50.known && !censored.p95.known);

    const auto invalidOrder = bind_v5g_p1_quantiles_v0_7(
        0.04, 0.01, false,
        kV5gP1UncertaintyBindingSha256V07,
        kV5gP1FeatureSchemaSha256V07);
    assert(invalidOrder.bindingAccepted);
    assert(!invalidOrder.p50.known && !invalidOrder.p95.known);

    // Zero uncertainty is fail-hard unless storage is exactly lossless.
    const auto zeroSigma = gaussian_sigma_anchor_v0_7(0.0);
    const auto exact = assess_f64_to_f32_storage_v0_7(0.5, zeroSigma, 0.0);
    assert(exact.comparable && exact.absStorageError == 0.0);
    assert(exact.errorOverAnchor == 0.0 && exact.withinRequestedRatio);
    const auto nonExact = assess_f64_to_f32_storage_v0_7(0.1, zeroSigma, 1.0);
    assert(nonExact.comparable);
    assert(std::isinf(nonExact.errorOverAnchor));
    assert(!nonExact.withinRequestedRatio);

    // Free Scientific Space is not bounded by Float32. A finite F64 scene value
    // outside F32 range must fail closed rather than becoming a stored infinity.
    const double beyondF32 = static_cast<double>(std::numeric_limits<float>::max()) * 2.0;
    const auto overflow = assess_f64_to_f32_storage_v0_7(beyondF32, sigma, 1.0);
    assert(!overflow.comparable);
    assert(overflow.storageNonFinite);
    assert(!overflow.withinRequestedRatio);

    // Invalid variances do not become uncertainty.
    truthraw_precision_v01::Covariance3dV01 invalid;
    invalid.at(2,2) = -1.0;
    invalid.knownMask |= static_cast<std::uint16_t>(1u << 8);
    assert(!covariance_diagonal_sigma_anchor_v0_7(invalid, 2).known);

    // Batch accounting preserves semantic classes, unresolved entries, and F32 overflow.
    const double values[5] = {0.123456789, 0.25, 0.75, 0.333333333333, beyondF32};
    const UncertaintyAnchorV07 anchors[5] = {
        sigma,
        bound.p50,
        unknown,
        bound.p95,
        sigma
    };
    const auto batch = assess_f64_to_f32_storage_batch_v0_7(values, anchors, 5, 1e-3);
    assert(batch.samples == 5);
    assert(batch.gaussianComparable == 1);
    assert(batch.quantileComparable == 2);
    assert(batch.unknownOrInvalidAnchors == 1);
    assert(batch.storageNonFinite == 1);
    assert(batch.outsideRequestedRatio == 0);
    assert(batch.withinRequestedRatio == 3);

    std::cout << "test_uncertainty_relative_storage_v0_7 PASS"
              << " max_sigma_ratio=" << batch.maxGaussianErrorOverSigma
              << " max_quantile_ratio=" << batch.maxQuantileErrorOverAnchor
              << " storage_nonfinite=" << batch.storageNonFinite
              << "\n";
    return 0;
}

#include "camera_rgb_covariance_v0_6_storage_adapter_v0_7.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace truthraw_precision_v07;

int main() {
    // Measured Gaussian-equivalent source: a known marginal variance is allowed
    // to become a componentwise sigma. Unknown off-diagonals remain untouched.
    truthraw_v06::MarginalChannelKnowledgeV06 measured;
    measured.source = truthraw_v06::MarginalSourceV06::NoiseProfileGaussianEquivalent;
    measured.sigmaEquivalentKnown = true;
    measured.sigmaEquivalent = 0.02f;
    measured.topologyCertified = true;

    truthraw_v06::PixelCameraRgbCovarianceV06 cov;
    cov.variance[0] = 0.0004f;
    cov.varianceKnownMask = 0x01u;
    cov.topologyCertifiedMask = 0x01u;

    const auto m = bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(measured, cov, 0);
    assert(!m.semanticConflict);
    assert(m.gaussian.known);
    assert(m.gaussian.semantics == UncertaintySemanticsV07::GaussianEquivalentSigma);
    assert(std::abs(m.gaussian.value - 0.02) < 1e-6);
    assert(m.topologyCertified);
    assert(!m.p50.known && !m.p95.known);
    assert(std::isnan(cov.covariance[0]));

    // Reconstructed/backend source: p50/p95 remain empirical quantiles and no
    // Gaussian anchor is manufactured.
    truthraw_v06::MarginalChannelKnowledgeV06 reconstructed;
    reconstructed.source = truthraw_v06::MarginalSourceV06::BackendErrorQuantilesOnly;
    reconstructed.p50AbsKnown = true;
    reconstructed.p50Abs = 0.008743f;
    reconstructed.p95AbsKnown = true;
    reconstructed.p95Abs = 0.036181f;
    reconstructed.topologyCertified = true;

    truthraw_v06::PixelCameraRgbCovarianceV06 unresolvedCov;
    unresolvedCov.topologyCertifiedMask = 0x02u;
    const auto r = bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(
        reconstructed, unresolvedCov, 1);
    assert(!r.semanticConflict);
    assert(!r.gaussian.known);
    assert(r.p50.known && r.p95.known);
    assert(r.p50.semantics == UncertaintySemanticsV07::ErrorQuantileP50);
    assert(r.p95.semantics == UncertaintySemanticsV07::ErrorQuantileP95);
    assert(r.topologyCertified);

    // If a quantile-only source accidentally carries sigma/variance, the bridge
    // must expose a conflict and still refuse Gaussian authority.
    reconstructed.sigmaEquivalentKnown = true;
    reconstructed.sigmaEquivalent = 0.01f;
    unresolvedCov.variance[1] = 0.0001f;
    unresolvedCov.varianceKnownMask = 0x02u;
    const auto badQuantile = bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(
        reconstructed, unresolvedCov, 1);
    assert(badQuantile.semanticConflict);
    assert(!badQuantile.gaussian.known);
    assert(badQuantile.p50.known && badQuantile.p95.known);

    // FutureCertifiedVariance cannot sneak in through a loose sigma field.
    truthraw_v06::MarginalChannelKnowledgeV06 future;
    future.source = truthraw_v06::MarginalSourceV06::FutureCertifiedVariance;
    future.sigmaEquivalentKnown = true;
    future.sigmaEquivalent = 0.03f;
    truthraw_v06::PixelCameraRgbCovarianceV06 noVariance;
    const auto f = bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(future, noVariance, 2);
    assert(f.semanticConflict);
    assert(!f.gaussian.known);

    // Conflicting sigma vs variance also fails closed.
    truthraw_v06::MarginalChannelKnowledgeV06 conflict = measured;
    conflict.sigmaEquivalent = 0.03f;
    const auto c = bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(conflict, cov, 0);
    assert(c.semanticConflict);
    assert(!c.gaussian.known);

    std::cout << "test_camera_rgb_covariance_v0_6_storage_adapter_v0_7 PASS\n";
    return 0;
}

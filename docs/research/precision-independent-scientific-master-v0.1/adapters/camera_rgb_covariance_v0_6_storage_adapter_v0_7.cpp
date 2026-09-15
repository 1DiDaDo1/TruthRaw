#include "camera_rgb_covariance_v0_6_storage_adapter_v0_7.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v07 {
namespace {

bool finite_nonnegative(float x) {
    return std::isfinite(x) && x >= 0.0f;
}

} // namespace

ChannelUncertaintyBindingV07 bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(
    const truthraw_v06::MarginalChannelKnowledgeV06& marginal,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    int channel) {
    ChannelUncertaintyBindingV07 out;
    out.source = marginal.source;
    if (channel < 0 || channel > 2) return out;

    const std::uint8_t bit = static_cast<std::uint8_t>(1u << channel);
    out.topologyCertified = marginal.topologyCertified &&
                            ((covariance.topologyCertifiedMask & bit) != 0u);

    // Quantiles remain quantiles regardless of whether a Gaussian marginal is
    // also available for another reason.
    if (marginal.p50AbsKnown && finite_nonnegative(marginal.p50Abs)) {
        out.p50 = p50_error_anchor_v0_7(static_cast<double>(marginal.p50Abs));
    }
    if (marginal.p95AbsKnown && finite_nonnegative(marginal.p95Abs)) {
        out.p95 = p95_error_anchor_v0_7(static_cast<double>(marginal.p95Abs));
    }

    // BackendErrorQuantilesOnly is a hard semantic boundary: even if a finite
    // sigma field appears accidentally, do not promote it to Gaussian authority.
    if (marginal.source == truthraw_v06::MarginalSourceV06::BackendErrorQuantilesOnly) {
        if (marginal.sigmaEquivalentKnown || ((covariance.varianceKnownMask & bit) != 0u)) {
            out.semanticConflict = true;
        }
        return out;
    }

    const bool varianceKnown = (covariance.varianceKnownMask & bit) != 0u;
    const float variance = covariance.variance[static_cast<std::size_t>(channel)];
    const bool sigmaKnown = marginal.sigmaEquivalentKnown &&
                            finite_nonnegative(marginal.sigmaEquivalent);

    if (varianceKnown) {
        if (!finite_nonnegative(variance)) {
            out.semanticConflict = true;
            return out;
        }
        const double sigmaFromVariance = std::sqrt(static_cast<double>(variance));
        if (sigmaKnown) {
            const double sigma = static_cast<double>(marginal.sigmaEquivalent);
            const double tol = 1e-6 * std::max(1.0, std::max(sigma, sigmaFromVariance));
            if (std::abs(sigma - sigmaFromVariance) > tol) {
                out.semanticConflict = true;
                return out;
            }
        }
        out.gaussian = gaussian_sigma_anchor_v0_7(sigmaFromVariance);
        return out;
    }

    if (sigmaKnown &&
        marginal.source == truthraw_v06::MarginalSourceV06::NoiseProfileGaussianEquivalent) {
        out.gaussian = gaussian_sigma_anchor_v0_7(
            static_cast<double>(marginal.sigmaEquivalent));
    }

    // FutureCertifiedVariance must arrive through the explicit covariance
    // variance-known path above; a loose sigma field is insufficient.
    if (marginal.source == truthraw_v06::MarginalSourceV06::FutureCertifiedVariance &&
        sigmaKnown && !varianceKnown) {
        out.semanticConflict = true;
        out.gaussian = {};
    }

    return out;
}

} // namespace truthraw_precision_v07

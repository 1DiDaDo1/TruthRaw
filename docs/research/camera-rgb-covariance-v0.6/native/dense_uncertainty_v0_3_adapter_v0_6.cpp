#include "dense_uncertainty_v0_3_adapter_v0_6.h"

#include <cmath>
#include <limits>

namespace truthraw_v06 {
namespace {

bool finite_nonnegative(float x) {
    return std::isfinite(x) && x >= 0.0f;
}

StatusV06 map_entry(
    const truthraw::DenseUncertaintyEntryV03& e,
    MarginalChannelKnowledgeV06& out) {

    out = MarginalChannelKnowledgeV06{};

    using S = truthraw::DenseUncertaintySourceV03;
    switch (e.source) {
        case S::Unresolved:
            if (e.valid)
                return StatusV06::error("v0.3 unresolved entry must not be valid");
            return StatusV06::success();

        case S::MeasuredHighCensored:
            if (e.valid)
                return StatusV06::error("v0.3 high-censored entry must not carry a point uncertainty");
            if (std::isfinite(e.sigmaEquivalent))
                return StatusV06::error("high-censored entry must not leak sigmaEquivalent into covariance");
            out.topologyCertified = e.topologyCertified;
            return StatusV06::success();

        case S::MeasuredNoiseProfileGaussianEquivalent:
            if (!e.valid)
                return StatusV06::error("measured Gaussian-equivalent v0.3 entry must be valid");
            if (!e.topologyCertified)
                return StatusV06::error("measured Gaussian-equivalent v0.3 entry must be topology-certified");
            if (!finite_nonnegative(e.sigmaEquivalent))
                return StatusV06::error("measured Gaussian-equivalent v0.3 entry requires finite nonnegative sigmaEquivalent");
            if (!finite_nonnegative(e.p50Abs) || !finite_nonnegative(e.p95Abs) || e.p95Abs < e.p50Abs)
                return StatusV06::error("invalid measured v0.3 error quantiles");
            out.sigmaEquivalentKnown = true;
            out.sigmaEquivalent = e.sigmaEquivalent;
            out.p50AbsKnown = true;
            out.p50Abs = e.p50Abs;
            out.p95AbsKnown = true;
            out.p95Abs = e.p95Abs;
            out.topologyCertified = true;
            out.source = MarginalSourceV06::NoiseProfileGaussianEquivalent;
            return StatusV06::success();

        case S::V5GMeasuredRoleAnchor:
        case S::V5GLocalMaxTransportProxy:
            if (!e.valid)
                return StatusV06::error("numeric v5.0g v0.3 proxy entry must be valid");
            if (!finite_nonnegative(e.p50Abs) || !finite_nonnegative(e.p95Abs) || e.p95Abs < e.p50Abs)
                return StatusV06::error("invalid v5.0g v0.3 error quantiles");
            // Critical boundary: v0.3 explicitly stores NaN sigma for transported
            // reconstructed channels. A finite value here would be an unsupported
            // semantic upgrade from empirical error quantile to variance.
            if (std::isfinite(e.sigmaEquivalent))
                return StatusV06::error("v5.0g quantile-only entry must not be promoted to sigma/variance");
            out.p50AbsKnown = true;
            out.p50Abs = e.p50Abs;
            out.p95AbsKnown = true;
            out.p95Abs = e.p95Abs;
            out.topologyCertified = e.topologyCertified;
            out.source = MarginalSourceV06::BackendErrorQuantilesOnly;
            return StatusV06::success();
    }
    return StatusV06::error("unknown v0.3 dense uncertainty source");
}

} // namespace

StatusV06 adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(
    const truthraw::DenseUncertaintyFieldV03& field,
    std::size_t pixelIndex,
    std::array<MarginalChannelKnowledgeV06, 3>& marginalOut,
    PixelCameraRgbCovarianceV06& covarianceOut) {

    if (field.width <= 0 || field.height <= 0)
        return StatusV06::error("invalid v0.3 field geometry");
    const std::size_t n = static_cast<std::size_t>(field.width) * static_cast<std::size_t>(field.height);
    if (field.rgb.size() != 3u * n)
        return StatusV06::error("v0.3 dense uncertainty RGB size mismatch");
    if (pixelIndex >= n)
        return StatusV06::error("pixel index out of range");

    marginalOut = {};
    for (int c = 0; c < 3; ++c) {
        const auto st = map_entry(field.rgb[3u * pixelIndex + static_cast<std::size_t>(c)],
                                  marginalOut[static_cast<std::size_t>(c)]);
        if (!st) return st;
    }

    auto st = build_pixel_covariance_from_marginals_v0_6(marginalOut, covarianceOut);
    if (!st) return st;

    // The source field itself explicitly says covariance is unresolved. The adapter
    // therefore MUST leave every off-diagonal unknown/NaN.
    if (covarianceOut.covarianceKnownMask != 0u)
        return StatusV06::error("adapter must not invent v0.3 off-diagonal covariance");
    for (float x : covarianceOut.covariance)
        if (!std::isnan(x))
            return StatusV06::error("adapter must preserve unknown off-diagonals as NaN");

    return StatusV06::success();
}

} // namespace truthraw_v06

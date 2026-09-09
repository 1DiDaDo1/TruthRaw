#include "camera_rgb_covariance_v0_6.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace truthraw_v06 {
namespace {

constexpr float kTol = 1.0e-5f;

int pair_a(RgbPairV06 p) {
    switch (p) {
        case RgbPairV06::RG: return 0;
        case RgbPairV06::RB: return 0;
        case RgbPairV06::GB: return 1;
    }
    return 0;
}

int pair_b(RgbPairV06 p) {
    switch (p) {
        case RgbPairV06::RG: return 1;
        case RgbPairV06::RB: return 2;
        case RgbPairV06::GB: return 2;
    }
    return 1;
}

int pair_i(RgbPairV06 p) { return static_cast<int>(p); }

bool known(std::uint8_t mask, int bit) {
    return (mask & static_cast<std::uint8_t>(1u << bit)) != 0;
}

bool finite_nonnegative(float x) {
    return std::isfinite(x) && x >= 0.0f;
}

bool full_psd(const PixelCameraRgbCovarianceV06& v) {
    if (v.varianceKnownMask != 0x7u || v.covarianceKnownMask != 0x7u) return false;
    const double a = v.variance[0];
    const double d = v.variance[1];
    const double f = v.variance[2];
    const double b = v.covariance[0]; // RG
    const double c = v.covariance[1]; // RB
    const double e = v.covariance[2]; // GB
    const double scale = std::max({1.0, std::abs(a), std::abs(d), std::abs(f)});
    const double tol2 = 1.0e-6 * std::max(1.0, scale * scale);
    const double tol3 = 1.0e-6 * std::max(1.0, scale * scale * scale);
    if (a < 0.0 || d < 0.0 || f < 0.0) return false;
    if (a*d - b*b < -tol2) return false;
    if (a*f - c*c < -tol2) return false;
    if (d*f - e*e < -tol2) return false;
    const double det = a*d*f + 2.0*b*c*e - a*e*e - d*c*c - f*b*b;
    return det >= -tol3;
}

} // namespace

const char* covariance_knowledge_name_v0_6(CovarianceKnowledgeV06 s) {
    switch (s) {
        case CovarianceKnowledgeV06::NoVarianceKnown: return "NO_VARIANCE_KNOWN";
        case CovarianceKnowledgeV06::PartialDiagonalOnly: return "PARTIAL_DIAGONAL_ONLY";
        case CovarianceKnowledgeV06::FullDiagonalOffDiagonalUnknown: return "FULL_DIAGONAL_OFF_DIAGONAL_UNKNOWN";
        case CovarianceKnowledgeV06::PartialOffDiagonalCertified: return "PARTIAL_OFF_DIAGONAL_CERTIFIED";
        case CovarianceKnowledgeV06::FullCovarianceCertifiedPsd: return "FULL_COVARIANCE_CERTIFIED_PSD";
    }
    return "UNKNOWN";
}

StatusV06 build_pixel_covariance_from_marginals_v0_6(
    const std::array<MarginalChannelKnowledgeV06, 3>& marginal,
    PixelCameraRgbCovarianceV06& out) {

    out = PixelCameraRgbCovarianceV06{};
    for (int c = 0; c < 3; ++c) {
        const auto& m = marginal[static_cast<std::size_t>(c)];
        if (m.sigmaEquivalentKnown) {
            if (!finite_nonnegative(m.sigmaEquivalent))
                return StatusV06::error("known sigmaEquivalent must be finite and nonnegative");
            const float var = m.sigmaEquivalent * m.sigmaEquivalent;
            if (!finite_nonnegative(var))
                return StatusV06::error("variance overflow/invalid");
            out.variance[static_cast<std::size_t>(c)] = var;
            out.varianceKnownMask |= static_cast<std::uint8_t>(1u << c);
        }
        if (m.p50AbsKnown && (!finite_nonnegative(m.p50Abs)))
            return StatusV06::error("known p50Abs must be finite and nonnegative");
        if (m.p95AbsKnown && (!finite_nonnegative(m.p95Abs)))
            return StatusV06::error("known p95Abs must be finite and nonnegative");
        if (m.p50AbsKnown && m.p95AbsKnown && m.p95Abs < m.p50Abs)
            return StatusV06::error("p95Abs must be >= p50Abs");
        if (m.topologyCertified)
            out.topologyCertifiedMask |= static_cast<std::uint8_t>(1u << c);
        // Deliberately no p50/p95 -> sigma conversion. Quantile-only backends do not define variance.
    }
    return validate_pixel_covariance_v0_6(out);
}

StatusV06 validate_pixel_covariance_v0_6(const PixelCameraRgbCovarianceV06& v) {
    if ((v.varianceKnownMask & ~0x7u) != 0 || (v.covarianceKnownMask & ~0x7u) != 0)
        return StatusV06::error("invalid knowledge mask bits");

    for (int c = 0; c < 3; ++c) {
        const float x = v.variance[static_cast<std::size_t>(c)];
        if (known(v.varianceKnownMask, c)) {
            if (!finite_nonnegative(x)) return StatusV06::error("known variance invalid");
        } else if (!std::isnan(x)) {
            return StatusV06::error("unknown variance must be NaN, never a numeric assumption");
        }
    }

    for (int p = 0; p < 3; ++p) {
        const auto pair = static_cast<RgbPairV06>(p);
        const float x = v.covariance[static_cast<std::size_t>(p)];
        if (known(v.covarianceKnownMask, p)) {
            const int a = pair_a(pair), b = pair_b(pair);
            if (!known(v.varianceKnownMask, a) || !known(v.varianceKnownMask, b))
                return StatusV06::error("known covariance requires both marginal variances");
            if (!std::isfinite(x)) return StatusV06::error("known covariance must be finite");
            const float bound = std::sqrt(v.variance[static_cast<std::size_t>(a)] *
                                          v.variance[static_cast<std::size_t>(b)]);
            if (std::abs(x) > bound + kTol * std::max(1.0f, bound))
                return StatusV06::error("covariance violates Cauchy-Schwarz bound");
        } else if (!std::isnan(x)) {
            return StatusV06::error("unknown covariance must be NaN, never zero-by-default");
        }
    }

    if (v.covarianceKnownMask == 0x7u && v.varianceKnownMask == 0x7u) {
        if (!full_psd(v)) return StatusV06::error("full covariance matrix is not positive semidefinite");
        if (!v.fullPsdCertified) return StatusV06::error("full numeric covariance requires explicit PSD certification flag");
    } else if (v.fullPsdCertified) {
        return StatusV06::error("PSD certification flag requires all six matrix terms known");
    }

    return StatusV06::success();
}

StatusV06 set_certified_covariance_v0_6(
    PixelCameraRgbCovarianceV06& io,
    RgbPairV06 pair,
    float covariance) {

    const int p = pair_i(pair);
    const int a = pair_a(pair), b = pair_b(pair);
    if (!known(io.varianceKnownMask, a) || !known(io.varianceKnownMask, b))
        return StatusV06::error("cannot certify covariance without both variances");
    if (!std::isfinite(covariance))
        return StatusV06::error("certified covariance must be finite");
    const float bound = std::sqrt(io.variance[static_cast<std::size_t>(a)] *
                                  io.variance[static_cast<std::size_t>(b)]);
    if (std::abs(covariance) > bound + kTol * std::max(1.0f, bound))
        return StatusV06::error("certified covariance violates Cauchy-Schwarz bound");

    const auto oldCov = io.covariance;
    const auto oldMask = io.covarianceKnownMask;
    const bool oldPsd = io.fullPsdCertified;

    io.covariance[static_cast<std::size_t>(p)] = covariance;
    io.covarianceKnownMask |= static_cast<std::uint8_t>(1u << p);
    io.fullPsdCertified = false;

    if (io.covarianceKnownMask == 0x7u && io.varianceKnownMask == 0x7u) {
        if (!full_psd(io)) {
            io.covariance = oldCov;
            io.covarianceKnownMask = oldMask;
            io.fullPsdCertified = oldPsd;
            return StatusV06::error("adding covariance would make matrix non-PSD");
        }
        io.fullPsdCertified = true;
    }

    return validate_pixel_covariance_v0_6(io);
}

CovarianceKnowledgeV06 covariance_knowledge_v0_6(const PixelCameraRgbCovarianceV06& v) {
    const auto count3 = [](std::uint8_t m) {
        int n = 0;
        for (int i = 0; i < 3; ++i) if ((m & static_cast<std::uint8_t>(1u << i)) != 0) ++n;
        return n;
    };
    const int diagCount = count3(static_cast<std::uint8_t>(v.varianceKnownMask & 0x7u));
    const int covCount = count3(static_cast<std::uint8_t>(v.covarianceKnownMask & 0x7u));
    if (diagCount == 0) return CovarianceKnowledgeV06::NoVarianceKnown;
    if (covCount == 0 && diagCount < 3) return CovarianceKnowledgeV06::PartialDiagonalOnly;
    if (covCount == 0 && diagCount == 3) return CovarianceKnowledgeV06::FullDiagonalOffDiagonalUnknown;
    if (diagCount == 3 && covCount == 3 && v.fullPsdCertified)
        return CovarianceKnowledgeV06::FullCovarianceCertifiedPsd;
    return CovarianceKnowledgeV06::PartialOffDiagonalCertified;
}

bool can_exactly_propagate_linear_covariance_v0_6(const PixelCameraRgbCovarianceV06& v) {
    return covariance_knowledge_v0_6(v) == CovarianceKnowledgeV06::FullCovarianceCertifiedPsd;
}

CovarianceOuterBoundV06 cauchy_outer_bound_v0_6(
    const PixelCameraRgbCovarianceV06& v,
    RgbPairV06 pair) {
    CovarianceOuterBoundV06 out;
    const int p = pair_i(pair);
    const int a = pair_a(pair), b = pair_b(pair);
    if (!known(v.varianceKnownMask, a) || !known(v.varianceKnownMask, b)) return out;
    if (known(v.covarianceKnownMask, p)) {
        out.valid = true;
        out.lower = out.upper = v.covariance[static_cast<std::size_t>(p)];
        out.exact = true;
        return out;
    }
    const float bound = std::sqrt(v.variance[static_cast<std::size_t>(a)] *
                                  v.variance[static_cast<std::size_t>(b)]);
    out.valid = true;
    out.lower = -bound;
    out.upper = bound;
    out.exact = false;
    return out;
}

StatusV06 scale_pixel_covariance_v0_6(PixelCameraRgbCovarianceV06& io, float scalar) {
    if (!std::isfinite(scalar)) return StatusV06::error("scale must be finite");
    const float s2 = scalar * scalar;
    if (!std::isfinite(s2)) return StatusV06::error("scale squared overflow");
    for (int c = 0; c < 3; ++c)
        if (known(io.varianceKnownMask, c)) io.variance[static_cast<std::size_t>(c)] *= s2;
    for (int p = 0; p < 3; ++p)
        if (known(io.covarianceKnownMask, p)) io.covariance[static_cast<std::size_t>(p)] *= s2;
    return validate_pixel_covariance_v0_6(io);
}

StatusV06 init_covariance_tile_v0_6(
    int originX, int originY, int width, int height, CameraRgbCovarianceTileV06& out) {
    if (originX < 0 || originY < 0 || width <= 0 || height <= 0)
        return StatusV06::error("invalid tile geometry");
    const std::size_t n = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (n > (static_cast<std::size_t>(1) << 28))
        return StatusV06::error("tile too large");
    out = CameraRgbCovarianceTileV06{};
    out.originX = originX;
    out.originY = originY;
    out.width = width;
    out.height = height;
    out.pixels.resize(n);
    out.covarianceStatus = "PARTIAL_KNOWLEDGE_ALLOWED_UNKNOWN_IS_NAN";
    out.claimBoundary =
        "Camera-RGB covariance representation only. Unknown off-diagonals remain NaN, not zero. "
        "Quantile-only reconstructed-channel uncertainty is not converted into variance. Exact linear covariance propagation is allowed only for a fully known PSD-certified 3x3 matrix.";
    return StatusV06::success();
}

} // namespace truthraw_v06

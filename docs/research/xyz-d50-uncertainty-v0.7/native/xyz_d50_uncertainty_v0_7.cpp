#include "xyz_d50_uncertainty_v0_7.h"

#include <algorithm>
#include <cmath>

namespace truthraw_v07 {
namespace {

constexpr double kTol = 1.0e-10;

bool known(std::uint8_t mask, int bit) {
    return (mask & static_cast<std::uint8_t>(1u << bit)) != 0;
}

int pair_index(int a, int b) {
    if (a > b) std::swap(a, b);
    if (a == 0 && b == 1) return 0; // RG / XY
    if (a == 0 && b == 2) return 1; // RB / XZ
    if (a == 1 && b == 2) return 2; // GB / YZ
    return -1;
}

double m_at(const CameraToXyzD50MatrixV07& m, int r, int c) {
    return m.m[static_cast<std::size_t>(3 * r + c)];
}

bool finite_matrix(const CameraToXyzD50MatrixV07& matrix) {
    for (double v : matrix.m) if (!std::isfinite(v)) return false;
    return true;
}

bool psd3(const double s[3][3]) {
    const double scale = std::max({
        1.0, std::abs(s[0][0]), std::abs(s[1][1]), std::abs(s[2][2])
    });
    const double tol2 = 1.0e-9 * std::max(1.0, scale * scale);
    const double tol3 = 1.0e-9 * std::max(1.0, scale * scale * scale);
    if (s[0][0] < -kTol || s[1][1] < -kTol || s[2][2] < -kTol) return false;
    if (s[0][0]*s[1][1] - s[0][1]*s[0][1] < -tol2) return false;
    if (s[0][0]*s[2][2] - s[0][2]*s[0][2] < -tol2) return false;
    if (s[1][1]*s[2][2] - s[1][2]*s[1][2] < -tol2) return false;
    const double det =
        s[0][0]*s[1][1]*s[2][2] +
        2.0*s[0][1]*s[0][2]*s[1][2] -
        s[0][0]*s[1][2]*s[1][2] -
        s[1][1]*s[0][2]*s[0][2] -
        s[2][2]*s[0][1]*s[0][1];
    return det >= -tol3;
}

double clean_variance(double v) {
    if (v < 0.0 && v > -kTol) return 0.0;
    return v;
}

StatusV07 build_component_bound(
    const truthraw_v06::PixelCameraRgbCovarianceV06& in,
    const CameraToXyzD50MatrixV07& matrix,
    int row,
    VarianceOuterBoundV07& out,
    double& exactVariance,
    bool& exactKnown) {

    out = VarianceOuterBoundV07{};
    exactVariance = std::numeric_limits<double>::quiet_NaN();
    exactKnown = false;

    double center = 0.0;
    double radius = 0.0;

    for (int i = 0; i < 3; ++i) {
        const double a = m_at(matrix, row, i);
        if (a == 0.0) continue;
        if (!known(in.varianceKnownMask, i))
            return StatusV07::success(); // finite bound unresolved for this output component
        const double var = static_cast<double>(in.variance[static_cast<std::size_t>(i)]);
        if (!std::isfinite(var) || var < 0.0)
            return StatusV07::error("known input variance invalid during bound propagation");
        center += a * a * var;
    }

    for (int i = 0; i < 3; ++i) {
        const double ai = m_at(matrix, row, i);
        if (ai == 0.0) continue;
        for (int j = i + 1; j < 3; ++j) {
            const double aj = m_at(matrix, row, j);
            if (aj == 0.0) continue;
            if (!known(in.varianceKnownMask, i) || !known(in.varianceKnownMask, j))
                return StatusV07::success();
            const int p = pair_index(i, j);
            if (p < 0) return StatusV07::error("internal pair mapping failure");
            if (known(in.covarianceKnownMask, p)) {
                const double cov = static_cast<double>(in.covariance[static_cast<std::size_t>(p)]);
                if (!std::isfinite(cov))
                    return StatusV07::error("known input covariance invalid during bound propagation");
                center += 2.0 * ai * aj * cov;
            } else {
                const double vi = static_cast<double>(in.variance[static_cast<std::size_t>(i)]);
                const double vj = static_cast<double>(in.variance[static_cast<std::size_t>(j)]);
                // Cauchy-Schwarz: |Cov_ij| <= sqrt(Var_i Var_j).
                // Unknown pair terms are interval-expanded independently. This is a safe outer
                // bound; it is not claimed to be the tight PSD-coupled feasible interval.
                radius += 2.0 * std::abs(ai * aj) * std::sqrt(vi * vj);
            }
        }
    }

    if (!std::isfinite(center) || !std::isfinite(radius))
        return StatusV07::error("non-finite XYZ variance bound");

    out.valid = true;
    out.lower = std::max(0.0, center - radius);
    out.upper = std::max(0.0, center + radius);
    if (out.upper + kTol < out.lower)
        return StatusV07::error("invalid XYZ variance bound ordering");
    out.exact = radius == 0.0;
    if (out.exact) {
        const double v = clean_variance(center);
        if (!std::isfinite(v) || v < 0.0)
            return StatusV07::error("exact component variance is negative/invalid");
        out.lower = out.upper = v;
        exactVariance = v;
        exactKnown = true;
    }
    return StatusV07::success();
}

} // namespace

const char* matrix_authority_name_v0_7(MatrixAuthorityV07 authority) {
    switch (authority) {
        case MatrixAuthorityV07::Unresolved: return "UNRESOLVED";
        case MatrixAuthorityV07::SourceBoundMetadataDerived: return "SOURCE_BOUND_METADATA_DERIVED";
        case MatrixAuthorityV07::IndependentTargetCalibration: return "INDEPENDENT_TARGET_CALIBRATION";
        case MatrixAuthorityV07::ExplicitResearchFixture: return "EXPLICIT_RESEARCH_FIXTURE";
    }
    return "UNKNOWN";
}

const char* xyz_propagation_knowledge_name_v0_7(
    XyzPropagationKnowledgeV07 knowledge) {
    switch (knowledge) {
        case XyzPropagationKnowledgeV07::Unresolved: return "UNRESOLVED";
        case XyzPropagationKnowledgeV07::ComponentVarianceBounds: return "COMPONENT_VARIANCE_BOUNDS";
        case XyzPropagationKnowledgeV07::ExactFullCovariance: return "EXACT_FULL_COVARIANCE";
    }
    return "UNKNOWN";
}

StatusV07 validate_camera_to_xyz_d50_matrix_v0_7(
    const CameraToXyzD50MatrixV07& matrix) {

    if (!finite_matrix(matrix))
        return StatusV07::error("camera->XYZ(D50) matrix must be finite");
    if (matrix.authority == MatrixAuthorityV07::Unresolved)
        return StatusV07::error("matrix authority unresolved");
    if (matrix.sourceId.empty())
        return StatusV07::error("matrix sourceId is required");
    if (!matrix.declaredLinearSceneTransform)
        return StatusV07::error("matrix must be explicitly declared scene-linear");
    if (!matrix.declaredOutputXyzD50)
        return StatusV07::error("matrix output must be explicitly declared XYZ(D50)");
    return StatusV07::success();
}

StatusV07 propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
    const truthraw_v06::PixelCameraRgbCovarianceV06& cameraRgb,
    const CameraToXyzD50MatrixV07& matrix,
    PixelXyzD50UncertaintyV07& out) {

    out = PixelXyzD50UncertaintyV07{};

    const auto ms = validate_camera_to_xyz_d50_matrix_v0_7(matrix);
    if (!ms) return ms;

    const auto vs = truthraw_v06::validate_pixel_covariance_v0_6(cameraRgb);
    if (!vs) return StatusV07::error("invalid v0.6 camera-RGB covariance: " + vs.message);

    out.matrixSourceId = matrix.sourceId;

    if (truthraw_v06::can_exactly_propagate_linear_covariance_v0_6(cameraRgb)) {
        double sin[3][3] {};
        for (int i = 0; i < 3; ++i)
            sin[i][i] = static_cast<double>(cameraRgb.variance[static_cast<std::size_t>(i)]);
        sin[0][1] = sin[1][0] = static_cast<double>(cameraRgb.covariance[0]);
        sin[0][2] = sin[2][0] = static_cast<double>(cameraRgb.covariance[1]);
        sin[1][2] = sin[2][1] = static_cast<double>(cameraRgb.covariance[2]);

        double sout[3][3] {};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                double v = 0.0;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        v += m_at(matrix, r, i) * sin[i][j] * m_at(matrix, c, j);
                if (!std::isfinite(v))
                    return StatusV07::error("non-finite exact XYZ covariance");
                sout[r][c] = v;
            }
        }

        // Symmetrize only round-off disagreement from the two equivalent evaluations.
        for (int i = 0; i < 3; ++i) {
            sout[i][i] = clean_variance(sout[i][i]);
            if (sout[i][i] < 0.0)
                return StatusV07::error("exact XYZ marginal variance became negative");
            for (int j = i + 1; j < 3; ++j) {
                const double avg = 0.5 * (sout[i][j] + sout[j][i]);
                sout[i][j] = sout[j][i] = avg;
            }
        }
        if (!psd3(sout))
            return StatusV07::error("exact propagated XYZ covariance failed PSD check");

        out.variance = {sout[0][0], sout[1][1], sout[2][2]};
        out.varianceKnownMask = 0x7u;
        out.covariance = {sout[0][1], sout[0][2], sout[1][2]};
        out.covarianceKnownMask = 0x7u;
        out.fullPsdCertified = true;
        out.knowledge = XyzPropagationKnowledgeV07::ExactFullCovariance;
        for (int i = 0; i < 3; ++i) {
            out.varianceBounds[static_cast<std::size_t>(i)].valid = true;
            out.varianceBounds[static_cast<std::size_t>(i)].lower = out.variance[static_cast<std::size_t>(i)];
            out.varianceBounds[static_cast<std::size_t>(i)].upper = out.variance[static_cast<std::size_t>(i)];
            out.varianceBounds[static_cast<std::size_t>(i)].exact = true;
        }
        out.claimBoundary =
            "Exact XYZ(D50) covariance is a deterministic linear propagation of a fully known PSD-certified "
            "camera-RGB covariance through the caller-supplied matrix. Matrix physical/colorimetric validity "
            "remains upstream provenance and is not certified by v0.7.";
        return StatusV07::success();
    }

    bool anyBound = false;
    for (int r = 0; r < 3; ++r) {
        double exactVariance = std::numeric_limits<double>::quiet_NaN();
        bool exactKnown = false;
        auto s = build_component_bound(
            cameraRgb, matrix, r,
            out.varianceBounds[static_cast<std::size_t>(r)],
            exactVariance, exactKnown);
        if (!s) return s;
        if (out.varianceBounds[static_cast<std::size_t>(r)].valid) anyBound = true;
        if (exactKnown) {
            out.variance[static_cast<std::size_t>(r)] = exactVariance;
            out.varianceKnownMask |= static_cast<std::uint8_t>(1u << r);
        }
    }

    out.knowledge = anyBound
        ? XyzPropagationKnowledgeV07::ComponentVarianceBounds
        : XyzPropagationKnowledgeV07::Unresolved;
    out.fullPsdCertified = false;
    // Output off-diagonals intentionally remain NaN/unknown in non-full mode.
    out.covarianceKnownMask = 0u;
    out.claimBoundary =
        "Camera-RGB off-diagonals are not assumed zero. XYZ component variance intervals use known terms plus "
        "independent Cauchy-Schwarz expansion of unresolved input pair terms, so they are conservative outer bounds, "
        "not a reconstructed XYZ covariance matrix. p50/p95 error bands are not converted to variance.";
    return StatusV07::success();
}

StatusV07 propagate_camera_rgb_tile_to_xyz_d50_v0_7(
    const truthraw_v06::CameraRgbCovarianceTileV06& cameraRgb,
    const CameraToXyzD50MatrixV07& matrix,
    XyzD50UncertaintyTileV07& out) {

    if (cameraRgb.originX < 0 || cameraRgb.originY < 0 ||
        cameraRgb.width <= 0 || cameraRgb.height <= 0)
        return StatusV07::error("invalid input tile geometry");
    const std::size_t expected =
        static_cast<std::size_t>(cameraRgb.width) * static_cast<std::size_t>(cameraRgb.height);
    if (cameraRgb.pixels.size() != expected)
        return StatusV07::error("input tile pixel count mismatch");
    if (expected > (static_cast<std::size_t>(1) << 28))
        return StatusV07::error("tile too large");

    const auto ms = validate_camera_to_xyz_d50_matrix_v0_7(matrix);
    if (!ms) return ms;

    out = XyzD50UncertaintyTileV07{};
    out.originX = cameraRgb.originX;
    out.originY = cameraRgb.originY;
    out.width = cameraRgb.width;
    out.height = cameraRgb.height;
    out.matrixSourceId = matrix.sourceId;
    out.claimBoundary =
        "Dense XYZ(D50) uncertainty propagation; per-pixel knowledge state is preserved and unresolved "
        "camera-RGB correlation is never silently replaced by independence.";
    out.pixels.resize(expected);

    for (std::size_t i = 0; i < expected; ++i) {
        auto s = propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
            cameraRgb.pixels[i], matrix, out.pixels[i]);
        if (!s)
            return StatusV07::error("tile pixel " + std::to_string(i) + ": " + s.message);
    }
    return StatusV07::success();
}

} // namespace truthraw_v07

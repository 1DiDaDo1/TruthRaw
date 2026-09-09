#include "xyz_d50_uncertainty_v0_7.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using truthraw_v06::PixelCameraRgbCovarianceV06;
using truthraw_v07::CameraToXyzD50MatrixV07;
using truthraw_v07::MatrixAuthorityV07;
using truthraw_v07::PixelXyzD50UncertaintyV07;
using truthraw_v07::XyzPropagationKnowledgeV07;

namespace {

bool near(double a, double b, double eps = 1e-9) {
    return std::abs(a - b) <= eps * std::max({1.0, std::abs(a), std::abs(b)});
}

CameraToXyzD50MatrixV07 matrix(std::array<double, 9> m) {
    CameraToXyzD50MatrixV07 out;
    out.m = m;
    out.authority = MatrixAuthorityV07::ExplicitResearchFixture;
    out.sourceId = "synthetic-test-fixture";
    out.declaredLinearSceneTransform = true;
    out.declaredOutputXyzD50 = true;
    return out;
}

PixelCameraRgbCovarianceV06 full_input() {
    PixelCameraRgbCovarianceV06 p;
    p.variance = {4.0f, 9.0f, 16.0f};
    p.varianceKnownMask = 0x7u;
    p.covariance = {1.0f, -1.0f, 2.0f}; // RG, RB, GB
    p.covarianceKnownMask = 0x7u;
    p.fullPsdCertified = true;
    return p;
}

} // namespace

int main() {
    // Matrix contract is explicit and fail-closed.
    {
        CameraToXyzD50MatrixV07 bad;
        bad.m = {1,0,0,0,1,0,0,0,1};
        PixelXyzD50UncertaintyV07 out;
        assert(!truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
            full_input(), bad, out));

        auto bad2 = matrix({1,0,0,0,1,0,0,0,1});
        bad2.m[2] = std::numeric_limits<double>::quiet_NaN();
        assert(!truthraw_v07::validate_camera_to_xyz_d50_matrix_v0_7(bad2));
    }

    // Identity transform preserves a fully certified covariance exactly.
    {
        auto m = matrix({1,0,0,0,1,0,0,0,1});
        PixelXyzD50UncertaintyV07 out;
        auto s = truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
            full_input(), m, out);
        assert(s);
        assert(out.knowledge == XyzPropagationKnowledgeV07::ExactFullCovariance);
        assert(out.varianceKnownMask == 0x7u);
        assert(out.covarianceKnownMask == 0x7u);
        assert(out.fullPsdCertified);
        assert(near(out.variance[0], 4.0));
        assert(near(out.variance[1], 9.0));
        assert(near(out.variance[2], 16.0));
        assert(near(out.covariance[0], 1.0));
        assert(near(out.covariance[1], -1.0));
        assert(near(out.covariance[2], 2.0));
    }

    // Nontrivial transform: check independent hand-derived entries of M Sigma M^T.
    {
        auto m = matrix({
            1, 0, 0,
            0, 2, 0,
            1,-1,0.5
        });
        PixelXyzD50UncertaintyV07 out;
        auto s = truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(
            full_input(), m, out);
        assert(s && out.fullPsdCertified);
        assert(near(out.variance[0], 4.0));
        assert(near(out.variance[1], 36.0));
        assert(near(out.variance[2], 12.0));
        assert(near(out.covariance[0], 2.0));   // Cov(R,2G)
        assert(near(out.covariance[1], 2.5));   // Cov(R,R-G+0.5B)
        assert(near(out.covariance[2], -14.0)); // Cov(2G,R-G+0.5B)
    }

    // Unknown off-diagonals are NOT treated as zero. For R-G, independence would
    // give variance 13; the valid outer interval is [1,25].
    {
        PixelCameraRgbCovarianceV06 p;
        p.variance = {4.0f, 9.0f, 16.0f};
        p.varianceKnownMask = 0x7u;
        auto m = matrix({
            1,-1,0,
            0, 1,0,
            0, 0,1
        });
        PixelXyzD50UncertaintyV07 out;
        auto s = truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(p, m, out);
        assert(s);
        assert(out.knowledge == XyzPropagationKnowledgeV07::ComponentVarianceBounds);
        assert(out.covarianceKnownMask == 0u);
        assert(std::isnan(out.covariance[0]));
        assert(out.varianceBounds[0].valid && !out.varianceBounds[0].exact);
        assert(near(out.varianceBounds[0].lower, 1.0));
        assert(near(out.varianceBounds[0].upper, 25.0));
        assert(std::isnan(out.variance[0]));
    }

    // A certified RG covariance can make a component variance exact even though
    // unrelated RGB pair terms remain unresolved.
    {
        PixelCameraRgbCovarianceV06 p;
        p.variance = {4.0f, 9.0f, 16.0f};
        p.varianceKnownMask = 0x7u;
        p.covariance[0] = 1.0f;
        p.covarianceKnownMask = 0x1u;
        auto m = matrix({
            1,-1,0,
            0, 1,0,
            0, 0,1
        });
        PixelXyzD50UncertaintyV07 out;
        auto s = truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(p, m, out);
        assert(s);
        assert((out.varianceKnownMask & 0x1u) != 0u);
        assert(out.varianceBounds[0].exact);
        assert(near(out.variance[0], 11.0));
        assert(out.covarianceKnownMask == 0u);
        assert(!out.fullPsdCertified);
    }

    // Quantile-only / no-variance state remains unresolved; no sigma is invented.
    {
        PixelCameraRgbCovarianceV06 p;
        auto m = matrix({1,0,0,0,1,0,0,0,1});
        PixelXyzD50UncertaintyV07 out;
        auto s = truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(p, m, out);
        assert(s);
        assert(out.knowledge == XyzPropagationKnowledgeV07::Unresolved);
        assert(out.varianceKnownMask == 0u);
        assert(out.covarianceKnownMask == 0u);
        for (const auto& b : out.varianceBounds) assert(!b.valid);
    }

    // A non-PSD fully numeric v0.6 input must fail before propagation.
    {
        PixelCameraRgbCovarianceV06 p;
        p.variance = {1.0f,1.0f,1.0f};
        p.varianceKnownMask = 0x7u;
        p.covariance = {0.9f,0.9f,-0.9f};
        p.covarianceKnownMask = 0x7u;
        p.fullPsdCertified = true;
        auto m = matrix({1,0,0,0,1,0,0,0,1});
        PixelXyzD50UncertaintyV07 out;
        assert(!truthraw_v07::propagate_camera_rgb_uncertainty_to_xyz_d50_v0_7(p, m, out));
    }

    // Dense tile preserves geometry and per-pixel knowledge state.
    {
        truthraw_v06::CameraRgbCovarianceTileV06 tile;
        tile.originX = 3; tile.originY = 7; tile.width = 2; tile.height = 1;
        tile.pixels = {full_input(), PixelCameraRgbCovarianceV06{}};
        truthraw_v07::XyzD50UncertaintyTileV07 out;
        auto m = matrix({1,0,0,0,1,0,0,0,1});
        auto s = truthraw_v07::propagate_camera_rgb_tile_to_xyz_d50_v0_7(tile, m, out);
        assert(s);
        assert(out.originX == 3 && out.originY == 7 && out.width == 2 && out.height == 1);
        assert(out.pixels.size() == 2);
        assert(out.pixels[0].knowledge == XyzPropagationKnowledgeV07::ExactFullCovariance);
        assert(out.pixels[1].knowledge == XyzPropagationKnowledgeV07::Unresolved);
    }

    std::cout << "TRUTHRAW_XYZ_D50_UNCERTAINTY_V0_7_TEST_PASS\n";
    return 0;
}

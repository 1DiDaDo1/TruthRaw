#include "camera_rgb_covariance_v0_6.h"

#include <cmath>
#include <iostream>
#include <random>

using namespace truthraw_v06;

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (0)

static bool near(float a, float b, float eps=1e-5f) { return std::abs(a-b) <= eps; }

int main() {
    {
        PixelCameraRgbCovarianceV06 p;
        CHECK(covariance_knowledge_v0_6(p) == CovarianceKnowledgeV06::NoVarianceKnown);
        CHECK(std::isnan(p.variance[0]) && std::isnan(p.covariance[0]));
        CHECK(validate_pixel_covariance_v0_6(p));
    }

    {
        std::array<MarginalChannelKnowledgeV06,3> m{};
        m[0].sigmaEquivalentKnown = true; m[0].sigmaEquivalent = 2.f;
        m[0].p50AbsKnown = true; m[0].p50Abs = 1.35f;
        m[0].p95AbsKnown = true; m[0].p95Abs = 3.92f;
        m[0].topologyCertified = true;
        m[0].source = MarginalSourceV06::NoiseProfileGaussianEquivalent;
        m[1].p50AbsKnown = true; m[1].p50Abs = 3.f;
        m[1].p95AbsKnown = true; m[1].p95Abs = 9.f;
        m[1].source = MarginalSourceV06::BackendErrorQuantilesOnly;
        PixelCameraRgbCovarianceV06 p;
        CHECK(build_pixel_covariance_from_marginals_v0_6(m,p));
        CHECK(p.varianceKnownMask == 0x1u);
        CHECK(near(p.variance[0],4.f));
        CHECK(std::isnan(p.variance[1])); // quantile-only MUST NOT become variance
        CHECK(std::isnan(p.covariance[0])); // independence MUST NOT be assumed
        CHECK(covariance_knowledge_v0_6(p) == CovarianceKnowledgeV06::PartialDiagonalOnly);
    }

    PixelCameraRgbCovarianceV06 fullDiag;
    {
        std::array<MarginalChannelKnowledgeV06,3> m{};
        m[0].sigmaEquivalentKnown=true; m[0].sigmaEquivalent=1.f;
        m[1].sigmaEquivalentKnown=true; m[1].sigmaEquivalent=2.f;
        m[2].sigmaEquivalentKnown=true; m[2].sigmaEquivalent=3.f;
        CHECK(build_pixel_covariance_from_marginals_v0_6(m,fullDiag));
        CHECK(fullDiag.varianceKnownMask == 0x7u);
        CHECK(covariance_knowledge_v0_6(fullDiag) == CovarianceKnowledgeV06::FullDiagonalOffDiagonalUnknown);
        CHECK(!can_exactly_propagate_linear_covariance_v0_6(fullDiag));
        const auto b = cauchy_outer_bound_v0_6(fullDiag,RgbPairV06::RB);
        CHECK(b.valid && !b.exact && near(b.lower,-3.f) && near(b.upper,3.f));
    }

    {
        auto p=fullDiag;
        CHECK(!set_certified_covariance_v0_6(p,RgbPairV06::RG,3.f)); // bound is sqrt(1*4)=2
        CHECK(p.covarianceKnownMask==0u && std::isnan(p.covariance[0]));
    }

    {
        // Pairwise-valid correlations can still make a 3x3 matrix non-PSD.
        std::array<MarginalChannelKnowledgeV06,3> m{};
        for(auto& x:m){x.sigmaEquivalentKnown=true;x.sigmaEquivalent=1.f;}
        PixelCameraRgbCovarianceV06 p;
        CHECK(build_pixel_covariance_from_marginals_v0_6(m,p));
        CHECK(set_certified_covariance_v0_6(p,RgbPairV06::RG,0.9f));
        CHECK(set_certified_covariance_v0_6(p,RgbPairV06::RB,0.9f));
        CHECK(!set_certified_covariance_v0_6(p,RgbPairV06::GB,-0.9f));
        CHECK(p.covarianceKnownMask==0x3u); // rejected third term rolled back
        CHECK(!p.fullPsdCertified);
    }

    {
        auto p=fullDiag;
        CHECK(set_certified_covariance_v0_6(p,RgbPairV06::RG,0.5f));
        CHECK(set_certified_covariance_v0_6(p,RgbPairV06::RB,0.2f));
        CHECK(set_certified_covariance_v0_6(p,RgbPairV06::GB,1.0f));
        CHECK(p.fullPsdCertified);
        CHECK(covariance_knowledge_v0_6(p)==CovarianceKnowledgeV06::FullCovarianceCertifiedPsd);
        CHECK(can_exactly_propagate_linear_covariance_v0_6(p));
        const float rg=p.covariance[0];
        CHECK(scale_pixel_covariance_v0_6(p,2.f));
        CHECK(near(p.variance[0],4.f));
        CHECK(near(p.variance[1],16.f));
        CHECK(near(p.variance[2],36.f));
        CHECK(near(p.covariance[0],4.f*rg));
        CHECK(p.fullPsdCertified);
    }


    {
        // Deterministic stress: every A*A^T matrix is PSD by construction.
        std::mt19937 rng(0x54525554u);
        std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
        for (int iter = 0; iter < 1000; ++iter) {
            float a[3][3];
            for (auto& row : a) for (float& x : row) x = dist(rng);
            float sgm[3][3]{};
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    for (int k = 0; k < 3; ++k) sgm[i][j] += a[i][k] * a[j][k];
                }
                sgm[i][i] += 1.0e-3f;
            }
            std::array<MarginalChannelKnowledgeV06,3> m{};
            for (int c = 0; c < 3; ++c) {
                m[c].sigmaEquivalentKnown = true;
                m[c].sigmaEquivalent = std::sqrt(sgm[c][c]);
            }
            PixelCameraRgbCovarianceV06 px;
            CHECK(build_pixel_covariance_from_marginals_v0_6(m, px));
            CHECK(set_certified_covariance_v0_6(px, RgbPairV06::RG, sgm[0][1]));
            CHECK(set_certified_covariance_v0_6(px, RgbPairV06::RB, sgm[0][2]));
            CHECK(set_certified_covariance_v0_6(px, RgbPairV06::GB, sgm[1][2]));
            CHECK(px.fullPsdCertified);
            CHECK(can_exactly_propagate_linear_covariance_v0_6(px));
        }
    }

    {
        CameraRgbCovarianceTileV06 tile;
        CHECK(init_covariance_tile_v0_6(128,256,64,32,tile));
        CHECK(tile.pixels.size()==2048u);
        CHECK(tile.width==64 && tile.height==32);
        CHECK(std::isnan(tile.pixels[0].covariance[0]));
    }

    std::cout << "TRUTHRAW_CAMERA_RGB_COVARIANCE_V0_6_TESTS_PASS\n";
    std::cout << "sizeof(PixelCameraRgbCovarianceV06)=" << sizeof(PixelCameraRgbCovarianceV06) << "\n";
    return 0;
}

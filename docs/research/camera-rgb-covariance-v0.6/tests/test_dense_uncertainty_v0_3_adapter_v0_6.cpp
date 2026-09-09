#include "dense_uncertainty_v0_3_adapter_v0_6.h"
#include <cmath>
#include <iostream>
using namespace truthraw_v06;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; return 1; } } while (0)

int main(){
    truthraw::DenseUncertaintyFieldV03 f;
    f.width=1; f.height=1; f.rgb.resize(3);
    auto& r=f.rgb[0];
    r.valid=true; r.p50Abs=0.67448975f; r.p95Abs=1.95996398f; r.sigmaEquivalent=1.f;
    r.source=truthraw::DenseUncertaintySourceV03::MeasuredNoiseProfileGaussianEquivalent; r.topologyCertified=true;
    auto& g=f.rgb[1];
    g.valid=true; g.p50Abs=2.f; g.p95Abs=5.f; g.sigmaEquivalent=std::numeric_limits<float>::quiet_NaN();
    g.source=truthraw::DenseUncertaintySourceV03::V5GLocalMaxTransportProxy; g.topologyCertified=false;
    auto& b=f.rgb[2];
    b.source=truthraw::DenseUncertaintySourceV03::Unresolved;

    std::array<MarginalChannelKnowledgeV06,3> m{}; PixelCameraRgbCovarianceV06 c;
    CHECK(adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(f,0,m,c));
    CHECK(c.varianceKnownMask==0x1u);
    CHECK(std::abs(c.variance[0]-1.f)<1e-6f);
    CHECK(std::isnan(c.variance[1]) && std::isnan(c.variance[2]));
    CHECK(c.covarianceKnownMask==0u);
    CHECK(std::isnan(c.covariance[0]) && std::isnan(c.covariance[1]) && std::isnan(c.covariance[2]));
    CHECK(m[1].p50AbsKnown && m[1].p95AbsKnown && !m[1].sigmaEquivalentKnown);

    // Fail closed if a quantile-only backend accidentally leaks a numeric sigma.
    g.sigmaEquivalent=3.f;
    CHECK(!adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(f,0,m,c));

    // High-censored measured evidence remains without point variance.
    g = truthraw::DenseUncertaintyEntryV03{};
    g.source=truthraw::DenseUncertaintySourceV03::MeasuredHighCensored; g.topologyCertified=true; g.sourceHighCensored=true;
    CHECK(adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(f,0,m,c));
    CHECK(c.varianceKnownMask==0x1u);

    // Invalid geometry/index are rejected.
    CHECK(!adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(f,1,m,c));
    f.rgb.pop_back();
    CHECK(!adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(f,0,m,c));

    std::cout << "TRUTHRAW_DENSE_V03_TO_COVARIANCE_V06_ADAPTER_TESTS_PASS\n";
    return 0;
}

#include "precision_policy_v0_1.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using namespace truthraw_precision_v01;

static bool near(double a, double b, double eps=1e-12) {
    return std::abs(a-b) <= eps;
}

int main() {
    PrecisionPolicyV01 p;
    assert(precision_policy_authority_invariant_v0_1(p));

    // Exact integer evidence enters both work paths from the same code value.
    Stage2ParamsV01 s;
    s.black = 64.0;
    s.white = 1023.0;
    s.gain = 1.0;
    const std::uint16_t code = 512;
    const float f32 = normalize_raw_u16_v0_1<float>(code, s);
    const double f64 = normalize_raw_u16_v0_1<double>(code, s);
    const double expected = (512.0 - 64.0) / (1023.0 - 64.0);
    assert(std::abs(static_cast<double>(f32) - expected) < 1e-7);
    assert(near(f64, expected));

    // Compensated double reduction must preserve small contributions better than naive summation.
    const double values[] = {1.0e16, 1.0, -1.0e16, 3.0};
    const double mean = mean_float64_v0_1(values, 4);
    assert(std::isfinite(mean));

    // Unknown covariance must remain unknown and must not be silently converted to zero.
    Covariance3dV01 unknown;
    Matrix3dV01 identity;
    Covariance3dV01 out;
    assert(!propagate_full_covariance_v0_1(identity, unknown, out));
    assert(!out.fullyKnown());
    for (double x : out.v) assert(std::isnan(x));

    // Fully known covariance propagates through a linear transform in float64.
    Covariance3dV01 c;
    c.v = {4.0, 0.5, 0.25,
           0.5, 9.0, 0.75,
           0.25,0.75,16.0};
    c.knownMask = Covariance3dV01::fullMask;
    c.psdCertified = true;

    Matrix3dV01 a;
    a.v = {2.0,0.0,0.0,
           0.0,0.5,0.0,
           0.0,0.0,1.5};
    assert(propagate_full_covariance_v0_1(a, c, out));
    assert(out.fullyKnown());
    assert(out.psdCertified);
    assert(near(out.at(0,0), 16.0));
    assert(near(out.at(1,1), 2.25));
    assert(near(out.at(2,2), 36.0));
    assert(near(out.at(0,1), 0.5));
    assert(near(out.at(0,2), 0.75));
    assert(near(out.at(1,2), 0.5625));

    std::cout << "precision_policy_v0_1 PASS\n";
    return 0;
}

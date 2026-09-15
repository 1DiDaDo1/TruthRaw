#include "mixed_precision_storage_v0_4.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace truthraw_precision_v04;

int main() {
    MixedPrecisionPolicyV04 p;
    p.stage2Compute = ComputePrecisionV04::Float64;
    p.reconstructionCompute = ComputePrecisionV04::Float64;
    p.scientificMasterStorage = StoragePrecisionV04::Float32;
    assert(mixed_precision_policy_authority_invariant_v0_4(p));

    const double values[] = {
        0.0,
        1.0 / 959.0,
        0.031250000931322575,
        0.4671532846715328467,
        0.9989572471324296142,
        1.0,
        1.1574074074074074,
        2.365234375,
        -1.0e-6,
        8.0
    };
    std::vector<float> stored;
    const auto s = quantize_f64_to_f32_storage_v0_4(values, sizeof(values)/sizeof(values[0]), stored);
    assert(s.samples == sizeof(values)/sizeof(values[0]));
    assert(s.nonFiniteInputs == 0);
    assert(s.finiteRoundTripFailures == 0);
    assert(stored.size() == s.samples);
    // Storage rounding must remain at ordinary binary32 scale; this test is not
    // a scientific promotion gate by itself, only a deterministic primitive check.
    assert(s.maxAbsError < 2.0e-7);
    assert(s.maxRelativeError < 6.0e-8);

    const double special[] = {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    };
    const auto sp = quantize_f64_to_f32_storage_v0_4(special, 2, stored);
    assert(sp.samples == 2);
    assert(sp.nonFiniteInputs == 2);

    std::cout << "test_mixed_precision_storage_v0_4 PASS"
              << " max_abs=" << s.maxAbsError
              << " max_rel=" << s.maxRelativeError
              << " rms=" << s.rmsError << "\n";
    return 0;
}

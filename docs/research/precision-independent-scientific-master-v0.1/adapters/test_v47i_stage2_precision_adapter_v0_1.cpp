#include "v47i_stage2_precision_adapter_v0_1.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace truthraw_precision_v01;

int main() {
    truthraw::DecodedDngFrame frame;
    frame.meta.width = 4;
    frame.meta.height = 4;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 64.25f, 63.75f, 64.5f};
    frame.meta.hasResidualBlack = true;
    frame.meta.hasGainField = true;
    frame.raw = {
        64,100,200,300,
        400,500,600,700,
        800,900,1000,1023,
        65,66,67,68
    };
    frame.rowBias = {0.125f, -0.25f, 0.5f, -0.125f};
    frame.colBias = {0.0f, 0.0625f, -0.03125f, 0.125f};
    frame.gainField.assign(16, 1.0f);
    frame.gainField[5] = 1.00000011920928955078125f;
    frame.gainField[10] = 1.03125f;

    std::vector<float> f32;
    std::vector<double> f64;
    assert(v47i_stage2_tile_f32_v0_1(frame, 1, 1, 2, 2, f32));
    assert(v47i_stage2_tile_f64_v0_1(frame, 1, 1, 2, 2, f64));
    assert(f32.size() == 4);
    assert(f64.size() == 4);

    const auto cmp = compare_stage2_precision_v0_1(f32, f64);
    assert(cmp.sampleCount == 4);
    assert(std::isfinite(cmp.maxAbsDifference));
    assert(std::isfinite(cmp.rmsDifference));
    assert(cmp.maxAbsDifference < 1e-6);

    // Invalid geometry must fail closed rather than read outside evidence.
    std::vector<float> bad;
    assert(!v47i_stage2_tile_f32_v0_1(frame, 3, 3, 2, 2, bad));

    std::cout << "v47i_stage2_precision_adapter_v0_1 PASS maxAbs="
              << cmp.maxAbsDifference << " rms=" << cmp.rmsDifference << "\n";
    return 0;
}

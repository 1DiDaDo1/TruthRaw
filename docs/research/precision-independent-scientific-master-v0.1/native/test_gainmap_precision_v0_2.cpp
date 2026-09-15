#include "gainmap_precision_v0_2.h"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace truthraw_precision_v02;

static GainMapGridV02 make_map() {
    GainMapGridV02 m;
    m.pointsV = 13;
    m.pointsH = 17;
    m.planes = 1;
    m.spacingV = 1.0 / 12.0;
    m.spacingH = 1.0 / 16.0;
    m.originV = 0.0;
    m.originH = 0.0;
    m.samples.resize(static_cast<std::size_t>(m.pointsV) * m.pointsH);
    for (int r = 0; r < m.pointsV; ++r) {
        for (int c = 0; c < m.pointsH; ++c) {
            const float radial = static_cast<float>((r - 6) * (r - 6) + (c - 8) * (c - 8));
            m.samples[static_cast<std::size_t>(r) * m.pointsH + c] = 1.0f + radial * (1.3f / 100.0f);
        }
    }
    return m;
}

int main() {
    const GainMapBoundsV02 bounds{0, 0, 3072, 4080};
    const GainMapAreaV02 area{1, 1, 3072, 4080, 0, 1, 2, 2};
    GainMapGridV02 map = make_map();
    assert(map.valid());

    // A constant map must agree exactly because no interpolation slope exists.
    GainMapGridV02 constant = map;
    for (float& x : constant.samples) x = 1.25f;
    std::vector<float> c32;
    std::vector<double> c64;
    assert(gainmap_row_sdk_f32_v0_2(constant, bounds, area, 1535, 0, c32));
    assert(gainmap_row_f64_reference_v0_2(constant, bounds, area, 1535, 0, c64));
    assert(c32.size() == c64.size());
    for (std::size_t i = 0; i < c32.size(); ++i) assert(static_cast<double>(c32[i]) == c64[i]);

    // A varying map should expose real F32 arithmetic error, but remain small.
    std::vector<float> f32;
    std::vector<double> f64;
    assert(gainmap_row_sdk_f32_v0_2(map, bounds, area, 1535, 0, f32));
    assert(gainmap_row_f64_reference_v0_2(map, bounds, area, 1535, 0, f64));
    const auto stats = compare_gainmap_rows_v0_2(f32, f64);
    assert(stats.samples == f32.size());
    assert(stats.maxAbsGainError > 0.0);
    assert(stats.maxAbsGainError < 1.0e-6);
    assert(stats.rmsGainError < 2.0e-7);

    // The F64 path must use the same stored F32 samples; precision may change
    // interpolation arithmetic, never the evidence/sample values themselves.
    const float before = map.samples[42];
    const double v = gainmap_interpolate_f64_reference_v0_2(map, bounds, 1001, 2003, 0);
    assert(std::isfinite(v));
    assert(map.samples[42] == before);

    // AreaSpec pitch must preserve CFA phase: 4080-wide image, start at x=1,
    // pitch 2 => 2040 gains on the row.
    assert(f32.size() == 2040u);

    std::cout << "test_gainmap_precision_v0_2 PASS max_abs=" << stats.maxAbsGainError
              << " rms=" << stats.rmsGainError << "\n";
    return 0;
}

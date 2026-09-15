#pragma once

#include <cstddef>
#include <vector>

#include "truthraw/core.h"

namespace truthraw_precision_v01 {

struct Stage2PrecisionComparisonV01 {
    std::size_t sampleCount = 0;
    double maxAbsDifference = 0.0;
    double rmsDifference = 0.0;
};

// Reproduces the historical v4.7i Stage-2 arithmetic in float32 without
// modifying canonical v4.7i source files.
bool v47i_stage2_tile_f32_v0_1(
    const truthraw::DecodedDngFrame& frame,
    int x0, int y0, int width, int height,
    std::vector<float>& out);

// Uses the same decoded evidence and metadata semantics, but performs Stage-2
// arithmetic in float64. Metadata that historically exists only as float is
// promoted numerically; this does not create higher-authority calibration data.
bool v47i_stage2_tile_f64_v0_1(
    const truthraw::DecodedDngFrame& frame,
    int x0, int y0, int width, int height,
    std::vector<double>& out);

Stage2PrecisionComparisonV01 compare_stage2_precision_v0_1(
    const std::vector<float>& f32,
    const std::vector<double>& f64);

} // namespace truthraw_precision_v01

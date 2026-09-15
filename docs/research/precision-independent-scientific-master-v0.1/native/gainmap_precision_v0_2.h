#pragma once

#include <cstdint>
#include <vector>

namespace truthraw_precision_v02 {

struct GainMapBoundsV02 {
    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;

    int height() const { return bottom - top; }
    int width() const { return right - left; }
    bool valid() const { return height() > 0 && width() > 0; }
};

struct GainMapGridV02 {
    int pointsV = 0;
    int pointsH = 0;
    int planes = 1;
    double spacingV = 1.0;
    double spacingH = 1.0;
    double originV = 0.0;
    double originH = 0.0;
    std::vector<float> samples; // DNG GainMap samples are stored as float32.

    bool valid() const;
    float sample(int row, int col, int plane = 0) const;
};

struct GainMapAreaV02 {
    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;
    int plane = 0;
    int planes = 1;
    int rowPitch = 1;
    int colPitch = 1;

    bool valid() const {
        return bottom > top && right > left && planes > 0 && rowPitch > 0 && colPitch > 0;
    }
};

// Reproduces the historical DNG-SDK numerical structure for one AreaSpec row:
// double map coordinates, float row fraction / interpolated values / stepping.
// The map samples themselves remain their exact stored float32 values.
bool gainmap_row_sdk_f32_v0_2(
    const GainMapGridV02& map,
    const GainMapBoundsV02& imageBounds,
    const GainMapAreaV02& area,
    int row,
    int mapPlane,
    std::vector<float>& out);

// Higher-precision reference: same stored float32 GainMap samples and same
// pixel-center geometry, but interpolation arithmetic remains in float64.
bool gainmap_row_f64_reference_v0_2(
    const GainMapGridV02& map,
    const GainMapBoundsV02& imageBounds,
    const GainMapAreaV02& area,
    int row,
    int mapPlane,
    std::vector<double>& out);

float gainmap_interpolate_sdk_f32_v0_2(
    const GainMapGridV02& map,
    const GainMapBoundsV02& imageBounds,
    int row,
    int col,
    int mapPlane = 0);

double gainmap_interpolate_f64_reference_v0_2(
    const GainMapGridV02& map,
    const GainMapBoundsV02& imageBounds,
    int row,
    int col,
    int mapPlane = 0);

struct GainMapPrecisionStatsV02 {
    std::uint64_t samples = 0;
    double maxAbsGainError = 0.0;
    double rmsGainError = 0.0;
};

GainMapPrecisionStatsV02 compare_gainmap_rows_v0_2(
    const std::vector<float>& sdkF32,
    const std::vector<double>& refF64);

} // namespace truthraw_precision_v02

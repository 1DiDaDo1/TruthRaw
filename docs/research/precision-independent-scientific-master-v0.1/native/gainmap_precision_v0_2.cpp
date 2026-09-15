#include "gainmap_precision_v0_2.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw_precision_v02 {

bool GainMapGridV02::valid() const {
    if (pointsV < 1 || pointsH < 1 || planes < 1 || spacingV <= 0.0 || spacingH <= 0.0) return false;
    const std::size_t expected = static_cast<std::size_t>(pointsV) * static_cast<std::size_t>(pointsH) * static_cast<std::size_t>(planes);
    return samples.size() == expected;
}

float GainMapGridV02::sample(int row, int col, int plane) const {
    const std::size_t i = (static_cast<std::size_t>(row) * static_cast<std::size_t>(pointsH) + static_cast<std::size_t>(col)) * static_cast<std::size_t>(planes) + static_cast<std::size_t>(plane);
    return samples[i];
}

namespace {

class SdkF32InterpolatorV02 {
public:
    SdkF32InterpolatorV02(const GainMapGridV02& map,
                         const GainMapBoundsV02& bounds,
                         int row,
                         int column,
                         int plane)
        : map_(map),
          scaleV_(1.0 / static_cast<double>(bounds.height())),
          scaleH_(1.0 / static_cast<double>(bounds.width())),
          offsetV_(0.5 - static_cast<double>(bounds.top)),
          offsetH_(0.5 - static_cast<double>(bounds.left)),
          column_(column),
          plane_(plane) {
        const double rowIndexF = (scaleV_ * (static_cast<double>(row) + offsetV_) - map_.originV) / map_.spacingV;
        if (rowIndexF <= 0.0) {
            rowIndex1_ = rowIndex2_ = 0;
            rowFract_ = 0.0f;
        } else {
            const int lastRow = map_.pointsV - 1;
            if (rowIndexF >= static_cast<double>(lastRow)) {
                rowIndex1_ = rowIndex2_ = lastRow;
                rowFract_ = 0.0f;
            } else {
                rowIndex1_ = static_cast<int>(rowIndexF);
                rowIndex2_ = rowIndex1_ + 1;
                rowFract_ = static_cast<float>(rowIndexF - static_cast<double>(rowIndex1_));
            }
        }
        resetColumn();
    }

    float value() const { return valueBase_ + valueStep_ * valueIndex_; }

    void increment() {
        ++column_;
        if (column_ >= resetColumnAt_) resetColumn();
        else valueIndex_ += 1.0f;
    }

private:
    float interpolateEntry(int colIndex) const {
        const float a = map_.sample(rowIndex1_, colIndex, plane_);
        const float b = map_.sample(rowIndex2_, colIndex, plane_);
        return a * (1.0f - rowFract_) + b * rowFract_;
    }

    void resetColumn() {
        const double colIndexF = (scaleH_ * (static_cast<double>(column_) + offsetH_) - map_.originH) / map_.spacingH;
        if (colIndexF <= 0.0) {
            valueBase_ = interpolateEntry(0);
            valueStep_ = 0.0f;
            resetColumnAt_ = static_cast<int>(std::ceil(map_.originH / scaleH_ - offsetH_));
        } else {
            const int lastCol = map_.pointsH - 1;
            if (colIndexF >= static_cast<double>(lastCol)) {
                valueBase_ = interpolateEntry(lastCol);
                valueStep_ = 0.0f;
                resetColumnAt_ = std::numeric_limits<int>::max();
            } else {
                const int colIndex = static_cast<int>(colIndexF);
                const double base = static_cast<double>(interpolateEntry(colIndex));
                const double delta = static_cast<double>(interpolateEntry(colIndex + 1)) - base;
                valueBase_ = static_cast<float>(base + delta * (colIndexF - static_cast<double>(colIndex)));
                valueStep_ = static_cast<float>((delta * scaleH_) / map_.spacingH);
                resetColumnAt_ = static_cast<int>(std::ceil(
                    ((static_cast<double>(colIndex + 1) * map_.spacingH + map_.originH) / scaleH_) - offsetH_));
            }
        }
        valueIndex_ = 0.0f;
    }

    const GainMapGridV02& map_;
    double scaleV_ = 0.0;
    double scaleH_ = 0.0;
    double offsetV_ = 0.0;
    double offsetH_ = 0.0;
    int column_ = 0;
    int plane_ = 0;
    int rowIndex1_ = 0;
    int rowIndex2_ = 0;
    float rowFract_ = 0.0f;
    int resetColumnAt_ = 0;
    float valueBase_ = 0.0f;
    float valueStep_ = 0.0f;
    float valueIndex_ = 0.0f;
};

bool inputsValid(const GainMapGridV02& map,
                 const GainMapBoundsV02& bounds,
                 int row,
                 int col,
                 int plane) {
    return map.valid() && bounds.valid() && row >= bounds.top && row < bounds.bottom &&
           col >= bounds.left && col < bounds.right && plane >= 0 && plane < map.planes;
}

} // namespace

float gainmap_interpolate_sdk_f32_v0_2(const GainMapGridV02& map,
                                        const GainMapBoundsV02& imageBounds,
                                        int row,
                                        int col,
                                        int mapPlane) {
    if (!inputsValid(map, imageBounds, row, col, mapPlane)) return std::numeric_limits<float>::quiet_NaN();
    SdkF32InterpolatorV02 interp(map, imageBounds, row, col, mapPlane);
    return interp.value();
}

double gainmap_interpolate_f64_reference_v0_2(const GainMapGridV02& map,
                                               const GainMapBoundsV02& imageBounds,
                                               int row,
                                               int col,
                                               int mapPlane) {
    if (!inputsValid(map, imageBounds, row, col, mapPlane)) return std::numeric_limits<double>::quiet_NaN();

    const double scaleV = 1.0 / static_cast<double>(imageBounds.height());
    const double scaleH = 1.0 / static_cast<double>(imageBounds.width());
    const double offsetV = 0.5 - static_cast<double>(imageBounds.top);
    const double offsetH = 0.5 - static_cast<double>(imageBounds.left);

    const double rowF0 = (scaleV * (static_cast<double>(row) + offsetV) - map.originV) / map.spacingV;
    const double colF0 = (scaleH * (static_cast<double>(col) + offsetH) - map.originH) / map.spacingH;
    const double rowF = std::max(0.0, std::min(static_cast<double>(map.pointsV - 1), rowF0));
    const double colF = std::max(0.0, std::min(static_cast<double>(map.pointsH - 1), colF0));

    const int r0 = static_cast<int>(std::floor(rowF));
    const int c0 = static_cast<int>(std::floor(colF));
    const int r1 = std::min(r0 + 1, map.pointsV - 1);
    const int c1 = std::min(c0 + 1, map.pointsH - 1);
    const double fy = rowF - static_cast<double>(r0);
    const double fx = colF - static_cast<double>(c0);

    const double q00 = static_cast<double>(map.sample(r0, c0, mapPlane));
    const double q10 = static_cast<double>(map.sample(r1, c0, mapPlane));
    const double q01 = static_cast<double>(map.sample(r0, c1, mapPlane));
    const double q11 = static_cast<double>(map.sample(r1, c1, mapPlane));
    const double v0 = q00 * (1.0 - fy) + q10 * fy;
    const double v1 = q01 * (1.0 - fy) + q11 * fy;
    return v0 * (1.0 - fx) + v1 * fx;
}

bool gainmap_row_sdk_f32_v0_2(const GainMapGridV02& map,
                               const GainMapBoundsV02& imageBounds,
                               const GainMapAreaV02& area,
                               int row,
                               int mapPlane,
                               std::vector<float>& out) {
    out.clear();
    if (!map.valid() || !imageBounds.valid() || !area.valid() || mapPlane < 0 || mapPlane >= map.planes) return false;
    if (row < area.top || row >= area.bottom || ((row - area.top) % area.rowPitch) != 0) return false;
    const int start = std::max(area.left, imageBounds.left);
    const int stop = std::min(area.right, imageBounds.right);
    if (start >= stop) return true;

    SdkF32InterpolatorV02 interp(map, imageBounds, row, start, mapPlane);
    for (int col = start; col < stop; col += area.colPitch) {
        out.push_back(interp.value());
        for (int j = 0; j < area.colPitch; ++j) interp.increment();
    }
    return true;
}

bool gainmap_row_f64_reference_v0_2(const GainMapGridV02& map,
                                     const GainMapBoundsV02& imageBounds,
                                     const GainMapAreaV02& area,
                                     int row,
                                     int mapPlane,
                                     std::vector<double>& out) {
    out.clear();
    if (!map.valid() || !imageBounds.valid() || !area.valid() || mapPlane < 0 || mapPlane >= map.planes) return false;
    if (row < area.top || row >= area.bottom || ((row - area.top) % area.rowPitch) != 0) return false;
    const int start = std::max(area.left, imageBounds.left);
    const int stop = std::min(area.right, imageBounds.right);
    for (int col = start; col < stop; col += area.colPitch) {
        out.push_back(gainmap_interpolate_f64_reference_v0_2(map, imageBounds, row, col, mapPlane));
    }
    return true;
}

GainMapPrecisionStatsV02 compare_gainmap_rows_v0_2(const std::vector<float>& sdkF32,
                                                    const std::vector<double>& refF64) {
    GainMapPrecisionStatsV02 s{};
    if (sdkF32.size() != refF64.size() || sdkF32.empty()) return s;
    long double sumSq = 0.0L;
    for (std::size_t i = 0; i < sdkF32.size(); ++i) {
        const double e = std::abs(static_cast<double>(sdkF32[i]) - refF64[i]);
        s.maxAbsGainError = std::max(s.maxAbsGainError, e);
        sumSq += static_cast<long double>(e) * static_cast<long double>(e);
    }
    s.samples = static_cast<std::uint64_t>(sdkF32.size());
    s.rmsGainError = std::sqrt(static_cast<double>(sumSq / static_cast<long double>(s.samples)));
    return s;
}

} // namespace truthraw_precision_v02

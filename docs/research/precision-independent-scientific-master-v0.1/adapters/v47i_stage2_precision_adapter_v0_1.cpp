#include "v47i_stage2_precision_adapter_v0_1.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v01 {
namespace {

inline int phase_index(int y, int x) {
    return (y & 1) * 2 + (x & 1);
}

bool valid_tile(const truthraw::DecodedDngFrame& frame,
                int x0, int y0, int width, int height) {
    if (frame.meta.width <= 0 || frame.meta.height <= 0 ||
        x0 < 0 || y0 < 0 || width <= 0 || height <= 0 ||
        x0 + width > frame.meta.width || y0 + height > frame.meta.height) return false;
    const std::size_t pixels = static_cast<std::size_t>(frame.meta.width) * static_cast<std::size_t>(frame.meta.height);
    if (frame.raw.size() < pixels) return false;
    if (frame.meta.hasGainField && frame.gainField.size() < pixels) return false;
    if (frame.meta.hasResidualBlack &&
        (frame.rowBias.size() < static_cast<std::size_t>(frame.meta.height) ||
         frame.colBias.size() < static_cast<std::size_t>(frame.meta.width))) return false;
    return true;
}

} // namespace

bool v47i_stage2_tile_f32_v0_1(
    const truthraw::DecodedDngFrame& frame,
    int x0, int y0, int width, int height,
    std::vector<float>& out) {

    if (!valid_tile(frame, x0, y0, width, height)) return false;
    out.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0.0f);

    for (int yy = 0; yy < height; ++yy) {
        const int y = y0 + yy;
        for (int xx = 0; xx < width; ++xx) {
            const int x = x0 + xx;
            const std::size_t gi = static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.meta.width) + static_cast<std::size_t>(x);
            const int ph = phase_index(y, x);
            float black = frame.meta.blackPhase[static_cast<std::size_t>(ph)];
            if (frame.meta.hasResidualBlack) black += frame.rowBias[static_cast<std::size_t>(y)] + frame.colBias[static_cast<std::size_t>(x)];
            const float denom = std::max(frame.meta.whiteLevel - black, 1.0f);
            float s = (static_cast<float>(frame.raw[gi]) - black) / denom;
            if (frame.meta.hasGainField) s *= frame.gainField[gi];
            out[static_cast<std::size_t>(yy) * static_cast<std::size_t>(width) + static_cast<std::size_t>(xx)] = s;
        }
    }
    return true;
}

bool v47i_stage2_tile_f64_v0_1(
    const truthraw::DecodedDngFrame& frame,
    int x0, int y0, int width, int height,
    std::vector<double>& out) {

    if (!valid_tile(frame, x0, y0, width, height)) return false;
    out.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0.0);

    for (int yy = 0; yy < height; ++yy) {
        const int y = y0 + yy;
        for (int xx = 0; xx < width; ++xx) {
            const int x = x0 + xx;
            const std::size_t gi = static_cast<std::size_t>(y) * static_cast<std::size_t>(frame.meta.width) + static_cast<std::size_t>(x);
            const int ph = phase_index(y, x);
            double black = static_cast<double>(frame.meta.blackPhase[static_cast<std::size_t>(ph)]);
            if (frame.meta.hasResidualBlack) {
                black += static_cast<double>(frame.rowBias[static_cast<std::size_t>(y)])
                       + static_cast<double>(frame.colBias[static_cast<std::size_t>(x)]);
            }
            const double denom = std::max(static_cast<double>(frame.meta.whiteLevel) - black, 1.0);
            double s = (static_cast<double>(frame.raw[gi]) - black) / denom;
            if (frame.meta.hasGainField) s *= static_cast<double>(frame.gainField[gi]);
            out[static_cast<std::size_t>(yy) * static_cast<std::size_t>(width) + static_cast<std::size_t>(xx)] = s;
        }
    }
    return true;
}

Stage2PrecisionComparisonV01 compare_stage2_precision_v0_1(
    const std::vector<float>& f32,
    const std::vector<double>& f64) {

    Stage2PrecisionComparisonV01 r;
    if (f32.size() != f64.size()) return r;
    r.sampleCount = f32.size();
    if (r.sampleCount == 0) return r;

    long double sumSq = 0.0L;
    for (std::size_t i = 0; i < r.sampleCount; ++i) {
        const double d = static_cast<double>(f32[i]) - f64[i];
        r.maxAbsDifference = std::max(r.maxAbsDifference, std::abs(d));
        sumSq += static_cast<long double>(d) * static_cast<long double>(d);
    }
    r.rmsDifference = std::sqrt(static_cast<double>(sumSq / static_cast<long double>(r.sampleCount)));
    return r;
}

} // namespace truthraw_precision_v01

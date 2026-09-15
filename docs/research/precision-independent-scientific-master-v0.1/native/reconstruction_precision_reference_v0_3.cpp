#include "reconstruction_precision_reference_v0_3.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace truthraw_precision_v03 {
namespace {

static inline int phase_index(int y, int x) {
    return (y & 1) * 2 + (x & 1);
}

static inline int color_for_phase(truthraw::CfaPattern cfa, int phase) {
    static const int BGGR[4] = {2,1,1,0};
    static const int RGGB[4] = {0,1,1,2};
    static const int GRBG[4] = {1,0,2,1};
    static const int GBRG[4] = {1,2,0,1};
    const int* p = BGGR;
    if (cfa == truthraw::CfaPattern::RGGB) p = RGGB;
    else if (cfa == truthraw::CfaPattern::GRBG) p = GRBG;
    else if (cfa == truthraw::CfaPattern::GBRG) p = GBRG;
    return p[phase];
}

template <typename T>
static inline T sample_clamped(const T* a, int w, int h, int x, int y) {
    x = std::max(0, std::min(w - 1, x));
    y = std::max(0, std::min(h - 1, y));
    return a[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)];
}

template <typename T>
static inline T clamp_value(T x, T lo, T hi) {
    return std::max(lo, std::min(hi, x));
}

template <typename T>
static inline T support_limited(T estimate,
                                const T* vals,
                                int n,
                                std::uint8_t* decision) {
    T lo = vals[0];
    T hi = vals[0];
    for (int i = 1; i < n; ++i) {
        lo = std::min(lo, vals[i]);
        hi = std::max(hi, vals[i]);
    }
    const T margin = T(0.125) * (hi - lo) + T(1e-5);
    const T low = lo - margin;
    const T high = hi + margin;
    if (decision) {
        if (estimate < low) *decision = static_cast<std::uint8_t>(ClampDecisionV03::Low);
        else if (estimate > high) *decision = static_cast<std::uint8_t>(ClampDecisionV03::High);
        else *decision = static_cast<std::uint8_t>(ClampDecisionV03::None);
    }
    return clamp_value(estimate, low, high);
}

template <typename T>
static inline T directional_blend(T a,
                                  T b,
                                  T ga,
                                  T gb,
                                  std::uint8_t* decision) {
    const T ratio = T(0.72);
    if (ga < ratio * gb) {
        if (decision) *decision = static_cast<std::uint8_t>(DirectionDecisionV03::Horizontal);
        return a;
    }
    if (gb < ratio * ga) {
        if (decision) *decision = static_cast<std::uint8_t>(DirectionDecisionV03::Vertical);
        return b;
    }
    if (decision) *decision = static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend);
    const T e = T(1e-7);
    const T wa = T(1) / (ga + e);
    const T wb = T(1) / (gb + e);
    return (wa * a + wb * b) / (wa + wb);
}

template <typename T>
static inline void directional_axis(const T* a,
                                    int w,
                                    int h,
                                    int x,
                                    int y,
                                    int dx,
                                    int dy,
                                    T measured,
                                    T& est,
                                    T& grad) {
    const T p = sample_clamped(a, w, h, x - dx, y - dy);
    const T q = sample_clamped(a, w, h, x + dx, y + dy);
    const T c0 = sample_clamped(a, w, h, x - 2 * dx, y - 2 * dy);
    const T c1 = sample_clamped(a, w, h, x + 2 * dx, y + 2 * dy);
    const T curvature = T(2) * measured - c0 - c1;
    est = T(0.5) * (p + q) + T(0.25) * curvature;
    grad = std::abs(p - q) + T(0.5) * std::abs(curvature);
}

template <typename T>
bool reconstruct_impl(const T* a,
                      int w,
                      int h,
                      int ghx0,
                      int ghy0,
                      int coreX0,
                      int coreY0,
                      int coreW,
                      int coreH,
                      truthraw::CfaPattern cfa,
                      std::vector<T>& out,
                      ReconstructionTraceV03* trace) {
    static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value,
                  "TruthRaw reconstruction precision reference supports float or double only");
    if (!a || w <= 0 || h <= 0 || coreW <= 0 || coreH <= 0) return false;
    if (coreX0 < ghx0 || coreY0 < ghy0 ||
        coreX0 + coreW > ghx0 + w || coreY0 + coreH > ghy0 + h) return false;

    out.assign(static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH) * 3u, T(0));
    if (trace) {
        trace->greenDirection.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                                     static_cast<std::uint8_t>(DirectionDecisionV03::MeasuredGreen));
        trace->greenClamp.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                                 static_cast<std::uint8_t>(ClampDecisionV03::None));
        trace->colorClamp.assign(static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH) * 3u,
                                 static_cast<std::uint8_t>(ClampDecisionV03::None));
        trace->measuredChannelChecks = 0;
        trace->measuredChannelViolations = 0;
    }

    std::vector<T> green(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));

    // Pass 1: exact v4.7i topology with precision-selected arithmetic.
    for (int ly = 0; ly < h; ++ly) {
        const int gy = ghy0 + ly;
        for (int lx = 0; lx < w; ++lx) {
            const int gx = ghx0 + lx;
            const int c = color_for_phase(cfa, phase_index(gy, gx));
            const T measured = sample_clamped(a, w, h, lx, ly);
            const std::size_t gi = static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) + static_cast<std::size_t>(lx);
            if (c == 1) {
                green[gi] = measured;
                if (trace) trace->greenDirection[gi] = static_cast<std::uint8_t>(DirectionDecisionV03::MeasuredGreen);
                continue;
            }

            T eh, ev, gh, gv;
            directional_axis(a, w, h, lx, ly, 1, 0, measured, eh, gh);
            directional_axis(a, w, h, lx, ly, 0, 1, measured, ev, gv);
            std::uint8_t dir = static_cast<std::uint8_t>(DirectionDecisionV03::WeightedBlend);
            const T ge = directional_blend(eh, ev, gh, gv, &dir);
            const T support[4] = {
                sample_clamped(a, w, h, lx - 1, ly),
                sample_clamped(a, w, h, lx + 1, ly),
                sample_clamped(a, w, h, lx, ly - 1),
                sample_clamped(a, w, h, lx, ly + 1)
            };
            std::uint8_t clampDecision = static_cast<std::uint8_t>(ClampDecisionV03::None);
            green[gi] = support_limited(ge, support, 4, &clampDecision);
            if (trace) {
                trace->greenDirection[gi] = dir;
                trace->greenClamp[gi] = clampDecision;
            }
        }
    }

    auto colorDiffEstimate = [&](int lx,
                                 int ly,
                                 int targetColor,
                                 const int* xy,
                                 int n,
                                 std::uint8_t* clampDecision) -> T {
        const T gc = green[static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) + static_cast<std::size_t>(lx)];
        T sum = T(0);
        T ws = T(0);
        for (int i = 0; i < n; ++i) {
            const int xx = std::max(0, std::min(w - 1, xy[2 * i]));
            const int yy = std::max(0, std::min(h - 1, xy[2 * i + 1]));
            const int gx = ghx0 + xx;
            const int gy = ghy0 + yy;
            if (color_for_phase(cfa, phase_index(gy, gx)) != targetColor) continue;
            const T gn = green[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(xx)];
            const T cn = a[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(xx)];
            const T wt = T(1) / (T(1e-4) + std::abs(gn - gc));
            sum += wt * (cn - gn);
            ws += wt;
        }
        const T est = ws > T(0) ? gc + sum / ws : gc;
        T vals[4] = {gc, gc, gc, gc};
        int vc = 0;
        for (int i = 0; i < n && vc < 4; ++i) {
            const int xx = std::max(0, std::min(w - 1, xy[2 * i]));
            const int yy = std::max(0, std::min(h - 1, xy[2 * i + 1]));
            const int gx = ghx0 + xx;
            const int gy = ghy0 + yy;
            if (color_for_phase(cfa, phase_index(gy, gx)) == targetColor) {
                vals[vc++] = a[static_cast<std::size_t>(yy) * static_cast<std::size_t>(w) + static_cast<std::size_t>(xx)];
            }
        }
        if (vc > 0) return support_limited(est, vals, vc, clampDecision);
        if (clampDecision) *clampDecision = static_cast<std::uint8_t>(ClampDecisionV03::None);
        return gc;
    };

    // Pass 2: color-difference reconstruction with exact measured-channel reinjection.
    for (int cy = 0; cy < coreH; ++cy) {
        const int gy = coreY0 + cy;
        const int ly = gy - ghy0;
        for (int cx = 0; cx < coreW; ++cx) {
            const int gx = coreX0 + cx;
            const int lx = gx - ghx0;
            const int c = color_for_phase(cfa, phase_index(gy, gx));
            const T measured = sample_clamped(a, w, h, lx, ly);
            T rgb[3] = {T(0), green[static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) + static_cast<std::size_t>(lx)], T(0)};
            std::uint8_t clampR = 0, clampG = 0, clampB = 0;

            if (c == 0) {
                rgb[0] = measured;
                const int diag[] = {lx - 1, ly - 1, lx + 1, ly - 1, lx - 1, ly + 1, lx + 1, ly + 1};
                rgb[2] = colorDiffEstimate(lx, ly, 2, diag, 4, &clampB);
            } else if (c == 2) {
                rgb[2] = measured;
                const int diag[] = {lx - 1, ly - 1, lx + 1, ly - 1, lx - 1, ly + 1, lx + 1, ly + 1};
                rgb[0] = colorDiffEstimate(lx, ly, 0, diag, 4, &clampR);
            } else {
                rgb[1] = measured;
                const int leftColor = color_for_phase(cfa, phase_index(gy, gx - 1));
                if (leftColor == 0) {
                    const int rxy[] = {lx - 1, ly, lx + 1, ly};
                    const int bxy[] = {lx, ly - 1, lx, ly + 1};
                    rgb[0] = colorDiffEstimate(lx, ly, 0, rxy, 2, &clampR);
                    rgb[2] = colorDiffEstimate(lx, ly, 2, bxy, 2, &clampB);
                } else {
                    const int bxy[] = {lx - 1, ly, lx + 1, ly};
                    const int rxy[] = {lx, ly - 1, lx, ly + 1};
                    rgb[2] = colorDiffEstimate(lx, ly, 2, bxy, 2, &clampB);
                    rgb[0] = colorDiffEstimate(lx, ly, 0, rxy, 2, &clampR);
                }
            }

            const std::size_t oi = static_cast<std::size_t>(cy) * static_cast<std::size_t>(coreW) + static_cast<std::size_t>(cx);
            out[3 * oi] = rgb[0];
            out[3 * oi + 1] = rgb[1];
            out[3 * oi + 2] = rgb[2];

            if (trace) {
                trace->colorClamp[3 * oi] = clampR;
                trace->colorClamp[3 * oi + 1] = clampG;
                trace->colorClamp[3 * oi + 2] = clampB;
                ++trace->measuredChannelChecks;
                if (rgb[c] != measured) ++trace->measuredChannelViolations;
            }
        }
    }
    return true;
}

} // namespace

bool v47i_edge_aware_reconstruct_f32_v0_3(
    const float* stage2,
    int tileW,
    int tileH,
    int globalHx0,
    int globalHy0,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    truthraw::CfaPattern cfa,
    std::vector<float>& outCameraRgb,
    ReconstructionTraceV03* trace) {
    return reconstruct_impl(stage2, tileW, tileH, globalHx0, globalHy0,
                            coreX0, coreY0, coreW, coreH, cfa, outCameraRgb, trace);
}

bool v47i_edge_aware_reconstruct_f64_v0_3(
    const double* stage2,
    int tileW,
    int tileH,
    int globalHx0,
    int globalHy0,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    truthraw::CfaPattern cfa,
    std::vector<double>& outCameraRgb,
    ReconstructionTraceV03* trace) {
    return reconstruct_impl(stage2, tileW, tileH, globalHx0, globalHy0,
                            coreX0, coreY0, coreW, coreH, cfa, outCameraRgb, trace);
}

ReconstructionPrecisionStatsV03 compare_reconstruction_precision_v0_3(
    const std::vector<float>& f32,
    const ReconstructionTraceV03& traceF32,
    const std::vector<double>& f64,
    const ReconstructionTraceV03& traceF64) {
    ReconstructionPrecisionStatsV03 out{};
    if (f32.size() != f64.size() || f32.empty()) return out;

    long double sumSq = 0.0L;
    for (std::size_t i = 0; i < f32.size(); ++i) {
        const double e = std::abs(static_cast<double>(f32[i]) - f64[i]);
        out.maxAbsError = std::max(out.maxAbsError, e);
        sumSq += static_cast<long double>(e) * static_cast<long double>(e);
    }
    out.rgbSamples = static_cast<std::uint64_t>(f32.size());
    out.rmsError = std::sqrt(static_cast<double>(sumSq / static_cast<long double>(out.rgbSamples)));

    if (traceF32.greenDirection.size() == traceF64.greenDirection.size()) {
        for (std::size_t i = 0; i < traceF32.greenDirection.size(); ++i) {
            if (traceF32.greenDirection[i] != traceF64.greenDirection[i]) ++out.greenDirectionDivergence;
        }
    }
    if (traceF32.greenClamp.size() == traceF64.greenClamp.size()) {
        for (std::size_t i = 0; i < traceF32.greenClamp.size(); ++i) {
            if (traceF32.greenClamp[i] != traceF64.greenClamp[i]) ++out.greenClampDivergence;
        }
    }
    if (traceF32.colorClamp.size() == traceF64.colorClamp.size()) {
        for (std::size_t i = 0; i < traceF32.colorClamp.size(); ++i) {
            if (traceF32.colorClamp[i] != traceF64.colorClamp[i]) ++out.colorClampDivergence;
        }
    }
    out.measuredChannelViolationsF32 = traceF32.measuredChannelViolations;
    out.measuredChannelViolationsF64 = traceF64.measuredChannelViolations;
    return out;
}

} // namespace truthraw_precision_v03

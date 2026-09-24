#include "scientific_master_f64_reconstruction_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace truthraw::scientific_master_f64_reconstruction_v0_1 {
namespace {

int phase_index(int y, int x) {
    return (y & 1) * 2 + (x & 1);
}

int color_for_phase(CfaPattern cfa, int phase) {
    static constexpr int bggr[4] = {2, 1, 1, 0};
    static constexpr int rggb[4] = {0, 1, 1, 2};
    static constexpr int grbg[4] = {1, 0, 2, 1};
    static constexpr int gbrg[4] = {1, 2, 0, 1};

    const int* map = bggr;
    switch (cfa) {
        case CfaPattern::RGGB: map = rggb; break;
        case CfaPattern::GRBG: map = grbg; break;
        case CfaPattern::GBRG: map = gbrg; break;
        case CfaPattern::BGGR: map = bggr; break;
    }
    return map[phase & 3];
}

float sample_clamped_f32(const float* a, int w, int h, int x, int y) {
    x = std::max(0, std::min(w - 1, x));
    y = std::max(0, std::min(h - 1, y));
    return a[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) +
             static_cast<std::size_t>(x)];
}

double sample_clamped_f64(const float* a, int w, int h, int x, int y) {
    return static_cast<double>(sample_clamped_f32(a, w, h, x, y));
}

double directional_blend_f64(double a, double b, double ga, double gb) {
    constexpr double epsilon = 1.0e-7;
    if (ga < 0.72 * gb) return a;
    if (gb < 0.72 * ga) return b;
    const double wa = 1.0 / (ga + epsilon);
    const double wb = 1.0 / (gb + epsilon);
    return (wa * a + wb * b) / (wa + wb);
}

void directional_axis_f64(
    const float* a,
    int w,
    int h,
    int x,
    int y,
    int dx,
    int dy,
    double measured,
    double& estimate,
    double& gradient) {

    const double p = sample_clamped_f64(a, w, h, x - dx, y - dy);
    const double q = sample_clamped_f64(a, w, h, x + dx, y + dy);
    const double c0 = sample_clamped_f64(a, w, h, x - 2 * dx, y - 2 * dy);
    const double c1 = sample_clamped_f64(a, w, h, x + 2 * dx, y + 2 * dy);
    const double curvature = 2.0 * measured - c0 - c1;
    estimate = 0.5 * (p + q) + 0.25 * curvature;
    gradient = std::abs(p - q) + 0.5 * std::abs(curvature);
}

double support_limited_f64(double estimate, const double* values, int count) {
    double low = values[0];
    double high = values[0];
    for (int i = 1; i < count; ++i) {
        low = std::min(low, values[i]);
        high = std::max(high, values[i]);
    }
    const double margin = 0.125 * (high - low) + 1.0e-5;
    return std::max(low - margin, std::min(high + margin, estimate));
}

} // namespace

Status ResearchEdgeAwareMeasuredPreservingReconstructionF64::reconstructTile(
    const float* a,
    int w,
    int h,
    int globalHx0,
    int globalHy0,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    CfaPattern cfa,
    float* out) {

    if (a == nullptr || out == nullptr || w <= 0 || h <= 0) {
        return Status::error(
            StatusCode::InvalidArgument,
            "invalid f64 research reconstruction tile");
    }

    thread_local std::vector<double> greenScratch;
    greenScratch.resize(
        static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    auto& green = greenScratch;

    for (int ly = 0; ly < h; ++ly) {
        const int gy = globalHy0 + ly;
        for (int lx = 0; lx < w; ++lx) {
            const int gx = globalHx0 + lx;
            const int color = color_for_phase(cfa, phase_index(gy, gx));
            const float measuredF32 = sample_clamped_f32(a, w, h, lx, ly);
            const double measured = static_cast<double>(measuredF32);

            if (color == 1) {
                green[
                    static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(lx)] = measured;
                continue;
            }

            double horizontal = 0.0;
            double vertical = 0.0;
            double gradHorizontal = 0.0;
            double gradVertical = 0.0;
            directional_axis_f64(
                a, w, h, lx, ly, 1, 0, measured, horizontal, gradHorizontal);
            directional_axis_f64(
                a, w, h, lx, ly, 0, 1, measured, vertical, gradVertical);

            const double estimate = directional_blend_f64(
                horizontal, vertical, gradHorizontal, gradVertical);
            const double support[4] = {
                sample_clamped_f64(a, w, h, lx - 1, ly),
                sample_clamped_f64(a, w, h, lx + 1, ly),
                sample_clamped_f64(a, w, h, lx, ly - 1),
                sample_clamped_f64(a, w, h, lx, ly + 1),
            };

            green[
                static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) +
                static_cast<std::size_t>(lx)] =
                support_limited_f64(estimate, support, 4);
        }
    }

    const auto colorDifferenceEstimate =
        [&](int lx, int ly, int targetColor, const int* xy, int count) -> double {
            const double centerGreen =
                green[
                    static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(lx)];

            double weightedDifference = 0.0;
            double weightSum = 0.0;

            for (int i = 0; i < count; ++i) {
                const int xx = std::max(0, std::min(w - 1, xy[2 * i]));
                const int yy = std::max(0, std::min(h - 1, xy[2 * i + 1]));
                const int gx = globalHx0 + xx;
                const int gy = globalHy0 + yy;
                if (color_for_phase(cfa, phase_index(gy, gx)) != targetColor) {
                    continue;
                }

                const double neighbourGreen =
                    green[
                        static_cast<std::size_t>(yy) *
                            static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(xx)];
                const double neighbourColor = static_cast<double>(
                    a[
                        static_cast<std::size_t>(yy) *
                            static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(xx)]);

                const double weight =
                    1.0 / (1.0e-4 + std::abs(neighbourGreen - centerGreen));
                weightedDifference +=
                    weight * (neighbourColor - neighbourGreen);
                weightSum += weight;
            }

            const double estimate =
                weightSum > 0.0
                    ? centerGreen + weightedDifference / weightSum
                    : centerGreen;

            double support[4] = {
                centerGreen, centerGreen, centerGreen, centerGreen};
            int supportCount = 0;

            for (int i = 0; i < count && supportCount < 4; ++i) {
                const int xx = std::max(0, std::min(w - 1, xy[2 * i]));
                const int yy = std::max(0, std::min(h - 1, xy[2 * i + 1]));
                const int gx = globalHx0 + xx;
                const int gy = globalHy0 + yy;
                if (color_for_phase(cfa, phase_index(gy, gx)) == targetColor) {
                    support[supportCount++] = static_cast<double>(
                        a[
                            static_cast<std::size_t>(yy) *
                                static_cast<std::size_t>(w) +
                            static_cast<std::size_t>(xx)]);
                }
            }

            return supportCount > 0
                ? support_limited_f64(estimate, support, supportCount)
                : centerGreen;
        };

    for (int cy = 0; cy < coreH; ++cy) {
        const int gy = coreY0 + cy;
        const int ly = gy - globalHy0;

        for (int cx = 0; cx < coreW; ++cx) {
            const int gx = coreX0 + cx;
            const int lx = gx - globalHx0;
            const int color = color_for_phase(cfa, phase_index(gy, gx));
            const float measured = sample_clamped_f32(a, w, h, lx, ly);

            double rgb[3] = {
                0.0,
                green[
                    static_cast<std::size_t>(ly) * static_cast<std::size_t>(w) +
                    static_cast<std::size_t>(lx)],
                0.0};

            if (color == 0) {
                const int diagonal[] = {
                    lx - 1, ly - 1,
                    lx + 1, ly - 1,
                    lx - 1, ly + 1,
                    lx + 1, ly + 1};
                rgb[0] = static_cast<double>(measured);
                rgb[2] = colorDifferenceEstimate(lx, ly, 2, diagonal, 4);
            } else if (color == 2) {
                const int diagonal[] = {
                    lx - 1, ly - 1,
                    lx + 1, ly - 1,
                    lx - 1, ly + 1,
                    lx + 1, ly + 1};
                rgb[2] = static_cast<double>(measured);
                rgb[0] = colorDifferenceEstimate(lx, ly, 0, diagonal, 4);
            } else {
                rgb[1] = static_cast<double>(measured);
                const int leftColor =
                    color_for_phase(cfa, phase_index(gy, gx - 1));

                if (leftColor == 0) {
                    const int red[] = {lx - 1, ly, lx + 1, ly};
                    const int blue[] = {lx, ly - 1, lx, ly + 1};
                    rgb[0] = colorDifferenceEstimate(lx, ly, 0, red, 2);
                    rgb[2] = colorDifferenceEstimate(lx, ly, 2, blue, 2);
                } else {
                    const int blue[] = {lx - 1, ly, lx + 1, ly};
                    const int red[] = {lx, ly - 1, lx, ly + 1};
                    rgb[2] = colorDifferenceEstimate(lx, ly, 2, blue, 2);
                    rgb[0] = colorDifferenceEstimate(lx, ly, 0, red, 2);
                }
            }

            const std::size_t outputIndex =
                static_cast<std::size_t>(cy) *
                    static_cast<std::size_t>(coreW) +
                static_cast<std::size_t>(cx);

            // Measured CFA component bypasses Float64 storage entirely.
            if (color == 0) {
                out[3u * outputIndex] = measured;
                out[3u * outputIndex + 1u] = static_cast<float>(rgb[1]);
                out[3u * outputIndex + 2u] = static_cast<float>(rgb[2]);
            } else if (color == 1) {
                out[3u * outputIndex] = static_cast<float>(rgb[0]);
                out[3u * outputIndex + 1u] = measured;
                out[3u * outputIndex + 2u] = static_cast<float>(rgb[2]);
            } else {
                out[3u * outputIndex] = static_cast<float>(rgb[0]);
                out[3u * outputIndex + 1u] = static_cast<float>(rgb[1]);
                out[3u * outputIndex + 2u] = measured;
            }
        }
    }

    return Status::ok();
}

} // namespace truthraw::scientific_master_f64_reconstruction_v0_1

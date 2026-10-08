#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::streaming_v0_1::presentation_gamut_v0_1 {

// Presentation/output-only negative-gamut fit for linear sRGB / Rec.709.
//
// This helper is deliberately downstream of camera->XYZ->linear-sRGB,
// appearance and the SDR tone LUT. It must never alter sealed source samples,
// Scientific Master values or scientific authority. Legacy streaming behaviour
// remains the default; callers opt into this fit explicitly for derived
// presentation output.
constexpr double kLumaR = 0.2126;
constexpr double kLumaG = 0.7152;
constexpr double kLumaB = 0.0722;
constexpr double kEpsilon = 1.0e-12;

inline bool finite_rgb(float r, float g, float b) noexcept {
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

inline double luminance709(float r, float g, float b) noexcept {
    return kLumaR * static_cast<double>(r) +
           kLumaG * static_cast<double>(g) +
           kLumaB * static_cast<double>(b);
}

inline bool fit_nonnegative_preserve_luminance(
    float& r,
    float& g,
    float& b) noexcept {
    if (!finite_rgb(r, g, b)) return false;
    if (r >= 0.0f && g >= 0.0f && b >= 0.0f) return true;

    const double y = luminance709(r, g, b);
    if (!std::isfinite(y)) return false;
    if (y <= kEpsilon) {
        r = g = b = 0.0f;
        return true;
    }

    double t = 1.0;
    const auto constrain_lower = [&](float c) noexcept {
        if (c < 0.0f) {
            const double cd = static_cast<double>(c);
            const double denom = y - cd;
            if (denom > kEpsilon) t = std::min(t, y / denom);
        }
    };
    constrain_lower(r);
    constrain_lower(g);
    constrain_lower(b);
    t = std::clamp(t, 0.0, 1.0);

    const auto fit = [&](float c) noexcept -> float {
        const double out = y + t * (static_cast<double>(c) - y);
        return static_cast<float>(std::max(0.0, out));
    };
    r = fit(r);
    g = fit(g);
    b = fit(b);
    return finite_rgb(r, g, b);
}

} // namespace truthraw::streaming_v0_1::presentation_gamut_v0_1

#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::streaming_v0_1::presentation_gamut_v0_1 {

// Presentation/output-only gamut/headroom helpers for linear sRGB / Rec.709.
//
// These helpers live strictly downstream of camera->XYZ->linear-sRGB and do
// not alter sealed source samples, Scientific Master values, authority,
// censoring or Zero-Line. Legacy streaming behaviour remains the default;
// derived presentation callers must opt in explicitly.
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

// The historical SDR LUT is defined on [0,1]. Clamping an input luminance
// above 1.0 to the last LUT sample makes every larger scene/presentation value
// collapse to the same luminance before the real output boundary. For an
// explicitly opted-in derived presentation path, continue the endpoint slope
// linearly above 1.0 instead: mappedY = inputY * mappedYAtUnit.
//
// This does not claim that values above 1.0 are measured display luminance. It
// only preserves their relative float headroom until the downstream highlight
// shoulder/gamut mapping performs the one bounded conversion to display range.
inline bool extend_luminance_above_unit_preserve_headroom(
    float inputY,
    float boundedMappedY,
    float mappedYAtUnit,
    float& mappedYOut) noexcept {
    if (!std::isfinite(inputY) || !std::isfinite(boundedMappedY) ||
        !std::isfinite(mappedYAtUnit) || inputY < 0.0f ||
        boundedMappedY < 0.0f || mappedYAtUnit < 0.0f) {
        return false;
    }
    if (inputY <= 1.0f) {
        mappedYOut = boundedMappedY;
        return std::isfinite(mappedYOut);
    }

    const double extended =
        static_cast<double>(inputY) * static_cast<double>(mappedYAtUnit);
    if (!std::isfinite(extended) ||
        extended > static_cast<double>(std::numeric_limits<float>::max())) {
        return false;
    }
    mappedYOut = static_cast<float>(extended);
    return std::isfinite(mappedYOut);
}

} // namespace truthraw::streaming_v0_1::presentation_gamut_v0_1

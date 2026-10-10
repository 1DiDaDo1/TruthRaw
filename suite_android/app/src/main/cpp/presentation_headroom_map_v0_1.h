#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_headroom_map::v0_1 {

// Derived-presentation-only mapping that reserves 10% of the normalized SDR
// display range for highlights. This is the vectorized full-resolution form of
// the already researched 90/100 Appearance Highlight Headroom candidate:
// values at/below reference white are unchanged; values above reference white
// are compressed monotonically toward display peak.
//
// It never changes RAW BlackLevel/WhiteLevel, Zero-Line, Scientific Master,
// censoring or authority. Input and output are linear-light presentation RGB.
inline constexpr float kReferenceWhite = 0.90f;
inline constexpr float kDisplayPeak = 1.00f;
inline constexpr float kCompressionStrength = 1.00f;
inline constexpr float kLumaR = 0.2126f;
inline constexpr float kLumaG = 0.7152f;
inline constexpr float kLumaB = 0.0722f;
inline constexpr float kEpsilon = 1.0e-8f;

inline float luminance709(float r, float g, float b) noexcept {
    return kLumaR * r + kLumaG * g + kLumaB * b;
}

inline bool map_90_100(float& r, float& g, float& b) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
        return false;
    }

    const float y = luminance709(r, g, b);
    if (!std::isfinite(y)) return false;
    if (y <= kReferenceWhite) return true;
    if (y <= kEpsilon) return true;

    constexpr float headroom = kDisplayPeak - kReferenceWhite;
    const float excess = y - kReferenceWhite;
    const float mappedY =
        kReferenceWhite +
        headroom *
            (1.0f - std::exp(
                -excess / (headroom * kCompressionStrength)));
    if (!std::isfinite(mappedY) || mappedY < 0.0f || mappedY > kDisplayPeak) {
        return false;
    }

    const float scale = mappedY / y;
    if (!std::isfinite(scale) || scale < 0.0f) return false;
    r *= scale;
    g *= scale;
    b *= scale;
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_headroom_map::v0_1

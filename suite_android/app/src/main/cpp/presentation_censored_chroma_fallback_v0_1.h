#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_censored_chroma_fallback::v0_1 {

// APPEARANCE_ONLY fallback for chromaticity whose reconstruction support is
// explicitly CENSORED. This is not highlight recovery and never claims the
// neutral result is scene truth. It preserves Rec.709 luminance and contracts
// only the unsupported chroma component toward the D50-neutral presentation
// axis. Source-bound Warm Illuminant retention remains downstream.
//
// Authority rule:
// - fraction == 0: bit-preserving no-op;
// - strength depends ONLY on reconstruction-support CENSOR fraction;
// - no hue, camera/vendor identity, brightness or semantic detector;
// - no source/Scientific-Master mutation or scientific writeback.

constexpr float kCensorFractionStart = 0.15f;
constexpr float kCensorFractionFull = 0.85f;
constexpr float kMaxChromaContraction = 0.84f;
constexpr double kLumaR = 0.2126;
constexpr double kLumaG = 0.7152;
constexpr double kLumaB = 0.0722;

inline float smoothstep01(float x) noexcept {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

inline double luminance709(float r, float g, float b) noexcept {
    return kLumaR * static_cast<double>(r) +
           kLumaG * static_cast<double>(g) +
           kLumaB * static_cast<double>(b);
}

inline float contraction_amount(float censorFraction) noexcept {
    if (!std::isfinite(censorFraction) || censorFraction <= kCensorFractionStart) {
        return 0.0f;
    }
    const float t =
        (censorFraction - kCensorFractionStart) /
        (kCensorFractionFull - kCensorFractionStart);
    return kMaxChromaContraction * smoothstep01(t);
}

inline bool apply(
    float& r,
    float& g,
    float& b,
    float censorFraction) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b) ||
        !std::isfinite(censorFraction)) {
        return false;
    }

    const float f = std::clamp(censorFraction, 0.0f, 1.0f);
    const float amount = contraction_amount(f);
    if (amount <= 0.0f) return true;

    const double yd = luminance709(r, g, b);
    if (!std::isfinite(yd)) return false;
    const float y = static_cast<float>(yd);
    const float chromaScale = 1.0f - amount;

    r = y + (r - y) * chromaScale;
    g = y + (g - y) * chromaScale;
    b = y + (b - y) * chromaScale;

    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_censored_chroma_fallback::v0_1

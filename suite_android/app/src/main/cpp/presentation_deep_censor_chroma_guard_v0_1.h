#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_deep_censor_chroma_guard::v0_1 {

// APPEARANCE_ONLY deep-censor residual chroma guard.
//
// This stage exists only for regions whose reconstruction-support CENSOR
// fraction is already high enough that finite chromaticity is not trustworthy.
// It does not recover scene colour. It preserves Rec.709 luminance and contracts
// only the remaining unsupported chroma after the accepted Censored Chroma
// Fallback v0.1. Warm Illuminant retention remains downstream.
//
// Authority law:
// - fraction <= 0.50: exact no-op;
// - response is driven only by reconstruction-support CENSOR fraction;
// - no hue/purple detector, brightness threshold, object semantics, camera/vendor
//   identity, source mutation or Scientific-Master writeback;
// - this stage must remain downstream Appearance only.

constexpr float kCensorFractionStart = 0.50f;
constexpr float kCensorFractionFull = 0.80f;
constexpr float kMaxRemainingChromaContraction = 0.78f;
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
    return kMaxRemainingChromaContraction * smoothstep01(t);
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

} // namespace truthraw::presentation_deep_censor_chroma_guard::v0_1

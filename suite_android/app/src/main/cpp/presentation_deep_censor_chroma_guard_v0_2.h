#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_deep_censor_chroma_guard::v0_2 {

// APPEARANCE_ONLY deep-censor residual chroma guard with bounded micro-detail relief.
//
// v0.2 preserves the v0.1 CENSOR authority response for clearly chromatic residuals,
// while reducing only the *additional deep-guard contraction* for low-amplitude
// relative chroma. This retains subtle local colour texture without sharpening,
// blurring, spatial edge exceptions, hue detection or semantic inference.
//
// The accepted Censored Chroma Fallback v0.1 remains upstream and unchanged.
// Rec.709 luminance is preserved exactly by construction. Strong residual chroma
// (relative chroma >= kDetailReliefChromaRatioEnd) receives the exact v0.1
// contraction. PURE bypass and all scientific firewalls remain outside this file.

constexpr float kCensorFractionStart = 0.50f;
constexpr float kCensorFractionFull = 0.80f;
constexpr float kMaxRemainingChromaContraction = 0.78f;

// Detail relief applies only to low-amplitude chroma relative to local luminance.
// It never expands chroma and never weakens the upstream accepted fallback.
constexpr float kDetailReliefChromaRatioStart = 0.02f;
constexpr float kDetailReliefChromaRatioEnd = 0.12f;
constexpr float kMaxDetailRelief = 0.35f;
constexpr float kRelativeChromaLumaFloor = 0.05f;

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

inline float base_contraction_amount(float censorFraction) noexcept {
    if (!std::isfinite(censorFraction) || censorFraction <= kCensorFractionStart) {
        return 0.0f;
    }
    const float t =
        (censorFraction - kCensorFractionStart) /
        (kCensorFractionFull - kCensorFractionStart);
    return kMaxRemainingChromaContraction * smoothstep01(t);
}

inline float relative_chroma(float r, float g, float b, float y) noexcept {
    const float dr = r - y;
    const float dg = g - y;
    const float db = b - y;
    const float c2 = dr * dr + dg * dg + db * db;
    if (!std::isfinite(c2) || c2 < 0.0f) return 0.0f;
    const float chroma = std::sqrt(c2);
    const float denom = std::max(std::fabs(y), kRelativeChromaLumaFloor);
    return chroma / denom;
}

inline float detail_relief_fraction(float relChroma) noexcept {
    if (!std::isfinite(relChroma) || relChroma >= kDetailReliefChromaRatioEnd) {
        return 0.0f;
    }
    if (relChroma <= kDetailReliefChromaRatioStart) {
        return kMaxDetailRelief;
    }
    const float t =
        (relChroma - kDetailReliefChromaRatioStart) /
        (kDetailReliefChromaRatioEnd - kDetailReliefChromaRatioStart);
    return kMaxDetailRelief * (1.0f - smoothstep01(t));
}

inline float effective_contraction_amount(
    float censorFraction,
    float relChroma) noexcept {
    const float base = base_contraction_amount(censorFraction);
    if (base <= 0.0f) return 0.0f;
    const float relief = detail_relief_fraction(relChroma);
    return base * (1.0f - relief);
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
    const float baseAmount = base_contraction_amount(f);
    if (baseAmount <= 0.0f) return true;

    const double yd = luminance709(r, g, b);
    if (!std::isfinite(yd)) return false;
    const float y = static_cast<float>(yd);
    const float relChroma = relative_chroma(r, g, b, y);
    const float amount = effective_contraction_amount(f, relChroma);
    const float chromaScale = 1.0f - amount;

    r = y + (r - y) * chromaScale;
    g = y + (g - y) * chromaScale;
    b = y + (b - y) * chromaScale;

    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_deep_censor_chroma_guard::v0_2

#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_near_censor_chroma_shoulder::v0_1 {

// APPEARANCE_ONLY residual near-censor shoulder.
//
// Purpose:
// - cover the low reconstruction-support CENSOR-fraction gap below the accepted
//   Censored Chroma Fallback v0.1 start point;
// - retain Rec.709 luminance exactly while applying only a small chroma
//   contraction;
// - preserve spatial detail/acutance because no neighbourhood blur, resampling,
//   luminance modification or geometry change is performed.
//
// Authority rule:
// - driven only by reconstruction-support CENSOR fraction;
// - no hue/object/camera/vendor/brightness detector;
// - no source or Scientific-Master mutation;
// - no scientific recovery claim, promotion or writeback;
// - deliberately bounded to 12% maximum chroma contraction;
// - fades out as the already accepted full Censored Chroma Fallback gains
//   authority, so the combined response remains monotonic.

constexpr float kCensorFractionStart = 0.02f;
constexpr float kCensorFractionFullShoulder = 0.15f;
constexpr float kCensorFractionFadeStart = 0.35f;
constexpr float kCensorFractionFadeEnd = 0.55f;
constexpr float kMaxChromaContraction = 0.12f;
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
    const float f = std::clamp(censorFraction, 0.0f, 1.0f);
    const float rise = smoothstep01(
        (f - kCensorFractionStart) /
        (kCensorFractionFullShoulder - kCensorFractionStart));
    const float fade = 1.0f - smoothstep01(
        (f - kCensorFractionFadeStart) /
        (kCensorFractionFadeEnd - kCensorFractionFadeStart));
    return kMaxChromaContraction * rise * fade;
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

    const float amount = contraction_amount(censorFraction);
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

} // namespace truthraw::presentation_near_censor_chroma_shoulder::v0_1

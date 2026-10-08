#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_highlight_chroma_rolloff::v0_1 {

// Presentation-only near-white highlight chroma roll-off.
//
// Evidence boundary:
// - never changes sealed source values or Scientific Master;
// - never claims clipped/censored colour recovery;
// - only reduces display chroma near the finite white boundary;
// - is intended for downstream ADVANCED/PRO presentation output only.
//
// Rationale:
// a legal in-gamut RGB triplet can still carry unstable chroma very close to
// display white. A hard/common RGB shoulder preserves that chroma exactly, so
// a small channel imbalance can remain visible as a pink/purple highlight.
// This guard contracts only near-white chroma toward the Rec.709 neutral axis
// while preserving luminance. Strong saturated colours are protected by the
// minimum-channel gate. Censored pixels receive the stronger conservative
// contraction because their true highlight chromaticity is not known.

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

inline bool apply_near_neutral_rolloff(
    float& r,
    float& g,
    float& b,
    bool censored) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
        return false;
    }

    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const double yd = luminance709(r, g, b);
    if (!std::isfinite(yd)) return false;
    const float y = static_cast<float>(yd);

    // Leave normal tones and strongly coloured highlights untouched.
    if (mx <= 0.92f || y <= 0.78f || mn <= 0.65f) {
        return true;
    }

    const float maxGate = smoothstep01((mx - 0.92f) / 0.08f);
    const float lumaGate = smoothstep01((y - 0.78f) / 0.18f);
    const float minGate = smoothstep01((mn - 0.65f) / 0.25f);
    const float authorityStrength = censored ? 1.0f : 0.60f;
    const float amount = std::clamp(
        maxGate * lumaGate * minGate * authorityStrength,
        0.0f,
        1.0f);
    if (amount <= 1.0e-7f) return true;

    const float chromaScale = 1.0f - amount;
    r = y + (r - y) * chromaScale;
    g = y + (g - y) * chromaScale;
    b = y + (b - y) * chromaScale;
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_highlight_chroma_rolloff::v0_1

#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_highlight_chroma_rolloff::v0_1 {

// Presentation-only near-white / censored white-boundary chroma roll-off.
//
// Evidence boundary:
// - never changes sealed source values or Scientific Master;
// - never claims clipped/censored colour recovery;
// - only contracts display chroma at the finite white boundary;
// - is intended for downstream ADVANCED/PRO presentation output only.
//
// Rationale:
// a legal in-gamut RGB triplet can still carry unstable chroma very close to
// display white. A hard/common RGB shoulder preserves that chroma exactly, so
// a channel imbalance can remain visible as a pink/purple highlight.
//
// v0.1 originally protected only near-neutral highlights. Real-device evidence
// later exposed a second failure class: an actually censored bright sample can
// arrive at presentation with R and B both near the white boundary while G has
// collapsed far enough that the old minimum-channel gate deliberately skipped
// it. The true censored chromaticity is not known, so this file still performs
// no colour recovery. It adds only a bounded APPEARANCE contraction for that
// specific white-boundary/censored magenta-collapse signature. Non-censored
// saturated colours remain protected, and censored colours that do not match
// this signature retain the original behaviour.

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

inline void contract_chroma_preserve_luminance(
    float& r,
    float& g,
    float& b,
    float y,
    float amount) noexcept {
    const float chromaScale = 1.0f - std::clamp(amount, 0.0f, 1.0f);
    r = y + (r - y) * chromaScale;
    g = y + (g - y) * chromaScale;
    b = y + (b - y) * chromaScale;
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

    // Real-device censored-white regression guard.
    //
    // Only a censored sample can enter this extension. The two opponent R/B
    // channels must both sit at the white boundary, remain mutually balanced,
    // and G must show the characteristic collapse. This intentionally does not
    // turn into a blanket highlight desaturator: uncensored magenta, yellow,
    // cyan and other legitimate saturated colours retain their colour.
    if (censored && mx > 0.92f && y > 0.45f) {
        const float rbFloor = std::min(r, b);
        const float rbBalance = std::abs(r - b);
        const float greenDeficit = std::max(0.0f, 0.5f * (r + b) - g);

        const float whiteGate = smoothstep01((mx - 0.92f) / 0.08f);
        const float rbHighGate = smoothstep01((rbFloor - 0.86f) / 0.14f);
        const float rbBalanceGate =
            1.0f - smoothstep01(rbBalance / 0.28f);
        const float deficitGate =
            smoothstep01((greenDeficit - 0.10f) / 0.30f);
        const float lowGreenGate =
            smoothstep01((0.70f - g) / 0.22f);
        const float lowLumaGuard =
            smoothstep01((y - 0.45f) / 0.18f);

        const float severeAmount = std::clamp(
            0.95f * whiteGate * rbHighGate * rbBalanceGate *
                deficitGate * lowGreenGate * lowLumaGuard,
            0.0f,
            0.95f);
        if (severeAmount > 1.0e-7f) {
            contract_chroma_preserve_luminance(r, g, b, y, severeAmount);
            return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
        }
    }

    // Original v0.1 path: leave normal tones and strongly coloured highlights
    // untouched unless the censored-white signature above was proven.
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

    contract_chroma_preserve_luminance(r, g, b, y, amount);
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_highlight_chroma_rolloff::v0_1

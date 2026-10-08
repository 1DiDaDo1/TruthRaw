#pragma once

#include <algorithm>
#include <cmath>

namespace truthraw::presentation_natural_light_field_tone::v0_1 {

inline float clamp01(float x) noexcept {
    return std::clamp(x, 0.0f, 1.0f);
}

inline float smoothstep01(float x) noexcept {
    x = clamp01(x);
    return x * x * (3.0f - 2.0f * x);
}

inline float luminance709(float r, float g, float b) noexcept {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Presentation-only local luminous-field tone response.
//
// This is deliberately NOT a physical light-transport solver. localFieldY is
// an image-space neighbourhood statistic from the already rendered observation.
// It may support a natural-looking View/Appearance, but it never proves source
// intensity, geometry, reflectance, spectrum, bounce light or scene-light kind.
//
// Safety:
// - Natural Light must explicitly enable the stage;
// - censored pixels are never lifted/recovered;
// - deep blacks and near-white highlights are protected;
// - the operation is a common RGB gain, so hue/chroma ratios are preserved;
// - gain is capped at +0.14 EV;
// - no source/Scientific-Master mutation, evidence creation or authority change.
inline float requested_ev(
    float pixelY,
    float localFieldY,
    bool censored,
    bool enabled) noexcept {
    if (!enabled || censored) return 0.0f;
    if (!std::isfinite(pixelY) || !std::isfinite(localFieldY)) return 0.0f;

    pixelY = std::max(pixelY, 0.0f);
    localFieldY = clamp01(localFieldY);

    // A visibly luminous local field may receive a small brightness-preserving
    // appearance lift. Dark fields do not receive a generic exposure boost.
    const float fieldGate = smoothstep01((localFieldY - 0.12f) / 0.36f);

    // Do not turn noise-floor/black regions into invented illumination.
    const float blackProtect = smoothstep01((pixelY - 0.035f) / 0.125f);

    // Keep the existing highlight/headroom/chroma guards authoritative.
    const float highlightProtect =
        1.0f - smoothstep01((pixelY - 0.62f) / 0.28f);

    const float amount = clamp01(fieldGate * blackProtect * highlightProtect);
    return 0.14f * amount;
}

inline bool apply(
    float& r,
    float& g,
    float& b,
    float localFieldY,
    bool censored,
    bool enabled) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) return false;
    if (!enabled || censored || !std::isfinite(localFieldY)) return true;

    const float pixelY = luminance709(r, g, b);
    const float ev = requested_ev(pixelY, localFieldY, censored, enabled);
    if (!(ev > 1.0e-7f)) return true;

    const float gain = std::exp2(ev);
    if (!std::isfinite(gain) || gain < 1.0f || gain > std::exp2(0.14001f)) {
        return false;
    }

    r *= gain;
    g *= gain;
    b *= gain;
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

}  // namespace truthraw::presentation_natural_light_field_tone::v0_1

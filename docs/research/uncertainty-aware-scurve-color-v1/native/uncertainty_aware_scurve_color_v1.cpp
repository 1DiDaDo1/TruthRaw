#include "uncertainty_aware_scurve_color_v1.h"

#include <algorithm>
#include <cmath>

namespace truthraw::appearance::v1 {
namespace {
constexpr float kWr = 0.2126f;
constexpr float kWg = 0.7152f;
constexpr float kWb = 0.0722f;
constexpr float kEps = 1.0e-8f;

float clamp01(float x) noexcept {
    return std::clamp(x, 0.0f, 1.0f);
}

bool finite_rgb(const RgbLinearV1& x) noexcept {
    return std::isfinite(x.r) && std::isfinite(x.g) && std::isfinite(x.b);
}

float smoothstep01(float x) noexcept {
    const float t = clamp01(x);
    return t * t * (3.0f - 2.0f * t);
}

RgbLinearV1 clamp_rgb01(const RgbLinearV1& x) noexcept {
    return {clamp01(x.r), clamp01(x.g), clamp01(x.b)};
}
}  // namespace

float luminance709(const RgbLinearV1& rgb) noexcept {
    return kWr * rgb.r + kWg * rgb.g + kWb * rgb.b;
}

float s_curve01(float x, float strength) noexcept {
    const float t = clamp01(x);
    const float y = t + strength * t * (1.0f - t) * (2.0f * t - 1.0f);
    return clamp01(y);
}

bool validate_config(const AppearanceConfigV1& c) noexcept {
    if (!std::isfinite(c.globalCurveStrength) ||
        !std::isfinite(c.lowConfidenceShadowExtra) ||
        !std::isfinite(c.shadowPivot) ||
        !std::isfinite(c.lowConfidenceChromaGain) ||
        !std::isfinite(c.highConfidenceChromaGain) ||
        !std::isfinite(c.censoredMaxChromaGain)) {
        return false;
    }
    if (c.globalCurveStrength < 0.0f || c.globalCurveStrength > 0.8f) return false;
    if (c.lowConfidenceShadowExtra < 0.0f ||
        c.globalCurveStrength + c.lowConfidenceShadowExtra > 0.8f) return false;
    if (c.shadowPivot <= 0.0f || c.shadowPivot > 0.5f) return false;
    if (c.lowConfidenceChromaGain < 0.0f || c.lowConfidenceChromaGain > 1.0f) return false;
    if (c.highConfidenceChromaGain < 1.0f || c.highConfidenceChromaGain > 1.5f) return false;
    if (c.censoredMaxChromaGain < 0.0f || c.censoredMaxChromaGain > 1.0f) return false;
    return true;
}

AppearanceResultV1 apply_pixel(
    const RgbLinearV1& input,
    const AppearanceEvidenceV1& e,
    const AppearanceConfigV1& c) noexcept {
    AppearanceResultV1 out{
        AppearanceStatusV1::InvalidInput,
        input,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        false};

    if (!validate_config(c)) {
        out.status = AppearanceStatusV1::InvalidConfig;
        return out;
    }
    if (!finite_rgb(input) || input.r < 0.0f || input.g < 0.0f || input.b < 0.0f ||
        input.r > 1.0f || input.g > 1.0f || input.b > 1.0f ||
        !std::isfinite(e.lumaConfidence) || !std::isfinite(e.chromaConfidence) ||
        e.lumaConfidence < 0.0f || e.lumaConfidence > 1.0f ||
        e.chromaConfidence < 0.0f || e.chromaConfidence > 1.0f) {
        return out;
    }

    const float y = clamp01(luminance709(input));
    out.inputLuminance = y;

    // Confidence only modulates the tone curve inside the shadow zone. Above
    // shadowPivot every pixel uses the same global monotonic curve. This avoids
    // turning uncertain midtone differences into extra local contrast.
    const float shadowWeight = 1.0f - smoothstep01(y / c.shadowPivot);
    const float uncertainty = 1.0f - e.lumaConfidence;
    const float strength = c.globalCurveStrength +
        c.lowConfidenceShadowExtra * shadowWeight * uncertainty;
    out.effectiveCurveStrength = strength;

    const float yTone = s_curve01(y, strength);
    const float toneScale = (y > kEps) ? (yTone / y) : 0.0f;
    RgbLinearV1 tone{
        input.r * toneScale,
        input.g * toneScale,
        input.b * toneScale};

    // Scaling all RGB channels together preserves chromaticity before gamut
    // clipping. Chroma is then adjusted only along the RGB vector around the
    // neutral axis at fixed luminance, which changes colorfulness without
    // rotating that chroma direction.
    const float q = smoothstep01(e.chromaConfidence);
    float chromaGain = c.lowConfidenceChromaGain +
        q * (c.highConfidenceChromaGain - c.lowConfidenceChromaGain);
    if (e.sourceHighCensored) {
        chromaGain = std::min(chromaGain, c.censoredMaxChromaGain);
    }
    out.effectiveChromaGain = chromaGain;

    const float neutral = yTone;
    RgbLinearV1 result{
        neutral + (tone.r - neutral) * chromaGain,
        neutral + (tone.g - neutral) * chromaGain,
        neutral + (tone.b - neutral) * chromaGain};

    out.outputLuminanceBeforeClamp = luminance709(result);
    out.rgb = clamp_rgb01(result);
    out.status = AppearanceStatusV1::Applied;
    return out;
}

}  // namespace truthraw::appearance::v1

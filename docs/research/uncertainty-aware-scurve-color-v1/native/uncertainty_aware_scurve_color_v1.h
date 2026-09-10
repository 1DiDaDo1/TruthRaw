#pragma once

#include <cstdint>

namespace truthraw::appearance::v1 {

enum class AppearanceStatusV1 : std::uint8_t {
    Applied = 0,
    InvalidInput = 1,
    InvalidConfig = 2
};

struct RgbLinearV1 {
    float r;
    float g;
    float b;
};

struct AppearanceEvidenceV1 {
    // Upstream, already-calibrated/qualified confidence scalars in [0,1].
    // This module never derives measurement confidence from RGB values.
    float lumaConfidence;
    float chromaConfidence;
    bool sourceHighCensored;
};

struct AppearanceConfigV1 {
    // Polynomial S-curve strength on display-linear normalized RGB after
    // exposure/shoulder/gamut preparation. 0 = identity; values <= 0.8 are
    // kept comfortably inside the monotonic range for x in [0,1].
    float globalCurveStrength = 0.42f;

    // Extra shadow compression available only in low-confidence shadows.
    // This is appearance-only and must be fed a spatially regularized
    // confidence field by the caller when used on images.
    float lowConfidenceShadowExtra = 0.12f;
    float shadowPivot = 0.18f;

    // Chroma gain interpolates between restraint and modest enhancement.
    float lowConfidenceChromaGain = 0.82f;
    float highConfidenceChromaGain = 1.25f;

    // Censored source support cannot receive positive chroma enhancement.
    float censoredMaxChromaGain = 1.0f;
};

struct AppearanceResultV1 {
    AppearanceStatusV1 status;
    RgbLinearV1 rgb;
    float inputLuminance;
    float outputLuminanceBeforeClamp;
    float effectiveCurveStrength;
    float effectiveChromaGain;
    bool scientificMasterModified;
};

// Rec.709/sRGB linear luminance weights. This module is an appearance
// transform and assumes RGB is already in the intended linear display space.
float luminance709(const RgbLinearV1& rgb) noexcept;

// Monotonic polynomial S-curve for x in [0,1] when 0 <= strength < 1.
float s_curve01(float x, float strength) noexcept;

bool validate_config(const AppearanceConfigV1& config) noexcept;

// Applies the appearance transform to one display-linear pixel. The input is
// passed by value/const data and the function has no route to the scientific
// Scene Master. Invalid inputs fail closed to the original RGB value.
AppearanceResultV1 apply_pixel(
    const RgbLinearV1& displayLinearRgb,
    const AppearanceEvidenceV1& evidence,
    const AppearanceConfigV1& config = {}) noexcept;

}  // namespace truthraw::appearance::v1

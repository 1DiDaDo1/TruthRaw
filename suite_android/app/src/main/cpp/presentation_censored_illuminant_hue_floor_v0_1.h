#pragma once

#include <algorithm>
#include <cmath>

#include "presentation_illuminant_warmth_retention_v0_1.h"

namespace truthraw::presentation_censored_illuminant_hue_floor::v0_1 {

// APPEARANCE_ONLY source-white hue floor for nearly neutral, deeply censored
// highlights after purple/magenta suppression.
//
// It does not recover scene colour. It only prevents a warm source-white
// appearance from collapsing all the way to neutral gray/white after the
// authority-bound deep-censor chroma guards.
//
// Hard boundaries:
// - source-bound white point must already be valid;
// - the existing Warm Illuminant CCT gate must admit warmth;
// - high CENSOR authority is required;
// - only bright, nearly neutral output is eligible;
// - clearly chromatic output is untouched;
// - Rec.709 luminance is preserved by construction;
// - no hue/object/semantic detector, camera/vendor identity, sharpening, blur,
//   resampling, source mutation, Scientific-Master writeback or new evidence.
//
// PURE bypass is enforced by the caller. Final gamut fit remains downstream.

namespace warm = truthraw::presentation_illuminant_warmth_retention::v0_1;
using SourceWhitePoint = warm::SourceWhitePoint;

constexpr float kCensorFractionStart = 0.50f;
constexpr float kCensorFractionFull = 0.80f;
constexpr float kLumaStart = 0.72f;
constexpr float kLumaFull = 0.95f;
constexpr float kNeutralRelativeChromaFull = 0.015f;
constexpr float kNeutralRelativeChromaEnd = 0.090f;
constexpr float kMaxRelativeChromaFloor = 0.060f;
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

inline float censor_gate(float f) noexcept {
    if (!std::isfinite(f) || f <= kCensorFractionStart) return 0.0f;
    if (f >= kCensorFractionFull) return 1.0f;
    return smoothstep01(
        (f - kCensorFractionStart) /
        (kCensorFractionFull - kCensorFractionStart));
}

inline float luma_gate(float y) noexcept {
    if (!std::isfinite(y) || y <= kLumaStart) return 0.0f;
    if (y >= kLumaFull) return 1.0f;
    return smoothstep01((y - kLumaStart) / (kLumaFull - kLumaStart));
}

inline float relative_chroma(float r, float g, float b, float y) noexcept {
    const float dr = r - y;
    const float dg = g - y;
    const float db = b - y;
    const float c2 = dr * dr + dg * dg + db * db;
    if (!std::isfinite(c2) || c2 < 0.0f) return 0.0f;
    return std::sqrt(c2) /
           std::max(std::fabs(y), kRelativeChromaLumaFloor);
}

inline float neutral_gate(float relChroma) noexcept {
    if (!std::isfinite(relChroma) ||
        relChroma >= kNeutralRelativeChromaEnd) {
        return 0.0f;
    }
    if (relChroma <= kNeutralRelativeChromaFull) return 1.0f;
    const float t =
        (relChroma - kNeutralRelativeChromaFull) /
        (kNeutralRelativeChromaEnd - kNeutralRelativeChromaFull);
    return 1.0f - smoothstep01(t);
}

inline float warm_gate(const SourceWhitePoint& white) noexcept {
    if (!warm::valid_white(white)) return 0.0f;
    const float strength =
        warm::retention_strength(white.correlatedColorTemperatureK);
    if (!(strength > 0.0f)) return 0.0f;
    return std::clamp(strength / 0.18f, 0.0f, 1.0f);
}

inline bool source_white_direction(
    const SourceWhitePoint& white,
    float& dr,
    float& dg,
    float& db) noexcept {
    dr = dg = db = 0.0f;
    if (!warm::valid_white(white)) return true;

    const double X = white.x / white.y;
    const double Y = 1.0;
    const double Z = (1.0 - white.x - white.y) / white.y;
    if (!std::isfinite(X) || !std::isfinite(Z) ||
        !(X > 0.0) || !(Z > 0.0)) {
        return true;
    }

    constexpr double m00 =  3.1338561;
    constexpr double m01 = -1.6168667;
    constexpr double m02 = -0.4906146;
    constexpr double m10 = -0.9787684;
    constexpr double m11 =  1.9161415;
    constexpr double m12 =  0.0334540;
    constexpr double m20 =  0.0719453;
    constexpr double m21 = -0.2289914;
    constexpr double m22 =  1.4052427;

    double rr = m00 * X + m01 * Y + m02 * Z;
    double gg = m10 * X + m11 * Y + m12 * Z;
    double bb = m20 * X + m21 * Y + m22 * Z;
    const double yy = kLumaR * rr + kLumaG * gg + kLumaB * bb;
    if (!std::isfinite(rr) || !std::isfinite(gg) ||
        !std::isfinite(bb) || !std::isfinite(yy) ||
        !(yy > 1.0e-12)) {
        return true;
    }

    rr /= yy;
    gg /= yy;
    bb /= yy;
    double drr = rr - 1.0;
    double dgg = gg - 1.0;
    double dbb = bb - 1.0;
    const double n2 = drr * drr + dgg * dgg + dbb * dbb;
    if (!std::isfinite(n2) || !(n2 > 1.0e-12)) return true;

    const double inv = 1.0 / std::sqrt(n2);
    drr *= inv;
    dgg *= inv;
    dbb *= inv;

    if (!(drr > 0.0) || !(dbb < 0.0)) return true;

    dr = static_cast<float>(drr);
    dg = static_cast<float>(dgg);
    db = static_cast<float>(dbb);
    return std::isfinite(dr) && std::isfinite(dg) && std::isfinite(db);
}

inline bool apply(
    float& r,
    float& g,
    float& b,
    const SourceWhitePoint& white,
    float censorFraction,
    bool enabled) noexcept {
    if (!enabled) return true;
    if (!std::isfinite(r) || !std::isfinite(g) ||
        !std::isfinite(b) || !std::isfinite(censorFraction)) {
        return false;
    }

    const float wg = warm_gate(white);
    if (!(wg > 0.0f)) return true;

    const double yd = luminance709(r, g, b);
    if (!std::isfinite(yd)) return false;
    const float y = static_cast<float>(yd);

    const float cg =
        censor_gate(std::clamp(censorFraction, 0.0f, 1.0f));
    const float lg = luma_gate(y);
    if (!(cg > 0.0f) || !(lg > 0.0f)) return true;

    const float rel = relative_chroma(r, g, b, y);
    const float ng = neutral_gate(rel);
    if (!(ng > 0.0f)) return true;

    float dirR = 0.0f, dirG = 0.0f, dirB = 0.0f;
    if (!source_white_direction(white, dirR, dirG, dirB)) return false;
    if (dirR == 0.0f && dirG == 0.0f && dirB == 0.0f) return true;

    const float targetRel =
        kMaxRelativeChromaFloor * wg * cg * lg * ng;
    if (!(targetRel > 0.0f) || rel >= targetRel) return true;

    const float denom =
        std::max(std::fabs(y), kRelativeChromaLumaFloor);
    const float targetAbs = targetRel * denom;
    const float currentAbs = rel * denom;
    const float blend =
        std::clamp(
            (targetAbs - currentAbs) /
            std::max(targetAbs, 1.0e-12f),
            0.0f, 1.0f);

    const float targetR = y + dirR * targetAbs;
    const float targetG = y + dirG * targetAbs;
    const float targetB = y + dirB * targetAbs;

    r += (targetR - r) * blend;
    g += (targetG - g) * blend;
    b += (targetB - b) * blend;

    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_censored_illuminant_hue_floor::v0_1

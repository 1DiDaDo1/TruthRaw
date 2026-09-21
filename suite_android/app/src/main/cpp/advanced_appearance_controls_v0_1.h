#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace truthraw::advanced_appearance_controls::v0_1 {

inline constexpr std::uint32_t kFlagLight = 1u << 0u;
inline constexpr std::uint32_t kFlagHdr = 1u << 1u;
inline constexpr std::uint32_t kFlagDetail = 1u << 2u;
inline constexpr std::uint32_t kFlagRestoration = 1u << 3u;
inline constexpr std::uint32_t kFlagColorControl = 1u << 4u;
inline constexpr std::uint32_t kFlagExposureControl = 1u << 5u;

inline constexpr std::uint32_t kShadowStrengthShift = 6u;
inline constexpr std::uint32_t kShadowStrengthMask = 0x3u << kShadowStrengthShift;
inline constexpr std::uint32_t kDetailStrengthShift = 8u;
inline constexpr std::uint32_t kDetailStrengthMask = 0x7fu << kDetailStrengthShift;
inline constexpr std::uint32_t kColorFullnessShift = 16u;
inline constexpr std::uint32_t kColorFullnessMask = 0x7fu << kColorFullnessShift;
inline constexpr std::uint32_t kExposureShift = 24u;
inline constexpr std::uint32_t kExposureMask = 0x7fu << kExposureShift;

inline constexpr std::uint32_t kAllowedFlags =
    kFlagLight | kFlagHdr | kFlagDetail | kFlagRestoration | kFlagColorControl |
    kFlagExposureControl | kShadowStrengthMask |
    kDetailStrengthMask | kColorFullnessMask | kExposureMask;

inline int detail_strength_percent(std::uint32_t flags) noexcept {
    if ((flags & kFlagDetail) == 0u) return 0;
    const int encoded = static_cast<int>(
        (flags & kDetailStrengthMask) >> kDetailStrengthShift);
    // Legacy v0.84.2 DETAIL flag had no packed strength and meant full strength.
    return encoded == 0 ? 100 : std::clamp(encoded, 1, 100);
}

inline float detail_mix(std::uint32_t flags) noexcept {
    return static_cast<float>(detail_strength_percent(flags)) / 100.0f;
}

inline int color_fullness(std::uint32_t flags) noexcept {
    if ((flags & kFlagColorControl) == 0u) return 0;
    const int encoded = std::clamp(
        static_cast<int>((flags & kColorFullnessMask) >> kColorFullnessShift),
        0,
        100);
    return encoded - 50;
}

inline float color_fullness_mix(std::uint32_t flags) noexcept {
    return static_cast<float>(color_fullness(flags)) / 50.0f;
}

inline float exposure_compensation_ev(std::uint32_t flags) noexcept {
    if ((flags & kFlagExposureControl) == 0u) return 0.0f;
    const int encoded = std::clamp(
        static_cast<int>((flags & kExposureMask) >> kExposureShift),
        0,
        100);
    return (static_cast<float>(encoded) - 50.0f) * 0.04f;
}

inline float shadow_recovery_mix(std::uint32_t flags) noexcept {
    const int encoded = static_cast<int>(
        (flags & kShadowStrengthMask) >> kShadowStrengthShift);
    return static_cast<float>(encoded) / 3.0f;
}

inline float clamp01(float x) noexcept {
    return std::max(0.0f, std::min(1.0f, x));
}

inline float smoothstep01(float x) noexcept {
    x = clamp01(x);
    return x * x * (3.0f - 2.0f * x);
}

// Conservative presentation-only auto exposure for under-dark rendered scenes.
// It never changes RAW black/white calibration or Scientific Master values.
inline float auto_dark_exposure_ev(
    float renderedMedian,
    float evidenceConfidence) noexcept {
    if (!std::isfinite(renderedMedian) ||
        !std::isfinite(evidenceConfidence) ||
        renderedMedian <= 0.0f) {
        return 0.0f;
    }
    const float confidence = clamp01(evidenceConfidence);
    const float darkGate =
        1.0f - smoothstep01((renderedMedian - 0.055f) / (0.135f - 0.055f));
    if (darkGate <= 0.0f) return 0.0f;
    const float desired = 0.145f;
    const float rawEv = std::log2(desired / std::max(renderedMedian, 0.025f));
    return std::clamp(rawEv, 0.0f, 0.70f) * darkGate * confidence;
}

inline float presentation_exposure_gain(
    std::uint32_t flags,
    float renderedMedian,
    float evidenceConfidence,
    bool naturalLightEnabled) noexcept {
    float ev = exposure_compensation_ev(flags);
    if (naturalLightEnabled) {
        ev += auto_dark_exposure_ev(renderedMedian, evidenceConfidence);
    }
    ev = std::clamp(ev, -2.0f, 2.70f);
    return std::exp2(ev);
}

inline bool apply_shadow_recovery(
    float& r,
    float& g,
    float& b,
    float amount,
    float evidenceConfidence) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b) ||
        !std::isfinite(amount) || !std::isfinite(evidenceConfidence)) {
        return false;
    }
    amount = clamp01(amount);
    if (amount <= 1.0e-7f) return true;

    const float y = std::max(0.2126f * r + 0.7152f * g + 0.0722f * b, 0.0f);
    const float darkGate = 1.0f - smoothstep01((y - 0.025f) / 0.34f);
    const float blackProtect = smoothstep01(y / 0.018f);
    const float strength =
        0.48f * amount * clamp01(evidenceConfidence) * darkGate * blackProtect;
    const float scale = 1.0f + strength;
    r *= scale;
    g *= scale;
    b *= scale;
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

// Appearance-only, luminance-preserving colour-fullness control.
// Positive values behave vibrance-like: already saturated colours receive less
// additional chroma. Negative values gently reduce chroma. No semantic
// segmentation, hue-specific rule, clipping, evidence promotion or writeback.
inline bool apply_color_fullness(
    float& r,
    float& g,
    float& b,
    float amount) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b) ||
        !std::isfinite(amount)) {
        return false;
    }
    amount = std::clamp(amount, -1.0f, 1.0f);
    if (std::abs(amount) <= 1.0e-7f) return true;

    constexpr float wr = 0.2126f;
    constexpr float wg = 0.7152f;
    constexpr float wb = 0.0722f;
    const float y = wr * r + wg * g + wb * b;

    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const float chroma = std::max(mx - mn, 0.0f);
    const float saturationProxy =
        mx > 1.0e-8f ? clamp01(chroma / mx) : 0.0f;

    float chromaScale = 1.0f;
    if (amount > 0.0f) {
        // Protect already-vivid colours and very bright extended-linear values.
        const float saturationGuard = 1.0f - 0.70f * saturationProxy;
        const float highlightGuard =
            1.0f - 0.30f * smoothstep01((y - 0.70f) / 0.60f);
        chromaScale += 0.34f * amount * saturationGuard * highlightGuard;
    } else {
        chromaScale += 0.30f * amount;
    }

    r = y + (r - y) * chromaScale;
    g = y + (g - y) * chromaScale;
    b = y + (b - y) * chromaScale;
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

}  // namespace truthraw::advanced_appearance_controls::v0_1

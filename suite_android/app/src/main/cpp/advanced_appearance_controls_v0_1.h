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

inline constexpr std::uint32_t kDetailStrengthShift = 8u;
inline constexpr std::uint32_t kDetailStrengthMask = 0x7fu << kDetailStrengthShift;
inline constexpr std::uint32_t kColorFullnessShift = 16u;
inline constexpr std::uint32_t kColorFullnessMask = 0x7fu << kColorFullnessShift;

inline constexpr std::uint32_t kAllowedFlags =
    kFlagLight | kFlagHdr | kFlagDetail | kFlagRestoration | kFlagColorControl |
    kDetailStrengthMask | kColorFullnessMask;

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

inline float clamp01(float x) noexcept {
    return std::max(0.0f, std::min(1.0f, x));
}

inline float smoothstep01(float x) noexcept {
    x = clamp01(x);
    return x * x * (3.0f - 2.0f * x);
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

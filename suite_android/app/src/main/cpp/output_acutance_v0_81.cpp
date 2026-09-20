#include "output_acutance_v0_81.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>

namespace truthraw::output_acutance_v0_81 {
namespace {

constexpr float kLegacyHdrBlend = 0.68f;
constexpr float kLegacyMaxHalfLogGain = 2.0f;
constexpr float kMaximumDisplayGain =
    1.0f + kLegacyHdrBlend * ((1u << 2u) - 1.0f);

float luminance_nonnegative(const std::vector<float>& rgb, std::size_t i) noexcept {
    return truthraw_v47k::luminance709(
        rgb[3u * i],
        rgb[3u * i + 1u],
        rgb[3u * i + 2u]);
}

bool finite_nonnegative(float value) noexcept {
    return std::isfinite(value) && value >= 0.0f;
}

}  // namespace

float legacy_display_gain_from_half_log(float halfLogGain) noexcept {
    if (!std::isfinite(halfLogGain)) return 1.0f;
    const float lg = std::clamp(halfLogGain, 0.0f, kLegacyMaxHalfLogGain);
    const float rawGain = std::exp2(lg);
    return 1.0f + kLegacyHdrBlend * (rawGain - 1.0f);
}

bool apply_final_resize_acutance_and_rebase_hdr(
    const std::vector<float>& resizedLinearSdrBase,
    int width,
    int height,
    float noiseSigmaAt2Pct,
    float resizeRatio,
    truthraw_v47k::OutputProfile profile,
    bool hdrEnabled,
    const std::vector<float>& halfLogGain,
    const std::vector<std::uint8_t>& censorMask,
    std::vector<float>& acutanceAdjustedSdrBase,
    std::vector<float>& rebasedDisplayGain,
    Result& result) noexcept {
    result = Result{};
    result.profile = profile;

    if (width <= 0 || height <= 0 ||
        !std::isfinite(noiseSigmaAt2Pct) || noiseSigmaAt2Pct < 0.0f ||
        !std::isfinite(resizeRatio) || resizeRatio < 1.0f) {
        return false;
    }

    const std::size_t pixels =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (pixels == 0u ||
        resizedLinearSdrBase.size() != 3u * pixels ||
        halfLogGain.size() != pixels ||
        censorMask.size() != pixels) {
        return false;
    }

    for (const float value : resizedLinearSdrBase) {
        if (!std::isfinite(value)) return false;
    }

    result.plan = truthraw_v47k::choose_output_acutance_plan(
        noiseSigmaAt2Pct, resizeRatio, profile);

    acutanceAdjustedSdrBase.resize(3u * pixels);
    if (!truthraw_v47k::apply_output_acutance(
            resizedLinearSdrBase.data(),
            width,
            height,
            result.plan,
            acutanceAdjustedSdrBase.data())) {
        acutanceAdjustedSdrBase.clear();
        return false;
    }

    result.changedPixels = 0u;
    for (std::size_t i = 0u; i < pixels; ++i) {
        const bool changed =
            std::bit_cast<std::uint32_t>(resizedLinearSdrBase[3u * i]) !=
                std::bit_cast<std::uint32_t>(acutanceAdjustedSdrBase[3u * i]) ||
            std::bit_cast<std::uint32_t>(resizedLinearSdrBase[3u * i + 1u]) !=
                std::bit_cast<std::uint32_t>(acutanceAdjustedSdrBase[3u * i + 1u]) ||
            std::bit_cast<std::uint32_t>(resizedLinearSdrBase[3u * i + 2u]) !=
                std::bit_cast<std::uint32_t>(acutanceAdjustedSdrBase[3u * i + 2u]);
        if (changed) ++result.changedPixels;
    }

    rebasedDisplayGain.assign(pixels, 1.0f);
    result.applied = true;
    result.hdrRebased = hdrEnabled;
    result.hdrRebasedPixels = 0u;
    result.maxEffectiveHdrTargetAbsError = 0.0f;

    if (!hdrEnabled) return true;

    for (std::size_t i = 0u; i < pixels; ++i) {
        if (censorMask[i] != 0u) {
            rebasedDisplayGain[i] = 1.0f;
            continue;
        }

        const float beforeY = luminance_nonnegative(resizedLinearSdrBase, i);
        const float afterY = luminance_nonnegative(acutanceAdjustedSdrBase, i);
        if (!finite_nonnegative(beforeY) || !finite_nonnegative(afterY)) return false;

        const float legacyGain = legacy_display_gain_from_half_log(halfLogGain[i]);
        const float effectiveTarget = beforeY * legacyGain;

        float rebasedGain = 1.0f;
        if (afterY > 1e-8f && effectiveTarget > afterY) {
            rebasedGain = std::clamp(
                effectiveTarget / afterY,
                1.0f,
                kMaximumDisplayGain);
        }
        if (!std::isfinite(rebasedGain)) return false;
        rebasedDisplayGain[i] = rebasedGain;
        if (rebasedGain > 1.0f + 1e-5f) ++result.hdrRebasedPixels;

        const float realizedTarget = afterY * rebasedGain;
        result.maxEffectiveHdrTargetAbsError = std::max(
            result.maxEffectiveHdrTargetAbsError,
            std::abs(realizedTarget - effectiveTarget));
    }

    return true;
}

}  // namespace truthraw::output_acutance_v0_81

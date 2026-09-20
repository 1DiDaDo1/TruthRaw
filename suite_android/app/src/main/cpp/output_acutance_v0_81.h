#pragma once

#include "output_acutance.h"

#include <cstdint>
#include <vector>

namespace truthraw::output_acutance_v0_81 {

struct Result final {
    truthraw_v47k::OutputAcutancePlan plan{};
    truthraw_v47k::OutputProfile profile = truthraw_v47k::OutputProfile::Neutral;
    bool applied = false;
    bool hdrRebased = false;
    std::uint64_t changedPixels = 0u;
    std::uint64_t hdrRebasedPixels = 0u;
    float maxEffectiveHdrTargetAbsError = 0.0f;
};

float legacy_display_gain_from_half_log(float halfLogGain) noexcept;

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
    Result& result) noexcept;

}  // namespace truthraw::output_acutance_v0_81

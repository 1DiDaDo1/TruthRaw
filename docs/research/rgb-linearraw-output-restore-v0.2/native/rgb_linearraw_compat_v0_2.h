#pragma once

#include <cstdint>

namespace truthraw::rgb_linearraw_restore::v0_2 {

struct CompatibilityWindow final {
    double scale = 1.0;
    double baselineExposureEv = 0.0;
};

enum class WindowStatus : std::uint8_t {
    Ok = 0,
    InvalidMaximum,
    SceneExceedsValidatedWindow,
};

// Selects only historically exercised finite LinearRaw windows.
// No new scientific scale is created: this is a downstream DNG representation
// choice and BaselineExposure compensates the finite encoding window.
WindowStatus choose_compatibility_window(double maximumPositiveSceneValue,
                                         CompatibilityWindow& out) noexcept;

// Unsigned 16-bit finite representation. Negative values clip at zero and
// values above the selected compatibility window clip at 65535. Callers keep
// explicit clipping counters; this function never changes scientific state.
std::uint16_t encode_u16(double sceneValue,
                         const CompatibilityWindow& window,
                         bool& clippedLow,
                         bool& clippedHigh,
                         bool& valid) noexcept;

double decode_u16(std::uint16_t code,
                  const CompatibilityWindow& window) noexcept;

}  // namespace truthraw::rgb_linearraw_restore::v0_2

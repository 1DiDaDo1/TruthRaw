#include "rgb_linearraw_compat_v0_2.h"

#include <algorithm>
#include <cmath>

namespace truthraw::rgb_linearraw_restore::v0_2 {

WindowStatus choose_compatibility_window(double maximumPositiveSceneValue,
                                         CompatibilityWindow& out) noexcept {
    out = {};
    if (!std::isfinite(maximumPositiveSceneValue) || maximumPositiveSceneValue < 0.0) {
        return WindowStatus::InvalidMaximum;
    }

    if (maximumPositiveSceneValue <= 1.0) {
        out.scale = 1.0;
    } else if (maximumPositiveSceneValue <= 1.25) {
        out.scale = 1.25;
    } else if (maximumPositiveSceneValue <= 2.0) {
        out.scale = 2.0;
    } else {
        return WindowStatus::SceneExceedsValidatedWindow;
    }

    out.baselineExposureEv = std::log2(out.scale);
    return WindowStatus::Ok;
}

std::uint16_t encode_u16(double sceneValue,
                         const CompatibilityWindow& window,
                         bool& clippedLow,
                         bool& clippedHigh,
                         bool& valid) noexcept {
    clippedLow = false;
    clippedHigh = false;
    valid = std::isfinite(sceneValue) && std::isfinite(window.scale) && window.scale > 0.0;
    if (!valid) return 0u;

    double bounded = sceneValue;
    if (bounded < 0.0) {
        bounded = 0.0;
        clippedLow = true;
    } else if (bounded > window.scale) {
        bounded = window.scale;
        clippedHigh = true;
    }

    const double normalized = bounded / window.scale;
    const double q = std::floor(normalized * 65535.0 + 0.5);
    return static_cast<std::uint16_t>(std::clamp(q, 0.0, 65535.0));
}

double decode_u16(std::uint16_t code,
                  const CompatibilityWindow& window) noexcept {
    if (!std::isfinite(window.scale) || window.scale <= 0.0) return 0.0;
    return (static_cast<double>(code) / 65535.0) * window.scale;
}

}  // namespace truthraw::rgb_linearraw_restore::v0_2

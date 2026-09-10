#include "xyz_uncertainty_adapter_v1.h"
#include <cmath>
#include <limits>

namespace truthraw_scurve_v1 {
StatusV1 extract_y_sigma_upper_from_xyz_v0_7(
    const truthraw_v07::PixelXyzD50UncertaintyV07& xyz,
    AppearanceEvidenceV1& evidence) {
    evidence.luminanceSigmaKnown = false;
    evidence.luminanceSigmaUpper = std::numeric_limits<double>::quiet_NaN();
    constexpr std::uint8_t yBit = 1u << 1;
    if ((xyz.varianceKnownMask & yBit) != 0u) {
        const double v = xyz.variance[1];
        if (!(std::isfinite(v) && v >= 0.0)) return StatusV1::error("known XYZ Y variance invalid");
        evidence.luminanceSigmaUpper = std::sqrt(v);
        evidence.luminanceSigmaKnown = true;
        return StatusV1::success();
    }
    const auto& b = xyz.varianceBounds[1];
    if (b.valid) {
        if (!(std::isfinite(b.upper) && b.upper >= 0.0)) return StatusV1::error("XYZ Y variance upper bound invalid");
        evidence.luminanceSigmaUpper = std::sqrt(b.upper);
        evidence.luminanceSigmaKnown = true;
    }
    return StatusV1::success();
}
}

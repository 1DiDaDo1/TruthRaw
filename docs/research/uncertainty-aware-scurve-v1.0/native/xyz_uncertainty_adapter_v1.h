#pragma once
#include "uncertainty_aware_scurve_v1.h"
#include "xyz_d50_uncertainty_v0_7.h"

namespace truthraw_scurve_v1 {
StatusV1 extract_y_sigma_upper_from_xyz_v0_7(
    const truthraw_v07::PixelXyzD50UncertaintyV07& xyz,
    AppearanceEvidenceV1& evidence);
}

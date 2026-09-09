#pragma once

#include "camera_rgb_covariance_v0_6.h"
#include "truthrange_dense_uncertainty_v0_3.h"

#include <array>
#include <cstddef>

namespace truthraw_v06 {

// Fail-closed adapter from the closed v0.3 dense marginal contract into the v0.6
// camera-RGB covariance representation. Only an explicitly Gaussian-equivalent
// sigma may become a variance. Error quantiles alone never define variance.
StatusV06 adapt_dense_uncertainty_pixel_v0_3_to_covariance_v0_6(
    const truthraw::DenseUncertaintyFieldV03& field,
    std::size_t pixelIndex,
    std::array<MarginalChannelKnowledgeV06, 3>& marginalOut,
    PixelCameraRgbCovarianceV06& covarianceOut);

} // namespace truthraw_v06

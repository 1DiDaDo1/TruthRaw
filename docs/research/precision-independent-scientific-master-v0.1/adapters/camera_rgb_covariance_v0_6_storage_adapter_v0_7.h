#pragma once

#include "uncertainty_relative_storage_v0_7.h"
#include "../../camera-rgb-covariance-v0.6/native/camera_rgb_covariance_v0_6.h"

namespace truthraw_precision_v07 {

struct ChannelUncertaintyBindingV07 {
    UncertaintyAnchorV07 gaussian{};
    UncertaintyAnchorV07 p50{};
    UncertaintyAnchorV07 p95{};
    bool topologyCertified = false;
    bool semanticConflict = false;
    truthraw_v06::MarginalSourceV06 source = truthraw_v06::MarginalSourceV06::Unresolved;
};

// Fail-closed bridge from the existing v0.6 camera-RGB covariance contract.
// It preserves the semantic distinction between Gaussian-equivalent sigma and
// empirical p50/p95 reconstruction-error anchors.
ChannelUncertaintyBindingV07 bind_camera_rgb_uncertainty_v0_6_to_storage_v0_7(
    const truthraw_v06::MarginalChannelKnowledgeV06& marginal,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    int channel);

} // namespace truthraw_precision_v07

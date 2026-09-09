#pragma once
#include "truthrange_latent_v0_2.h"
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace truthraw {

enum class DenseUncertaintySourceV03 : std::uint8_t {
    Unresolved = 0,
    MeasuredNoiseProfileGaussianEquivalent = 1,
    MeasuredHighCensored = 2,
    V5GMeasuredRoleAnchor = 3,
    V5GLocalMaxTransportProxy = 4,
};

struct DenseUncertaintyEntryV03 {
    bool valid = false;
    float p50Abs = std::numeric_limits<float>::quiet_NaN();
    float p95Abs = std::numeric_limits<float>::quiet_NaN();
    float sigmaEquivalent = std::numeric_limits<float>::quiet_NaN();
    DenseUncertaintySourceV03 source = DenseUncertaintySourceV03::Unresolved;
    bool topologyCertified = false;
    bool sourceHighCensored = false;
};

struct DenseUncertaintyFieldV03 {
    int width = 0;
    int height = 0;
    // RGB interleaved, 3*N entries in the exact LatentCameraSceneV02 cameraRgb scale.
    std::vector<DenseUncertaintyEntryV03> rgb;
    std::string measuredModel = "UNRESOLVED";
    std::string reconstructedModel = "UNRESOLVED";
    std::string covarianceStatus = "UNRESOLVED";
    std::string claimBoundary;
};

struct BackendAnchorFieldV03 {
    int width = 0;
    int height = 0;
    // RGB interleaved. Valid entries must only exist on actually measured same-channel CFA sites.
    std::vector<DenseUncertaintyEntryV03> rgb;
    std::string binding;
};

struct DenseTruthRangeFieldV03 {
    int width = 0;
    int height = 0;
    std::vector<TruthRangeSampleV02> rgb;
};

// Build source/noise uncertainty for the directly measured CFA component only.
// DNG NoiseProfile is interpreted in normalized pre-GainMap signal, then transformed
// into Stage-2 units with exactly the same GainMap factor as the latent scene:
// Var(stage2) = g*S*max(stage2,0) + g^2*O.
Status build_measured_noiseprofile_field_v0_3(
    const DecodedDngFrame& frame,
    const LatentCameraSceneV02& scene,
    DenseUncertaintyFieldV03& out);

// Merge backend uncertainty anchors without evaluating v5.0g outside its measured-role domain.
// Missing-channel entries receive the local maximum p50/p95 from same-color measured anchors
// in a finite neighborhood. This is a transported proxy, never topology-certified.
Status transport_backend_anchors_v0_3(
    const LatentCameraSceneV02& scene,
    const BackendAnchorFieldV03& anchors,
    int radius,
    DenseUncertaintyFieldV03& io);

Status map_dense_uncertainty_to_truthrange_v0_3(
    const LatentCameraSceneV02& scene,
    const DenseUncertaintyFieldV03& uncertainty,
    const TruthRangeGaugeV02& gauge,
    DenseTruthRangeFieldV03& out);

const char* dense_uncertainty_source_name_v0_3(DenseUncertaintySourceV03 s);

} // namespace truthraw

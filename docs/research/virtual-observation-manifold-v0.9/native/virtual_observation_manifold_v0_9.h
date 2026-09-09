#pragma once

#include "truthrange_latent_v0_2.h"
#include "camera_rgb_covariance_v0_6.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace truthraw_v09 {

enum class VirtualIsoSemanticsV09 : std::uint8_t {
    None = 0,
    GainEncodingOnly = 1,
    CalibratedForwardModel = 2,
};

enum class SensorModelAuthorityV09 : std::uint8_t {
    Unresolved = 0,
    ExplicitResearchFixture = 1,
    IndependentSensorCalibration = 2,
};

struct StatusV09 {
    bool ok = true;
    std::string message;
    explicit operator bool() const { return ok; }
    static StatusV09 success() { return {}; }
    static StatusV09 error(std::string m) { return {false, std::move(m)}; }
};

struct VirtualObservationSpecV09 {
    std::string viewId;
    double exposureEv = 0.0;  // Scene/exposure reparameterization: TruthRange shifts by +exposureEv.
    double gainEv = 0.0;      // Output encoding gain only unless a calibrated forward model is supplied.
    double nominalVirtualIso = std::numeric_limits<double>::quiet_NaN();
    VirtualIsoSemanticsV09 isoSemantics = VirtualIsoSemanticsV09::None;
};

struct VirtualObservationManifoldV09 {
    std::string sourceEvidenceId;
    std::string sceneScaleId;
    std::vector<VirtualObservationSpecV09> views;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    bool viewsAreIndependentMeasurements = false;
    double evidenceConfidenceMultiplier = 1.0;
    std::string claimBoundary =
        "Virtual observations are deterministic/shared-evidence views of one admitted scene estimate. "
        "Adding EV/ISO/gain nodes never creates photons, frames, or independent sensor evidence.";
};

struct VirtualCameraRgbPixelV09 {
    std::array<float, 3> encodedCameraRgb {0.f, 0.f, 0.f};
    truthraw_v06::PixelCameraRgbCovarianceV06 encodedCovariance;
    double exposureScale = 1.0;
    double gainScale = 1.0;
    double totalEncodingScale = 1.0;
    std::string sourceEvidenceId;
    std::string viewId;
};

struct VirtualSensorForwardModelV09 {
    SensorModelAuthorityV09 authority = SensorModelAuthorityV09::Unresolved;
    std::string modelId;
    std::string sourceClassId;
    double referenceIso = std::numeric_limits<double>::quiet_NaN();
    double sceneToPregainSignal = std::numeric_limits<double>::quiet_NaN();
    double shotVarianceSlopePregain = std::numeric_limits<double>::quiet_NaN();
    double readVariancePregain = std::numeric_limits<double>::quiet_NaN();
    double saturationPregain = std::numeric_limits<double>::quiet_NaN();
};

struct VirtualSensorPredictionV09 {
    bool valid = false;
    double expectedPregainSignal = std::numeric_limits<double>::quiet_NaN();
    double expectedEncodedSignal = std::numeric_limits<double>::quiet_NaN();
    double conditionalVarianceEncoded = std::numeric_limits<double>::quiet_NaN();
    bool highCensored = false;
    double censorLowerBoundEncoded = std::numeric_limits<double>::quiet_NaN();
    std::string modelId;
    std::string claimBoundary =
        "Conditional virtual-sensor prediction only. It is not a new physical capture and does not add evidence to the source frame.";
};

StatusV09 validate_virtual_observation_spec_v0_9(const VirtualObservationSpecV09& spec);

StatusV09 build_virtual_observation_manifold_v0_9(
    const truthraw::LatentCameraSceneV02& scene,
    const std::string& sourceEvidenceId,
    const std::vector<VirtualObservationSpecV09>& views,
    VirtualObservationManifoldV09& out);

StatusV09 project_truthrange_sample_v0_9(
    const truthraw::TruthRangeSampleV02& in,
    const VirtualObservationSpecV09& spec,
    truthraw::TruthRangeSampleV02& out);

StatusV09 project_camera_rgb_pixel_v0_9(
    const std::array<float, 3>& cameraRgb,
    const truthraw_v06::PixelCameraRgbCovarianceV06& covariance,
    const std::string& sourceEvidenceId,
    const VirtualObservationSpecV09& spec,
    VirtualCameraRgbPixelV09& out);

StatusV09 normalized_virtual_ev_weights_v0_9(
    double positiveSceneSignal,
    const std::vector<VirtualObservationSpecV09>& views,
    std::vector<double>& weights,
    double target = 0.18,
    double widthStops = 2.0);

StatusV09 validate_virtual_sensor_forward_model_v0_9(
    const VirtualSensorForwardModelV09& model);

StatusV09 predict_virtual_sensor_observation_v0_9(
    double positiveSceneSignal,
    const VirtualObservationSpecV09& spec,
    const VirtualSensorForwardModelV09& model,
    VirtualSensorPredictionV09& out);

const char* virtual_iso_semantics_name_v0_9(VirtualIsoSemanticsV09 s);
const char* sensor_model_authority_name_v0_9(SensorModelAuthorityV09 a);

} // namespace truthraw_v09

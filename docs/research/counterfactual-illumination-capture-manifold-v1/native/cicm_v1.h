#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>

namespace truthraw::counterfactual::v1 {

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    CalibrationMissing,
    BindingMismatch,
    RelightStateIncomplete,
    FullRelightNotImplemented,
    NoAdmissibleCapture
};

enum class WorldSemantics : std::uint8_t {
    RelativeRadianceScaleOnly = 0,
    CalibratedNeutralIlluminationForward = 1,
    GeometryBrdfSpectralRelightReserved = 2
};

enum class CalibrationAuthority : std::uint8_t {
    Unresolved = 0,
    ExplicitResearchFixture = 1,
    IndependentMeasurement = 2
};

struct SceneBinding {
    std::string sourceEvidenceId;
    std::string sceneScaleId;
    std::string zeroLineId;
    std::string scientificMasterSha256;
};

struct EvidenceLedger {
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    std::uint32_t counterfactualObservationCount = 0;
    bool counterfactualObservationsAreEvidence = false;
    bool scientificMasterModified = false;
    bool zeroLineModified = false;
};

struct CounterfactualWorldSpec {
    std::string worldId;
    WorldSemantics semantics = WorldSemantics::RelativeRadianceScaleOnly;
    double illuminationScale = 1.0;
    std::string illuminationReferenceId;
    SceneBinding sceneBinding;
};

struct RelativeCaptureSpec { double shutterScale = 1.0; };

struct RelativeWorldPrediction {
    bool valid = false;
    double sourceSceneSignal = std::numeric_limits<double>::quiet_NaN();
    double counterfactualSceneSignal = std::numeric_limits<double>::quiet_NaN();
    double relativeExposureSignal = std::numeric_limits<double>::quiet_NaN();
    double counterfactualDeltaEv = std::numeric_limits<double>::quiet_NaN();
    bool physicalSnrAvailable = false;
    SceneBinding sceneBinding;
    EvidenceLedger ledger;
    std::string claimBoundary;
};

struct SensorModeCalibration {
    CalibrationAuthority authority = CalibrationAuthority::Unresolved;
    std::string calibrationId;
    std::string calibrationProtocolId;
    std::string calibrationEvidenceSha256;
    std::string sourceClassId;
    std::string opticalModeId;
    std::string sensorModeId;
    std::string sceneScaleId;
    std::string illuminationReferenceId;
    double nominalIso = std::numeric_limits<double>::quiet_NaN();
    double sceneUnitToElectronsPerSecond = std::numeric_limits<double>::quiet_NaN();
    double readNoiseElectronsRms = std::numeric_limits<double>::quiet_NaN();
    double darkCurrentElectronsPerSecond = std::numeric_limits<double>::quiet_NaN();
    double fullWellElectrons = std::numeric_limits<double>::quiet_NaN();
    double systemGainDnPerElectron = std::numeric_limits<double>::quiet_NaN();
    double blackOffsetDn = std::numeric_limits<double>::quiet_NaN();
    double adcWhiteDn = std::numeric_limits<double>::quiet_NaN();
};

struct PhysicalCaptureSpec {
    std::string expectedSourceClassId;
    std::string expectedOpticalModeId;
    double shutterSeconds = std::numeric_limits<double>::quiet_NaN();
};

struct SensorPrediction {
    bool valid = false;
    bool physicalForwardClaimAllowed = false;
    bool highCensored = false;
    double signalElectrons = std::numeric_limits<double>::quiet_NaN();
    double darkElectrons = std::numeric_limits<double>::quiet_NaN();
    double expectedChargeElectrons = std::numeric_limits<double>::quiet_NaN();
    double temporalVarianceElectrons2 = std::numeric_limits<double>::quiet_NaN();
    double electronDomainSnr = std::numeric_limits<double>::quiet_NaN();
    double expectedDnBeforeClip = std::numeric_limits<double>::quiet_NaN();
    double temporalVarianceDn2 = std::numeric_limits<double>::quiet_NaN();
    double saturationChargeElectrons = std::numeric_limits<double>::quiet_NaN();
    double headroomStops = std::numeric_limits<double>::quiet_NaN();
    double nominalIso = std::numeric_limits<double>::quiet_NaN();
    std::string calibrationId;
    std::string calibrationEvidenceSha256;
    SceneBinding sceneBinding;
    EvidenceLedger ledger;
    std::string claimBoundary;
};

struct CaptureCandidate { PhysicalCaptureSpec capture; SensorModeCalibration calibration; };
struct BestCapturePolicy { double minimumHeadroomStops = 0.0; };
struct BestCaptureResult {
    bool valid = false;
    std::size_t candidateIndex = 0;
    SensorPrediction prediction;
    std::string objective = "maximize calibrated electron-domain SNR subject to censor/headroom gates";
};

struct FullRelightStateAvailability {
    bool geometry = false;
    bool surfaceNormals = false;
    bool visibility = false;
    bool materialBrdf = false;
    bool illuminantSpatialField = false;
    bool illuminantSpectrum = false;
};

Status validate_scene_binding(const SceneBinding& binding) noexcept;
Status validate_world(const CounterfactualWorldSpec& world) noexcept;
Status simulate_relative_world(double nonnegativeSceneSignal, const CounterfactualWorldSpec& world,
    const RelativeCaptureSpec& capture, RelativeWorldPrediction& out) noexcept;
Status validate_sensor_mode_calibration(const SensorModeCalibration& calibration) noexcept;
Status predict_calibrated_capture(double nonnegativeSceneSignal, const CounterfactualWorldSpec& world,
    const PhysicalCaptureSpec& capture, const SensorModeCalibration& calibration, SensorPrediction& out) noexcept;
Status choose_best_calibrated_capture(double nonnegativeSceneSignal, const CounterfactualWorldSpec& world,
    std::span<const CaptureCandidate> candidates, const BestCapturePolicy& policy, BestCaptureResult& out) noexcept;
Status assess_full_relight_readiness(const FullRelightStateAvailability& state) noexcept;
const char* status_name(Status s) noexcept;
const char* world_semantics_name(WorldSemantics s) noexcept;
const char* calibration_authority_name(CalibrationAuthority a) noexcept;

} // namespace truthraw::counterfactual::v1

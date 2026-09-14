#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::fotograaf::shadow_v0_1 {

struct SourceMeasurementModel {
    std::string sourceEvidenceSha256;
    std::array<float,4> blackPhaseCode{64.f,64.f,64.f,64.f};
    float whiteLevelCode = 1023.f;
    std::array<float,6> noiseProfileRgb{0.f,0.f,0.f,0.f,0.f,0.f};
    bool hasNoiseProfile = false;
    bool gainMapAppliedExactlyOnce = true;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

struct AdmittedCalibrationBinding {
    bool admitted = false;
    std::string sourceEvidenceSha256;
    std::string bindingSha256;
    std::string protocolSha256;
    std::string modelSha256;
    std::string modelId;
    std::string uncertaintyModelId;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

struct CalibrationMeasurementModel {
    std::string bindingSha256;
    std::string protocolSha256;
    std::string modelSha256;
    bool useCalibratedBlack = false;
    bool useCalibratedNoise = false;
    bool useCalibratedResponseScale = false;
    bool useCalibratedSaturation = false;
    std::array<float,4> calibratedBlackPhaseCode{64.f,64.f,64.f,64.f};
    std::array<float,6> calibratedNoiseProfileRgb{0.f,0.f,0.f,0.f,0.f,0.f};
    float responseScale = 1.f;
    float calibratedSaturationCode = 1023.f;
    float darkSnrThreshold = 1.f;
    bool requestsAdditionalGainMapCorrection = false;
};

struct SampleInput {
    float rawCode = 0.f;
    int phase = 0;      // 0..3, used for phase black
    int color = 1;      // 0=R, 1=G, 2=B, used for NoiseProfile pair
    float sourceGainField = 1.f; // existing source GainMap multiplier, applied exactly once
};

struct BranchDiagnostics {
    float signedNormalized = 0.f;
    float noiseSigmaNormalized = 0.f;
    bool noiseSigmaAvailable = false;
    bool highCensored = false;
    bool darkNoiseLimited = false;
};

struct ShadowSampleComparison {
    BranchDiagnostics source;
    BranchDiagnostics calibratedShadow;
    float calibratedMinusSource = 0.f;
    bool calibratedBlackUsed = false;
    bool calibratedNoiseUsed = false;
    bool calibratedResponseScaleUsed = false;
    bool calibratedSaturationUsed = false;
};

struct ShadowSummary {
    std::uint64_t sampleCount = 0;
    std::uint64_t sourceNegativeCount = 0;
    std::uint64_t calibratedNegativeCount = 0;
    std::uint64_t sourceHighCensoredCount = 0;
    std::uint64_t calibratedHighCensoredCount = 0;
    std::uint64_t sourceDarkNoiseLimitedCount = 0;
    std::uint64_t calibratedDarkNoiseLimitedCount = 0;
    double meanAbsoluteDelta = 0.0;
    float maxAbsoluteDelta = 0.f;
};

struct ValidationResult {
    bool ok = false;
    std::string message;
};

ValidationResult validateShadowInputs(
    const SourceMeasurementModel& source,
    const AdmittedCalibrationBinding& binding,
    const CalibrationMeasurementModel& calibrated);

ValidationResult compareSample(
    const SourceMeasurementModel& source,
    const AdmittedCalibrationBinding& binding,
    const CalibrationMeasurementModel& calibrated,
    const SampleInput& input,
    ShadowSampleComparison& out);

ShadowSummary summarize(const std::vector<ShadowSampleComparison>& samples);

} // namespace truthraw::fotograaf::shadow_v0_1

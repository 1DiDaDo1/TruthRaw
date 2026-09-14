#include "shadow_measurement_model_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace truthraw::fotograaf::shadow_v0_1 {
namespace {

bool finite(float x) { return std::isfinite(x); }

bool hex64(const std::string& s) {
    if (s.size() != 64) return false;
    for (const char c : s) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

ValidationResult fail(const char* message) { return {false, message}; }
ValidationResult pass() { return {true, "OK"}; }

bool validNoise(const std::array<float,6>& p) {
    for (const float x : p) {
        if (!finite(x) || x < 0.f) return false;
    }
    return true;
}

BranchDiagnostics makeBranch(float rawCode,
                             float black,
                             float saturation,
                             const std::array<float,6>& noise,
                             bool noiseAvailable,
                             int color,
                             float responseScale,
                             float gainField,
                             float darkSnrThreshold,
                             bool preserveSourceCensor,
                             bool canonicalSourceDenominator,
                             float sourceWhite) {
    BranchDiagnostics out{};
    const float rawDenom = saturation - black;
    const float denom = canonicalSourceDenominator ? std::max(rawDenom, 1.f) : rawDenom;
    const float preGain = (rawCode - black) / denom;
    out.signedNormalized = preGain * responseScale * gainField;

    if (noiseAvailable) {
        const float S = noise[std::size_t(2 * color)];
        const float O = noise[std::size_t(2 * color + 1)];
        const float variance = std::max(S * std::max(preGain, 0.f) + O, 0.f);
        out.noiseSigmaNormalized = std::sqrt(variance) * responseScale * gainField;
        out.noiseSigmaAvailable = true;
        out.darkNoiseLimited = std::max(out.signedNormalized, 0.f) <=
                               darkSnrThreshold * out.noiseSigmaNormalized;
    }

    const bool sourceCensored = rawCode >= sourceWhite;
    const bool localCensored = rawCode >= saturation;
    out.highCensored = preserveSourceCensor ? (sourceCensored || localCensored) : localCensored;
    return out;
}

} // namespace

ValidationResult validateShadowInputs(const SourceMeasurementModel& source,
                                      const AdmittedCalibrationBinding& binding,
                                      const CalibrationMeasurementModel& calibrated) {
    if (!hex64(source.sourceEvidenceSha256)) return fail("sourceEvidenceSha256 must be lowercase 64-hex");
    if (!binding.admitted) return fail("calibration binding is not admitted");
    if (binding.sourceEvidenceSha256 != source.sourceEvidenceSha256) return fail("binding/source evidence mismatch");
    if (!hex64(binding.bindingSha256)) return fail("bindingSha256 must be lowercase 64-hex");
    if (!hex64(binding.protocolSha256)) return fail("protocolSha256 must be lowercase 64-hex");
    if (!hex64(binding.modelSha256)) return fail("modelSha256 must be lowercase 64-hex");
    if (calibrated.bindingSha256 != binding.bindingSha256) return fail("measurement model is not bound to admitted binding");
    if (calibrated.protocolSha256 != binding.protocolSha256) return fail("measurement model protocol hash does not match admitted binding");
    if (calibrated.modelSha256 != binding.modelSha256) return fail("measurement model hash does not match admitted binding");
    if (binding.modelId.empty() || binding.uncertaintyModelId.empty()) return fail("binding model identities are required");
    if (source.physicalFrameCount != 1 || source.independentEvidenceCount != 1 ||
        binding.physicalFrameCount != 1 || binding.independentEvidenceCount != 1) {
        return fail("shadow calibration may not change 1/1 photographic evidence counts");
    }
    if (!source.gainMapAppliedExactlyOnce) return fail("source GainMap application state is not exactly-once");
    if (calibrated.requestsAdditionalGainMapCorrection) return fail("second GainMap/lens-shading correction is forbidden in v0.1 shadow mode");
    if (!finite(source.whiteLevelCode)) return fail("source white level is not finite");
    for (const float black : source.blackPhaseCode) {
        if (!finite(black) || source.whiteLevelCode <= black) return fail("invalid source black/white interval");
    }
    if (source.hasNoiseProfile && !validNoise(source.noiseProfileRgb)) return fail("invalid source NoiseProfile");
    if (!finite(calibrated.darkSnrThreshold) || calibrated.darkSnrThreshold <= 0.f) return fail("darkSnrThreshold must be finite and > 0");
    if (calibrated.useCalibratedBlack) {
        for (const float black : calibrated.calibratedBlackPhaseCode) {
            if (!finite(black)) return fail("calibrated black phase is not finite");
        }
    }
    if (calibrated.useCalibratedNoise && !validNoise(calibrated.calibratedNoiseProfileRgb)) {
        return fail("invalid calibrated NoiseProfile");
    }
    if (calibrated.useCalibratedResponseScale &&
        (!finite(calibrated.responseScale) || calibrated.responseScale <= 0.f)) {
        return fail("calibrated responseScale must be finite and > 0");
    }
    if (calibrated.useCalibratedSaturation && !finite(calibrated.calibratedSaturationCode)) {
        return fail("calibrated saturation code is not finite");
    }

    for (int phase = 0; phase < 4; ++phase) {
        const float black = calibrated.useCalibratedBlack
            ? calibrated.calibratedBlackPhaseCode[std::size_t(phase)]
            : source.blackPhaseCode[std::size_t(phase)];
        const float saturation = calibrated.useCalibratedSaturation
            ? calibrated.calibratedSaturationCode
            : source.whiteLevelCode;
        if (saturation <= black) return fail("calibrated shadow black/saturation interval is invalid");
    }
    return pass();
}

ValidationResult compareSample(const SourceMeasurementModel& source,
                               const AdmittedCalibrationBinding& binding,
                               const CalibrationMeasurementModel& calibrated,
                               const SampleInput& input,
                               ShadowSampleComparison& out) {
    const ValidationResult validation = validateShadowInputs(source, binding, calibrated);
    if (!validation.ok) return validation;
    if (input.phase < 0 || input.phase > 3) return fail("sample phase must be 0..3");
    if (input.color < 0 || input.color > 2) return fail("sample color must be 0..2");
    if (!finite(input.rawCode)) return fail("rawCode must be finite");
    if (!finite(input.sourceGainField) || input.sourceGainField <= 0.f) return fail("sourceGainField must be finite and > 0");

    const float sourceBlack = source.blackPhaseCode[std::size_t(input.phase)];
    out.source = makeBranch(input.rawCode,
                            sourceBlack,
                            source.whiteLevelCode,
                            source.noiseProfileRgb,
                            source.hasNoiseProfile,
                            input.color,
                            1.f,
                            input.sourceGainField,
                            calibrated.darkSnrThreshold,
                            false,
                            true,
                            source.whiteLevelCode);

    const float calibratedBlack = calibrated.useCalibratedBlack
        ? calibrated.calibratedBlackPhaseCode[std::size_t(input.phase)]
        : sourceBlack;
    const float calibratedSaturation = calibrated.useCalibratedSaturation
        ? calibrated.calibratedSaturationCode
        : source.whiteLevelCode;
    const auto& calibratedNoise = calibrated.useCalibratedNoise
        ? calibrated.calibratedNoiseProfileRgb
        : source.noiseProfileRgb;
    const bool calibratedNoiseAvailable = calibrated.useCalibratedNoise || source.hasNoiseProfile;
    const float responseScale = calibrated.useCalibratedResponseScale ? calibrated.responseScale : 1.f;

    out.calibratedShadow = makeBranch(input.rawCode,
                                      calibratedBlack,
                                      calibratedSaturation,
                                      calibratedNoise,
                                      calibratedNoiseAvailable,
                                      input.color,
                                      responseScale,
                                      input.sourceGainField,
                                      calibrated.darkSnrThreshold,
                                      true,
                                      false,
                                      source.whiteLevelCode);
    out.calibratedMinusSource = out.calibratedShadow.signedNormalized - out.source.signedNormalized;
    out.calibratedBlackUsed = calibrated.useCalibratedBlack;
    out.calibratedNoiseUsed = calibrated.useCalibratedNoise;
    out.calibratedResponseScaleUsed = calibrated.useCalibratedResponseScale;
    out.calibratedSaturationUsed = calibrated.useCalibratedSaturation;
    return pass();
}

ShadowSummary summarize(const std::vector<ShadowSampleComparison>& samples) {
    ShadowSummary out{};
    out.sampleCount = static_cast<std::uint64_t>(samples.size());
    double sumAbs = 0.0;
    for (const auto& sample : samples) {
        if (sample.source.signedNormalized < 0.f) ++out.sourceNegativeCount;
        if (sample.calibratedShadow.signedNormalized < 0.f) ++out.calibratedNegativeCount;
        if (sample.source.highCensored) ++out.sourceHighCensoredCount;
        if (sample.calibratedShadow.highCensored) ++out.calibratedHighCensoredCount;
        if (sample.source.darkNoiseLimited) ++out.sourceDarkNoiseLimitedCount;
        if (sample.calibratedShadow.darkNoiseLimited) ++out.calibratedDarkNoiseLimitedCount;
        const float absoluteDelta = std::abs(sample.calibratedMinusSource);
        sumAbs += static_cast<double>(absoluteDelta);
        out.maxAbsoluteDelta = std::max(out.maxAbsoluteDelta, absoluteDelta);
    }
    if (!samples.empty()) out.meanAbsoluteDelta = sumAbs / static_cast<double>(samples.size());
    return out;
}

} // namespace truthraw::fotograaf::shadow_v0_1

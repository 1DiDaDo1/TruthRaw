#include "shadow_measurement_model_v0_1.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

using namespace truthraw::fotograaf::shadow_v0_1;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

std::string sha(char c) { return std::string(64, c); }

SourceMeasurementModel sourceModel() {
    SourceMeasurementModel s;
    s.sourceEvidenceSha256 = sha('a');
    s.blackPhaseCode = {64.f, 64.f, 64.f, 64.f};
    s.whiteLevelCode = 1023.f;
    s.noiseProfileRgb = {1.0e-4f,1.0e-6f, 1.2e-4f,1.2e-6f, 1.5e-4f,1.5e-6f};
    s.hasNoiseProfile = true;
    s.gainMapAppliedExactlyOnce = true;
    return s;
}

AdmittedCalibrationBinding binding() {
    AdmittedCalibrationBinding b;
    b.admitted = true;
    b.sourceEvidenceSha256 = sha('a');
    b.bindingSha256 = sha('b');
    b.protocolSha256 = sha('c');
    b.modelSha256 = sha('d');
    b.modelId = "fixture-relative-radiometry-v1";
    b.uncertaintyModelId = "fixture-relative-radiometry-uncertainty-v1";
    return b;
}

CalibrationMeasurementModel calibratedModel() {
    CalibrationMeasurementModel m;
    m.bindingSha256 = sha('b');
    m.protocolSha256 = sha('c');
    m.modelSha256 = sha('d');
    m.useCalibratedBlack = true;
    m.useCalibratedNoise = true;
    m.useCalibratedResponseScale = true;
    m.useCalibratedSaturation = true;
    m.calibratedBlackPhaseCode = {63.5f,63.75f,63.75f,64.f};
    m.calibratedNoiseProfileRgb = {1.1e-4f,1.1e-6f, 1.3e-4f,1.3e-6f, 1.6e-4f,1.6e-6f};
    m.responseScale = 0.985f;
    m.calibratedSaturationCode = 1018.f;
    m.darkSnrThreshold = 1.f;
    return m;
}

bool sameFloat(float a, float b) {
    return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
}

void requireSame(const ShadowSampleComparison& a, const ShadowSampleComparison& b) {
    require(sameFloat(a.source.signedNormalized, b.source.signedNormalized), "source signal differs by worker count");
    require(sameFloat(a.source.noiseSigmaNormalized, b.source.noiseSigmaNormalized), "source sigma differs by worker count");
    require(a.source.noiseSigmaAvailable == b.source.noiseSigmaAvailable, "source noise-availability differs");
    require(a.source.highCensored == b.source.highCensored, "source censor state differs");
    require(a.source.darkNoiseLimited == b.source.darkNoiseLimited, "source dark state differs");
    require(sameFloat(a.calibratedShadow.signedNormalized, b.calibratedShadow.signedNormalized), "calibrated signal differs by worker count");
    require(sameFloat(a.calibratedShadow.noiseSigmaNormalized, b.calibratedShadow.noiseSigmaNormalized), "calibrated sigma differs by worker count");
    require(a.calibratedShadow.noiseSigmaAvailable == b.calibratedShadow.noiseSigmaAvailable, "calibrated noise-availability differs");
    require(a.calibratedShadow.highCensored == b.calibratedShadow.highCensored, "calibrated censor state differs");
    require(a.calibratedShadow.darkNoiseLimited == b.calibratedShadow.darkNoiseLimited, "calibrated dark state differs");
    require(sameFloat(a.calibratedMinusSource, b.calibratedMinusSource), "shadow delta differs by worker count");
}

std::vector<ShadowSampleComparison> runWorkers(int workerCount,
                                                const std::vector<SampleInput>& inputs) {
    const auto source = sourceModel();
    const auto admitted = binding();
    const auto model = calibratedModel();
    std::vector<ShadowSampleComparison> out(inputs.size());
    std::vector<std::thread> threads;
    for (int worker = 0; worker < workerCount; ++worker) {
        threads.emplace_back([&, worker] {
            for (std::size_t i = static_cast<std::size_t>(worker); i < inputs.size(); i += static_cast<std::size_t>(workerCount)) {
                const auto status = compareSample(source, admitted, model, inputs[i], out[i]);
                require(status.ok, "parallel compareSample failed");
            }
        });
    }
    for (auto& thread : threads) thread.join();
    return out;
}

} // namespace

int main() {
    const auto source = sourceModel();
    const auto admitted = binding();
    auto model = calibratedModel();

    require(validateShadowInputs(source, admitted, model).ok, "valid shadow fixture rejected");

    // Source branch must preserve the arithmetic shape of canonical v4.7i Stage-2.
    {
        SampleInput input{100.f, 0, 2, 1.25f};
        ShadowSampleComparison out;
        require(compareSample(source, admitted, model, input, out).ok, "source-equivalence sample failed");
        const float expected = ((100.f - 64.f) / (1023.f - 64.f)) * 1.25f;
        require(sameFloat(out.source.signedNormalized, expected), "source Stage-2 arithmetic changed");
        require(out.calibratedBlackUsed && out.calibratedNoiseUsed &&
                out.calibratedResponseScaleUsed && out.calibratedSaturationUsed,
                "enabled calibration parameters not reported");
    }

    // True below-black numerical values remain signed; shadow calibration cannot force them to zero.
    {
        SampleInput input{60.f, 1, 1, 1.f};
        ShadowSampleComparison out;
        require(compareSample(source, admitted, model, input, out).ok, "below-black sample failed");
        require(out.source.signedNormalized < 0.f, "source below-black value was clipped");
        require(out.calibratedShadow.signedNormalized < 0.f, "calibrated below-black value was clipped");
    }

    // A calibrated saturation model may tighten censoring, never uncensor source WhiteLevel censoring.
    {
        SampleInput input{1023.f, 2, 1, 1.f};
        ShadowSampleComparison out;
        require(compareSample(source, admitted, model, input, out).ok, "source-censor sample failed");
        require(out.source.highCensored, "source WhiteLevel censor lost");
        require(out.calibratedShadow.highCensored, "calibrated branch lost source censor");

        auto higherSaturation = model;
        higherSaturation.calibratedSaturationCode = 1100.f;
        ShadowSampleComparison higherOut;
        require(compareSample(source, admitted, higherSaturation, input, higherOut).ok, "higher saturation model failed");
        require(higherOut.calibratedShadow.highCensored, "calibration illegally uncensored source clip");
    }

    // Partial calibration is explicit: calibrating only noise must not move the Stage-2 signal coordinate.
    {
        auto noiseOnly = model;
        noiseOnly.useCalibratedBlack = false;
        noiseOnly.useCalibratedResponseScale = false;
        noiseOnly.useCalibratedSaturation = false;
        SampleInput input{180.f, 3, 0, 1.f};
        ShadowSampleComparison out;
        require(compareSample(source, admitted, noiseOnly, input, out).ok, "noise-only model failed");
        require(sameFloat(out.source.signedNormalized, out.calibratedShadow.signedNormalized),
                "noise-only calibration moved signal coordinate");
        require(!sameFloat(out.source.noiseSigmaNormalized, out.calibratedShadow.noiseSigmaNormalized),
                "noise-only calibration did not change sigma");
    }

    // Binding identity, exact model/protocol bytes and exactly-once source correction are hard gates.
    {
        auto badBinding = admitted;
        badBinding.sourceEvidenceSha256 = sha('e');
        require(!validateShadowInputs(source, badBinding, model).ok, "wrong source binding was admitted");

        auto badModelHash = model;
        badModelHash.modelSha256 = sha('e');
        require(!validateShadowInputs(source, admitted, badModelHash).ok, "wrong model hash was admitted");

        auto badProtocolHash = model;
        badProtocolHash.protocolSha256 = sha('f');
        require(!validateShadowInputs(source, admitted, badProtocolHash).ok, "wrong protocol hash was admitted");

        auto secondGain = model;
        secondGain.requestsAdditionalGainMapCorrection = true;
        require(!validateShadowInputs(source, admitted, secondGain).ok, "second GainMap correction was admitted");

        auto extraEvidence = admitted;
        extraEvidence.independentEvidenceCount = 2;
        require(!validateShadowInputs(source, extraEvidence, model).ok, "extra scene evidence root was admitted");
    }

    // Worker count is execution only. 1 and 4 photographers must produce bit-identical diagnostics.
    std::vector<SampleInput> inputs;
    for (int i = 0; i < 4096; ++i) {
        SampleInput sample;
        sample.rawCode = static_cast<float>((i * 37) % 1100);
        sample.phase = i & 3;
        sample.color = (i % 3);
        sample.sourceGainField = 0.8f + 0.01f * static_cast<float>(i % 41);
        inputs.push_back(sample);
    }
    const auto one = runWorkers(1, inputs);
    const auto four = runWorkers(4, inputs);
    require(one.size() == four.size(), "worker outputs have different size");
    for (std::size_t i = 0; i < one.size(); ++i) requireSame(one[i], four[i]);

    const auto summaryOne = summarize(one);
    const auto summaryFour = summarize(four);
    require(summaryOne.sampleCount == summaryFour.sampleCount, "summary sample count differs");
    require(summaryOne.sourceNegativeCount == summaryFour.sourceNegativeCount, "summary source negative count differs");
    require(summaryOne.calibratedNegativeCount == summaryFour.calibratedNegativeCount, "summary calibrated negative count differs");
    require(summaryOne.sourceHighCensoredCount == summaryFour.sourceHighCensoredCount, "summary source censor count differs");
    require(summaryOne.calibratedHighCensoredCount == summaryFour.calibratedHighCensoredCount, "summary calibrated censor count differs");
    require(summaryOne.sourceDarkNoiseLimitedCount == summaryFour.sourceDarkNoiseLimitedCount, "summary source dark count differs");
    require(summaryOne.calibratedDarkNoiseLimitedCount == summaryFour.calibratedDarkNoiseLimitedCount, "summary calibrated dark count differs");
    require(summaryOne.meanAbsoluteDelta == summaryFour.meanAbsoluteDelta, "summary mean delta differs");
    require(sameFloat(summaryOne.maxAbsoluteDelta, summaryFour.maxAbsoluteDelta), "summary max delta differs");

    std::cout << "FotoGraaf shadow MeasurementLab adapter v0.1 PASS\n";
    std::cout << "samples=" << summaryOne.sampleCount
              << " source_negative=" << summaryOne.sourceNegativeCount
              << " calibrated_negative=" << summaryOne.calibratedNegativeCount
              << " source_censored=" << summaryOne.sourceHighCensoredCount
              << " calibrated_censored=" << summaryOne.calibratedHighCensoredCount
              << " mean_abs_delta=" << summaryOne.meanAbsoluteDelta
              << " max_abs_delta=" << summaryOne.maxAbsoluteDelta << "\n";
    return 0;
}

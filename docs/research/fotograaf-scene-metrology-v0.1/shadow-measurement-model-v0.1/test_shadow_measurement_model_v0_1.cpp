#include "shadow_measurement_model_v0_1.h"

#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using namespace truthraw::fotograaf::shadow_v0_1;

namespace {

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

void assertSame(const ShadowSampleComparison& a, const ShadowSampleComparison& b) {
    assert(sameFloat(a.source.signedNormalized, b.source.signedNormalized));
    assert(sameFloat(a.source.noiseSigmaNormalized, b.source.noiseSigmaNormalized));
    assert(a.source.noiseSigmaAvailable == b.source.noiseSigmaAvailable);
    assert(a.source.highCensored == b.source.highCensored);
    assert(a.source.darkNoiseLimited == b.source.darkNoiseLimited);
    assert(sameFloat(a.calibratedShadow.signedNormalized, b.calibratedShadow.signedNormalized));
    assert(sameFloat(a.calibratedShadow.noiseSigmaNormalized, b.calibratedShadow.noiseSigmaNormalized));
    assert(a.calibratedShadow.noiseSigmaAvailable == b.calibratedShadow.noiseSigmaAvailable);
    assert(a.calibratedShadow.highCensored == b.calibratedShadow.highCensored);
    assert(a.calibratedShadow.darkNoiseLimited == b.calibratedShadow.darkNoiseLimited);
    assert(sameFloat(a.calibratedMinusSource, b.calibratedMinusSource));
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
                assert(status.ok);
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

    {
        const auto status = validateShadowInputs(source, admitted, model);
        assert(status.ok);
    }

    // Source branch must preserve the byte-level arithmetic shape of canonical v4.7i Stage-2:
    // ((raw - phaseBlack) / max(white - phaseBlack, 1)) * existing GainMap.
    {
        SampleInput input{100.f, 0, 2, 1.25f};
        ShadowSampleComparison out;
        const auto status = compareSample(source, admitted, model, input, out);
        assert(status.ok);
        const float expected = ((100.f - 64.f) / (1023.f - 64.f)) * 1.25f;
        assert(sameFloat(out.source.signedNormalized, expected));
        assert(out.calibratedBlackUsed && out.calibratedNoiseUsed &&
               out.calibratedResponseScaleUsed && out.calibratedSaturationUsed);
    }

    // True below-black numerical values remain signed; shadow calibration cannot force them to zero.
    {
        SampleInput input{60.f, 1, 1, 1.f};
        ShadowSampleComparison out;
        assert(compareSample(source, admitted, model, input, out).ok);
        assert(out.source.signedNormalized < 0.f);
        assert(out.calibratedShadow.signedNormalized < 0.f);
    }

    // A calibrated saturation model may tighten censoring, never uncensor source WhiteLevel censoring.
    {
        SampleInput input{1023.f, 2, 1, 1.f};
        ShadowSampleComparison out;
        assert(compareSample(source, admitted, model, input, out).ok);
        assert(out.source.highCensored);
        assert(out.calibratedShadow.highCensored);

        auto higherSaturation = model;
        higherSaturation.calibratedSaturationCode = 1100.f;
        ShadowSampleComparison higherOut;
        assert(compareSample(source, admitted, higherSaturation, input, higherOut).ok);
        assert(higherOut.calibratedShadow.highCensored); // source censor is preserved
    }

    // Partial calibration is explicit: calibrating only noise must not move the Stage-2 signal coordinate.
    {
        auto noiseOnly = model;
        noiseOnly.useCalibratedBlack = false;
        noiseOnly.useCalibratedResponseScale = false;
        noiseOnly.useCalibratedSaturation = false;
        SampleInput input{180.f, 3, 0, 1.f};
        ShadowSampleComparison out;
        assert(compareSample(source, admitted, noiseOnly, input, out).ok);
        assert(sameFloat(out.source.signedNormalized, out.calibratedShadow.signedNormalized));
        assert(!sameFloat(out.source.noiseSigmaNormalized, out.calibratedShadow.noiseSigmaNormalized));
    }

    // Binding identity, exact model/protocol bytes and exactly-once source correction are hard gates.
    {
        auto badBinding = admitted;
        badBinding.sourceEvidenceSha256 = sha('e');
        assert(!validateShadowInputs(source, badBinding, model).ok);

        auto badModelHash = model;
        badModelHash.modelSha256 = sha('e');
        assert(!validateShadowInputs(source, admitted, badModelHash).ok);

        auto badProtocolHash = model;
        badProtocolHash.protocolSha256 = sha('f');
        assert(!validateShadowInputs(source, admitted, badProtocolHash).ok);

        auto secondGain = model;
        secondGain.requestsAdditionalGainMapCorrection = true;
        assert(!validateShadowInputs(source, admitted, secondGain).ok);

        auto extraEvidence = admitted;
        extraEvidence.independentEvidenceCount = 2;
        assert(!validateShadowInputs(source, extraEvidence, model).ok);
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
    assert(one.size() == four.size());
    for (std::size_t i = 0; i < one.size(); ++i) assertSame(one[i], four[i]);

    const auto summaryOne = summarize(one);
    const auto summaryFour = summarize(four);
    assert(summaryOne.sampleCount == summaryFour.sampleCount);
    assert(summaryOne.sourceNegativeCount == summaryFour.sourceNegativeCount);
    assert(summaryOne.calibratedNegativeCount == summaryFour.calibratedNegativeCount);
    assert(summaryOne.sourceHighCensoredCount == summaryFour.sourceHighCensoredCount);
    assert(summaryOne.calibratedHighCensoredCount == summaryFour.calibratedHighCensoredCount);
    assert(summaryOne.sourceDarkNoiseLimitedCount == summaryFour.sourceDarkNoiseLimitedCount);
    assert(summaryOne.calibratedDarkNoiseLimitedCount == summaryFour.calibratedDarkNoiseLimitedCount);
    assert(summaryOne.meanAbsoluteDelta == summaryFour.meanAbsoluteDelta);
    assert(sameFloat(summaryOne.maxAbsoluteDelta, summaryFour.maxAbsoluteDelta));

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

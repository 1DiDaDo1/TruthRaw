#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace truthraw_scurve_v1 {

struct StatusV1 {
    bool ok = true;
    std::string message;
    explicit operator bool() const { return ok; }
    static StatusV1 success() { return {}; }
    static StatusV1 error(std::string m) { return {false, std::move(m)}; }
};

struct AppearanceEvidenceV1 {
    double luminanceSigmaUpper = std::numeric_limits<double>::quiet_NaN();
    bool luminanceSigmaKnown = false;
    double chromaConfidence = std::numeric_limits<double>::quiet_NaN();
    bool chromaConfidenceKnown = false;
    bool sourceHighCensored = false;
};

struct AppearanceParamsV1 {
    double snrLow = 2.0;
    double snrHigh = 8.0;
    double reliableSCurvePower = 1.38;
    double uncertainShadowFloor = 0.58;
    double uncertainShadowEnd = 0.18;
    double chromaGainLowConfidence = 0.88;
    double chromaGainHighConfidence = 1.14;
    double chromaConfidenceLow = 0.20;
    double chromaConfidenceHigh = 0.85;
    int gamutSearchIterations = 24;
};

struct AppearanceResultV1 {
    std::array<double,3> rgb {0.0,0.0,0.0};
    double luminanceIn = 0.0;
    double luminanceAfterTone = 0.0;
    double luminanceConfidence = 0.0;
    double localToneSlope = 1.0;
    double sigmaOutUpper = std::numeric_limits<double>::quiet_NaN();
    double chromaConfidenceUsed = 0.0;
    double requestedChromaGain = 1.0;
    double appliedChromaGain = 1.0;
    bool toneGamutLimited = false;
    bool chromaGamutLimited = false;
    bool appearanceOnly = true;
    std::string claimBoundary =
        "Appearance-only. No sensor evidence, scene estimate, covariance, or topology claim is modified.";
};

double linear_srgb_luminance_v1(const std::array<double,3>& rgb);
double smoothstep01_v1(double x);
double power_s_curve_v1(double x, double power);
double uncertain_shadow_curve_v1(double x, double floorSlope, double shadowEnd);

StatusV1 apply_uncertainty_aware_scurve_v1(
    const std::array<double,3>& displayLinearRgb,
    const AppearanceEvidenceV1& evidence,
    const AppearanceParamsV1& params,
    AppearanceResultV1& out);

} // namespace truthraw_scurve_v1

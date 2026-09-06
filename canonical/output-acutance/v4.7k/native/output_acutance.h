#pragma once
#include <cstddef>
#include <string>

namespace truthraw_v47k {

enum class OutputProfile { Neutral=0, SkinSafe=1, AdaptiveDetail=2 };

struct OutputAcutancePlan {
    float noiseSigmaAt2Pct = 0.0f;
    float noiseConfidence = 0.65f;
    float resizeRatio = 1.0f;
    float resizeNeed = 0.0f;
    float strength = 0.02f;
    float deltaCap = 0.006f;
    float hardEdgeNormStart = 0.035f;
    float hardEdgeNormFull = 0.18f;
    float shadowStart = 0.012f;
    float shadowFull = 0.07f;
    float highlightStart = 0.80f;
    float highlightFull = 1.03f;
};

OutputAcutancePlan choose_output_acutance_plan(float noiseSigmaAt2Pct,
                                                float resizeRatio,
                                                OutputProfile profile);

bool apply_output_acutance(const float* rgbLinear, int width, int height,
                           const OutputAcutancePlan& plan, float* outRgbLinear);

float luminance709(float r, float g, float b);

} // namespace truthraw_v47k

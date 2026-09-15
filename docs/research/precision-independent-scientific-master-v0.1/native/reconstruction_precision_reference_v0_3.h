#pragma once

#include "truthraw/core.h"

#include <cstdint>
#include <vector>

namespace truthraw_precision_v03 {

enum class DirectionDecisionV03 : std::uint8_t {
    MeasuredGreen = 0,
    Horizontal = 1,
    Vertical = 2,
    WeightedBlend = 3
};

enum class ClampDecisionV03 : std::uint8_t {
    None = 0,
    Low = 1,
    High = 2
};

struct ReconstructionTraceV03 {
    // Full halo tile: one decision per Stage-2 sample.
    std::vector<std::uint8_t> greenDirection;
    std::vector<std::uint8_t> greenClamp;

    // Core output: one clamp decision per RGB channel.
    std::vector<std::uint8_t> colorClamp;

    std::uint64_t measuredChannelChecks = 0;
    std::uint64_t measuredChannelViolations = 0;
};

struct ReconstructionPrecisionStatsV03 {
    std::uint64_t rgbSamples = 0;
    double maxAbsError = 0.0;
    double rmsError = 0.0;
    std::uint64_t greenDirectionDivergence = 0;
    std::uint64_t greenClampDivergence = 0;
    std::uint64_t colorClampDivergence = 0;
    std::uint64_t measuredChannelViolationsF32 = 0;
    std::uint64_t measuredChannelViolationsF64 = 0;
};

// Precision-instrumented reproduction of the frozen v4.7i edge-aware,
// measured-preserving reconstruction. This is research/reference code only.
bool v47i_edge_aware_reconstruct_f32_v0_3(
    const float* stage2,
    int tileW,
    int tileH,
    int globalHx0,
    int globalHy0,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    truthraw::CfaPattern cfa,
    std::vector<float>& outCameraRgb,
    ReconstructionTraceV03* trace = nullptr);

bool v47i_edge_aware_reconstruct_f64_v0_3(
    const double* stage2,
    int tileW,
    int tileH,
    int globalHx0,
    int globalHy0,
    int coreX0,
    int coreY0,
    int coreW,
    int coreH,
    truthraw::CfaPattern cfa,
    std::vector<double>& outCameraRgb,
    ReconstructionTraceV03* trace = nullptr);

ReconstructionPrecisionStatsV03 compare_reconstruction_precision_v0_3(
    const std::vector<float>& f32,
    const ReconstructionTraceV03& traceF32,
    const std::vector<double>& f64,
    const ReconstructionTraceV03& traceF64);

} // namespace truthraw_precision_v03

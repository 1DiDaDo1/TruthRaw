#include "reconstruction_precision_reference_v0_3.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

using namespace truthraw_precision_v03;

int main() {
    constexpr int w = 24;
    constexpr int h = 22;
    constexpr int coreX0 = 4;
    constexpr int coreY0 = 4;
    constexpr int coreW = 16;
    constexpr int coreH = 14;

    std::vector<float> f32(static_cast<std::size_t>(w) * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // Deterministic mixed flat/edge/texture field. Values are already
            // Stage-2 normalized scientific samples, not display RGB.
            float v = 0.03f + 0.0065f * float(x) + 0.004f * float(y);
            if (x >= 11) v += 0.27f;
            if (y >= 13) v -= 0.035f;
            if (((x * 7 + y * 11) % 13) == 0) v += 0.021f;
            if (((x + 2 * y) % 9) == 0) v -= 0.013f;
            f32[static_cast<std::size_t>(y) * w + x] = v;
        }
    }

    // Verify the research F32 path reproduces the frozen canonical v4.7i
    // reconstruction exactly for the same float tile.
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction canonical;
    std::vector<float> canonicalOut(static_cast<std::size_t>(coreW) * coreH * 3u);
    const auto status = canonical.reconstructTile(
        f32.data(), w, h, 0, 0,
        coreX0, coreY0, coreW, coreH,
        truthraw::CfaPattern::BGGR,
        canonicalOut.data());
    assert(status);

    ReconstructionTraceV03 trace32;
    std::vector<float> ref32;
    assert(v47i_edge_aware_reconstruct_f32_v0_3(
        f32.data(), w, h, 0, 0,
        coreX0, coreY0, coreW, coreH,
        truthraw::CfaPattern::BGGR,
        ref32, &trace32));
    assert(ref32.size() == canonicalOut.size());
    for (std::size_t i = 0; i < ref32.size(); ++i) {
        assert(std::memcmp(&ref32[i], &canonicalOut[i], sizeof(float)) == 0);
    }

    // Arithmetic-only F64 comparison: promote the identical F32 Stage-2
    // samples. This isolates reconstruction arithmetic from Stage-2 input error.
    std::vector<double> f64(f32.begin(), f32.end());
    ReconstructionTraceV03 trace64;
    std::vector<double> ref64;
    assert(v47i_edge_aware_reconstruct_f64_v0_3(
        f64.data(), w, h, 0, 0,
        coreX0, coreY0, coreW, coreH,
        truthraw::CfaPattern::BGGR,
        ref64, &trace64));

    const auto stats = compare_reconstruction_precision_v0_3(ref32, trace32, ref64, trace64);
    assert(stats.rgbSamples == ref32.size());
    assert(std::isfinite(stats.maxAbsError));
    assert(std::isfinite(stats.rmsError));
    assert(stats.measuredChannelViolationsF32 == 0);
    assert(stats.measuredChannelViolationsF64 == 0);

    // This synthetic case is not a scientific error-budget certification; the
    // bounds only catch implementation explosions/regressions.
    assert(stats.maxAbsError < 1.0e-4);
    assert(stats.rmsError < 1.0e-5);

    std::cout << "test_reconstruction_precision_reference_v0_3 PASS"
              << " max_abs=" << stats.maxAbsError
              << " rms=" << stats.rmsError
              << " dir_div=" << stats.greenDirectionDivergence
              << " green_clamp_div=" << stats.greenClampDivergence
              << " color_clamp_div=" << stats.colorClampDivergence
              << "\n";
    return 0;
}

#include "mixed_precision_runtime_v0_5.h"

#include "mixed_precision_storage_v0_4.h"
#include "reconstruction_precision_reference_v0_3.h"
#include "v47i_stage2_precision_adapter_v0_1.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

struct Collector {
    int width = 0;
    int height = 0;
    std::vector<float> rgb;
    std::uint64_t callbacks = 0;
};

bool collect_tile(int x0,
                  int y0,
                  int w,
                  int h,
                  const float* rgb,
                  std::size_t samples,
                  void* user) {
    auto* c = static_cast<Collector*>(user);
    if (!c || !rgb || w <= 0 || h <= 0 ||
        x0 < 0 || y0 < 0 || x0 + w > c->width || y0 + h > c->height ||
        samples != static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u) {
        return false;
    }

    for (int yy = 0; yy < h; ++yy) {
        for (int xx = 0; xx < w; ++xx) {
            const std::size_t si = 3u * (static_cast<std::size_t>(yy) * static_cast<std::size_t>(w)
                                       + static_cast<std::size_t>(xx));
            const std::size_t di = 3u * (static_cast<std::size_t>(y0 + yy) * static_cast<std::size_t>(c->width)
                                       + static_cast<std::size_t>(x0 + xx));
            c->rgb[di] = rgb[si];
            c->rgb[di + 1u] = rgb[si + 1u];
            c->rgb[di + 2u] = rgb[si + 2u];
        }
    }
    ++c->callbacks;
    return true;
}

} // namespace

int main() {
    constexpr int W = 17;
    constexpr int H = 13;

    truthraw::DecodedDngFrame frame;
    frame.meta.width = W;
    frame.meta.height = H;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 64.25f, 63.75f, 64.5f};
    frame.meta.hasGainField = true;
    frame.meta.hasResidualBlack = true;

    const std::size_t pixels = static_cast<std::size_t>(W) * static_cast<std::size_t>(H);
    frame.raw.resize(pixels);
    frame.gainField.resize(pixels);
    frame.rowBias.resize(H);
    frame.colBias.resize(W);

    for (int y = 0; y < H; ++y) {
        frame.rowBias[static_cast<std::size_t>(y)] = 0.03125f * static_cast<float>((y % 5) - 2);
        for (int x = 0; x < W; ++x) {
            const std::size_t i = static_cast<std::size_t>(y) * static_cast<std::size_t>(W)
                                + static_cast<std::size_t>(x);
            const int structured = (x * 37 + y * 53 + ((x * y) % 29) * 11 + (x ^ y) * 7) % 900;
            frame.raw[i] = static_cast<std::uint16_t>(80 + structured);
            frame.gainField[i] = 0.93f + 0.0025f * static_cast<float>((x * 3 + y * 5) % 41);
        }
    }
    for (int x = 0; x < W; ++x) {
        frame.colBias[static_cast<std::size_t>(x)] = 0.015625f * static_cast<float>((x % 7) - 3);
    }

    const auto rawBefore = frame.raw;
    const auto gainBefore = frame.gainField;
    const auto rowBefore = frame.rowBias;
    const auto colBefore = frame.colBias;

    Collector collector;
    collector.width = W;
    collector.height = H;
    collector.rgb.assign(pixels * 3u, 0.0f);

    truthraw_precision_v05::MixedPrecisionRuntimeConfigV05 cfg;
    cfg.core = 5;
    cfg.halo = 4;

    truthraw_precision_v05::MixedPrecisionRuntimeDiagnosticsV05 diag;
    assert(truthraw_precision_v05::reconstruct_f64_compute_f32_stream_v0_5(
        frame, cfg, collect_tile, &collector, diag));

    // The sealed numerical source representation remains read-only.
    assert(frame.raw == rawBefore);
    assert(frame.gainField == gainBefore);
    assert(frame.rowBias == rowBefore);
    assert(frame.colBias == colBefore);

    // 17x13 with 5x5 cores => 4x3 streamed tiles.
    assert(collector.callbacks == 12u);
    assert(diag.tiles == 12u);
    assert(diag.stage2SamplesComputed > pixels); // halo overlap is intentionally recomputed.
    assert(diag.reconstructedRgbSamples == pixels * 3u);
    assert(diag.measuredChannelChecks == pixels);
    assert(diag.measuredChannelViolations == 0u);
    assert(diag.estimatedPeakTileWorkspaceBytes > 0u);
    assert(diag.storage.samples == pixels * 3u);
    assert(diag.storage.nonFiniteInputs == 0u);
    assert(diag.storage.finiteRoundTripFailures == 0u);
    assert(diag.exactIntegerEvidenceReadOnly);
    assert(diag.stage2UsesFloat64Arithmetic);
    assert(diag.reconstructionUsesFloat64Arithmetic);
    assert(diag.storageUsesFloat32);
    assert(!diag.appearanceApplied);
    assert(diag.stage2MetadataPromotedFromLegacyFloatContainers);

    // Independent non-tiled F64 scientific reference over the same decoded
    // evidence and metadata semantics.
    std::vector<double> stage2Full;
    assert(truthraw_precision_v01::v47i_stage2_tile_f64_v0_1(
        frame, 0, 0, W, H, stage2Full));

    truthraw_precision_v03::ReconstructionTraceV03 fullTrace;
    std::vector<double> fullF64;
    assert(truthraw_precision_v03::v47i_edge_aware_reconstruct_f64_v0_3(
        stage2Full.data(), W, H,
        0, 0,
        0, 0, W, H,
        frame.meta.cfa,
        fullF64,
        &fullTrace));
    assert(fullTrace.measuredChannelChecks == pixels);
    assert(fullTrace.measuredChannelViolations == 0u);

    std::vector<float> fullStoredF32;
    const auto fullStorage = truthraw_precision_v04::quantize_f64_to_f32_storage_v0_4(
        fullF64.data(), fullF64.size(), fullStoredF32);
    assert(fullStorage.samples == pixels * 3u);
    assert(fullStorage.nonFiniteInputs == 0u);
    assert(fullStorage.finiteRoundTripFailures == 0u);

    // Critical parity gate: tiling, halo handling and streaming may not change
    // a single stored float32 Scientific Master sample relative to the one-shot
    // F64 compute -> F32 storage reference.
    assert(collector.rgb.size() == fullStoredF32.size());
    for (std::size_t i = 0; i < collector.rgb.size(); ++i) {
        assert(collector.rgb[i] == fullStoredF32[i]);
    }

    // Aggregated storage diagnostics must equal the one-shot reference within
    // reduction round-off; max error is exactly reduction-order independent.
    assert(diag.storage.maxAbsError == fullStorage.maxAbsError);
    const double rmsDelta = diag.storage.rmsError - fullStorage.rmsError;
    assert(rmsDelta < 1e-18 && rmsDelta > -1e-18);

    std::cout << "test_mixed_precision_runtime_v0_5 PASS"
              << " tiles=" << diag.tiles
              << " rgb_samples=" << diag.reconstructedRgbSamples
              << " storage_max=" << diag.storage.maxAbsError
              << " storage_rms=" << diag.storage.rmsError
              << " peak_tile_bytes=" << diag.estimatedPeakTileWorkspaceBytes
              << "\n";
    return 0;
}

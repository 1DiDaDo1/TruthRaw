#include "mixed_precision_runtime_v0_5.h"

#include "mixed_precision_storage_v0_4.h"
#include "reconstruction_precision_reference_v0_3.h"
#include "v47i_stage2_precision_adapter_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace truthraw_precision_v05 {
namespace {

bool valid_frame(const truthraw::DecodedDngFrame& frame) {
    if (frame.meta.width <= 0 || frame.meta.height <= 0) return false;
    const std::size_t pixels = static_cast<std::size_t>(frame.meta.width) * static_cast<std::size_t>(frame.meta.height);
    if (frame.raw.size() < pixels) return false;
    if (frame.meta.hasGainField && frame.gainField.size() < pixels) return false;
    if (frame.meta.hasResidualBlack &&
        (frame.rowBias.size() < static_cast<std::size_t>(frame.meta.height) ||
         frame.colBias.size() < static_cast<std::size_t>(frame.meta.width))) return false;
    return true;
}

std::size_t estimated_workspace_bytes(
    int tileW,
    int tileH,
    int coreW,
    int coreH) {

    const std::size_t tileSamples = static_cast<std::size_t>(tileW) * static_cast<std::size_t>(tileH);
    const std::size_t corePixels = static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH);
    const std::size_t rgbSamples = corePixels * 3u;

    // Conservative research estimate excluding allocator/bookkeeping overhead.
    // Reconstruction phase: F64 Stage-2 + internal F64 green + F64 RGB output
    // + trace arrays. Quantization phase replaces green with F32 stored RGB.
    const std::size_t traceBytes = tileSamples * 2u + rgbSamples;
    const std::size_t reconstructPhase = tileSamples * sizeof(double) * 2u
                                       + rgbSamples * sizeof(double)
                                       + traceBytes;
    const std::size_t quantizePhase = tileSamples * sizeof(double)
                                    + rgbSamples * sizeof(double)
                                    + rgbSamples * sizeof(float)
                                    + traceBytes;
    return std::max(reconstructPhase, quantizePhase);
}

} // namespace

bool reconstruct_f64_compute_f32_stream_v0_5(
    const truthraw::DecodedDngFrame& frame,
    const MixedPrecisionRuntimeConfigV05& config,
    ScientificMasterTileSinkV05 sink,
    void* sinkUser,
    MixedPrecisionRuntimeDiagnosticsV05& diagnostics) {

    diagnostics = MixedPrecisionRuntimeDiagnosticsV05{};
    if (!valid_frame(frame) || !sink || config.core <= 0 || config.halo < 4) return false;

    long double storageSumSq = 0.0L;
    std::uint64_t storageFiniteCount = 0;

    for (int coreY0 = 0; coreY0 < frame.meta.height; coreY0 += config.core) {
        const int coreH = std::min(config.core, frame.meta.height - coreY0);
        for (int coreX0 = 0; coreX0 < frame.meta.width; coreX0 += config.core) {
            const int coreW = std::min(config.core, frame.meta.width - coreX0);

            const int hx0 = std::max(0, coreX0 - config.halo);
            const int hy0 = std::max(0, coreY0 - config.halo);
            const int hx1 = std::min(frame.meta.width, coreX0 + coreW + config.halo);
            const int hy1 = std::min(frame.meta.height, coreY0 + coreH + config.halo);
            const int tileW = hx1 - hx0;
            const int tileH = hy1 - hy0;

            std::vector<double> stage2;
            if (!truthraw_precision_v01::v47i_stage2_tile_f64_v0_1(
                    frame, hx0, hy0, tileW, tileH, stage2)) return false;

            truthraw_precision_v03::ReconstructionTraceV03 trace;
            std::vector<double> reconstructedF64;
            if (!truthraw_precision_v03::v47i_edge_aware_reconstruct_f64_v0_3(
                    stage2.data(), tileW, tileH,
                    hx0, hy0,
                    coreX0, coreY0, coreW, coreH,
                    frame.meta.cfa,
                    reconstructedF64,
                    &trace)) return false;

            std::vector<float> storedF32;
            const auto q = truthraw_precision_v04::quantize_f64_to_f32_storage_v0_4(
                reconstructedF64.data(), reconstructedF64.size(), storedF32);

            const std::uint64_t finiteCount = q.samples - q.nonFiniteInputs - q.finiteRoundTripFailures;
            storageSumSq += static_cast<long double>(q.rmsError) * static_cast<long double>(q.rmsError)
                          * static_cast<long double>(finiteCount);
            storageFiniteCount += finiteCount;

            diagnostics.storage.samples += q.samples;
            diagnostics.storage.maxAbsError = std::max(diagnostics.storage.maxAbsError, q.maxAbsError);
            diagnostics.storage.maxRelativeError = std::max(diagnostics.storage.maxRelativeError, q.maxRelativeError);
            diagnostics.storage.nonFiniteInputs += q.nonFiniteInputs;
            diagnostics.storage.finiteRoundTripFailures += q.finiteRoundTripFailures;

            diagnostics.tiles += 1;
            diagnostics.stage2SamplesComputed += static_cast<std::uint64_t>(stage2.size());
            diagnostics.reconstructedRgbSamples += static_cast<std::uint64_t>(reconstructedF64.size());
            diagnostics.measuredChannelChecks += trace.measuredChannelChecks;
            diagnostics.measuredChannelViolations += trace.measuredChannelViolations;
            diagnostics.estimatedPeakTileWorkspaceBytes = std::max(
                diagnostics.estimatedPeakTileWorkspaceBytes,
                estimated_workspace_bytes(tileW, tileH, coreW, coreH));

            if (!sink(coreX0, coreY0, coreW, coreH,
                      storedF32.data(), storedF32.size(), sinkUser)) return false;
        }
    }

    if (storageFiniteCount > 0) {
        diagnostics.storage.rmsError = std::sqrt(
            static_cast<double>(storageSumSq / static_cast<long double>(storageFiniteCount)));
    }
    return true;
}

} // namespace truthraw_precision_v05

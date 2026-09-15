#pragma once

#include "mixed_precision_storage_v0_4.h"
#include "truthraw/core.h"

#include <cstddef>
#include <cstdint>

namespace truthraw_precision_v05 {

struct MixedPrecisionRuntimeConfigV05 {
    int core = 512;
    int halo = 4;
};

struct MixedPrecisionRuntimeDiagnosticsV05 {
    std::uint64_t tiles = 0;
    std::uint64_t stage2SamplesComputed = 0;
    std::uint64_t reconstructedRgbSamples = 0;
    std::uint64_t measuredChannelChecks = 0;
    std::uint64_t measuredChannelViolations = 0;
    std::size_t estimatedPeakTileWorkspaceBytes = 0;

    truthraw_precision_v04::StorageQuantizationStatsV04 storage{};

    bool exactIntegerEvidenceReadOnly = true;
    bool stage2UsesFloat64Arithmetic = true;
    bool reconstructionUsesFloat64Arithmetic = true;
    bool storageUsesFloat32 = true;
    bool appearanceApplied = false;

    // Current DecodedDngFrame stores black/white/gain/residual metadata in
    // historical float containers. v0.5 performs arithmetic in float64 but
    // does not claim that promoting those float metadata values creates more
    // calibration authority or recovers precision that the decoder discarded.
    bool stage2MetadataPromotedFromLegacyFloatContainers = true;
};

// Streaming sink for one completed Scientific Master camera-RGB tile.
// rgb points to coreWidth*coreHeight*3 interleaved float32 samples and is valid
// only for the duration of the callback. Returning false aborts the run.
using ScientificMasterTileSinkV05 = bool (*)(
    int coreX0,
    int coreY0,
    int coreWidth,
    int coreHeight,
    const float* rgb,
    std::size_t rgbSamples,
    void* user);

// Research runtime candidate:
//   exact decoded integer evidence (read-only)
//   -> float64 Stage-2 tile
//   -> float64 frozen-v4.7i-topology reconstruction
//   -> one downstream float64-to-float32 storage quantization
//   -> streaming tile sink
//
// No appearance processing is permitted here. Precision affects numerical
// execution/storage only and cannot alter measured/reconstructed authority.
bool reconstruct_f64_compute_f32_stream_v0_5(
    const truthraw::DecodedDngFrame& frame,
    const MixedPrecisionRuntimeConfigV05& config,
    ScientificMasterTileSinkV05 sink,
    void* sinkUser,
    MixedPrecisionRuntimeDiagnosticsV05& diagnostics);

} // namespace truthraw_precision_v05

#pragma once

#include "mixed_precision_storage_v0_4.h"
#include "scientific_stage2_source_v0_6.h"

#include <cstddef>
#include <cstdint>

namespace truthraw_precision_v06 {

struct MixedPrecisionRuntimeConfigV06 {
    int core = 512;
    int halo = 4;
};

struct MixedPrecisionRuntimeDiagnosticsV06 {
    std::uint64_t tiles = 0;
    std::uint64_t stage2SamplesComputed = 0;
    std::uint64_t gainApplicationsComputed = 0;
    std::uint64_t reconstructedRgbSamples = 0;
    std::uint64_t measuredChannelChecks = 0;
    std::uint64_t measuredChannelViolations = 0;
    std::size_t estimatedPeakTileWorkspaceBytes = 0;
    std::size_t gainMapPrograms = 0;

    truthraw_precision_v04::StorageQuantizationStatsV04 storage{};

    bool exactIntegerEvidenceReadOnly = true;
    bool stage2UsesFloat64Arithmetic = true;
    bool gainMapInterpolationUsesFloat64Arithmetic = true;
    bool compactFloat32GainMapKnotsRetained = false;
    bool legacyFullFrameFloatGainFieldBypassed = true;
    bool reconstructionUsesFloat64Arithmetic = true;
    bool storageUsesFloat32 = true;
    bool appearanceApplied = false;
};

using ScientificMasterTileSinkV06 = bool (*)(
    int coreX0,
    int coreY0,
    int coreWidth,
    int coreHeight,
    const float* rgb,
    std::size_t rgbSamples,
    void* user);

// Streaming research runtime that closes the v0.5 GainMap precision gap:
// exact integer CFA evidence -> F64 Stage-2 with compact F32 GainMap knots and
// F64 interpolation -> F64 branch-sensitive reconstruction -> one downstream
// F32 storage quantization -> tile sink.
bool reconstruct_scientific_f64_compute_f32_stream_v0_6(
    const ScientificStage2SourceV06& source,
    const MixedPrecisionRuntimeConfigV06& config,
    ScientificMasterTileSinkV06 sink,
    void* sinkUser,
    MixedPrecisionRuntimeDiagnosticsV06& diagnostics);

} // namespace truthraw_precision_v06

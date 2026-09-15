#pragma once

#include "mixed_precision_runtime_v0_6.h"
#include "reconstruction_precision_reference_v0_3.h"

#include <cstddef>
#include <cstdint>

namespace truthraw_precision_v08 {

struct MixedPrecisionAuditDiagnosticsV08 {
    truthraw_precision_v06::MixedPrecisionRuntimeDiagnosticsV06 runtime{};
    std::uint64_t auditTiles = 0;
    std::uint64_t auditRgbSamples = 0;
    bool exposesPreStorageFloat64 = true;
    bool exposesPostStorageFloat32 = true;
    bool uncertaintyApplied = false;
    bool scientificAuthorityChanged = false;
};

// Research-only audit callback. F64 and F32 arrays describe the same already-
// reconstructed tile before and after storage quantization. The trace belongs to
// the F64 branch-sensitive reconstruction. The callback may bind local admitted
// uncertainty/covariance externally; this runtime does not manufacture it.
using ScientificMasterPrecisionAuditSinkV08 = bool (*)(
    int coreX0,
    int coreY0,
    int coreWidth,
    int coreHeight,
    const double* reconstructedF64,
    const float* storedF32,
    std::size_t rgbSamples,
    const truthraw_precision_v03::ReconstructionTraceV03& f64Trace,
    void* user);

// Exact integer CFA evidence -> F64 Stage-2 -> F64 reconstruction -> F32 storage,
// while exposing both pre- and post-storage representations to an audit sink.
// Appearance is forbidden. This function does not itself admit any uncertainty
// model and cannot change evidence/reconstruction authority.
bool reconstruct_scientific_f64_f32_audit_stream_v0_8(
    const truthraw_precision_v06::ScientificStage2SourceV06& source,
    const truthraw_precision_v06::MixedPrecisionRuntimeConfigV06& config,
    ScientificMasterPrecisionAuditSinkV08 sink,
    void* sinkUser,
    MixedPrecisionAuditDiagnosticsV08& diagnostics);

} // namespace truthraw_precision_v08

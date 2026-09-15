#include "mixed_precision_uncertainty_audit_v0_8.h"

#include "mixed_precision_storage_v0_4.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace truthraw_precision_v08 {
namespace {

std::size_t estimated_workspace_bytes_v0_8(int tileW, int tileH, int coreW, int coreH) {
    const std::size_t tileSamples = static_cast<std::size_t>(tileW) * static_cast<std::size_t>(tileH);
    const std::size_t rgbSamples = static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH) * 3u;
    const std::size_t traceBytes = tileSamples * 2u + rgbSamples;
    return tileSamples * sizeof(double) * 2u
         + rgbSamples * (sizeof(double) + sizeof(float))
         + traceBytes;
}

} // namespace

bool reconstruct_scientific_f64_f32_audit_stream_v0_8(
    const truthraw_precision_v06::ScientificStage2SourceV06& source,
    const truthraw_precision_v06::MixedPrecisionRuntimeConfigV06& config,
    ScientificMasterPrecisionAuditSinkV08 sink,
    void* sinkUser,
    MixedPrecisionAuditDiagnosticsV08& diagnostics) {

    diagnostics = MixedPrecisionAuditDiagnosticsV08{};
    auto& d = diagnostics.runtime;
    d.gainMapPrograms = source.gainMaps.size();
    d.compactFloat32GainMapKnotsRetained = !source.gainMaps.empty();
    d.exactIntegerEvidenceReadOnly = source.exactIntegerEvidenceRetained;

    if (!truthraw_precision_v06::scientific_stage2_source_valid_v0_6(source) ||
        !sink || config.core <= 0 || config.halo < 4 || source.appearanceApplied) return false;

    long double storageSumSq = 0.0L;
    std::uint64_t storageFiniteCount = 0;

    for (int coreY0 = 0; coreY0 < source.height; coreY0 += config.core) {
        const int coreH = std::min(config.core, source.height - coreY0);
        for (int coreX0 = 0; coreX0 < source.width; coreX0 += config.core) {
            const int coreW = std::min(config.core, source.width - coreX0);
            const int hx0 = std::max(0, coreX0 - config.halo);
            const int hy0 = std::max(0, coreY0 - config.halo);
            const int hx1 = std::min(source.width, coreX0 + coreW + config.halo);
            const int hy1 = std::min(source.height, coreY0 + coreH + config.halo);
            const int tileW = hx1 - hx0;
            const int tileH = hy1 - hy0;

            truthraw_precision_v06::ScientificStage2DiagnosticsV06 stage2Diag;
            std::vector<double> stage2;
            if (!truthraw_precision_v06::scientific_stage2_tile_f64_v0_6(
                    source, hx0, hy0, tileW, tileH, stage2, &stage2Diag)) return false;

            truthraw_precision_v03::ReconstructionTraceV03 trace;
            std::vector<double> reconstructedF64;
            if (!truthraw_precision_v03::v47i_edge_aware_reconstruct_f64_v0_3(
                    stage2.data(), tileW, tileH,
                    hx0, hy0,
                    coreX0, coreY0, coreW, coreH,
                    source.cfa,
                    reconstructedF64,
                    &trace)) return false;

            std::vector<float> storedF32;
            const auto q = truthraw_precision_v04::quantize_f64_to_f32_storage_v0_4(
                reconstructedF64.data(), reconstructedF64.size(), storedF32);

            const std::uint64_t finiteCount = q.samples - q.nonFiniteInputs - q.finiteRoundTripFailures;
            storageSumSq += static_cast<long double>(q.rmsError) * static_cast<long double>(q.rmsError)
                          * static_cast<long double>(finiteCount);
            storageFiniteCount += finiteCount;

            d.storage.samples += q.samples;
            d.storage.maxAbsError = std::max(d.storage.maxAbsError, q.maxAbsError);
            d.storage.maxRelativeError = std::max(d.storage.maxRelativeError, q.maxRelativeError);
            d.storage.nonFiniteInputs += q.nonFiniteInputs;
            d.storage.finiteRoundTripFailures += q.finiteRoundTripFailures;
            ++d.tiles;
            d.stage2SamplesComputed += stage2Diag.samples;
            d.gainApplicationsComputed += stage2Diag.gainApplications;
            d.reconstructedRgbSamples += static_cast<std::uint64_t>(reconstructedF64.size());
            d.measuredChannelChecks += trace.measuredChannelChecks;
            d.measuredChannelViolations += trace.measuredChannelViolations;
            d.estimatedPeakTileWorkspaceBytes = std::max(
                d.estimatedPeakTileWorkspaceBytes,
                estimated_workspace_bytes_v0_8(tileW, tileH, coreW, coreH));

            ++diagnostics.auditTiles;
            diagnostics.auditRgbSamples += static_cast<std::uint64_t>(reconstructedF64.size());

            if (!sink(coreX0, coreY0, coreW, coreH,
                      reconstructedF64.data(), storedF32.data(), reconstructedF64.size(),
                      trace, sinkUser)) return false;
        }
    }

    if (storageFiniteCount > 0) {
        d.storage.rmsError = std::sqrt(
            static_cast<double>(storageSumSq / static_cast<long double>(storageFiniteCount)));
    }
    diagnostics.uncertaintyApplied = false;
    diagnostics.scientificAuthorityChanged = false;
    return true;
}

} // namespace truthraw_precision_v08

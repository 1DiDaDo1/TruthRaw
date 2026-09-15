#include "mixed_precision_uncertainty_audit_v0_8.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

struct AuditCollector {
    std::uint64_t callbacks = 0;
    std::uint64_t rgbSamples = 0;
    std::uint64_t measuredChecks = 0;
    std::uint64_t measuredViolations = 0;
    double maxAbsStorageError = 0.0;
};

bool audit_tile(int x0, int y0, int w, int h,
                const double* reconstructedF64,
                const float* storedF32,
                std::size_t rgbSamples,
                const truthraw_precision_v03::ReconstructionTraceV03& trace,
                void* user) {
    auto* c = static_cast<AuditCollector*>(user);
    if (!c || !reconstructedF64 || !storedF32 || x0 < 0 || y0 < 0 ||
        w <= 0 || h <= 0 ||
        rgbSamples != static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u) return false;

    for (std::size_t i = 0; i < rgbSamples; ++i) {
        assert(storedF32[i] == static_cast<float>(reconstructedF64[i]));
        c->maxAbsStorageError = std::max(
            c->maxAbsStorageError,
            std::abs(static_cast<double>(storedF32[i]) - reconstructedF64[i]));
    }

    c->rgbSamples += static_cast<std::uint64_t>(rgbSamples);
    c->measuredChecks += trace.measuredChannelChecks;
    c->measuredViolations += trace.measuredChannelViolations;
    ++c->callbacks;
    return true;
}

} // namespace

int main() {
    constexpr int W = 11;
    constexpr int H = 9;

    truthraw_precision_v06::ScientificStage2SourceV06 source;
    source.width = W;
    source.height = H;
    source.rawRowStrideSamples = W;
    source.cfa = truthraw::CfaPattern::BGGR;
    source.blackPhase = {64.0, 63.75, 64.25, 64.0};
    source.whiteLevel = 1023.0;
    source.exactIntegerEvidenceRetained = true;
    source.appearanceApplied = false;
    source.raw.resize(static_cast<std::size_t>(W) * static_cast<std::size_t>(H));

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const int signal = 96 + ((x * 47 + y * 71 + (x * y) * 3) % 780);
            source.raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(W)
                     + static_cast<std::size_t>(x)] = static_cast<std::uint16_t>(signal);
        }
    }

    const auto rawBefore = source.raw;
    assert(truthraw_precision_v06::scientific_stage2_source_valid_v0_6(source));

    truthraw_precision_v06::MixedPrecisionRuntimeConfigV06 cfg;
    cfg.core = 4;
    cfg.halo = 4;

    AuditCollector collector;
    truthraw_precision_v08::MixedPrecisionAuditDiagnosticsV08 diagnostics;
    assert(truthraw_precision_v08::reconstruct_scientific_f64_f32_audit_stream_v0_8(
        source, cfg, audit_tile, &collector, diagnostics));

    const std::uint64_t pixels = static_cast<std::uint64_t>(W) * static_cast<std::uint64_t>(H);
    assert(source.raw == rawBefore);
    assert(diagnostics.auditTiles == 9u);
    assert(collector.callbacks == diagnostics.auditTiles);
    assert(diagnostics.auditRgbSamples == pixels * 3u);
    assert(collector.rgbSamples == diagnostics.auditRgbSamples);
    assert(diagnostics.runtime.reconstructedRgbSamples == pixels * 3u);
    assert(diagnostics.runtime.measuredChannelChecks == pixels);
    assert(diagnostics.runtime.measuredChannelViolations == 0u);
    assert(collector.measuredChecks == pixels);
    assert(collector.measuredViolations == 0u);
    assert(diagnostics.exposesPreStorageFloat64);
    assert(diagnostics.exposesPostStorageFloat32);
    assert(!diagnostics.uncertaintyApplied);
    assert(!diagnostics.scientificAuthorityChanged);
    assert(!diagnostics.runtime.appearanceApplied);
    assert(diagnostics.runtime.exactIntegerEvidenceReadOnly);
    assert(diagnostics.runtime.reconstructionUsesFloat64Arithmetic);
    assert(diagnostics.runtime.storageUsesFloat32);
    assert(diagnostics.runtime.storage.finiteRoundTripFailures == 0u);
    assert(collector.maxAbsStorageError == diagnostics.runtime.storage.maxAbsError);

    std::cout << "test_mixed_precision_uncertainty_audit_v0_8 PASS"
              << " tiles=" << diagnostics.auditTiles
              << " rgb=" << diagnostics.auditRgbSamples
              << " storage_max=" << diagnostics.runtime.storage.maxAbsError
              << "\n";
    return 0;
}

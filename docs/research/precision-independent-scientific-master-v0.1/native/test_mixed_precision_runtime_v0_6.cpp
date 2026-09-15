#include "mixed_precision_runtime_v0_6.h"

#include "gainmap_precision_v0_2.h"
#include "mixed_precision_storage_v0_4.h"
#include "reconstruction_precision_reference_v0_3.h"
#include "v47i_stage2_precision_adapter_v0_1.h"

#include <algorithm>
#include <cassert>
#include <cmath>
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

bool collect_tile(int x0, int y0, int w, int h,
                  const float* rgb, std::size_t samples, void* user) {
    auto* c = static_cast<Collector*>(user);
    if (!c || !rgb || w <= 0 || h <= 0 ||
        x0 < 0 || y0 < 0 || x0 + w > c->width || y0 + h > c->height ||
        samples != static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u) return false;

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

truthraw_precision_v06::GainMapProgramV06 make_program(int parityY, int parityX, int programIndex,
                                                        int width, int height) {
    truthraw_precision_v06::GainMapProgramV06 p;
    p.area.top = parityY;
    p.area.left = parityX;
    p.area.bottom = height;
    p.area.right = width;
    p.area.plane = 0;
    p.area.planes = 1;
    p.area.rowPitch = 2;
    p.area.colPitch = 2;

    p.grid.pointsV = 4;
    p.grid.pointsH = 5;
    p.grid.planes = 1;
    p.grid.spacingV = 1.0 / 3.0;
    p.grid.spacingH = 1.0 / 4.0;
    p.grid.originV = 0.0;
    p.grid.originH = 0.0;
    p.grid.samples.resize(static_cast<std::size_t>(p.grid.pointsV * p.grid.pointsH));
    for (int r = 0; r < p.grid.pointsV; ++r) {
        for (int c = 0; c < p.grid.pointsH; ++c) {
            const float base = 0.91f + 0.037f * static_cast<float>(programIndex);
            const float shape = 0.0137f * static_cast<float>(r)
                              - 0.0093f * static_cast<float>(c)
                              + 0.0011f * static_cast<float>(r * c);
            p.grid.samples[static_cast<std::size_t>(r * p.grid.pointsH + c)] = base + shape;
        }
    }
    return p;
}

} // namespace

int main() {
    constexpr int W = 17;
    constexpr int H = 13;
    const std::size_t pixels = static_cast<std::size_t>(W) * static_cast<std::size_t>(H);

    truthraw_precision_v06::ScientificStage2SourceV06 source;
    source.width = W;
    source.height = H;
    source.rawRowStrideSamples = W;
    source.cfa = truthraw::CfaPattern::BGGR;
    source.raw.resize(pixels);
    source.blackPhase = {64.0, 64.25, 63.75, 64.5};
    source.whiteLevel = 1023.0;
    source.hasResidualBlack = true;
    source.rowBias.resize(H);
    source.colBias.resize(W);

    for (int y = 0; y < H; ++y) {
        source.rowBias[static_cast<std::size_t>(y)] = 0.03125 * static_cast<double>((y % 5) - 2);
        for (int x = 0; x < W; ++x) {
            const std::size_t i = static_cast<std::size_t>(y) * static_cast<std::size_t>(W)
                                + static_cast<std::size_t>(x);
            const int structured = (x * 43 + y * 61 + ((x * y) % 31) * 13 + (x ^ y) * 5) % 900;
            source.raw[i] = static_cast<std::uint16_t>(80 + structured);
        }
    }
    for (int x = 0; x < W; ++x) {
        source.colBias[static_cast<std::size_t>(x)] = 0.015625 * static_cast<double>((x % 7) - 3);
    }

    // Four disjoint row/column parity programs reproduce the structure used by
    // the tested HONOR vendor GainMap family: one compact GainMap per CFA phase.
    source.gainMaps.push_back(make_program(0, 0, 0, W, H));
    source.gainMaps.push_back(make_program(0, 1, 1, W, H));
    source.gainMaps.push_back(make_program(1, 0, 2, W, H));
    source.gainMaps.push_back(make_program(1, 1, 3, W, H));

    assert(truthraw_precision_v06::scientific_stage2_source_valid_v0_6(source));
    const auto rawBefore = source.raw;

    // Full scientific F64 Stage-2 reference with compact GainMap interpolation.
    truthraw_precision_v06::ScientificStage2DiagnosticsV06 stage2Diag;
    std::vector<double> stage2Scientific;
    assert(truthraw_precision_v06::scientific_stage2_tile_f64_v0_6(
        source, 0, 0, W, H, stage2Scientific, &stage2Diag));
    assert(stage2Diag.samples == pixels);
    assert(stage2Diag.samplesWithGain == pixels);
    assert(stage2Diag.gainApplications == pixels);
    assert(stage2Diag.usedFloat64Arithmetic);
    assert(stage2Diag.usedCompactFloat32GainMapKnots);
    assert(stage2Diag.bypassedLegacyFullFrameFloatGainField);
    assert(source.raw == rawBefore);

    // Build the historical full-frame float gain raster using the validated
    // SDK-structured F32 row stepping, then run the legacy F64 Stage-2 adapter.
    // A non-zero delta demonstrates why merely promoting an already-rasterized
    // float gain field to double is not the same numerical reference.
    truthraw::DecodedDngFrame legacy;
    legacy.meta.width = W;
    legacy.meta.height = H;
    legacy.meta.cfa = source.cfa;
    legacy.meta.whiteLevel = static_cast<float>(source.whiteLevel);
    for (int i = 0; i < 4; ++i) legacy.meta.blackPhase[static_cast<std::size_t>(i)] = static_cast<float>(source.blackPhase[static_cast<std::size_t>(i)]);
    legacy.meta.hasGainField = true;
    legacy.meta.hasResidualBlack = true;
    legacy.raw = source.raw;
    legacy.gainField.assign(pixels, 1.0f);
    legacy.rowBias.resize(H);
    legacy.colBias.resize(W);
    for (int y = 0; y < H; ++y) legacy.rowBias[static_cast<std::size_t>(y)] = static_cast<float>(source.rowBias[static_cast<std::size_t>(y)]);
    for (int x = 0; x < W; ++x) legacy.colBias[static_cast<std::size_t>(x)] = static_cast<float>(source.colBias[static_cast<std::size_t>(x)]);

    const truthraw_precision_v02::GainMapBoundsV02 bounds{0, 0, H, W};
    for (const auto& program : source.gainMaps) {
        for (int y = program.area.top; y < program.area.bottom; y += program.area.rowPitch) {
            std::vector<float> row;
            assert(truthraw_precision_v02::gainmap_row_sdk_f32_v0_2(
                program.grid, bounds, program.area, y, program.mapPlane, row));
            int x = program.area.left;
            for (float g : row) {
                legacy.gainField[static_cast<std::size_t>(y) * static_cast<std::size_t>(W)
                               + static_cast<std::size_t>(x)] = g;
                x += program.area.colPitch;
            }
        }
    }

    std::vector<double> stage2LegacyPromoted;
    assert(truthraw_precision_v01::v47i_stage2_tile_f64_v0_1(
        legacy, 0, 0, W, H, stage2LegacyPromoted));
    double legacyMaxDelta = 0.0;
    for (std::size_t i = 0; i < pixels; ++i) {
        legacyMaxDelta = std::max(legacyMaxDelta,
                                  std::abs(stage2Scientific[i] - stage2LegacyPromoted[i]));
    }
    assert(legacyMaxDelta > 0.0);
    assert(legacyMaxDelta < 1e-5);

    Collector collector;
    collector.width = W;
    collector.height = H;
    collector.rgb.assign(pixels * 3u, 0.0f);

    truthraw_precision_v06::MixedPrecisionRuntimeConfigV06 cfg;
    cfg.core = 5;
    cfg.halo = 4;
    truthraw_precision_v06::MixedPrecisionRuntimeDiagnosticsV06 runtimeDiag;
    assert(truthraw_precision_v06::reconstruct_scientific_f64_compute_f32_stream_v0_6(
        source, cfg, collect_tile, &collector, runtimeDiag));

    assert(source.raw == rawBefore);
    assert(collector.callbacks == 12u);
    assert(runtimeDiag.tiles == 12u);
    assert(runtimeDiag.stage2SamplesComputed > pixels);
    assert(runtimeDiag.gainApplicationsComputed > pixels);
    assert(runtimeDiag.reconstructedRgbSamples == pixels * 3u);
    assert(runtimeDiag.measuredChannelChecks == pixels);
    assert(runtimeDiag.measuredChannelViolations == 0u);
    assert(runtimeDiag.gainMapPrograms == 4u);
    assert(runtimeDiag.compactFloat32GainMapKnotsRetained);
    assert(runtimeDiag.legacyFullFrameFloatGainFieldBypassed);
    assert(runtimeDiag.gainMapInterpolationUsesFloat64Arithmetic);
    assert(runtimeDiag.reconstructionUsesFloat64Arithmetic);
    assert(runtimeDiag.storageUsesFloat32);
    assert(!runtimeDiag.appearanceApplied);

    truthraw_precision_v03::ReconstructionTraceV03 fullTrace;
    std::vector<double> fullF64;
    assert(truthraw_precision_v03::v47i_edge_aware_reconstruct_f64_v0_3(
        stage2Scientific.data(), W, H,
        0, 0, 0, 0, W, H,
        source.cfa, fullF64, &fullTrace));
    assert(fullTrace.measuredChannelChecks == pixels);
    assert(fullTrace.measuredChannelViolations == 0u);

    std::vector<float> fullStored;
    const auto fullStorage = truthraw_precision_v04::quantize_f64_to_f32_storage_v0_4(
        fullF64.data(), fullF64.size(), fullStored);
    assert(fullStorage.samples == pixels * 3u);
    assert(fullStorage.nonFiniteInputs == 0u);
    assert(fullStorage.finiteRoundTripFailures == 0u);

    // Critical v0.6 parity gate: compact GainMap + tiled streaming must produce
    // exactly the same stored F32 Scientific Master as one-shot F64 Stage-2,
    // one-shot F64 reconstruction and one final F32 quantization.
    assert(collector.rgb.size() == fullStored.size());
    for (std::size_t i = 0; i < collector.rgb.size(); ++i) {
        assert(collector.rgb[i] == fullStored[i]);
    }
    assert(runtimeDiag.storage.maxAbsError == fullStorage.maxAbsError);
    assert(std::abs(runtimeDiag.storage.rmsError - fullStorage.rmsError) < 1e-15);

    std::cout << "test_mixed_precision_runtime_v0_6 PASS"
              << " legacy_gainfield_stage2_max_delta=" << legacyMaxDelta
              << " tiles=" << runtimeDiag.tiles
              << " storage_max=" << runtimeDiag.storage.maxAbsError
              << " peak_tile_bytes=" << runtimeDiag.estimatedPeakTileWorkspaceBytes
              << "\n";
    return 0;
}

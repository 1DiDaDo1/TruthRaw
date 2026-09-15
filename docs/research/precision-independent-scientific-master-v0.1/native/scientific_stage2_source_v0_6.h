#pragma once

#include "gainmap_precision_v0_2.h"
#include "truthraw/core.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace truthraw_precision_v06 {

struct GainMapProgramV06 {
    truthraw_precision_v02::GainMapAreaV02 area{};
    truthraw_precision_v02::GainMapGridV02 grid{};
    int mapPlane = 0;
};

// Research-only scientific Stage-2 source. It deliberately does not modify the
// frozen canonical DecodedDngFrame contract. Exact integer CFA codes remain the
// source samples; numerical metadata can be supplied in double precision and
// compact DNG GainMap grids stay compact instead of being rasterized into a
// full-frame float gain field before branch-sensitive reconstruction.
struct ScientificStage2SourceV06 {
    int width = 0;
    int height = 0;
    int rawRowStrideSamples = 0;
    truthraw::CfaPattern cfa = truthraw::CfaPattern::BGGR;

    std::vector<std::uint16_t> raw;
    std::array<double,4> blackPhase {0.0,0.0,0.0,0.0};
    double whiteLevel = 1.0;

    bool hasResidualBlack = false;
    std::vector<double> rowBias;
    std::vector<double> colBias;

    // Applied sequentially when an AreaSpec addresses the current CFA sample.
    // v0.6 supports single-plane CFA GainMap programs only and fails closed for
    // unsupported multi-plane AreaSpecs or invalid grids.
    std::vector<GainMapProgramV06> gainMaps;

    bool exactIntegerEvidenceRetained = true;
    bool appearanceApplied = false;
};

struct ScientificStage2DiagnosticsV06 {
    std::uint64_t samples = 0;
    std::uint64_t gainApplications = 0;
    std::uint64_t samplesWithGain = 0;
    bool usedFloat64Arithmetic = true;
    bool usedCompactFloat32GainMapKnots = false;
    bool bypassedLegacyFullFrameFloatGainField = true;
    bool exactIntegerEvidenceReadOnly = true;
};

bool scientific_stage2_source_valid_v0_6(const ScientificStage2SourceV06& source);

// Converts one tile from exact uint16 CFA evidence to a float64 Stage-2 working
// representation. GainMap samples remain their exact stored float32 values;
// coordinate/interpolation arithmetic is float64 via the validated v0.2
// reference. No full-frame double gain raster is required.
bool scientific_stage2_tile_f64_v0_6(
    const ScientificStage2SourceV06& source,
    int x0,
    int y0,
    int width,
    int height,
    std::vector<double>& out,
    ScientificStage2DiagnosticsV06* diagnostics = nullptr);

} // namespace truthraw_precision_v06

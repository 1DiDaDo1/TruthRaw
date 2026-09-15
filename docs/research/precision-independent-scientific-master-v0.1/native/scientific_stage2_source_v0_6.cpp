#include "scientific_stage2_source_v0_6.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace truthraw_precision_v06 {
namespace {

static inline int phase_index(int y, int x) {
    return (y & 1) * 2 + (x & 1);
}

static bool area_addresses_sample(const truthraw_precision_v02::GainMapAreaV02& a,
                                  int row,
                                  int col) {
    return row >= a.top && row < a.bottom &&
           col >= a.left && col < a.right &&
           ((row - a.top) % a.rowPitch) == 0 &&
           ((col - a.left) % a.colPitch) == 0;
}

static bool gain_program_supported(const GainMapProgramV06& p) {
    return p.area.valid() &&
           p.area.plane == 0 &&
           p.area.planes == 1 &&
           p.grid.valid() &&
           p.mapPlane >= 0 &&
           p.mapPlane < p.grid.planes;
}

} // namespace

bool scientific_stage2_source_valid_v0_6(const ScientificStage2SourceV06& source) {
    if (source.width <= 0 || source.height <= 0 ||
        source.rawRowStrideSamples < source.width ||
        !std::isfinite(source.whiteLevel)) return false;

    const std::size_t requiredRaw = static_cast<std::size_t>(source.rawRowStrideSamples)
                                  * static_cast<std::size_t>(source.height);
    if (source.raw.size() < requiredRaw) return false;

    for (double b : source.blackPhase) {
        if (!std::isfinite(b)) return false;
    }

    if (source.hasResidualBlack) {
        if (source.rowBias.size() < static_cast<std::size_t>(source.height) ||
            source.colBias.size() < static_cast<std::size_t>(source.width)) return false;
        for (int y = 0; y < source.height; ++y) {
            if (!std::isfinite(source.rowBias[static_cast<std::size_t>(y)])) return false;
        }
        for (int x = 0; x < source.width; ++x) {
            if (!std::isfinite(source.colBias[static_cast<std::size_t>(x)])) return false;
        }
    }

    for (const auto& p : source.gainMaps) {
        if (!gain_program_supported(p)) return false;
    }
    return true;
}

bool scientific_stage2_tile_f64_v0_6(
    const ScientificStage2SourceV06& source,
    int x0,
    int y0,
    int width,
    int height,
    std::vector<double>& out,
    ScientificStage2DiagnosticsV06* diagnostics) {

    out.clear();
    if (diagnostics) *diagnostics = ScientificStage2DiagnosticsV06{};
    if (!scientific_stage2_source_valid_v0_6(source) ||
        x0 < 0 || y0 < 0 || width <= 0 || height <= 0 ||
        x0 + width > source.width || y0 + height > source.height) return false;

    const truthraw_precision_v02::GainMapBoundsV02 imageBounds {
        0, 0, source.height, source.width
    };

    out.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0.0);
    std::uint64_t gainApplications = 0;
    std::uint64_t samplesWithGain = 0;

    for (int yy = 0; yy < height; ++yy) {
        const int gy = y0 + yy;
        for (int xx = 0; xx < width; ++xx) {
            const int gx = x0 + xx;
            const int ph = phase_index(gy, gx);
            double black = source.blackPhase[static_cast<std::size_t>(ph)];
            if (source.hasResidualBlack) {
                black += source.rowBias[static_cast<std::size_t>(gy)]
                       + source.colBias[static_cast<std::size_t>(gx)];
            }

            const std::size_t ri = static_cast<std::size_t>(gy)
                                 * static_cast<std::size_t>(source.rawRowStrideSamples)
                                 + static_cast<std::size_t>(gx);
            const double denom = std::max(source.whiteLevel - black, 1.0);
            double value = (static_cast<double>(source.raw[ri]) - black) / denom;

            bool gained = false;
            for (const auto& p : source.gainMaps) {
                if (!area_addresses_sample(p.area, gy, gx)) continue;
                const double gain = truthraw_precision_v02::gainmap_interpolate_f64_reference_v0_2(
                    p.grid, imageBounds, gy, gx, p.mapPlane);
                if (!std::isfinite(gain)) return false;
                value *= gain;
                ++gainApplications;
                gained = true;
            }
            if (gained) ++samplesWithGain;

            out[static_cast<std::size_t>(yy) * static_cast<std::size_t>(width)
                + static_cast<std::size_t>(xx)] = value;
        }
    }

    if (diagnostics) {
        diagnostics->samples = static_cast<std::uint64_t>(out.size());
        diagnostics->gainApplications = gainApplications;
        diagnostics->samplesWithGain = samplesWithGain;
        diagnostics->usedCompactFloat32GainMapKnots = !source.gainMaps.empty();
        diagnostics->exactIntegerEvidenceReadOnly = source.exactIntegerEvidenceRetained;
    }
    return true;
}

} // namespace truthraw_precision_v06

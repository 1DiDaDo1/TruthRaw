#include "truthnegative_dense_projection_v0_3.h"

#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <sstream>
#include <utility>
#include <vector>

namespace truthraw::truthnegative_dense_projection::v0_3 {
namespace {

namespace digest = truthraw::scientific_master_digest::v0_1;

struct SourceCell final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<float> rgb;
};

inline std::uint32_t clamp_index(
    std::int64_t value,
    std::uint32_t limit) noexcept {
    if (limit == 0u || value <= 0) return 0u;
    const auto u = static_cast<std::uint64_t>(value);
    return u >= limit ? limit - 1u : static_cast<std::uint32_t>(u);
}

inline double source_coordinate(std::uint32_t targetCoordinate) noexcept {
    return (static_cast<double>(targetCoordinate) + 0.5) /
               static_cast<double>(kScale) -
           0.5;
}

}  // namespace

struct DenseProjectionTileSource::Impl final {
    Geometry geometry{};
    bool valid = false;
    std::string error;
    std::vector<SourceCell> cells;

    const SourceCell* findCell(std::uint32_t sx, std::uint32_t sy) const noexcept {
        for (const auto& cell : cells) {
            if (sx >= cell.x && sy >= cell.y &&
                sx < cell.x + cell.width &&
                sy < cell.y + cell.height) {
                return &cell;
            }
        }
        return nullptr;
    }
};

DenseProjectionTileSource::DenseProjectionTileSource(
    float_dng::IScientificMasterTileSource& scientificMaster,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight) noexcept
    : source_(scientificMaster), impl_(new (std::nothrow) Impl{}) {
    if (!impl_) return;
    if (sourceWidth == 0u || sourceHeight == 0u ||
        sourceWidth > std::numeric_limits<std::uint32_t>::max() / kScale ||
        sourceHeight > std::numeric_limits<std::uint32_t>::max() / kScale) {
        impl_->error = "TruthNegative dense geometry is invalid or overflows";
        return;
    }
    impl_->geometry.sourceWidth = sourceWidth;
    impl_->geometry.sourceHeight = sourceHeight;
    impl_->geometry.targetWidth = sourceWidth * kScale;
    impl_->geometry.targetHeight = sourceHeight * kScale;
    impl_->valid = true;
}

DenseProjectionTileSource::~DenseProjectionTileSource() = default;

Geometry DenseProjectionTileSource::geometry() const noexcept {
    return impl_ ? impl_->geometry : Geometry{};
}

bool DenseProjectionTileSource::valid() const noexcept {
    return impl_ && impl_->valid;
}

const std::string& DenseProjectionTileSource::error() const noexcept {
    static const std::string kAllocationFailure =
        "TruthNegative dense projection workspace allocation failed";
    return impl_ ? impl_->error : kAllocationFailure;
}

std::size_t DenseProjectionTileSource::residentBytesUpperBound() const noexcept {
    std::size_t total = source_.residentBytesUpperBound();
    if (!impl_) return total;
    for (const auto& cell : impl_->cells) {
        const auto bytes = cell.rgb.capacity() * sizeof(float);
        if (total > std::numeric_limits<std::size_t>::max() - bytes) {
            return std::numeric_limits<std::size_t>::max();
        }
        total += bytes;
    }
    return total;
}

float_dng::Status DenseProjectionTileSource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    try {
        if (!valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument, error());
        }
        const auto g = impl_->geometry;
        if (!rgb || width == 0u || height == 0u ||
            x >= g.targetWidth || y >= g.targetHeight ||
            x + width > g.targetWidth || y + height > g.targetHeight) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "TruthNegative dense target tile is outside projection bounds");
        }
        const std::size_t pixels =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (pixels > std::numeric_limits<std::size_t>::max() / 3u ||
            floatCount != pixels * 3u) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "TruthNegative dense target tile size mismatch");
        }

        // The wrapped Scientific Master source admits only canonical 64x64
        // source cells. Determine the bounded source footprint for this target tile.
        const double sxf0 = source_coordinate(x);
        const double sxf1 = source_coordinate(x + width - 1u);
        const double syf0 = source_coordinate(y);
        const double syf1 = source_coordinate(y + height - 1u);
        const auto minSx = clamp_index(
            static_cast<std::int64_t>(std::floor(std::min(sxf0, sxf1))),
            g.sourceWidth);
        const auto maxSx = clamp_index(
            static_cast<std::int64_t>(std::ceil(std::max(sxf0, sxf1))),
            g.sourceWidth);
        const auto minSy = clamp_index(
            static_cast<std::int64_t>(std::floor(std::min(syf0, syf1))),
            g.sourceHeight);
        const auto maxSy = clamp_index(
            static_cast<std::int64_t>(std::ceil(std::max(syf0, syf1))),
            g.sourceHeight);

        const std::uint32_t cellEdge = float_dng::kCanonicalTileEdge;
        const std::uint32_t firstCellX = (minSx / cellEdge) * cellEdge;
        const std::uint32_t lastCellX = (maxSx / cellEdge) * cellEdge;
        const std::uint32_t firstCellY = (minSy / cellEdge) * cellEdge;
        const std::uint32_t lastCellY = (maxSy / cellEdge) * cellEdge;

        impl_->cells.clear();
        for (std::uint32_t cy = firstCellY;; cy += cellEdge) {
            for (std::uint32_t cx = firstCellX;; cx += cellEdge) {
                SourceCell cell{};
                cell.x = cx;
                cell.y = cy;
                cell.width = std::min(cellEdge, g.sourceWidth - cx);
                cell.height = std::min(cellEdge, g.sourceHeight - cy);
                const std::size_t cellFloats =
                    static_cast<std::size_t>(cell.width) * cell.height * 3u;
                cell.rgb.resize(cellFloats);
                const auto read = source_.readCameraNativeTile(
                    cell.x, cell.y, cell.width, cell.height,
                    cell.rgb.data(), cell.rgb.size());
                if (!read) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::SourceFailed,
                        "TruthNegative source Scientific Master tile failed: " +
                            read.message);
                }
                impl_->cells.push_back(std::move(cell));
                if (cx == lastCellX) break;
                if (cx > lastCellX - cellEdge) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::InvalidArgument,
                        "TruthNegative source-cell x iteration overflow");
                }
            }
            if (cy == lastCellY) break;
            if (cy > lastCellY - cellEdge) {
                return float_dng::Status::error(
                    float_dng::StatusCode::InvalidArgument,
                    "TruthNegative source-cell y iteration overflow");
            }
        }

        auto sample = [&](std::uint32_t sx, std::uint32_t sy, int channel,
                          double& value) -> bool {
            const auto* cell = impl_->findCell(sx, sy);
            if (!cell) return false;
            const std::size_t lx = sx - cell->x;
            const std::size_t ly = sy - cell->y;
            const std::size_t index =
                (ly * static_cast<std::size_t>(cell->width) + lx) * 3u +
                static_cast<std::size_t>(channel);
            if (index >= cell->rgb.size()) return false;
            value = static_cast<double>(cell->rgb[index]);
            return std::isfinite(value);
        };

        for (std::uint32_t oy = 0u; oy < height; ++oy) {
            const double sy = source_coordinate(y + oy);
            const auto iy = static_cast<std::int64_t>(std::floor(sy));
            const double fy = std::clamp(sy - static_cast<double>(iy), 0.0, 1.0);
            const std::uint32_t y0 = clamp_index(iy, g.sourceHeight);
            const std::uint32_t y1 = clamp_index(iy + 1, g.sourceHeight);

            for (std::uint32_t ox = 0u; ox < width; ++ox) {
                const double sx = source_coordinate(x + ox);
                const auto ix = static_cast<std::int64_t>(std::floor(sx));
                const float fx = static_cast<float>(
                    std::clamp(sx - static_cast<double>(ix), 0.0, 1.0));
                const std::uint32_t x0 = clamp_index(ix, g.sourceWidth);
                const std::uint32_t x1 = clamp_index(ix + 1, g.sourceWidth);

                for (int c = 0; c < 3; ++c) {
                    double p00d=0.0, p10d=0.0, p01d=0.0, p11d=0.0;
                    if (!sample(x0, y0, c, p00d) ||
                        !sample(x1, y0, c, p10d) ||
                        !sample(x0, y1, c, p01d) ||
                        !sample(x1, y1, c, p11d)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::SourceFailed,
                            "TruthNegative dense interpolation footprint incomplete");
                    }
                    const float p00=static_cast<float>(p00d);
                    const float p10=static_cast<float>(p10d);
                    const float p01=static_cast<float>(p01d);
                    const float p11=static_cast<float>(p11d);
                    const float fyf=static_cast<float>(fy);
                    // Canonical Float32 operation order. Android builds compile
                    // this module with FP contraction disabled. Vulkan uses the
                    // same ordered operations with precise/NoContraction.
                    const float top = p00 + (p10 - p00) * fx;
                    const float bottom = p01 + (p11 - p01) * fx;
                    const float value = top + (bottom - top) * fyf;
                    if (!std::isfinite(value)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::SourceFailed,
                            "TruthNegative dense interpolation produced non-finite value");
                    }
                    const std::size_t outIndex =
                        (static_cast<std::size_t>(oy) * width + ox) * 3u +
                        static_cast<std::size_t>(c);
                    rgb[outIndex] = value;
                }
            }
        }
        return float_dng::Status::ok();
    } catch (const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "TruthNegative dense projection allocation failed");
    } catch (...) {
        return float_dng::Status::error(
            float_dng::StatusCode::SourceFailed,
            "TruthNegative dense projection failed unexpectedly");
    }
}

float_dng::Status compute_projected_raster_identity(
    DenseProjectionTileSource& source,
    Result& out) noexcept {
    try {
        out = {};
        if (!source.valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument, source.error());
        }
        const auto g = source.geometry();
        out.geometry = g;
        out.createsNewEvidence = false;
        out.impliesPhysicalSensorGeometry = false;
        out.measuredTargetClaimCount = kMeasuredTargetClaimCount;
        out.physicalFrameCount = 1u;
        out.independentEvidenceCount = 1u;
        out.methodId = kMethodId;
        out.targetAuthority = kAuthority;

        digest::ScientificMasterDigestAccumulator accumulator(
            g.targetWidth, g.targetHeight);
        if (!accumulator.valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "TruthNegative projected-raster digest init failed: " +
                    accumulator.error());
        }

        std::vector<float> tile;
        for (std::uint32_t y=0u; y<g.targetHeight;
             y += float_dng::kCanonicalTileEdge) {
            const auto h = std::min(
                float_dng::kCanonicalTileEdge, g.targetHeight - y);
            for (std::uint32_t x=0u; x<g.targetWidth;
                 x += float_dng::kCanonicalTileEdge) {
                const auto w = std::min(
                    float_dng::kCanonicalTileEdge, g.targetWidth - x);
                tile.resize(static_cast<std::size_t>(w) * h * 3u);
                const auto read = source.readCameraNativeTile(
                    x, y, w, h, tile.data(), tile.size());
                if (!read) return read;

                for (const float v : tile) {
                    if (!std::isfinite(v)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::DigestFailed,
                            "TruthNegative projected raster contains non-finite value");
                    }
                    if (v < 0.0f) ++out.negativeComponentCount;
                    if (v > 1.0f) ++out.overOneComponentCount;
                }
                out.projectedPixels +=
                    static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h);

                digest::TileView view{};
                view.x=x; view.y=y; view.width=w; view.height=h;
                view.rgb=tile.data();
                view.rowStrideSamples=static_cast<std::size_t>(w)*3u;
                if (!accumulator.add_tile(view)) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::DigestFailed,
                        "TruthNegative projected-raster digest rejected tile: " +
                            accumulator.error());
                }
                out.logicalResidentUpperBound = std::max(
                    out.logicalResidentUpperBound,
                    source.residentBytesUpperBound() +
                        tile.capacity() * sizeof(float) +
                        accumulator.metrics().residentBytesUpperBound);
            }
        }
        if (!accumulator.finalize(out.projectedRasterSha256)) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "TruthNegative projected-raster digest finalization failed: " +
                    accumulator.error());
        }
        out.projectedRasterIdentityAvailable = true;
        return float_dng::Status::ok();
    } catch (const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "TruthNegative projected-raster digest allocation failed");
    } catch (...) {
        return float_dng::Status::error(
            float_dng::StatusCode::DigestFailed,
            "TruthNegative projected-raster digest failed unexpectedly");
    }
}

std::string authority_manifest(const Result& result) {
    std::ostringstream out;
    out << "schema=TruthNegativeDenseProjection/0.3\n"
        << "method=" << result.methodId << "\n"
        << "source_width=" << result.geometry.sourceWidth << "\n"
        << "source_height=" << result.geometry.sourceHeight << "\n"
        << "target_width=" << result.geometry.targetWidth << "\n"
        << "target_height=" << result.geometry.targetHeight << "\n"
        << "target_authority=" << result.targetAuthority << "\n"
        << "creates_new_evidence=0\n"
        << "implies_physical_sensor_geometry=0\n"
        << "measured_target_claim_count=0\n"
        << "physical_frame_count=1\n"
        << "independent_evidence_count=1\n"
        << "reconstructed_cfa_is_original_evidence=0\n"
        << "appearance_applied=0\n"
        << "scientific_master_replaced=0\n";
    return out.str();
}

}  // namespace truthraw::truthnegative_dense_projection::v0_3

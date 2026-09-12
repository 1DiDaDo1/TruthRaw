#include "streaming_scientific_master_tile_source_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <limits>
#include <new>
#include <string>

namespace truthraw::scientific_master_linear_dng_projection::v0_1 {

struct StreamingScientificMasterTileSource::Impl final {
    streaming_v0_1::detail::Workspace workspace{};
};

StreamingScientificMasterTileSource::StreamingScientificMasterTileSource(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction) noexcept
    : source_(source), reconstruction_(reconstruction), impl_(new (std::nothrow) Impl{}) {}

StreamingScientificMasterTileSource::~StreamingScientificMasterTileSource() = default;

std::size_t StreamingScientificMasterTileSource::residentBytesUpperBound() const noexcept {
    std::size_t total = source_.residentBytesUpperBound();
    if (!impl_) return total;
    const std::size_t workspace = streaming_v0_1::detail::vector_bytes(impl_->workspace);
    if (total > std::numeric_limits<std::size_t>::max() - workspace) {
        return std::numeric_limits<std::size_t>::max();
    }
    return total + workspace;
}

Status StreamingScientificMasterTileSource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    try {
        if (!impl_) {
            return Status::error(StatusCode::SizeOverflow,
                                 "Scientific Master tile adapter workspace allocation failed");
        }
        const auto& metadata = source_.metadata();
        if (metadata.width <= 1 || metadata.height <= 1 || rgb == nullptr ||
            width == 0u || height == 0u) {
            return Status::error(StatusCode::InvalidArgument,
                                 "invalid Scientific Master adapter request");
        }
        if (x >= static_cast<std::uint32_t>(metadata.width) ||
            y >= static_cast<std::uint32_t>(metadata.height)) {
            return Status::error(StatusCode::InvalidArgument,
                                 "Scientific Master tile origin outside source");
        }
        if ((x % kCanonicalTileEdge) != 0u || (y % kCanonicalTileEdge) != 0u) {
            return Status::error(StatusCode::InvalidArgument,
                                 "Scientific Master adapter requires canonical 64x64 tile origins");
        }

        const std::uint32_t expectedWidth = std::min(
            kCanonicalTileEdge,
            static_cast<std::uint32_t>(metadata.width) - x);
        const std::uint32_t expectedHeight = std::min(
            kCanonicalTileEdge,
            static_cast<std::uint32_t>(metadata.height) - y);
        if (width != expectedWidth || height != expectedHeight) {
            return Status::error(StatusCode::InvalidArgument,
                                 "Scientific Master adapter requires exact canonical tile extents");
        }

        const std::size_t pixels =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (pixels > std::numeric_limits<std::size_t>::max() / 3u ||
            floatCount != pixels * 3u) {
            return Status::error(StatusCode::InvalidArgument,
                                 "Scientific Master adapter output size mismatch");
        }

        const int halo = reconstruction_.requiredHalo();
        if (halo < 0) {
            return Status::error(StatusCode::InvalidArgument,
                                 "reconstruction backend returned negative halo");
        }

        streaming_v0_1::TileRect tile{};
        tile.x0 = static_cast<int>(x);
        tile.y0 = static_cast<int>(y);
        tile.x1 = static_cast<int>(x + width);
        tile.y1 = static_cast<int>(y + height);
        tile.hx0 = std::max(0, tile.x0 - halo);
        tile.hy0 = std::max(0, tile.y0 - halo);
        tile.hx1 = std::min(metadata.width, tile.x1 + halo);
        tile.hy1 = std::min(metadata.height, tile.y1 + halo);

        auto& workspace = impl_->workspace;
        const auto fill = streaming_v0_1::detail::fill_stage2(source_, tile, workspace);
        if (!fill) {
            return Status::error(StatusCode::SourceFailed,
                                 "Stage-2 source read failed: " + fill.message);
        }

        workspace.cam.resize(floatCount);
        const int tileWidth = tile.hx1 - tile.hx0;
        const int tileHeight = tile.hy1 - tile.hy0;
        const auto reconstructionStatus = reconstruction_.reconstructTile(
            workspace.stage2.data(), tileWidth, tileHeight,
            tile.hx0, tile.hy0,
            tile.x0, tile.y0,
            static_cast<int>(width), static_cast<int>(height),
            metadata.cfa, workspace.cam.data());
        if (!reconstructionStatus) {
            return Status::error(StatusCode::SourceFailed,
                                 "camera-native reconstruction failed: " +
                                     reconstructionStatus.message);
        }

        std::copy(workspace.cam.begin(), workspace.cam.end(), rgb);
        return Status::ok();
    } catch (const std::bad_alloc&) {
        return Status::error(StatusCode::SizeOverflow,
                             "Scientific Master tile adapter allocation failed");
    } catch (...) {
        return Status::error(StatusCode::SourceFailed,
                             "unexpected Scientific Master tile adapter failure");
    }
}

}  // namespace truthraw::scientific_master_linear_dng_projection::v0_1

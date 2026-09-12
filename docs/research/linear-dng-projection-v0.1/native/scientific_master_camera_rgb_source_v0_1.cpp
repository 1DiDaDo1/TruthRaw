#include "scientific_master_camera_rgb_source_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace truthraw::linear_dng_projection::v0_1 {
namespace {

using streaming_v0_1::TileRect;
using streaming_v0_1::detail::Workspace;
using streaming_v0_1::detail::fill_stage2;
using streaming_v0_1::detail::vector_bytes;

class ScientificCameraRgbSource final : public ICameraRgbTileSource {
public:
    ScientificCameraRgbSource(
        streaming_v0_1::IRawTileSource& source,
        std::shared_ptr<IReconstructionBackend> reconstruction,
        const ProjectionAdmission& admission)
        : source_(source),
          reconstruction_(std::move(reconstruction)),
          admission_(admission),
          digest_(static_cast<std::uint32_t>(source.metadata().width),
                  static_cast<std::uint32_t>(source.metadata().height)) {}

    Status initialize() noexcept {
        if (!reconstruction_) {
            return Status::error(StatusCode::InvalidArgument,
                                 "Linear DNG scientific source requires reconstruction backend");
        }
        const auto& m = source_.metadata();
        if (m.width <= 1 || m.height <= 1 ||
            m.sourceId != admission_.source.sourceEvidenceId) {
            return Status::error(StatusCode::SourceIdentityMismatch,
                                 "tile source identity/dimensions do not match projection admission");
        }
        const int halo = reconstruction_->requiredHalo();
        if (halo < 0) {
            return Status::error(StatusCode::InvalidArgument,
                                 "reconstruction backend returned negative halo");
        }
        halo_ = halo;
        if (!digest_.valid()) {
            return Status::error(StatusCode::ScientificIdentityMismatch,
                                 "Scientific Master export digest could not initialize");
        }
        initialized_ = true;
        return Status::ok();
    }

    int width() const noexcept override { return source_.metadata().width; }
    int height() const noexcept override { return source_.metadata().height; }
    Orientation orientation() const noexcept override { return source_.metadata().orientation; }

    Status readCameraRgbTile(int x0, int y0, int x1, int y1,
                             float* rgbOut, std::size_t floatCount) override {
        if (!initialized_ || finalized_ || rgbOut == nullptr) {
            return Status::error(StatusCode::SourceFailed,
                                 "Scientific Master export source is not in readable state");
        }
        const auto& m = source_.metadata();
        if (x0 < 0 || y0 < 0 || x1 <= x0 || y1 <= y0 ||
            x1 > m.width || y1 > m.height ||
            (x0 % scientific_master_digest::v0_1::kCanonicalCellEdge) != 0 ||
            (y0 % scientific_master_digest::v0_1::kCanonicalCellEdge) != 0 ||
            x1 != std::min(m.width,
                          x0 + static_cast<int>(scientific_master_digest::v0_1::kCanonicalCellEdge)) ||
            y1 != std::min(m.height,
                          y0 + static_cast<int>(scientific_master_digest::v0_1::kCanonicalCellEdge))) {
            return Status::error(StatusCode::SourceFailed,
                                 "export tile does not follow canonical 64x64 Scientific Master grid");
        }
        const int coreW = x1 - x0;
        const int coreH = y1 - y0;
        const std::size_t expected =
            static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH) * 3u;
        if (floatCount != expected) {
            return Status::error(StatusCode::SourceFailed,
                                 "camera RGB destination size does not match canonical tile");
        }

        TileRect tile{};
        tile.x0 = x0; tile.y0 = y0; tile.x1 = x1; tile.y1 = y1;
        tile.hx0 = std::max(0, x0 - halo_);
        tile.hy0 = std::max(0, y0 - halo_);
        tile.hx1 = std::min(m.width, x1 + halo_);
        tile.hy1 = std::min(m.height, y1 + halo_);

        const auto fill = fill_stage2(source_, tile, workspace_);
        if (!fill) {
            return Status::error(StatusCode::SourceFailed,
                                 "Stage-2 read failed during Linear DNG export: " + fill.message);
        }
        workspace_.cam.resize(expected);
        const auto reconstructed = reconstruction_->reconstructTile(
            workspace_.stage2.data(), tile.hx1 - tile.hx0, tile.hy1 - tile.hy0,
            tile.hx0, tile.hy0, x0, y0, coreW, coreH, m.cfa,
            workspace_.cam.data());
        if (!reconstructed) {
            return Status::error(StatusCode::SourceFailed,
                                 "camera-native reconstruction failed during export: " +
                                     reconstructed.message);
        }
        for (const float value : workspace_.cam) {
            if (!std::isfinite(value)) {
                return Status::error(StatusCode::NonFiniteSample,
                                     "Scientific Master reconstruction produced non-finite RGB");
            }
        }
        std::memcpy(rgbOut, workspace_.cam.data(), expected * sizeof(float));

        scientific_master_digest::v0_1::TileView digestTile{};
        digestTile.x = static_cast<std::uint32_t>(x0);
        digestTile.y = static_cast<std::uint32_t>(y0);
        digestTile.width = static_cast<std::uint32_t>(coreW);
        digestTile.height = static_cast<std::uint32_t>(coreH);
        digestTile.rgb = workspace_.cam.data();
        digestTile.rowStrideSamples = static_cast<std::size_t>(coreW) * 3u;
        if (!digest_.add_tile(digestTile)) {
            return Status::error(StatusCode::ScientificIdentityMismatch,
                                 "Scientific Master export digest rejected tile: " + digest_.error());
        }

        ++tiles_;
        workspacePeak_ = std::max(workspacePeak_, vector_bytes(workspace_));
        return Status::ok();
    }

    Status finalize_identity() noexcept {
        if (!initialized_ || finalized_) {
            return Status::error(StatusCode::ScientificIdentityMismatch,
                                 "Scientific Master export identity finalized in invalid state");
        }
        scientific_master_digest::v0_1::Sha256 actual{};
        if (!digest_.finalize(actual)) {
            return Status::error(StatusCode::ScientificIdentityMismatch,
                                 "Scientific Master export digest incomplete: " + digest_.error());
        }
        finalized_ = true;
        if (actual != admission_.scientificMasterHash) {
            return Status::error(StatusCode::ScientificIdentityMismatch,
                                 "export reconstruction does not match admitted Scientific Master hash");
        }
        return Status::ok();
    }

    std::size_t tiles() const noexcept { return tiles_; }
    std::size_t workspacePeak() const noexcept { return workspacePeak_; }

private:
    streaming_v0_1::IRawTileSource& source_;
    std::shared_ptr<IReconstructionBackend> reconstruction_;
    ProjectionAdmission admission_{};
    scientific_master_digest::v0_1::ScientificMasterDigestAccumulator digest_;
    Workspace workspace_{};
    int halo_ = 0;
    std::size_t tiles_ = 0;
    std::size_t workspacePeak_ = 0;
    bool initialized_ = false;
    bool finalized_ = false;
};

} // namespace

Status write_linear_dng_from_finalized_scientific_source(
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    ITransactionalByteSink& sink,
    const ProjectionAdmission& admission,
    const Options& options,
    ScientificExportAudit& out) noexcept {
    out = {};
    const auto fail = [&](Status status) -> Status {
        sink.abort();
        return status;
    };

    if (options.tileEdge != static_cast<int>(scientific_master_digest::v0_1::kCanonicalCellEdge)) {
        return fail(Status::error(
            StatusCode::InvalidArgument,
            "scientifically rebound Linear DNG export requires canonical 64x64 tiles"));
    }

    ScientificCameraRgbSource cameraSource(source, std::move(reconstruction), admission);
    auto status = cameraSource.initialize();
    if (!status) return fail(status);

    Audit projection;
    status = write_linear_dng(cameraSource, sink, admission, options, projection);
    if (!status) return fail(status);

    status = cameraSource.finalize_identity();
    if (!status) return fail(status);

    status = sink.commit();
    if (!status) {
        sink.abort();
        return Status::error(StatusCode::SinkFailed,
                             "Linear DNG transaction commit failed: " + status.message);
    }

    ScientificExportAudit audit;
    audit.projection = projection;
    audit.reconstructedTiles = cameraSource.tiles();
    audit.reconstructionWorkspacePeakBytes = cameraSource.workspacePeak();
    audit.sourceIdentityChecked = true;
    audit.scientificMasterDigestReverified = true;
    audit.transactionCommitted = true;
    audit.appearanceApplied = false;
    audit.cameraToXyzApplied = false;
    out = audit;
    return Status::ok();
}

}  // namespace truthraw::linear_dng_projection::v0_1

#pragma once

#include "linear_dng_projection_v0_2.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::linear_dng_preview_container::v0_3 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    PreviewTooLarge,
    PreviewReadFailed,
    PreviewJpegInvalid,
    UnderlyingLinearDngFailed,
    ContainerPatchFailed,
    SinkFailed,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

struct PreviewJpeg final {
    tile_dng_v0_1::IRandomAccessByteSource* bytes = nullptr;
    int width = 0;
    int height = 0;
};

struct Options final {
    std::size_t memoryBudgetBytes = 0;
    std::size_t maxPreviewJpegBytes = 16u * 1024u * 1024u;
    int maxPreviewLongEdge = 2048;
};

struct Result final {
    linear_dng_projection::v0_2::Result linearRaw{};
    std::uint64_t outputBytes = 0;
    std::uint64_t previewJpegBytes = 0;
    std::uint32_t previewIfdOffset = 0;
    std::uint32_t previewJpegOffset = 0;
    int previewWidth = 0;
    int previewHeight = 0;
    bool rawIfdRemainsPrimary = false;
    bool previewIsReducedSubIfd = false;
    bool previewColorSpaceSrgb = false;
    bool previewOrientationNormalized = false;
};

// v0.3 adds one reduced JPEG/sRGB preview SubIFD to the already validated
// v0.2 RGB LinearRaw representation. The raw IFD remains IFD0 with
// NewSubFileType=0. The preview is NewSubFileType=1 and is linked using the
// TIFF/DNG SubIFDs tag (330). The JPEG is presentation/compatibility only.
//
// The v0.2 raw bytes are shifted only as required by inserting the SubIFDs IFD
// entry. Their byte content is not transformed by this module. Scientific
// reconstruction, Scientific Master identity, TruthRange, Backplane and
// evidence authority remain upstream and unchanged.
Status write_finalized_linear_dng_with_preview(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const PreviewJpeg& preview,
    linear_dng_projection::v0_1::IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::linear_dng_preview_container::v0_3

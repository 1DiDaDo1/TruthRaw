#pragma once

#include "linear_dng_projection_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::linear_dng_projection::v0_2 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceIdentityFailed,
    UnderlyingProjectionFailed,
    SceneExceedsValidatedWindow,
    MetadataPatchFailed,
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

struct Options final {
    std::size_t memoryBudgetBytes = 0;
};

struct Result final {
    v0_1::Result base{};
    double compatibilityWindow = 2.0;
    double baselineExposureEv = 1.0;
    bool sourceCameraIdentityPreserved = false;
    bool overWindowRejected = false;
};

// v0.2 is a representation-only wrapper around the finalized v0.1 writer.
// Scientific reconstruction is unchanged. Camera-native reconstructed RGB is
// divided uniformly by 2 only at the DNG representation boundary, while the
// output BaselineExposure tag is changed from 0 EV to +1 EV. This restores the
// historically validated finite 2x LinearRaw headroom without creating a new
// scientific scale or changing the Scientific Master.
//
// The source Make/Model/UniqueCameraModel are restored in the output container
// so source-bound DNG colour metadata remains tied to its actual camera model.
// If any reconstructed sample still exceeds the 2x finite window, the output is
// erased and the export fails closed.
Status write_finalized_linear_dng(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    v0_1::IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::linear_dng_projection::v0_2

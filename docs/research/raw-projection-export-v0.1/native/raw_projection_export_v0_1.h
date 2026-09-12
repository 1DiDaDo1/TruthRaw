#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthraw/core.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::raw_projection_export::v0_1 {

enum class ProjectionKind : std::uint8_t {
    RawSensorCfa16 = 1,
    ReconstructedCfaDng16 = 2,
    LinearDng16 = 3,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    UnsupportedProjection,
    InvalidColorMatrix,
    SourceFailed,
    ReconstructionFailed,
    NonFiniteScientificSample,
    BudgetExceeded,
    OutputNotSeekable,
    OutputWriteFailed,
    TiffLayoutOverflow,
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
    int rowsPerStrip = 32;
    std::size_t memoryBudgetBytes = 0;
};

struct Result final {
    ProjectionKind kind = ProjectionKind::RawSensorCfa16;
    int width = 0;
    int height = 0;
    int samplesPerPixel = 0;
    std::uint32_t stripsWritten = 0;
    std::uint64_t outputBytes = 0;
    std::uint64_t projectedSamples = 0;
    std::uint64_t clippedLowSamples = 0;
    std::uint64_t clippedHighSamples = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    bool fullScientificMasterMaterialized = false;
    bool sourcePixelsClaimedMeasured = false;
    bool projectionOnly = true;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes a downstream compatibility/projection representation from the same
// camera-native reconstructed RGB domain used by the Scientific Master digest.
// The writer never mutates source/evidence/master state and never claims that
// projected samples are physically measured sensor samples.
//
// RawSensorCfa16:
//   headerless little-endian uint16 CFA projection in source CFA order.
// ReconstructedCfaDng16:
//   classic TIFF/DNG, uncompressed uint16 CFA, BlackLevel=0, WhiteLevel=65535.
// LinearDng16:
//   classic TIFF/DNG LinearRaw, uncompressed interleaved uint16 camera RGB.
//
// Both DNG forms are representation projections. Values are rounded from the
// camera-native reconstructed RGB domain after clipping to [0,1]. Values below
// 0 and above 1 are counted explicitly in Result; clipping never changes the
// Scientific Master or evidence.
Status export_projection(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    int outputFd,
    ProjectionKind kind,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;
const char* projection_name(ProjectionKind kind) noexcept;

}  // namespace truthraw::raw_projection_export::v0_1

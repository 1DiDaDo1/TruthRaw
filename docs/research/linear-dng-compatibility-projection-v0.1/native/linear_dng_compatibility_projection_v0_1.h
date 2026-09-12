#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_preview_source_binding_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::linear_dng_compatibility_projection::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    UnauthorizedColorBinding,
    ColorBindingMismatch,
    BudgetExceeded,
    OutputIoFailed,
    ClassicTiffLimitExceeded,
    SourceFailed,
    ReconstructionFailed,
    NonFiniteScientificSample,
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
    int tileEdge = 128;
    std::size_t memoryBudgetBytes = 0; // 0 = caller imposes no logical ceiling.
};

struct Result final {
    int width = 0;
    int height = 0;
    std::uint64_t outputBytes = 0;
    std::uint64_t tilesWritten = 0;
    std::uint64_t samplesWritten = 0;
    std::uint64_t clippedBelowZero = 0;
    std::uint64_t clippedAboveOne = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    bool fullFrameMaterialized = false;
    bool appearanceApplied = false;
    bool scientificMasterModified = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes a classic-TIFF DNG LinearRaw compatibility projection from the
// camera-native reconstructed RGB that underlies the Scientific Master.
//
// This function is a downstream representation only:
// - it never changes the source, Scientific Master digest, TruthRange gauge,
//   Technical Backplane, evidence count or color authority;
// - no appearance, XYZ->sRGB conversion or tone curve is applied;
// - camera-native RGB is quantized to unsigned 16-bit [0,1] for compatibility;
//   out-of-range samples are counted explicitly rather than reclassified as
//   measured truth;
// - output is tiled and bounded-memory; no full-frame RGB is materialized.
//
// The caller must only expose the result after a finalized Scientific Preview
// admission/release gate has already succeeded for the same source/color
// lineage.
Status write_linear_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const scientific_preview_binding_v0_1::ScientificColorBindingRecord& color,
    int outputFd,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::linear_dng_compatibility_projection::v0_1

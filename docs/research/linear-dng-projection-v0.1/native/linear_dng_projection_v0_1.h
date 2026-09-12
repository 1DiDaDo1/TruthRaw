#pragma once

#include "finalized_scientific_preview_release_v0_2.h"
#include "tile_native_dng_source_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::linear_dng_projection::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    FinalizedReleaseRequired,
    EvidenceInvariantViolation,
    SourceMetadataFailed,
    SourceReadFailed,
    UnsupportedContainer,
    ReconstructionFailed,
    NonFiniteScientificSample,
    BudgetExceeded,
    OutputTooLarge,
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

class IRandomAccessByteSink {
public:
    virtual ~IRandomAccessByteSink() = default;
    virtual bool resize(std::uint64_t bytes) = 0;
    virtual bool writeExact(std::uint64_t offset, const void* src, std::size_t bytes) = 0;
    virtual std::size_t residentBytesUpperBound() const = 0;
};

// Borrowed POSIX file descriptor. The caller owns fd lifetime.
class PosixFdByteSink final : public IRandomAccessByteSink {
public:
    explicit PosixFdByteSink(int fd) noexcept : fd_(fd) {}
    bool resize(std::uint64_t bytes) override;
    bool writeExact(std::uint64_t offset, const void* src, std::size_t bytes) override;
    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }
private:
    int fd_ = -1;
};

struct Options final {
    // Scientific Master uses a canonical 64x64 core grid. v0.1 deliberately
    // keeps the export traversal identical to that grid so the reconstructed
    // camera-native RGB samples come from the same bounded reconstruction
    // contract before quantization into a compatibility representation.
    std::size_t memoryBudgetBytes = 0; // 0 = no caller-imposed ceiling.
};

struct Result final {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t outputBytes = 0;
    std::uint64_t pixelPayloadBytes = 0;
    std::uint64_t tilesWritten = 0;
    std::uint64_t samplesClippedLow = 0;
    std::uint64_t samplesClippedHigh = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;
    bool linearRawPhotometric = false;
    bool boundedUnsigned16Projection = false;
    bool sourceColorMetadataCopied = false;
    bool fullScientificMasterMaterialized = false;
    std::uint32_t physicalFrameCount = 0;
    std::uint32_t independentEvidenceCount = 0;
};

// Writes a standards-oriented, uncompressed, chunky 16-bit LinearRaw DNG
// compatibility projection. The input samples are reconstructed camera-native
// RGB produced by the same reconstruction backend used for the Scientific
// Master, before camera_to_xyz(), appearance, tone mapping or sRGB conversion.
//
// The Android/public export boundary is responsible for re-binding
// sealedSourceBytes and source to the finalized phase-2 admission before this
// writer is invoked. The current Android bridge performs that comparison both
// before and after projection and reverifies the exact source SHA-256 bytes.
//
// Values outside [0,1] are clipped only in this bounded compatibility export;
// the Scientific Master identity is not modified. Source DNG color metadata
// (ColorMatrix*, CameraCalibration*, AnalogBalance, AsShotNeutral,
// CalibrationIlluminant*, signatures and ForwardMatrix*) is copied from the
// sealed source container into the new DNG without acquiring new authority.
Status write_finalized_linear_dng(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::linear_dng_projection::v0_1

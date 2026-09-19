#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "technical_backplane_v0_1.h"
#include "truthrange_latent_v0_2.h"

namespace truthraw::scientific_master_linear_dng_projection::v0_1 {

using Hash256 = std::array<std::uint8_t, 32>;

inline constexpr std::uint32_t kCanonicalTileEdge = 64u;
inline constexpr std::uint16_t kPhotometricLinearRaw = 34892u;
inline constexpr std::uint16_t kCalibrationIlluminantD50 = 23u;

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    InvalidColorTransform,
    SizeOverflow,
    SourceFailed,
    DigestFailed,
    ScientificMasterMismatch,
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

struct ProjectionDescriptor final {
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint16_t orientation = 1u;
    Hash256 sealedSourceSha256{};
    Hash256 scientificMasterSha256{};
    Hash256 zeroLineSha256{};
    Hash256 sceneScaleSha256{};
    TruthRangeGaugeV02 zeroLineGauge{};
    LatentSceneBindingV02 sceneBinding{};
    technical_backplane::v0_1::SerializedBackplane serializedBackplane{};
    std::string sourceEvidenceId;
    std::string colorBindingId;
    std::string precisionPolicyId;
    std::string runtimeReconstructionBackendId;
};

// Supplies camera-native, scene-linear reconstructed RGB from the Scientific
// Master domain. No appearance, display transfer function, gamut mapping or
// tone mapping is permitted in this source contract.
class IScientificMasterTileSource {
public:
    virtual ~IScientificMasterTileSource() = default;
    virtual std::size_t residentBytesUpperBound() const noexcept = 0;
    virtual Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept = 0;
};

// Transactionality is part of the authority boundary. The projection may be
// written while its camera-native Scientific Master digest is recomputed, but
// it is not released unless the exact canonical digest matches. abort() must
// leave no committed artifact visible to the caller.
class ITransactionalByteSink {
public:
    virtual ~ITransactionalByteSink() = default;
    virtual std::size_t residentBytesUpperBound() const noexcept = 0;
    virtual bool begin(std::uint64_t expectedBytes) noexcept = 0;
    virtual bool write(const std::uint8_t* data, std::size_t size) noexcept = 0;
    virtual bool commit() noexcept = 0;
    virtual void abort() noexcept = 0;
};

struct Result final {
    std::uint64_t bytesWritten = 0u;
    std::uint64_t projectedPixels = 0u;
    std::uint64_t negativeComponentCount = 0u;
    std::uint64_t overOneComponentCount = 0u;
    std::uint32_t tilesWritten = 0u;
    std::size_t logicalWorkspacePeakBytes = 0u;
    std::size_t logicalResidentUpperBound = 0u;

    bool scientificMasterIdentityVerified = false;
    bool artifactCommitted = false;
    bool representationOnly = true;
    bool scientificMasterModified = false;
    bool appearanceApplied = false;
    bool counterfactualObservationCreated = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

// Writes a standards-oriented DNG 1.4 classic-TIFF LinearRaw compatibility
// projection. The stored three-plane raw color space is synthetic XYZ D50:
// camera-native Scientific Master RGB is transformed with the already-authorized
// cameraToXyzD50 matrix, then stored as IEEE float32 LinearRaw. ColorMatrix1 is
// identity and AsShotNeutral is D50, so the DNG profile does not invent a new
// camera calibration.
//
// This is deliberately a projection, not the Scientific Master and not Direct
// CFA evidence. Values are not clipped before storage. DNG readers are allowed
// to apply their own raw-domain clipping/rendering behavior downstream.
Status write_xyz_d50_linear_dng_projection(
    IScientificMasterTileSource& source,
    const ProjectionDescriptor& descriptor,
    const std::array<float, 9>& cameraToXyzD50,
    ITransactionalByteSink& sink,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::scientific_master_linear_dng_projection::v0_1

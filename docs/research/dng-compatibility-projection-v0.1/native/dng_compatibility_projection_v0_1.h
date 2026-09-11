#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_preview_source_binding_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::dng_compatibility_projection::v0_1 {

using Hash256 = scientific_master_digest::v0_1::Sha256;

inline constexpr std::uint32_t kTileEdge =
    scientific_master_digest::v0_1::kCanonicalCellEdge;

enum class ProjectionRole : std::uint8_t {
    LinearScientificMaster = 0,
    MeasuredPreservingCfa = 1,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    InvalidAuthority,
    InvalidColorMatrix,
    InvalidWhitePoint,
    NumericOverflow,
    SourceFailed,
    ReconstructionFailed,
    DigestFailed,
    SinkFailed,
    ScientificMasterMismatch,
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

class ISequentialByteSink {
public:
    virtual ~ISequentialByteSink() = default;
    virtual bool write(const void* data, std::size_t count) = 0;
    virtual std::size_t residentBytesUpperBound() const = 0;
};

struct ProjectionMetadata final {
    scientific_preview_binding_v0_1::SourceSeal sourceSeal{};
    scientific_preview_binding_v0_1::ScientificColorBindingRecord color{};
    Hash256 expectedScientificMasterHash{};

    // Resolved source-bound scene white from the DNG color producer. The
    // generated compatibility DNG records this as AsShotWhiteXY; it does not
    // claim that the generated single profile is an independent calibration.
    double asShotWhiteX = 0.0;
    double asShotWhiteY = 0.0;

    std::string sourceDisplayName;
};

struct Result final {
    ProjectionRole role = ProjectionRole::LinearScientificMaster;
    Hash256 observedScientificMasterHash{};
    std::uint64_t bytesWritten = 0;
    std::uint32_t tilesWritten = 0;
    std::size_t workspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;
    bool scientificMasterMatched = false;
    bool fullFrameMaterialized = false;
    bool projectionIsEvidence = false;
    bool colorAuthorityPromoted = false;
    scientific_preview_binding_v0_1::ColorBindingAuthority colorAuthority =
        scientific_preview_binding_v0_1::ColorBindingAuthority::Unverified;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes uncompressed 32-bit IEEE-754 LinearRaw DNG tiles containing the exact
// camera-native reconstructed Scientific Master RGB samples. The exporter
// rebuilds the Scientific Master digest while writing and requires it to equal
// metadata.expectedScientificMasterHash.
Status write_linear_scientific_master_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept;

// Writes an uncompressed 32-bit IEEE-754 CFA DNG projection. At each CFA site
// only the camera-RGB component corresponding to the source CFA measurement is
// written. With a measured-preserving reconstruction backend this retains the
// measured Stage-2 site value, but the file remains a DERIVED projection: it is
// not the original sensor code-value evidence and never becomes a second truth.
Status write_measured_preserving_cfa_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const ProjectionMetadata& metadata,
    ISequentialByteSink& sink,
    Result& out) noexcept;

const char* role_name(ProjectionRole role) noexcept;
const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::dng_compatibility_projection::v0_1

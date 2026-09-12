#pragma once

#include "finalized_scientific_preview_release_v0_2.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw::dng_projection::v0_1 {

// This product includes DNG technology under license by Adobe.

enum class ProjectionKind : std::uint8_t {
    Stage2CfaFloat32 = 0,
    LinearRawCameraRgbFloat32 = 1,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    AdmissionRejected,
    SourceIdentityMismatch,
    SourceSealMismatch,
    SourceReadFailed,
    UnsupportedSourceTiff,
    MissingRequiredColorMetadata,
    ReconstructionFailed,
    DigestFailed,
    DigestMismatch,
    BudgetExceeded,
    TiffOverflow,
    OutputFailed,
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
    virtual bool write(const void* data, std::size_t bytes) = 0;
};

struct Options final {
    ProjectionKind kind = ProjectionKind::Stage2CfaFloat32;
    // 0 means no caller-imposed logical resident ceiling.
    std::size_t memoryBudgetBytes = 0;
};

struct Result final {
    ProjectionKind kind = ProjectionKind::Stage2CfaFloat32;
    std::uint64_t bytesWritten = 0;
    std::size_t stripsWritten = 0;
    std::size_t canonicalTilesProcessed = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;

    bool sourceVerifiedBefore = false;
    bool sourceVerifiedAfter = false;
    bool scientificMasterHashMatched = false;
    bool fullFrameMaterialized = false;
    bool createsEvidence = false;

    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    scientific_master_digest::v0_1::Sha256 recomputedScientificMasterHash{};
};

// Writes a bounded-memory DNG projection only after the exact source has already
// passed Finalized Scientific Preview release. The exporter recomputes the
// camera-native Scientific Master digest while producing pixels and requires an
// exact digest match before returning Ok.
//
// Stage2CfaFloat32 writes one measured CFA channel per sensor site after the
// existing Stage-2 black/white/gain normalization. It is a derived-measurement
// projection, not immutable Direct-CFA evidence.
//
// LinearRawCameraRgbFloat32 writes the camera-native reconstructed RGB that
// defines the Scientific Master digest, before camera_to_xyz(), appearance,
// tone mapping or display projection. It is a reconstructed compatibility
// projection, not sensor evidence.
Status export_projection(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const finalized_scientific_preview_release::v0_2::ReleaseResult& release,
    tile_dng_v0_1::IRandomAccessByteSource& sourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    ISequentialByteSink& output,
    const Options& options,
    Result& out) noexcept;

const char* projection_name(ProjectionKind kind) noexcept;
const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::dng_projection::v0_1

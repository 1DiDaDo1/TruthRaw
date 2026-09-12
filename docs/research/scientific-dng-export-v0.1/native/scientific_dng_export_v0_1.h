#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_digest_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace truthraw::scientific_dng_export::v0_1 {

enum class ProjectionRole : std::uint8_t {
    LinearRawCompatibility = 1,
    ReconstructedCfaCompatibility = 2,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    InvalidAuthority,
    InvalidFinalizedLineage,
    InvalidColorTransform,
    SourceFailed,
    ReconstructionFailed,
    DigestFailed,
    ScientificIdentityMismatch,
    OutputFailed,
    BudgetExceeded,
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
    virtual bool flush() = 0;
    virtual std::uint64_t bytesWritten() const noexcept = 0;
};

struct Options final {
    ProjectionRole role = ProjectionRole::LinearRawCompatibility;
    std::size_t memoryBudgetBytes = 0; // 0 = no caller-imposed logical ceiling.
};

struct Result final {
    scientific_master_digest::v0_1::Sha256 replayedScientificMasterHash{};
    ProjectionRole role = ProjectionRole::LinearRawCompatibility;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t tileCount = 0;
    std::uint64_t bytesWritten = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    bool scientificMasterIdentityMatched = false;
    bool finalizedLineageValidated = false;
    bool boundedCompatibilityProjection = true;
    bool fullScientificMasterMaterialized = false;
    bool sourceMetadataBoundColor = false;
    bool independentPhysicalColor = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes a standards-oriented classic-TIFF DNG projection of an already
// finalized Scientific Master lineage. Pixel output is deliberately bounded to
// unsigned 16-bit [0, 65535] after clamp-to-[0,1]. The Scientific Master itself
// remains float32 camera-native RGB and is not modified or replaced.
//
// The caller must supply the exact Phase-2 result that finalized this prepared
// source. The exporter validates the Technical Backplane, source seal, admitted
// TileNative color binding, claim scope, 1/1 evidence invariant, and master hash
// before any output byte can be written.
//
// LinearRawCompatibility stores three camera-native reconstructed components per
// pixel using PhotometricInterpretation=LinearRaw.
// ReconstructedCfaCompatibility remosaics the same camera-native reconstructed
// RGB by selecting the source CFA component at each site. It is therefore a
// RECONSTRUCTED_CFA_PROJECTION, never measured sensor evidence.
//
// During export the camera-native reconstruction is replayed on the canonical
// 64x64 Scientific Master grid and hashed again. The function fails closed if
// that digest differs from the Scientific Master hash sealed in Phase 2.
Status export_scientific_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane_phase2::v0_1::Phase2Result& finalizedLineage,
    ISequentialByteSink& sink,
    const Options& options,
    Result& out) noexcept;

const char* role_name(ProjectionRole role) noexcept;
const char* status_name(StatusCode code) noexcept;

} // namespace truthraw::scientific_dng_export::v0_1

#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw::dng_projection_export::v0_1 {

using ScientificColorBindingRecord =
    scientific_preview_binding_v0_1::ScientificColorBindingRecord;

enum class ProjectionKind : std::uint8_t {
    LinearDng16 = 1,
    CfaDng16 = 2,
    ScientificMasterF32 = 3,
    // Historical compatibility alias for the first implementation name only.
    // User-facing/canonical naming is ScientificMasterF32 / .trmaster; the
    // rawsensor role remains reserved for CFA/sensor-grid representations.
    ScientificRawSensorF32 = ScientificMasterF32,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    AuthorityRejected,
    BudgetExceeded,
    SourceFailed,
    ReconstructionFailed,
    DigestFailed,
    ScientificIdentityMismatch,
    UnsupportedProjection,
    SinkFailed,
    MetadataFailed,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode c, std::string m) {
        Status s;
        s.code = c;
        s.message = std::move(m);
        return s;
    }
};

class ISequentialByteSink {
public:
    virtual ~ISequentialByteSink() = default;
    virtual bool writeExact(const void* data, std::size_t bytes) noexcept = 0;
};

struct AuthorityContext final {
    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    scientific_master_streaming_binding::v0_2::Result scientificIdentity{};
    technical_backplane_phase2::v0_1::Phase2Result phase2{};
    ScientificColorBindingRecord color{};
};

struct Options final {
    std::size_t memoryBudgetBytes = 0; // 0 = caller imposes no logical bound.
    int stripRows = 64;
};

struct Result final {
    ProjectionKind kind = ProjectionKind::LinearDng16;
    std::uint64_t bytesWritten = 0;
    std::size_t tilesProcessed = 0;
    std::size_t logicalWorkspacePeakBytes = 0;
    std::size_t logicalResidentUpperBound = 0;
    std::uint64_t negativeSamplesClamped = 0;
    std::uint64_t overOneSamplesClamped = 0;
    bool scientificMasterDigestVerified = false;
    bool fullScientificMasterMaterialized = false;
    bool compatibilityProjection = true;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

// Writes a downstream projection only after Technical Backplane phase-2 has
// admitted the exact source/scientific-master lineage. During export the
// camera-native Scientific Master RGB is reconstructed again and hashed. The
// writer fails closed unless that digest exactly matches the already-finalized
// Scientific Master digest.
//
// LinearDng16: bounded 16-bit LinearRaw DNG, camera-native reconstructed RGB.
// CfaDng16: bounded 16-bit normalized Stage-2 CFA/rawsensor projection;
//           corrected representation, never relabeled as measured sensor data.
// ScientificMasterF32: TruthRaw-private exact float32 camera-native RGB
//           Scientific Master serialization plus source/master provenance;
//           canonical file suffix is .trmaster, not .rawsensor.
Status export_projection(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const AuthorityContext& authority,
    ProjectionKind kind,
    ISequentialByteSink& sink,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;
const char* projection_name(ProjectionKind kind) noexcept;

} // namespace truthraw::dng_projection_export::v0_1

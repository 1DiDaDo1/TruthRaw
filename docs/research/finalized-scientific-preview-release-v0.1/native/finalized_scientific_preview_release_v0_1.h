#pragma once

#include "bounded_srgb_preview_sink_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_v0_1.h"

#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::finalized_scientific_preview_release::v0_1 {

enum class PreviewAuthority : std::uint8_t {
    None = 0,
    FinalizedSourceBoundScientificPreview,
    FinalizedIndependentlyCalibratedScientificPreview,
};

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    BackplaneRejected,
    FinalizationRejected,
    SourceIdentityMismatch,
    ColorIdentityMismatch,
    StreamingFailed,
    ProvenanceRejected,
    PreviewIncomplete,
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

struct ReleaseResult final {
    PreviewAuthority authority = PreviewAuthority::None;
    scientific_preview_binding_v0_1::ScientificPreviewAdmission admission{};
    technical_backplane::v0_1::State backplane{};
    streaming_v0_1::StreamingResult streaming{};
};

// Revalidates the serialized 180-byte Technical Backplane, recomputes the
// existing v0.2 post-master admission from the prepared source/color state,
// verifies that the actually opened tile source has the same source and color
// identity, and only then permits the existing bounded sRGB preview sink to be
// populated.
//
// The release is a finalized Scientific Preview of the admitted lineage. Its
// color scope remains source-bound unless the existing admission explicitly
// carries IndependentCalibration authority. It is not the Scientific Master
// and never upgrades source metadata into FULL_PHYSICAL color truth.
Status release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::SerializedBackplane& serializedBackplane,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const streaming_v0_1::StreamingOptions& options,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept;

const char* authority_name(PreviewAuthority authority) noexcept;
const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_release::v0_1

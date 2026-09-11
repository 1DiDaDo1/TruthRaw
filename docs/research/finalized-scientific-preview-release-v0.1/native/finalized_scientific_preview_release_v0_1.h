#pragma once

#include "bounded_srgb_preview_sink_v0_1.h"
#include "scientific_master_streaming_binding_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"

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
    SourceIdentityMismatch,
    ColorIdentityMismatch,
    ScientificIdentityFailed,
    ScientificIdentityMismatch,
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
    scientific_master_streaming_binding::v0_1::Result scientificIdentity{};
    technical_backplane_phase2::v0_1::Phase2Result canonicalPhase2{};
    streaming_v0_1::StreamingResult streaming{};
};

// Fail-closed release of the already-designed bounded sRGB preview.
//
// The function does not trust a caller-supplied admission object. It:
// 1) deserializes and CRC-validates the supplied 180-byte Backplane;
// 2) verifies that the opened tile source matches the prepared source/color;
// 3) recomputes Scientific Master + TruthRange self-gauge from that same source;
// 4) rebuilds Technical Backplane phase 2 using the supplied room/claim states;
// 5) requires byte-for-byte equality with the supplied 180-byte Backplane;
// 6) only then runs the existing Full-Frame Streaming + BoundedSrgbPreviewSink.
//
// Therefore a fabricated non-zero master/zero-line/scene-scale hash cannot open
// the Scientific Preview release gate.
Status release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::SerializedBackplane& serializedBackplane,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const scientific_master_streaming_binding::v0_1::Options& scientificOptions,
    const streaming_v0_1::StreamingOptions& previewOptions,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept;

const char* authority_name(PreviewAuthority authority) noexcept;
const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_release::v0_1

#pragma once

#include "bounded_srgb_preview_sink_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::finalized_scientific_preview_release::v0_2 {

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
    scientific_master_streaming_binding::v0_2::Result scientificIdentity{};
    technical_backplane_phase2::v0_1::Phase2Result canonicalPhase2{};
    streaming_v0_1::StreamingResult streaming{};
};

// v0.2 is scientifically identical to finalized release v0.1. The only
// intended change is use of Scientific Master Streaming Binding v0.2, whose
// exact self-gauge median requires two Stage-2 scans rather than four.
Status create_and_release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const scientific_master_streaming_binding::v0_2::Options& scientificOptions,
    const streaming_v0_1::StreamingOptions& previewOptions,
    const std::array<technical_backplane::v0_1::RoomStatus,
                     technical_backplane::v0_1::kRoomCount>& roomStatus,
    technical_backplane::v0_1::ClaimStatus claimStatus,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept;

Status release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::SerializedBackplane& serializedBackplane,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const scientific_master_streaming_binding::v0_2::Options& scientificOptions,
    const streaming_v0_1::StreamingOptions& previewOptions,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept;

const char* authority_name(PreviewAuthority authority) noexcept;
const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_release::v0_2

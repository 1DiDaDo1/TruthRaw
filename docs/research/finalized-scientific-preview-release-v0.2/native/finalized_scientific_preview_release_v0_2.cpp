#include "finalized_scientific_preview_release_v0_2.h"

#include <array>
#include <utility>

namespace truthraw::finalized_scientific_preview_release::v0_2 {
namespace {

PreviewAuthority authority_from_scope(
    scientific_preview_binding_v0_1::ColorClaimScope scope) noexcept {
    switch (scope) {
        case scientific_preview_binding_v0_1::ColorClaimScope::SourceBoundPreview:
            return PreviewAuthority::FinalizedSourceBoundScientificPreview;
        case scientific_preview_binding_v0_1::ColorClaimScope::IndependentlyCalibratedPreview:
            return PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview;
        case scientific_preview_binding_v0_1::ColorClaimScope::None:
            return PreviewAuthority::None;
    }
    return PreviewAuthority::None;
}

bool color_matrix_matches(
    const std::array<float, 9>& a,
    const std::array<float, 9>& b) noexcept {
    return a == b;
}

Status verify_opened_source_identity(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const streaming_v0_1::IRawTileSource& source) noexcept {
    const auto& metadata = source.metadata();
    if (metadata.sourceId != prepared.source.sourceEvidenceId) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "opened tile source does not match prepared source evidence identity");
    }
    if (prepared.color.bindingId.empty() ||
        !color_matrix_matches(metadata.cameraToXyzD50, prepared.color.cameraToXyzD50)) {
        return Status::error(StatusCode::ColorIdentityMismatch,
                             "opened tile source color matrix differs from prepared color binding");
    }
    return Status::ok();
}

Status verify_phase2_admission_identity(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const streaming_v0_1::IRawTileSource& source,
    const technical_backplane_phase2::v0_1::Phase2Result& phase2) noexcept {
    const auto& metadata = source.metadata();
    if (phase2.admission.tileNativeOptions.sourceEvidenceId != metadata.sourceId ||
        phase2.admission.tileNativeOptions.color.bindingId != prepared.color.bindingId) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "recomputed phase-2 admission differs from opened source identity");
    }
    if (!phase2.admission.tileNativeOptions.color.valid ||
        !color_matrix_matches(
            metadata.cameraToXyzD50,
            phase2.admission.tileNativeOptions.color.cameraToXyzD50)) {
        return Status::error(StatusCode::ColorIdentityMismatch,
                             "recomputed phase-2 admission differs from opened source color identity");
    }
    return Status::ok();
}

Status stream_authorized_preview(
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const streaming_v0_1::StreamingOptions& previewOptions,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& result) noexcept {
    streaming_v0_1::StreamingTruthRawProcessor processor(reconstruction, std::move(appearance));
    const auto streamStatus = processor.process(source, sink, previewOptions, result.streaming);
    if (!streamStatus) {
        return Status::error(StatusCode::StreamingFailed,
                             "finalized preview streaming failed: " + streamStatus.message);
    }

    const auto& provenance = result.streaming.provenance;
    const auto& memory = result.streaming.memory;
    if (provenance.physicalFrameCount != 1u ||
        provenance.independentEvidenceCount != 1u ||
        provenance.scientificMasterModifiedByAppearance ||
        provenance.counterfactualObservationCreated ||
        !provenance.gainMapAppliedExactlyOnce ||
        memory.adapterOwnsFullRawFrame ||
        memory.adapterOwnsFullSdrFrame ||
        memory.adapterOwnsFullHalfGainFrame ||
        memory.adapterOwnsFullDiagnosticFrame) {
        return Status::error(StatusCode::ProvenanceRejected,
                             "streamed preview violated Scientific Preview provenance/resource invariants");
    }

    if (!sink.finished() || sink.argb8888().empty() ||
        sink.writtenPixelCount() != sink.argb8888().size()) {
        return Status::error(StatusCode::PreviewIncomplete,
                             "bounded sRGB preview surface did not complete");
    }
    return Status::ok();
}

}  // namespace

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
    ReleaseResult& out) noexcept {
    out = {};
    if (!reconstruction || !appearance) {
        return Status::error(StatusCode::InvalidArgument,
                             "preview creation requires reconstruction and appearance backends");
    }

    const auto sourceIdentity = verify_opened_source_identity(prepared, source);
    if (!sourceIdentity) return sourceIdentity;

    ReleaseResult result{};
    const auto scientificStatus =
        scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            source, *reconstruction, scientificOptions, result.scientificIdentity);
    if (!scientificStatus) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "Scientific Master/TruthRange streaming identity failed: " +
                                 scientificStatus.message);
    }

    technical_backplane_phase2::v0_1::Phase2Input phase2Input{};
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = result.scientificIdentity.scientificMasterHash;
    phase2Input.zeroLineGauge = result.scientificIdentity.zeroLineGauge;
    phase2Input.sceneBinding = result.scientificIdentity.sceneBinding;
    phase2Input.roomStatus = roomStatus;
    phase2Input.claimStatus = claimStatus;

    const auto phase2Status = technical_backplane_phase2::v0_1::finalize_phase2(
        phase2Input, result.canonicalPhase2);
    if (!phase2Status) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "canonical phase-2 creation failed: " + phase2Status.message);
    }

    const auto admissionIdentity = verify_phase2_admission_identity(
        prepared, source, result.canonicalPhase2);
    if (!admissionIdentity) return admissionIdentity;

    result.authority = authority_from_scope(result.canonicalPhase2.admission.claimScope);
    if (result.authority == PreviewAuthority::None) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "canonical phase-2 admission has no authorized preview color scope");
    }

    const auto streamed = stream_authorized_preview(
        source, reconstruction, std::move(appearance), previewOptions, sink, result);
    if (!streamed) return streamed;

    out = std::move(result);
    return Status::ok();
}

Status release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::SerializedBackplane& serializedBackplane,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const scientific_master_streaming_binding::v0_2::Options& scientificOptions,
    const streaming_v0_1::StreamingOptions& previewOptions,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept {
    out = {};
    if (!reconstruction || !appearance) {
        return Status::error(StatusCode::InvalidArgument,
                             "preview release requires reconstruction and appearance backends");
    }

    technical_backplane::v0_1::State suppliedBackplane{};
    const auto backplaneStatus = technical_backplane::v0_1::deserialize(
        serializedBackplane, suppliedBackplane);
    if (backplaneStatus != technical_backplane::v0_1::Status::Ok) {
        return Status::error(
            StatusCode::BackplaneRejected,
            std::string("serialized Technical Backplane rejected: ") +
                technical_backplane::v0_1::status_name(backplaneStatus));
    }

    const auto sourceIdentity = verify_opened_source_identity(prepared, source);
    if (!sourceIdentity) return sourceIdentity;

    ReleaseResult result{};
    const auto scientificStatus =
        scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            source, *reconstruction, scientificOptions, result.scientificIdentity);
    if (!scientificStatus) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "Scientific Master/TruthRange streaming identity failed: " +
                                 scientificStatus.message);
    }

    technical_backplane_phase2::v0_1::Phase2Input phase2Input{};
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = result.scientificIdentity.scientificMasterHash;
    phase2Input.zeroLineGauge = result.scientificIdentity.zeroLineGauge;
    phase2Input.sceneBinding = result.scientificIdentity.sceneBinding;
    phase2Input.roomStatus = suppliedBackplane.roomStatus;
    phase2Input.claimStatus = suppliedBackplane.claimStatus;

    const auto phase2Status = technical_backplane_phase2::v0_1::finalize_phase2(
        phase2Input, result.canonicalPhase2);
    if (!phase2Status) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "canonical phase-2 rebuild failed: " + phase2Status.message);
    }

    if (result.canonicalPhase2.serializedBackplane != serializedBackplane) {
        return Status::error(StatusCode::ScientificIdentityMismatch,
                             "supplied Backplane does not equal recomputed source/master/gauge/scale lineage");
    }

    const auto admissionIdentity = verify_phase2_admission_identity(
        prepared, source, result.canonicalPhase2);
    if (!admissionIdentity) return admissionIdentity;

    result.authority = authority_from_scope(result.canonicalPhase2.admission.claimScope);
    if (result.authority == PreviewAuthority::None) {
        return Status::error(StatusCode::ScientificIdentityFailed,
                             "canonical phase-2 admission has no authorized preview color scope");
    }

    const auto streamed = stream_authorized_preview(
        source, reconstruction, std::move(appearance), previewOptions, sink, result);
    if (!streamed) return streamed;

    out = std::move(result);
    return Status::ok();
}

const char* authority_name(PreviewAuthority authority) noexcept {
    switch (authority) {
        case PreviewAuthority::None:
            return "NONE";
        case PreviewAuthority::FinalizedSourceBoundScientificPreview:
            return "FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW";
        case PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview:
            return "FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW";
    }
    return "UNKNOWN";
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::BackplaneRejected: return "BACKPLANE_REJECTED";
        case StatusCode::SourceIdentityMismatch: return "SOURCE_IDENTITY_MISMATCH";
        case StatusCode::ColorIdentityMismatch: return "COLOR_IDENTITY_MISMATCH";
        case StatusCode::ScientificIdentityFailed: return "SCIENTIFIC_IDENTITY_FAILED";
        case StatusCode::ScientificIdentityMismatch: return "SCIENTIFIC_IDENTITY_MISMATCH";
        case StatusCode::StreamingFailed: return "STREAMING_FAILED";
        case StatusCode::ProvenanceRejected: return "PROVENANCE_REJECTED";
        case StatusCode::PreviewIncomplete: return "PREVIEW_INCOMPLETE";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::finalized_scientific_preview_release::v0_2

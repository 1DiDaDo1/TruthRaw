#include "finalized_scientific_preview_release_v0_1.h"

#include <array>
#include <utility>

namespace truthraw::finalized_scientific_preview_release::v0_1 {
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

}  // namespace

Status release_finalized_scientific_preview(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane::v0_1::SerializedBackplane& serializedBackplane,
    streaming_v0_1::IRawTileSource& source,
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance,
    const streaming_v0_1::StreamingOptions& options,
    preview_surface_v0_1::BoundedSrgbPreviewSink& sink,
    ReleaseResult& out) noexcept {
    out = {};
    if (!reconstruction || !appearance) {
        return Status::error(StatusCode::InvalidArgument,
                             "preview release requires reconstruction and appearance backends");
    }

    ReleaseResult result{};
    const auto backplaneStatus = technical_backplane::v0_1::deserialize(
        serializedBackplane, result.backplane);
    if (backplaneStatus != technical_backplane::v0_1::Status::Ok) {
        return Status::error(
            StatusCode::BackplaneRejected,
            std::string("serialized Technical Backplane rejected: ") +
                technical_backplane::v0_1::status_name(backplaneStatus));
    }

    const auto finalizeStatus =
        scientific_preview_binding_v0_2::finalize_scientific_color_lineage(
            prepared, result.backplane, result.admission);
    if (!finalizeStatus) {
        return Status::error(StatusCode::FinalizationRejected,
                             "post-master preview admission rejected: " + finalizeStatus.message);
    }

    result.authority = authority_from_scope(result.admission.claimScope);
    if (result.authority == PreviewAuthority::None) {
        return Status::error(StatusCode::FinalizationRejected,
                             "finalized preview admission has no authorized color scope");
    }

    const auto& metadata = source.metadata();
    if (metadata.sourceId != result.admission.tileNativeOptions.sourceEvidenceId ||
        metadata.sourceId != prepared.source.sourceEvidenceId) {
        return Status::error(StatusCode::SourceIdentityMismatch,
                             "opened tile source does not match finalized source evidence identity");
    }

    if (!result.admission.tileNativeOptions.color.valid ||
        result.admission.tileNativeOptions.color.bindingId != prepared.color.bindingId ||
        !color_matrix_matches(metadata.cameraToXyzD50,
                              result.admission.tileNativeOptions.color.cameraToXyzD50) ||
        !color_matrix_matches(metadata.cameraToXyzD50, prepared.color.cameraToXyzD50)) {
        return Status::error(StatusCode::ColorIdentityMismatch,
                             "opened tile source color identity differs from finalized admission");
    }

    streaming_v0_1::StreamingTruthRawProcessor processor(
        std::move(reconstruction), std::move(appearance));
    const auto streamStatus = processor.process(source, sink, options, result.streaming);
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
        case StatusCode::FinalizationRejected: return "FINALIZATION_REJECTED";
        case StatusCode::SourceIdentityMismatch: return "SOURCE_IDENTITY_MISMATCH";
        case StatusCode::ColorIdentityMismatch: return "COLOR_IDENTITY_MISMATCH";
        case StatusCode::StreamingFailed: return "STREAMING_FAILED";
        case StatusCode::ProvenanceRejected: return "PROVENANCE_REJECTED";
        case StatusCode::PreviewIncomplete: return "PREVIEW_INCOMPLETE";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::finalized_scientific_preview_release::v0_1

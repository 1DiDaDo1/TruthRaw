#include "finalized_scientific_preview_route_v0_1.h"

namespace truthraw::finalized_scientific_preview_route::v0_1 {
namespace {

bool prepared_is_pre_master(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared) noexcept {
    return prepared.mainHouseComputeAllowed &&
           prepared.sourceBoundAppearanceReleaseAllowed &&
           !prepared.scientificPreviewReleaseAllowed &&
           !prepared.scientificClaimAllowed &&
           prepared.physicalFrameCount == 1u &&
           prepared.independentEvidenceCount == 1u &&
           !prepared.source.sourceEvidenceId.empty();
}

}  // namespace

Status finalize_from_streaming_source(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept {
    out = {};

    if (!prepared_is_pre_master(prepared)) {
        return Status::error(StatusCode::InvalidPreparedSource,
                             "prepared source is not a canonical pre-master state");
    }

    const auto& metadata = source.metadata();
    if (metadata.sourceId.empty() || metadata.sourceId != prepared.source.sourceEvidenceId) {
        return Status::error(StatusCode::SourceLineageMismatch,
                             "IRawTileSource sourceId does not match prepared sealed source evidence");
    }

    Result result{};
    const auto scientificStatus =
        scientific_master_streaming_binding::v0_1::bind_scientific_master_streaming(
            source, reconstruction, options.scientificBinding, result.scientific);
    if (!scientificStatus) {
        return Status::error(
            StatusCode::ScientificMasterBindingFailed,
            std::string("streaming Scientific Master binding failed: ") +
                scientific_master_streaming_binding::v0_1::status_name(scientificStatus.code) +
                ": " + scientificStatus.message);
    }

    if (result.scientific.physicalFrameCount != 1u ||
        result.scientific.independentEvidenceCount != 1u) {
        return Status::error(StatusCode::ScientificMasterBindingFailed,
                             "streaming Scientific Master altered frame/evidence counts");
    }

    technical_backplane_phase2::v0_1::Phase2Input phase2Input{};
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = result.scientific.scientificMasterHash;
    phase2Input.zeroLineGauge = result.scientific.zeroLineGauge;
    phase2Input.sceneBinding = result.scientific.sceneBinding;
    phase2Input.roomStatus = options.roomStatus;
    phase2Input.claimStatus = options.claimStatus;

    const auto phase2Status = technical_backplane_phase2::v0_1::finalize_phase2(
        phase2Input, result.phase2);
    if (!phase2Status) {
        return Status::error(
            StatusCode::Phase2FinalizationFailed,
            std::string("Technical Backplane phase 2 failed: ") +
                technical_backplane_phase2::v0_1::status_name(phase2Status.code) +
                ": " + phase2Status.message);
    }

    const auto& admission = result.phase2.admission;
    if (admission.sourceSeal.sha256 != prepared.source.sha256 ||
        admission.sourceSeal.sourceEvidenceId != prepared.source.sourceEvidenceId ||
        admission.claimScope != prepared.eventualClaimScope ||
        admission.tileNativeOptions.sourceEvidenceId != prepared.tileNativeOptions.sourceEvidenceId ||
        admission.tileNativeOptions.color.bindingId != prepared.tileNativeOptions.color.bindingId ||
        admission.tileNativeOptions.color.cameraToXyzD50 !=
            prepared.tileNativeOptions.color.cameraToXyzD50) {
        return Status::error(StatusCode::FinalAdmissionMismatch,
                             "final Scientific Preview admission changed prepared source/color identity");
    }

    result.scientificPreviewReleaseAllowed = true;
    result.scientificClaimAllowed =
        admission.claimScope ==
        scientific_preview_binding_v0_1::ColorClaimScope::IndependentlyCalibratedPreview;

    out = result;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidPreparedSource: return "INVALID_PREPARED_SOURCE";
        case StatusCode::SourceLineageMismatch: return "SOURCE_LINEAGE_MISMATCH";
        case StatusCode::ScientificMasterBindingFailed: return "SCIENTIFIC_MASTER_BINDING_FAILED";
        case StatusCode::Phase2FinalizationFailed: return "PHASE2_FINALIZATION_FAILED";
        case StatusCode::FinalAdmissionMismatch: return "FINAL_ADMISSION_MISMATCH";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::finalized_scientific_preview_route::v0_1

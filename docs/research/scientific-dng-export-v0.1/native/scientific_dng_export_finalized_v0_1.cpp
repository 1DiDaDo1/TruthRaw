#include "scientific_dng_export_v0_1.h"

#include <array>

namespace truthraw::scientific_dng_export::v0_1 {

// Internal implementation overload. It is deliberately not exposed in the
// public header: public callers must present a complete Phase-2 result.
Status export_scientific_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const scientific_master_digest::v0_1::Sha256& expectedScientificMasterHash,
    ISequentialByteSink& sink,
    const Options& options,
    Result& out) noexcept;

namespace {

bool same_color_matrix(const std::array<float, 9>& a,
                       const std::array<float, 9>& b) noexcept {
    return a == b;
}

bool valid_finalized_lineage(
    const streaming_v0_1::IRawTileSource& source,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane_phase2::v0_1::Phase2Result& finalized) noexcept {
    using truthraw::technical_backplane::v0_1::Status;

    if (truthraw::technical_backplane::v0_1::validate(finalized.backplane) != Status::Ok) {
        return false;
    }
    if (finalized.backplane.physicalFrameCount != 1u ||
        finalized.backplane.independentEvidenceCount != 1u ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return false;
    }
    if (finalized.backplane.sourceEvidenceHash != prepared.source.sha256 ||
        finalized.admission.sourceSeal.sha256 != prepared.source.sha256 ||
        finalized.admission.sourceSeal.byteLength != prepared.source.byteLength ||
        finalized.admission.sourceSeal.sourceEvidenceId != prepared.source.sourceEvidenceId) {
        return false;
    }
    if (finalized.zeroLineHash != finalized.backplane.zeroLineHash ||
        finalized.sceneScaleHash != finalized.backplane.sceneScaleHash) {
        return false;
    }
    bool masterNonZero = false;
    for (const auto v : finalized.backplane.scientificMasterHash) {
        masterNonZero = masterNonZero || v != 0u;
    }
    if (!masterNonZero) return false;

    technical_backplane::v0_1::State roundtrip{};
    if (technical_backplane::v0_1::deserialize(
            finalized.serializedBackplane, roundtrip) != Status::Ok) {
        return false;
    }
    if (roundtrip.sourceEvidenceHash != finalized.backplane.sourceEvidenceHash ||
        roundtrip.scientificMasterHash != finalized.backplane.scientificMasterHash ||
        roundtrip.zeroLineHash != finalized.backplane.zeroLineHash ||
        roundtrip.sceneScaleHash != finalized.backplane.sceneScaleHash ||
        roundtrip.physicalFrameCount != finalized.backplane.physicalFrameCount ||
        roundtrip.independentEvidenceCount != finalized.backplane.independentEvidenceCount ||
        roundtrip.roomStatus != finalized.backplane.roomStatus ||
        roundtrip.claimStatus != finalized.backplane.claimStatus ||
        roundtrip.forbiddenFlags != finalized.backplane.forbiddenFlags) {
        return false;
    }

    if (finalized.admission.claimScope != prepared.eventualClaimScope ||
        finalized.admission.tileNativeOptions.sourceEvidenceId != prepared.source.sourceEvidenceId ||
        !finalized.admission.tileNativeOptions.color.valid ||
        finalized.admission.tileNativeOptions.color.bindingId != prepared.color.bindingId ||
        !same_color_matrix(finalized.admission.tileNativeOptions.color.cameraToXyzD50,
                           prepared.color.cameraToXyzD50)) {
        return false;
    }

    const auto& metadata = source.metadata();
    if (metadata.sourceId != prepared.source.sourceEvidenceId ||
        !same_color_matrix(metadata.cameraToXyzD50, prepared.color.cameraToXyzD50)) {
        return false;
    }
    return true;
}

} // namespace

Status export_scientific_dng(
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    const technical_backplane_phase2::v0_1::Phase2Result& finalizedLineage,
    ISequentialByteSink& sink,
    const Options& options,
    Result& out) noexcept {
    out = {};
    out.role = options.role;
    if (!valid_finalized_lineage(source, prepared, finalizedLineage)) {
        return Status::error(
            StatusCode::InvalidAuthority,
            "DNG export requires the exact validated Phase-2 source/master/color lineage");
    }

    const auto status = export_scientific_dng(
        source,
        reconstruction,
        prepared,
        finalizedLineage.backplane.scientificMasterHash,
        sink,
        options,
        out);
    if (!status) return status;
    out.finalizedLineageValidated = true;
    return Status::ok();
}

} // namespace truthraw::scientific_dng_export::v0_1

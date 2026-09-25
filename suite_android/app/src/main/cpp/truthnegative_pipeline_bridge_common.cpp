#include "truthnegative_pipeline_bridge_common.h"

#include "tile_native_dng_source_v0_1.h"

#include <utility>

namespace truthraw::android_truthnegative_pipeline::v0_1 {
namespace {

Status fail(int code, std::string message) {
    return {code, std::move(message)};
}

}  // namespace

Status prepare(
    int sourceFd,
    std::size_t maxSourceResidentBytes,
    std::size_t maxLogicalResidentBytes,
    Context& out) noexcept {
    out = Context{};
    try {
        if (sourceFd < 0 ||
            maxSourceResidentBytes == 0u ||
            maxLogicalResidentBytes == 0u) {
            return fail(-1, "invalid pipeline arguments");
        }

        out.bytes =
            std::make_shared<tile_dng_v0_1::PosixFdByteSource>(sourceFd);

        const auto sealed =
            scientific_preview_binding_v0_1::seal_source_sha256(
                *out.bytes, out.sourceSeal);
        if (!sealed) {
            return fail(
                2000 + static_cast<int>(sealed.code),
                sealed.message);
        }

        const auto color =
            dng_color_binding_producer_v0_2::
                produce_source_metadata_color_binding(
                    *out.bytes, out.sourceSeal, out.produced);
        if (!color) {
            return fail(
                2100 + static_cast<int>(color.code),
                color.message);
        }

        const auto preparedStatus =
            scientific_preview_binding_v0_2::
                prepare_scientific_color_source(
                    out.sourceSeal,
                    out.produced.color,
                    out.prepared);
        if (!preparedStatus) {
            return fail(
                2000 + static_cast<int>(preparedStatus.code),
                preparedStatus.message);
        }
        if (!out.prepared.mainHouseComputeAllowed ||
            out.prepared.physicalFrameCount != 1u ||
            out.prepared.independentEvidenceCount != 1u) {
            return fail(-2, "prepared source evidence invariant failed");
        }

        const auto pre =
            scientific_preview_binding_v0_1::reverify_source_sha256(
                *out.bytes, out.sourceSeal);
        if (!pre) {
            return fail(
                2000 + static_cast<int>(pre.code),
                pre.message);
        }

        auto options = out.prepared.tileNativeOptions;
        options.maxResidentBytes = maxSourceResidentBytes;
        const auto opened =
            android_raw_adapter_bridge::v0_1::openDngViaAdapter(
                out.bytes,
                out.sourceSeal,
                options,
                out.openedSource);
        if (!opened) {
            return fail(
                7000 + static_cast<int>(opened.code),
                opened.message);
        }

        out.reconstruction =
            std::make_shared<
                scientific_master_f64_reconstruction_v0_1::
                    ResearchEdgeAwareMeasuredPreservingReconstructionF64>();

        scientific_master_streaming_binding::v0_2::Options scienceOptions;
        scienceOptions.memoryBudgetBytes = maxLogicalResidentBytes;
        const auto scienceStatus =
            scientific_master_streaming_binding::v0_2::
                bind_scientific_master_streaming(
                    *out.openedSource.source,
                    *out.reconstruction,
                    scienceOptions,
                    out.scientific);
        if (!scienceStatus) {
            return fail(
                8000 + static_cast<int>(scienceStatus.code),
                scienceStatus.message);
        }

        technical_backplane_phase2::v0_1::Phase2Input phaseInput;
        phaseInput.prepared = out.prepared;
        phaseInput.scientificMasterHash =
            out.scientific.scientificMasterHash;
        phaseInput.zeroLineGauge = out.scientific.zeroLineGauge;
        phaseInput.sceneBinding = out.scientific.sceneBinding;
        phaseInput.roomStatus.fill(
            technical_backplane::v0_1::RoomStatus::ResearchOnly);
        phaseInput.claimStatus =
            technical_backplane::v0_1::ClaimStatus::Candidate;

        const auto phaseStatus =
            technical_backplane_phase2::v0_1::finalize_phase2(
                phaseInput, out.phase2);
        if (!phaseStatus) {
            return fail(
                9000 + static_cast<int>(phaseStatus.code),
                phaseStatus.message);
        }

        if (out.phase2.admission.claimScope ==
                scientific_preview_binding_v0_1::ColorClaimScope::None ||
            out.phase2.backplane.sourceEvidenceHash !=
                out.sourceSeal.sha256 ||
            out.phase2.backplane.scientificMasterHash !=
                out.scientific.scientificMasterHash ||
            out.phase2.backplane.forbiddenFlags != 0u ||
            out.phase2.backplane.physicalFrameCount != 1u ||
            out.phase2.backplane.independentEvidenceCount != 1u ||
            out.scientific.physicalFrameCount != 1u ||
            out.scientific.independentEvidenceCount != 1u) {
            return fail(-3, "phase2/master lineage invariant failed");
        }

        const auto& metadata = out.openedSource.source->metadata();
        if (metadata.width <= 0 || metadata.height <= 0) {
            return fail(-4, "invalid source geometry");
        }
        out.width = static_cast<std::uint32_t>(metadata.width);
        out.height = static_cast<std::uint32_t>(metadata.height);

        out.masterSource =
            std::make_unique<
                scientific_master_linear_dng_projection::v0_1::
                    StreamingScientificMasterTileSource>(
                        *out.openedSource.source,
                        *out.reconstruction);
        out.fieldSource =
            std::make_unique<
                truthnegative_dense_local_field_adapter::v0_4::
                    SourceFieldAdapter>(
                        *out.openedSource.source,
                        *out.masterSource);

        if (!truthnegative_continuous::v0_5::summarizeAuthorityField(
                *out.fieldSource, out.authorityField) ||
            out.authorityField.createsNewEvidence ||
            out.authorityField.scientificWritebackAllowed) {
            return fail(-5, "authority field binding failed");
        }

        truthnegative_continuous::v0_5::StateInput stateInput{};
        stateInput.sourceEvidenceSha256 = out.sourceSeal.sha256;
        stateInput.scientificMasterSha256 =
            out.scientific.scientificMasterHash;
        stateInput.authorityFieldSha256 =
            out.authorityField.contentSha256;
        stateInput.width = out.width;
        stateInput.height = out.height;
        stateInput.reconstructionBackendId =
            out.reconstruction->name();
        stateInput.colourBindingId =
            out.produced.color.bindingId;
        stateInput.physicalFrameCount = 1u;
        stateInput.independentEvidenceCount = 1u;

        if (!truthnegative_continuous::v0_5::finalizeState(
                stateInput, out.truthNegativeState) ||
            !out.truthNegativeState.isRasterIndependent ||
            out.truthNegativeState.createsNewEvidence ||
            out.truthNegativeState.scientificWritebackAllowed) {
            return fail(-6, "TruthNegative continuous state failed");
        }

        return {};
    } catch (...) {
        out = Context{};
        return fail(-99, "unexpected pipeline exception");
    }
}

bool reverify(const Context& context) noexcept {
    if (!context.bytes) return false;
    const auto status =
        scientific_preview_binding_v0_1::reverify_source_sha256(
            *context.bytes, context.sourceSeal);
    return static_cast<bool>(status);
}

}  // namespace truthraw::android_truthnegative_pipeline::v0_1

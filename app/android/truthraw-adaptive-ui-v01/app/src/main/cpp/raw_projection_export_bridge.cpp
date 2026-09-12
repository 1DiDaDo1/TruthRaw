#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "raw_projection_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::raw_projection_export::v0_1::ProjectionKind;
using truthraw::raw_projection_export::v0_1::Result;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jint kExportMagic = 0x54525831; // TRX1
constexpr std::size_t kHeaderInts = 19u;

jint clamp_metric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jintArray packet(JNIEnv* env, jint status) {
    std::vector<jint> out(kHeaderInts, 0);
    out[0] = kExportMagic;
    out[1] = status;
    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result != nullptr) env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

jint binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jint>(status.code);
}

jint producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jint>(status.code);
}

jint dng_status(const truthraw::tile_dng_v0_1::DngSourceStatus& status) {
    return 3000 + static_cast<jint>(status.code);
}

jint science_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& status) {
    return 8000 + static_cast<jint>(status.code);
}

jint phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& status) {
    return 7000 + static_cast<jint>(status.code);
}

jint export_status(const truthraw::raw_projection_export::v0_1::Status& status) {
    return 6000 + static_cast<jint>(status.code);
}

bool decode_kind(jint value, ProjectionKind& out) noexcept {
    switch (value) {
        case 1: out = ProjectionKind::RawSensorCfa16; return true;
        case 2: out = ProjectionKind::ReconstructedCfaDng16; return true;
        case 3: out = ProjectionKind::LinearDng16; return true;
        default: return false;
    }
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_exportFinalizedProjection(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint projectionKind,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    ProjectionKind kind{};
    if (sourceFd < 0 || outputFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0 || !decode_kind(projectionKind, kind)) {
        return packet(env, -1);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus = truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
        *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus));
    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return packet(env, -2);
    }

    const auto preVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_status(opened));

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus = truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
        *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return packet(env, science_status(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientific.scientificMasterHash;
    phaseInput.zeroLineGauge = scientific.zeroLineGauge;
    phaseInput.sceneBinding = scientific.sceneBinding;
    phaseInput.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto finalized = truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phaseInput, phase2);
    if (!finalized) return packet(env, phase2_status(finalized));
    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.admission.sourceSeal.sha256 != sourceSeal.sha256 ||
        phase2.admission.sourceSeal.byteLength != sourceSeal.byteLength ||
        scientific.physicalFrameCount != 1u || scientific.independentEvidenceCount != 1u) {
        return packet(env, -3);
    }

    const auto beforeExportVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!beforeExportVerified) return packet(env, binding_status(beforeExportVerified));

    truthraw::raw_projection_export::v0_1::Options exportOptions;
    exportOptions.rowsPerStrip = 32;
    exportOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    Result exported;
    const auto exportedStatus = truthraw::raw_projection_export::v0_1::export_projection(
        *source, *reconstruction, static_cast<int>(outputFd), kind, exportOptions, exported);
    if (!exportedStatus) return packet(env, export_status(exportedStatus));

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return packet(env, binding_status(postVerified));

    if (!exported.projectionOnly || exported.sourcePixelsClaimedMeasured ||
        exported.fullScientificMasterMaterialized ||
        exported.physicalFrameCount != 1u || exported.independentEvidenceCount != 1u ||
        exported.width != source->metadata().width || exported.height != source->metadata().height) {
        return packet(env, -4);
    }

    std::vector<jint> out(kHeaderInts, 0);
    out[0] = kExportMagic;
    out[1] = 0;
    out[2] = static_cast<jint>(kind);
    out[3] = exported.width;
    out[4] = exported.height;
    out[5] = exported.samplesPerPixel;
    out[6] = clamp_metric(exported.stripsWritten);
    out[7] = clamp_metric(exported.outputBytes);
    out[8] = clamp_metric(exported.projectedSamples);
    out[9] = clamp_metric(exported.clippedLowSamples);
    out[10] = clamp_metric(exported.clippedHighSamples);
    out[11] = clamp_metric(exported.logicalWorkspacePeakBytes);
    out[12] = exported.fullScientificMasterMaterialized ? 1 : 0;
    out[13] = exported.sourcePixelsClaimedMeasured ? 1 : 0;
    out[14] = static_cast<jint>(exported.physicalFrameCount);
    out[15] = static_cast<jint>(exported.independentEvidenceCount);
    out[16] = static_cast<jint>(scientific.stage2GaugeScanPasses);
    out[17] = static_cast<jint>(phase2.admission.claimScope);
    out[18] = produced.audit.cameraCalibrationApplied ? 1 : 0;

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result == nullptr) return nullptr;
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

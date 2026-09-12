#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "full_frame_streaming_v0_1.h"
#include "linear_dng_compatibility_projection_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <array>
#include <cstdint>
#include <memory>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::streaming_v0_1::StreamingOptions;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kLinearDngMagic = 0x544c4431ll; // TLD1
constexpr std::size_t kPacketLongs = 14u;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;
constexpr int kGatePreviewEdge = 32;

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kLinearDngMagic;
    values[1] = status;
    jlongArray out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000ll + static_cast<jlong>(status.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100ll + static_cast<jlong>(status.code);
}

jlong dng_status(const truthraw::tile_dng_v0_1::DngSourceStatus& status) {
    return 3000ll + static_cast<jlong>(status.code);
}

jlong finalized_status(const truthraw::finalized_scientific_preview_release::v0_2::Status& status) {
    return 5000ll + static_cast<jlong>(status.code);
}

jlong projection_status(const truthraw::linear_dng_compatibility_projection::v0_1::Status& status) {
    return 6100ll + static_cast<jlong>(status.code);
}

StreamingOptions gate_preview_options(std::size_t memoryBudgetBytes) {
    StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeLinearDngBridge_exportFinalizedLinearDng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || outputFd < 0 || maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
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

    const auto preVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_status(opened));

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    BoundedSrgbPreviewSink gateSink(kGatePreviewEdge);
    ReleaseResult release;
    const auto released =
        truthraw::finalized_scientific_preview_release::v0_2::create_and_release_finalized_scientific_preview(
            prepared,
            *source,
            reconstruction,
            appearance,
            scientificOptions,
            gate_preview_options(static_cast<std::size_t>(maxLogicalResidentBytes)),
            roomStatus,
            truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
            gateSink,
            release);
    if (!released) return packet(env, finalized_status(released));

    if (release.authority == PreviewAuthority::None ||
        release.streaming.provenance.physicalFrameCount != 1u ||
        release.streaming.provenance.independentEvidenceCount != 1u ||
        release.streaming.provenance.scientificMasterModifiedByAppearance ||
        release.streaming.provenance.counterfactualObservationCreated) {
        return packet(env, -3);
    }

    truthraw::linear_dng_compatibility_projection::v0_1::Options projectionOptions;
    projectionOptions.tileEdge = kTileCore;
    projectionOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::linear_dng_compatibility_projection::v0_1::Result projection;
    const auto projected =
        truthraw::linear_dng_compatibility_projection::v0_1::write_linear_dng(
            *source,
            *reconstruction,
            produced.color,
            static_cast<int>(outputFd),
            projectionOptions,
            projection);
    if (!projected) return packet(env, projection_status(projected));

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!postVerified) return packet(env, binding_status(postVerified));

    const auto& audit = source->audit();
    if (audit.fullRawMaterialized || audit.fullFileMaterialized ||
        projection.fullFrameMaterialized || projection.appearanceApplied ||
        projection.scientificMasterModified ||
        projection.physicalFrameCount != 1u || projection.independentEvidenceCount != 1u) {
        return packet(env, -4);
    }

    const bool independentlyCalibrated =
        release.authority == PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview;

    std::array<jlong, kPacketLongs> values{};
    values[0] = kLinearDngMagic;
    values[1] = 0;
    values[2] = static_cast<jlong>(projection.width);
    values[3] = static_cast<jlong>(projection.height);
    values[4] = static_cast<jlong>(projection.outputBytes);
    values[5] = static_cast<jlong>(projection.tilesWritten);
    values[6] = static_cast<jlong>(projection.samplesWritten);
    values[7] = static_cast<jlong>(projection.clippedBelowZero);
    values[8] = static_cast<jlong>(projection.clippedAboveOne);
    values[9] = static_cast<jlong>(projection.logicalWorkspacePeakBytes);
    values[10] = static_cast<jlong>(projection.physicalFrameCount);
    values[11] = static_cast<jlong>(projection.independentEvidenceCount);
    values[12] = independentlyCalibrated ? 1ll : 0ll;
    values[13] = static_cast<jlong>(release.authority);

    jlongArray out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out == nullptr) return nullptr;
    env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    return out;
}

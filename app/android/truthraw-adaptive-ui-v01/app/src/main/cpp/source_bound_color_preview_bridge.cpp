#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "full_frame_streaming_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
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

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::streaming_v0_1::StreamingOptions;
using truthraw::streaming_v0_1::StreamingResult;
using truthraw::streaming_v0_1::StreamingTruthRawProcessor;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jint kSourceBoundMagic = 0x54524331; // TRC1
constexpr jint kFinalizedMagic = 0x54524631;   // TRF1
constexpr std::size_t kSourceBoundHeaderInts = 23;
constexpr std::size_t kFinalizedHeaderInts = 24;
constexpr int kAbsoluteMaxPreviewEdge = 512;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;

jint clamp_metric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jintArray status_packet(JNIEnv* env, jint magic, std::size_t headerInts, jint status) {
    std::vector<jint> header(headerInts, 0);
    header[0] = magic;
    header[1] = status;
    jintArray out = env->NewIntArray(static_cast<jsize>(headerInts));
    if (out != nullptr) {
        env->SetIntArrayRegion(out, 0, static_cast<jsize>(headerInts), header.data());
    }
    return out;
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

jint stream_status(const truthraw::streaming_v0_1::StreamStatus& status) {
    return 4000 + static_cast<jint>(status.code);
}

jint finalized_status(const truthraw::finalized_scientific_preview_release::v0_2::Status& status) {
    return 5000 + static_cast<jint>(status.code);
}

StreamingOptions preview_options(std::size_t memoryBudgetBytes) {
    StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

bool valid_preview_surface(const BoundedSrgbPreviewSink& sink) {
    const int width = sink.width();
    const int height = sink.height();
    if (width <= 0 || height <= 0 || width > kAbsoluteMaxPreviewEdge || height > kAbsoluteMaxPreviewEdge) {
        return false;
    }
    const auto& pixels = sink.argb8888();
    const std::size_t expectedPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    return pixels.size() == expectedPixels && sink.writtenPixelCount() == expectedPixels;
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_buildSourceBoundColorPreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (fd < 0 || requestedMaxEdge < 32 || maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, -1);
    }

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus = truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
        *bytes, sourceSeal, produced);
    if (!colorStatus) return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, binding_status(preparedStatus));

    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, -2);
    }

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, dng_status(opened));

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    StreamingTruthRawProcessor processor(reconstruction, appearance);
    BoundedSrgbPreviewSink sink(maxEdge);

    StreamingResult streaming;
    const auto processed = processor.process(
        *source,
        sink,
        preview_options(static_cast<std::size_t>(maxLogicalResidentBytes)),
        streaming);
    if (!processed) return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, stream_status(processed));

    const auto& audit = source->audit();
    if (!sink.finished() || audit.fullRawMaterialized || audit.fullFileMaterialized ||
        streaming.memory.adapterOwnsFullRawFrame || streaming.memory.adapterOwnsFullSdrFrame ||
        streaming.memory.adapterOwnsFullHalfGainFrame || streaming.memory.adapterOwnsFullDiagnosticFrame) {
        return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, -3);
    }
    if (streaming.provenance.physicalFrameCount != 1u || streaming.provenance.independentEvidenceCount != 1u ||
        streaming.provenance.scientificMasterModifiedByAppearance) {
        return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, -4);
    }
    if (!valid_preview_surface(sink)) {
        return status_packet(env, kSourceBoundMagic, kSourceBoundHeaderInts, -5);
    }

    const int width = sink.width();
    const int height = sink.height();
    const auto& pixels = sink.argb8888();
    std::vector<jint> out(kSourceBoundHeaderInts + pixels.size(), 0);
    out[0] = kSourceBoundMagic;
    out[1] = 0;
    out[2] = width;
    out[3] = height;
    out[4] = source->metadata().width;
    out[5] = source->metadata().height;
    out[6] = clamp_metric(source->residentBytesUpperBound());
    out[7] = clamp_metric(audit.rawPayloadBytesRead);
    out[8] = clamp_metric(audit.metadataBytesRead + produced.audit.metadataBytesRead);
    out[9] = clamp_metric(audit.tileReadCalls);
    out[10] = audit.fullRawMaterialized ? 1 : 0;
    out[11] = source->metadata().hasGainField ? 1 : 0;
    out[12] = static_cast<jint>(source->metadata().orientation);
    out[13] = prepared.sourceBoundAppearanceReleaseAllowed ? 1 : 0;
    out[14] = prepared.scientificPreviewReleaseAllowed ? 1 : 0;
    out[15] = prepared.scientificClaimAllowed ? 1 : 0;
    out[16] = static_cast<jint>(streaming.provenance.physicalFrameCount);
    out[17] = static_cast<jint>(streaming.provenance.independentEvidenceCount);
    out[18] = clamp_metric(streaming.memory.logicalResidentUpperBound);
    out[19] = clamp_metric(streaming.tilesProcessedPass1);
    out[20] = clamp_metric(streaming.tilesProcessedPass2);
    out[21] = produced.audit.usedForwardMatrix ? 1 : 0;
    out[22] = produced.audit.cameraCalibrationApplied ? 1 : 0;

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        out[kSourceBoundHeaderInts + i] = static_cast<jint>(pixels[i]);
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result == nullptr) return nullptr;
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_buildFinalizedScientificColorPreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (fd < 0 || requestedMaxEdge < 32 || maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, -1);
    }

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus = truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
        *bytes, sourceSeal, produced);
    if (!colorStatus) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, binding_status(preparedStatus));

    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, -2);
    }

    const auto preVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, dng_status(opened));

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    BoundedSrgbPreviewSink sink(maxEdge);

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    ReleaseResult release;
    const auto released =
        truthraw::finalized_scientific_preview_release::v0_2::create_and_release_finalized_scientific_preview(
            prepared,
            *source,
            reconstruction,
            appearance,
            scientificOptions,
            preview_options(static_cast<std::size_t>(maxLogicalResidentBytes)),
            roomStatus,
            truthraw::technical_backplane::v0_1::ClaimStatus::Candidate,
            sink,
            release);
    if (!released) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, finalized_status(released));

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, binding_status(postVerified));

    const auto& audit = source->audit();
    if (audit.fullRawMaterialized || audit.fullFileMaterialized || !valid_preview_surface(sink)) {
        return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, -3);
    }

    const bool independentlyCalibrated =
        release.authority == PreviewAuthority::FinalizedIndependentlyCalibratedScientificPreview;
    if (release.authority == PreviewAuthority::None ||
        release.streaming.provenance.physicalFrameCount != 1u ||
        release.streaming.provenance.independentEvidenceCount != 1u ||
        release.streaming.provenance.scientificMasterModifiedByAppearance ||
        release.streaming.provenance.counterfactualObservationCreated) {
        return status_packet(env, kFinalizedMagic, kFinalizedHeaderInts, -4);
    }

    const int width = sink.width();
    const int height = sink.height();
    const auto& pixels = sink.argb8888();
    const std::uint64_t logicalResidentUpperBound = std::max<std::uint64_t>(
        static_cast<std::uint64_t>(release.streaming.memory.logicalResidentUpperBound),
        static_cast<std::uint64_t>(release.scientificIdentity.logicalResidentUpperBound));

    std::vector<jint> out(kFinalizedHeaderInts + pixels.size(), 0);
    out[0] = kFinalizedMagic;
    out[1] = 0;
    out[2] = width;
    out[3] = height;
    out[4] = source->metadata().width;
    out[5] = source->metadata().height;
    out[6] = clamp_metric(source->residentBytesUpperBound());
    out[7] = clamp_metric(audit.rawPayloadBytesRead);
    out[8] = clamp_metric(audit.metadataBytesRead + produced.audit.metadataBytesRead);
    out[9] = clamp_metric(audit.tileReadCalls);
    out[10] = audit.fullRawMaterialized ? 1 : 0;
    out[11] = source->metadata().hasGainField ? 1 : 0;
    out[12] = static_cast<jint>(source->metadata().orientation);
    out[13] = prepared.sourceBoundAppearanceReleaseAllowed ? 1 : 0;
    out[14] = 1;
    out[15] = independentlyCalibrated ? 1 : 0;
    out[16] = static_cast<jint>(release.streaming.provenance.physicalFrameCount);
    out[17] = static_cast<jint>(release.streaming.provenance.independentEvidenceCount);
    out[18] = clamp_metric(logicalResidentUpperBound);
    out[19] = clamp_metric(release.streaming.tilesProcessedPass1);
    out[20] = clamp_metric(release.streaming.tilesProcessedPass2);
    out[21] = produced.audit.usedForwardMatrix ? 1 : 0;
    out[22] = produced.audit.cameraCalibrationApplied ? 1 : 0;
    out[23] = static_cast<jint>(release.authority);

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        out[kFinalizedHeaderInts + i] = static_cast<jint>(pixels[i]);
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result == nullptr) return nullptr;
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

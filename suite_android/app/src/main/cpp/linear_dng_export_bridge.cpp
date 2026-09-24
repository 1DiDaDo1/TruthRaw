#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "finalized_scientific_preview_release_v0_2.h"
#include "linear_dng_projection_v0_1.h"
#include "raw_source_adapter_bridge_common.h"
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
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstructionF64;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::finalized_scientific_preview_release::v0_2::PreviewAuthority;
using truthraw::finalized_scientific_preview_release::v0_2::ReleaseResult;
using truthraw::linear_dng_projection::v0_1::PosixFdByteSink;
using truthraw::linear_dng_projection::v0_1::Result;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

constexpr jlong kMagic = 0x5452444c; // TRDL
constexpr std::size_t kPacketLongs = 13u;
constexpr int kExportPreviewEdge = 64;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jlong>(status.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jlong>(status.code);
}

jlong adapter_status(const truthraw::multivendor_raw_source_adapter::v0_1::AdapterStatus& status) {
    return 7000 + static_cast<jlong>(status.code);
}

jlong finalized_status(const truthraw::finalized_scientific_preview_release::v0_2::Status& status) {
    return 5000 + static_cast<jlong>(status.code);
}

jlong projection_status(const truthraw::linear_dng_projection::v0_1::Status& status) {
    return 6000 + static_cast<jlong>(status.code);
}

truthraw::streaming_v0_1::StreamingOptions preview_options(std::size_t memoryBudgetBytes) {
    truthraw::streaming_v0_1::StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

bool release_matches_source(
    const ReleaseResult& release,
    const SourceSeal& seal,
    const truthraw::streaming_v0_1::IRawTileSource& source) noexcept {
    const auto& phase2 = release.canonicalPhase2;
    return phase2.admission.sourceSeal.sha256 == seal.sha256 &&
           phase2.admission.sourceSeal.byteLength == seal.byteLength &&
           phase2.admission.sourceSeal.sourceEvidenceId == seal.sourceEvidenceId &&
           phase2.backplane.sourceEvidenceHash == seal.sha256 &&
           phase2.backplane.scientificMasterHash == release.scientificIdentity.scientificMasterHash &&
           source.metadata().sourceId == seal.sourceEvidenceId;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_LinearDngNativeBridge_exportFinalizedLinearDng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || destinationFd < 0 || maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
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
    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened = truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
        bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return packet(env, adapter_status(opened));
    auto& source = openedSource.source;

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstructionF64>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    BoundedSrgbPreviewSink releaseSink(kExportPreviewEdge);

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
            releaseSink,
            release);
    if (!released) return packet(env, finalized_status(released));
    if (release.authority == PreviewAuthority::None ||
        release.scientificIdentity.physicalFrameCount != 1u ||
        release.scientificIdentity.independentEvidenceCount != 1u) {
        return packet(env, -3);
    }
    if (!release_matches_source(release, sourceSeal, *source)) {
        return packet(env, -5);
    }

    const auto verifiedBeforeProjection =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!verifiedBeforeProjection) return packet(env, binding_status(verifiedBeforeProjection));

    PosixFdByteSink destination(static_cast<int>(destinationFd));
    truthraw::linear_dng_projection::v0_1::Options projectionOptions;
    projectionOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    Result projection;
    const auto projected = truthraw::linear_dng_projection::v0_1::write_finalized_linear_dng(
        release,
        *bytes,
        *source,
        *reconstruction,
        destination,
        projectionOptions,
        projection);
    if (!projected) return packet(env, projection_status(projected));

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return packet(env, binding_status(postVerified));
    if (!release_matches_source(release, sourceSeal, *source)) {
        return packet(env, -5);
    }

    if (!projection.linearRawPhotometric || !projection.boundedUnsigned16Projection ||
        !projection.sourceColorMetadataCopied || projection.fullScientificMasterMaterialized ||
        projection.physicalFrameCount != 1u || projection.independentEvidenceCount != 1u) {
        return packet(env, -4);
    }

    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = 0;
    values[2] = projection.width;
    values[3] = projection.height;
    values[4] = static_cast<jlong>(std::min<std::uint64_t>(projection.outputBytes, std::numeric_limits<jlong>::max()));
    values[5] = static_cast<jlong>(std::min<std::uint64_t>(projection.pixelPayloadBytes, std::numeric_limits<jlong>::max()));
    values[6] = static_cast<jlong>(std::min<std::uint64_t>(projection.tilesWritten, std::numeric_limits<jlong>::max()));
    values[7] = static_cast<jlong>(std::min<std::uint64_t>(projection.samplesClippedLow, std::numeric_limits<jlong>::max()));
    values[8] = static_cast<jlong>(std::min<std::uint64_t>(projection.samplesClippedHigh, std::numeric_limits<jlong>::max()));
    values[9] = static_cast<jlong>(projection.logicalResidentUpperBound);
    values[10] = projection.fullScientificMasterMaterialized ? 1 : 0;
    values[11] = projection.physicalFrameCount;
    values[12] = projection.independentEvidenceCount;

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    return out;
}

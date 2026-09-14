#include <jni.h>

#include "bounded_srgb_preview_sink_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "multiworker_streaming_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <vector>

namespace {

using truthraw::NeutralReferenceAppearance;
using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::streaming_v0_1::StreamingOptions;
using truthraw::streaming_v0_1::StreamingResult;
using truthraw::streaming_v0_1::StreamingTruthRawProcessor;
using truthraw::streaming_v0_2::MultiWorkerTelemetry;
using truthraw::streaming_v0_2::process_multiworker_streaming;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jint kMagic = 0x54525732; // TRW2
constexpr std::size_t kHeaderInts = 32;
constexpr int kAbsoluteMaxPreviewEdge = 512;
constexpr int kTileCore = 128;
constexpr int kTileHalo = 16;

jint clamp_u64(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jintArray emit_status(JNIEnv* env, jint status, jint requestedWorkers) {
    std::vector<jint> out(kHeaderInts, 0);
    out[0] = kMagic;
    out[1] = status;
    out[2] = requestedWorkers;
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

jint stream_status(const truthraw::streaming_v0_1::StreamStatus& status) {
    return 4000 + static_cast<jint>(status.code);
}

std::uint64_t process_cpu_nanos() {
    timespec ts{};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) return 0;
    return std::uint64_t(ts.tv_sec) * 1000000000ull + std::uint64_t(ts.tv_nsec);
}

StreamingOptions empirical_options(int workers, std::size_t memoryBudgetBytes) {
    StreamingOptions options;
    options.tile = {kTileCore, kTileHalo};
    options.workers = workers;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = memoryBudgetBytes;
    return options;
}

bool valid_preview_surface(const BoundedSrgbPreviewSink& sink) {
    if (!sink.finished() || sink.width() <= 0 || sink.height() <= 0 ||
        sink.width() > kAbsoluteMaxPreviewEdge || sink.height() > kAbsoluteMaxPreviewEdge) {
        return false;
    }
    const std::size_t expected = std::size_t(sink.width()) * std::size_t(sink.height());
    return sink.argb8888().size() == expected && sink.writtenPixelCount() == expected;
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeMultiWorkerEmpiricalBridge_probePreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedWorkers,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (fd < 0 || (requestedWorkers != 1 && requestedWorkers != 2 && requestedWorkers != 4) ||
        requestedMaxEdge < 32 || maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return emit_status(env, -1, requestedWorkers);
    }

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return emit_status(env, binding_status(sealed), requestedWorkers);

    ProducerResult produced;
    const auto colorStatus = truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
        *bytes, sourceSeal, produced);
    if (!colorStatus) return emit_status(env, producer_status(colorStatus), requestedWorkers);

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return emit_status(env, binding_status(preparedStatus), requestedWorkers);

    // Research-only benchmark path. It may compute from the source-bound scientific input,
    // but it must not strengthen release authority or create extra evidence.
    if (!prepared.mainHouseComputeAllowed || !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed || prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u || prepared.independentEvidenceCount != 1u) {
        return emit_status(env, -2, requestedWorkers);
    }

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return emit_status(env, dng_status(opened), requestedWorkers);

    BoundedSrgbPreviewSink sink(maxEdge);
    StreamingResult streaming;
    MultiWorkerTelemetry telemetry;
    const auto options = empirical_options(
        static_cast<int>(requestedWorkers), static_cast<std::size_t>(maxLogicalResidentBytes));

    const auto wallStart = std::chrono::steady_clock::now();
    const std::uint64_t cpuStart = process_cpu_nanos();

    truthraw::streaming_v0_1::StreamStatus processed;
    if (requestedWorkers == 1) {
        auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
        auto appearance = std::make_shared<NeutralReferenceAppearance>();
        StreamingTruthRawProcessor processor(reconstruction, appearance);
        processed = processor.process(*source, sink, options, streaming);
        telemetry.effectiveWorkers = 1;
        telemetry.orderedCommit = true;
    } else {
        // These exact v4.7i backends are stateless at object scope. Their scratch storage is
        // local or thread_local in the byte-frozen canonical implementation. Unknown/external
        // backends are deliberately not exposed through this research JNI entry point.
        ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
        NeutralReferenceAppearance appearance;
        processed = process_multiworker_streaming(
            *source, sink, reconstruction, appearance, options, streaming, telemetry);
    }

    const std::uint64_t cpuEnd = process_cpu_nanos();
    const auto wallEnd = std::chrono::steady_clock::now();
    if (!processed) return emit_status(env, stream_status(processed), requestedWorkers);

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) return emit_status(env, binding_status(postVerified), requestedWorkers);

    const auto& audit = source->audit();
    if (!valid_preview_surface(sink) || audit.fullRawMaterialized || audit.fullFileMaterialized ||
        streaming.provenance.physicalFrameCount != 1u ||
        streaming.provenance.independentEvidenceCount != 1u ||
        streaming.provenance.scientificMasterModifiedByAppearance ||
        !telemetry.orderedCommit) {
        return emit_status(env, -3, requestedWorkers);
    }

    const auto wallUs = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(wallEnd - wallStart).count());
    const std::uint64_t cpuUs = cpuEnd >= cpuStart ? (cpuEnd - cpuStart) / 1000ull : 0ull;

    const auto& pixels = sink.argb8888();
    std::vector<jint> out(kHeaderInts + pixels.size(), 0);
    out[0] = kMagic;
    out[1] = 0;
    out[2] = requestedWorkers;
    out[3] = telemetry.effectiveWorkers;
    out[4] = sink.width();
    out[5] = sink.height();
    out[6] = source->metadata().width;
    out[7] = source->metadata().height;
    out[8] = clamp_u64(source->residentBytesUpperBound());
    out[9] = clamp_u64(streaming.memory.logicalResidentUpperBound);
    out[10] = clamp_u64(audit.rawPayloadBytesRead);
    out[11] = clamp_u64(audit.metadataBytesRead + produced.audit.metadataBytesRead);
    out[12] = clamp_u64(audit.tileReadCalls);
    out[13] = clamp_u64(streaming.tilesProcessedPass1);
    out[14] = clamp_u64(streaming.tilesProcessedPass2);
    out[15] = clamp_u64(telemetry.queueDepth);
    out[16] = clamp_u64(telemetry.maxReadyPackets);
    out[17] = audit.fullRawMaterialized ? 1 : 0;
    out[18] = audit.fullFileMaterialized ? 1 : 0;
    out[19] = static_cast<jint>(streaming.provenance.physicalFrameCount);
    out[20] = static_cast<jint>(streaming.provenance.independentEvidenceCount);
    out[21] = clamp_u64(streaming.clippedCount);
    out[22] = clamp_u64(streaming.stage2Over1Count);
    out[23] = clamp_u64(sink.halfGainSamplesObserved());
    out[24] = telemetry.orderedCommit ? 1 : 0;
    out[25] = produced.audit.usedForwardMatrix ? 1 : 0;
    out[26] = produced.audit.cameraCalibrationApplied ? 1 : 0;
    out[27] = static_cast<jint>(produced.color.authority);
    out[28] = clamp_u64(wallUs);
    out[29] = clamp_u64(cpuUs);
    out[30] = prepared.sourceBoundAppearanceReleaseAllowed ? 1 : 0;
    out[31] = prepared.scientificClaimAllowed ? 1 : 0;

    for (std::size_t i = 0; i < pixels.size(); ++i) {
        out[kHeaderInts + i] = static_cast<jint>(pixels[i]);
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result == nullptr) return nullptr;
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

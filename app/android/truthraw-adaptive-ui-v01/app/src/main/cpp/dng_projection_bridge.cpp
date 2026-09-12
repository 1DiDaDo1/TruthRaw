#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"
#include "truthraw_dng_projection_v0_1.h"

#include <cerrno>
#include <cstdint>
#include <memory>
#include <unistd.h>

namespace {

using truthraw::dng_projection::v0_1::ProjectionKind;

constexpr jlong kPacketMagic = 0x54524450LL; // TRDP
constexpr std::size_t kPacketLongs = 21u;

jlongArray failure(JNIEnv* env, jlong code) {
    jlong packet[2] = {kPacketMagic, code};
    jlongArray out = env->NewLongArray(2);
    if (out != nullptr) env->SetLongArrayRegion(out, 0, 2, packet);
    return out;
}

jlong map_binding(truthraw::scientific_preview_binding_v0_1::BindingStatusCode code) {
    return 2000LL + static_cast<jlong>(code) + 1LL;
}

jlong map_color(truthraw::dng_color_binding_producer_v0_2::ProducerStatusCode code) {
    return 2100LL + static_cast<jlong>(code) + 1LL;
}

jlong map_source(truthraw::tile_dng_v0_1::DngSourceCode code) {
    return 3000LL + static_cast<jlong>(code) + 1LL;
}

jlong map_master(truthraw::scientific_master_streaming_binding::v0_2::StatusCode code) {
    return 5500LL + static_cast<jlong>(code) + 1LL;
}

jlong map_phase2(truthraw::technical_backplane_phase2::v0_1::StatusCode code) {
    return 5600LL + static_cast<jlong>(code) + 1LL;
}

jlong map_projection(truthraw::dng_projection::v0_1::StatusCode code) {
    return 6000LL + static_cast<jlong>(code) + 1LL;
}

class PosixFdSequentialSink final : public truthraw::dng_projection::v0_1::ISequentialByteSink {
public:
    explicit PosixFdSequentialSink(int fd) : fd_(fd) {}

    bool write(const void* data, std::size_t bytes) override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        std::size_t remaining = bytes;
        while (remaining != 0u) {
            const ssize_t written = ::write(fd_, p, remaining);
            if (written < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (written == 0) return false;
            const auto step = static_cast<std::size_t>(written);
            p += step;
            remaining -= step;
        }
        return true;
    }

private:
    int fd_ = -1;
};

bool reset_output_fd(int fd) {
    if (fd < 0) return false;
    if (::ftruncate(fd, 0) != 0) return false;
    return ::lseek(fd, 0, SEEK_SET) == 0;
}

void discard_output_fd(int fd) {
    if (fd < 0) return;
    (void)::ftruncate(fd, 0);
    (void)::lseek(fd, 0, SEEK_SET);
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeDngProjectionBridge_exportFinalizedProjection(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint kindCode,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || outputFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0 ||
        (kindCode != 0 && kindCode != 1)) {
        return failure(env, -1);
    }
    if (!reset_output_fd(outputFd)) return failure(env, -2);

    auto sourceBytes = std::make_shared<truthraw::tile_dng_v0_1::PosixFdByteSource>(sourceFd);
    if (sourceBytes->sizeBytes() == 0u) {
        discard_output_fd(outputFd);
        return failure(env, -3);
    }

    truthraw::scientific_preview_binding_v0_1::SourceSeal sourceSeal{};
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(
        *sourceBytes, sourceSeal);
    if (!sealed) {
        discard_output_fd(outputFd);
        return failure(env, map_binding(sealed.code));
    }

    truthraw::dng_color_binding_producer_v0_2::ProducerResult colorResult{};
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *sourceBytes, sourceSeal, colorResult);
    if (!colorStatus) {
        discard_output_fd(outputFd);
        return failure(env, map_color(colorStatus.code));
    }

    truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, colorResult.color, prepared);
    if (!preparedStatus) {
        discard_output_fd(outputFd);
        return failure(env, map_binding(preparedStatus.code));
    }
    prepared.tileNativeOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<truthraw::tile_dng_v0_1::TileNativeDngSource> source;
    const auto openStatus = truthraw::tile_dng_v0_1::TileNativeDngSource::open(
        sourceBytes, prepared.tileNativeOptions, source);
    if (!openStatus || !source) {
        discard_output_fd(outputFd);
        return failure(env, map_source(openStatus.code));
    }

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    truthraw::scientific_master_streaming_binding::v0_2::Options masterOptions{};
    masterOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientificIdentity{};
    const auto masterStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, reconstruction, masterOptions, scientificIdentity);
    if (!masterStatus) {
        discard_output_fd(outputFd);
        return failure(env, map_master(masterStatus.code));
    }

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput{};
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientificIdentity.scientificMasterHash;
    phaseInput.zeroLineGauge = scientificIdentity.zeroLineGauge;
    phaseInput.sceneBinding = scientificIdentity.sceneBinding;
    phaseInput.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;
    truthraw::technical_backplane_phase2::v0_1::Phase2Result finalized{};
    const auto phaseStatus =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phaseInput, finalized);
    if (!phaseStatus) {
        discard_output_fd(outputFd);
        return failure(env, map_phase2(phaseStatus.code));
    }

    PosixFdSequentialSink sink(outputFd);
    truthraw::dng_projection::v0_1::Options exportOptions{};
    exportOptions.kind = kindCode == 0
        ? ProjectionKind::Stage2CfaFloat32
        : ProjectionKind::LinearRawCameraRgbFloat32;
    exportOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::dng_projection::v0_1::Result exportResult{};
    const auto exportStatus = truthraw::dng_projection::v0_1::export_projection(
        prepared, scientificIdentity, finalized, *sourceBytes, *source,
        reconstruction, sink, exportOptions, exportResult);
    if (!exportStatus) {
        discard_output_fd(outputFd);
        return failure(env, map_projection(exportStatus.code));
    }

    const auto& audit = source->audit();
    jlong packet[kPacketLongs] = {};
    packet[0] = kPacketMagic;
    packet[1] = 0;
    packet[2] = static_cast<jlong>(kindCode);
    packet[3] = static_cast<jlong>(exportResult.bytesWritten);
    packet[4] = static_cast<jlong>(exportResult.stripsWritten);
    packet[5] = static_cast<jlong>(exportResult.canonicalTilesProcessed);
    packet[6] = static_cast<jlong>(exportResult.logicalWorkspacePeakBytes);
    packet[7] = static_cast<jlong>(exportResult.logicalResidentUpperBound);
    packet[8] = exportResult.sourceVerifiedBefore ? 1 : 0;
    packet[9] = exportResult.sourceVerifiedAfter ? 1 : 0;
    packet[10] = exportResult.scientificMasterHashMatched ? 1 : 0;
    packet[11] = exportResult.fullFrameMaterialized ? 1 : 0;
    packet[12] = exportResult.createsEvidence ? 1 : 0;
    packet[13] = static_cast<jlong>(exportResult.physicalFrameCount);
    packet[14] = static_cast<jlong>(exportResult.independentEvidenceCount);
    packet[15] = static_cast<jlong>(scientificIdentity.stage2GaugeScanPasses);
    packet[16] = static_cast<jlong>(audit.rawPayloadBytesRead);
    packet[17] = static_cast<jlong>(audit.metadataBytesRead);
    packet[18] = static_cast<jlong>(audit.tileReadCalls);
    packet[19] = static_cast<jlong>(finalized.admission.claimScope);
    packet[20] = static_cast<jlong>(colorResult.color.authority);

    jlongArray out = env->NewLongArray(static_cast<jsize>(kPacketLongs));
    if (out == nullptr) {
        discard_output_fd(outputFd);
        return nullptr;
    }
    env->SetLongArrayRegion(out, 0, static_cast<jsize>(kPacketLongs), packet);
    return out;
}

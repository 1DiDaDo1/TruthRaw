#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "dng_projection_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <memory>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::dng_projection_export::v0_1::AuthorityContext;
using truthraw::dng_projection_export::v0_1::Options;
using truthraw::dng_projection_export::v0_1::ProjectionKind;
using truthraw::dng_projection_export::v0_1::Result;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kMagic = 0x54524558LL; // TREX
constexpr std::size_t kPacketLongs = 16u;

class FdSequentialSink final : public truthraw::dng_projection_export::v0_1::ISequentialByteSink {
public:
    explicit FdSequentialSink(int fd) : fd_(fd) {}

    bool writeExact(const void* data, std::size_t bytes) noexcept override {
        const auto* p = static_cast<const std::uint8_t*>(data);
        std::size_t remaining = bytes;
        while (remaining != 0u) {
            const ssize_t wrote = ::write(fd_, p, remaining);
            if (wrote < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (wrote == 0) return false;
            p += static_cast<std::size_t>(wrote);
            remaining -= static_cast<std::size_t>(wrote);
        }
        return true;
    }

private:
    int fd_ = -1;
};

jlongArray packet(JNIEnv* env, jlong status, ProjectionKind kind, const Result* result,
                  std::uint32_t claimScope, std::uint32_t colorAuthority) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    values[2] = static_cast<jlong>(kind);
    values[14] = static_cast<jlong>(claimScope);
    values[15] = static_cast<jlong>(colorAuthority);
    if (result != nullptr) {
        values[3] = static_cast<jlong>(result->bytesWritten);
        values[4] = static_cast<jlong>(result->tilesProcessed);
        values[5] = static_cast<jlong>(result->logicalWorkspacePeakBytes);
        values[6] = static_cast<jlong>(result->logicalResidentUpperBound);
        values[7] = static_cast<jlong>(result->negativeSamplesClamped);
        values[8] = static_cast<jlong>(result->overOneSamplesClamped);
        values[9] = result->scientificMasterDigestVerified ? 1 : 0;
        values[10] = result->fullScientificMasterMaterialized ? 1 : 0;
        values[11] = result->compatibilityProjection ? 1 : 0;
        values[12] = static_cast<jlong>(result->physicalFrameCount);
        values[13] = static_cast<jlong>(result->independentEvidenceCount);
    }
    jlongArray out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& s) {
    return 2000 + static_cast<jlong>(s.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s) {
    return 2100 + static_cast<jlong>(s.code);
}

jlong dng_status(const truthraw::tile_dng_v0_1::DngSourceStatus& s) {
    return 3000 + static_cast<jlong>(s.code);
}

jlong scientific_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& s) {
    return 5200 + static_cast<jlong>(s.code);
}

jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& s) {
    return 5300 + static_cast<jlong>(s.code);
}

jlong export_status(const truthraw::dng_projection_export::v0_1::Status& s) {
    return 6000 + static_cast<jlong>(s.code);
}

bool reset_output(int fd) noexcept {
    if (fd < 0) return false;
    const bool truncated = ::ftruncate(fd, 0) == 0;
    const off_t seeked = ::lseek(fd, 0, SEEK_SET);
    return truncated && seeked == 0;
}

ProjectionKind decode_kind(jint kind, bool& ok) noexcept {
    ok = true;
    switch (kind) {
        case 1: return ProjectionKind::LinearDng16;
        case 2: return ProjectionKind::CfaDng16;
        case 3: return ProjectionKind::ScientificRawSensorF32;
        default:
            ok = false;
            return ProjectionKind::LinearDng16;
    }
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeProjectionExportBridge_exportScientificProjection(
    JNIEnv* env,
    jobject,
    jint inputFd,
    jint outputFd,
    jint projectionKind,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    bool kindOk = false;
    const ProjectionKind kind = decode_kind(projectionKind, kindOk);
    if (!kindOk || inputFd < 0 || outputFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0) {
        return packet(env, -1, kind, nullptr, 0u, 0u);
    }
    if (!reset_output(static_cast<int>(outputFd))) {
        return packet(env, -2, kind, nullptr, 0u, 0u);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(inputFd));
    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed), kind, nullptr, 0u, 0u);

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus), kind, nullptr, 0u, 0u);

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus), kind, nullptr, 0u, 0u);

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_status(opened), kind, nullptr, 0u, 0u);

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) {
        return packet(env, scientific_status(scientificStatus), kind, nullptr, 0u, 0u);
    }

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phase2Input;
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = scientific.scientificMasterHash;
    phase2Input.zeroLineGauge = scientific.zeroLineGauge;
    phase2Input.sceneBinding = scientific.sceneBinding;
    phase2Input.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phase2Input.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto phase2Status = truthraw::technical_backplane_phase2::v0_1::finalize_phase2(
        phase2Input, phase2);
    if (!phase2Status) {
        return packet(env, phase2_status(phase2Status), kind, nullptr, 0u, 0u);
    }

    const auto preExportVerify = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!preExportVerify) {
        return packet(env, binding_status(preExportVerify), kind, nullptr, 0u, 0u);
    }

    AuthorityContext authority;
    authority.prepared = prepared;
    authority.scientificIdentity = scientific;
    authority.phase2 = phase2;
    authority.color = produced.color;

    FdSequentialSink output(static_cast<int>(outputFd));
    Options exportOptions;
    exportOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    exportOptions.stripRows = truthraw::scientific_master_streaming_binding::v0_2::kCanonicalCore;
    Result result;
    const auto exported = truthraw::dng_projection_export::v0_1::export_projection(
        *source, *reconstruction, authority, kind, output, exportOptions, result);
    if (!exported) {
        reset_output(static_cast<int>(outputFd));
        return packet(env, export_status(exported), kind, nullptr,
                      static_cast<std::uint32_t>(phase2.admission.claimScope),
                      static_cast<std::uint32_t>(produced.color.authority));
    }

    const auto postExportVerify = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!postExportVerify) {
        reset_output(static_cast<int>(outputFd));
        return packet(env, binding_status(postExportVerify), kind, nullptr,
                      static_cast<std::uint32_t>(phase2.admission.claimScope),
                      static_cast<std::uint32_t>(produced.color.authority));
    }

    const auto& audit = source->audit();
    if (audit.fullRawMaterialized || audit.fullFileMaterialized ||
        !result.scientificMasterDigestVerified || result.fullScientificMasterMaterialized ||
        result.physicalFrameCount != 1u || result.independentEvidenceCount != 1u) {
        reset_output(static_cast<int>(outputFd));
        return packet(env, -3, kind, nullptr,
                      static_cast<std::uint32_t>(phase2.admission.claimScope),
                      static_cast<std::uint32_t>(produced.color.authority));
    }

    return packet(env, 0, kind, &result,
                  static_cast<std::uint32_t>(phase2.admission.claimScope),
                  static_cast<std::uint32_t>(produced.color.authority));
}

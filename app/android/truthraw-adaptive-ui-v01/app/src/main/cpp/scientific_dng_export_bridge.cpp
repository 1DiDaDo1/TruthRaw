#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "scientific_dng_export_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include <unistd.h>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_dng_export::v0_1::ProjectionRole;
using truthraw::scientific_dng_export::v0_1::Result;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jint kDngExportMagic = 0x54524431; // TRD1
constexpr std::size_t kHeaderInts = 18u;

jint clamp_metric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jintArray packet(JNIEnv* env, jint status, const std::array<jint, kHeaderInts>* values = nullptr) {
    std::array<jint, kHeaderInts> out{};
    out[0] = kDngExportMagic;
    out[1] = status;
    if (values != nullptr) {
        for (std::size_t i = 2u; i < out.size(); ++i) out[i] = (*values)[i];
    }
    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result != nullptr) {
        env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    }
    return result;
}

jint binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jint>(status.code);
}
jint producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jint>(status.code);
}
jint dng_source_status(const truthraw::tile_dng_v0_1::DngSourceStatus& status) {
    return 3000 + static_cast<jint>(status.code);
}
jint scientific_status(const truthraw::scientific_master_streaming_binding::v0_2::Status& status) {
    return 6000 + static_cast<jint>(status.code);
}
jint phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& status) {
    return 7000 + static_cast<jint>(status.code);
}
jint export_status(const truthraw::scientific_dng_export::v0_1::Status& status) {
    return 8000 + static_cast<jint>(status.code);
}

class PosixSequentialSink final : public truthraw::scientific_dng_export::v0_1::ISequentialByteSink {
public:
    explicit PosixSequentialSink(int fd) : fd_(fd) {}

    bool write(const void* data, std::size_t bytes) override {
        if (fd_ < 0 || (data == nullptr && bytes != 0u)) return false;
        const auto* cursor = static_cast<const std::uint8_t*>(data);
        std::size_t remaining = bytes;
        while (remaining != 0u) {
            const ssize_t n = ::write(fd_, cursor, remaining);
            if (n < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (n == 0) return false;
            const auto consumed = static_cast<std::size_t>(n);
            cursor += consumed;
            remaining -= consumed;
            total_ += consumed;
        }
        return true;
    }

    bool flush() override {
        if (fd_ < 0) return false;
        if (::fsync(fd_) == 0) return true;
        // Some document providers expose a sequential pipe rather than a
        // regular file. A completed write to such a descriptor cannot be
        // fsync'ed; close by ParcelFileDescriptor remains the provider commit.
        return errno == EINVAL || errno == EROFS;
    }

    std::uint64_t bytesWritten() const noexcept override { return total_; }

private:
    int fd_ = -1;
    std::uint64_t total_ = 0u;
};

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeScientificDngBridge_exportScientificDng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint roleCode,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || outputFd < 0 || sourceFd == outputFd ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0 ||
        (roleCode != 1 && roleCode != 2)) {
        return packet(env, -1);
    }

    const ProjectionRole role = roleCode == 1
        ? ProjectionRole::LinearRawCompatibility
        : ProjectionRole::ReconstructedCfaCompatibility;

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));
    SourceSeal sourceSeal{};
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed));

    ProducerResult produced{};
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus));

    PreparedScientificPreviewSource prepared{};
    const auto preparedStatus = truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
        sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus));

    const auto preVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!preVerified) return packet(env, binding_status(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_source_status(opened));

    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions{};
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientific{};
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return packet(env, scientific_status(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phase2Input{};
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = scientific.scientificMasterHash;
    phase2Input.zeroLineGauge = scientific.zeroLineGauge;
    phase2Input.sceneBinding = scientific.sceneBinding;
    phase2Input.roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phase2Input.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result finalized{};
    const auto finalizedStatus =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phase2Input, finalized);
    if (!finalizedStatus) return packet(env, phase2_status(finalizedStatus));

    PosixSequentialSink output(static_cast<int>(outputFd));
    truthraw::scientific_dng_export::v0_1::Options exportOptions{};
    exportOptions.role = role;
    exportOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    Result exported{};
    const auto exportedStatus = truthraw::scientific_dng_export::v0_1::export_scientific_dng(
        *source, reconstruction, prepared, finalized, output, exportOptions, exported);
    if (!exportedStatus) return packet(env, export_status(exportedStatus));

    const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
        *bytes, sourceSeal);
    if (!postVerified) return packet(env, binding_status(postVerified));

    const auto& audit = source->audit();
    if (audit.fullRawMaterialized || audit.fullFileMaterialized ||
        !exported.scientificMasterIdentityMatched || !exported.finalizedLineageValidated ||
        exported.fullScientificMasterMaterialized ||
        exported.physicalFrameCount != 1u || exported.independentEvidenceCount != 1u) {
        return packet(env, -2);
    }

    std::array<jint, kHeaderInts> values{};
    values[2] = roleCode;
    values[3] = static_cast<jint>(exported.width);
    values[4] = static_cast<jint>(exported.height);
    values[5] = clamp_metric(exported.tileCount);
    values[6] = clamp_metric(exported.bytesWritten);
    values[7] = clamp_metric(exported.logicalWorkspacePeakBytes);
    values[8] = exported.scientificMasterIdentityMatched ? 1 : 0;
    values[9] = exported.finalizedLineageValidated ? 1 : 0;
    values[10] = exported.fullScientificMasterMaterialized ? 1 : 0;
    values[11] = exported.sourceMetadataBoundColor ? 1 : 0;
    values[12] = exported.independentPhysicalColor ? 1 : 0;
    values[13] = static_cast<jint>(exported.physicalFrameCount);
    values[14] = static_cast<jint>(exported.independentEvidenceCount);
    values[15] = clamp_metric(audit.tileReadCalls);
    values[16] = clamp_metric(audit.rawPayloadBytesRead);
    values[17] = clamp_metric(audit.metadataBytesRead + produced.audit.metadataBytesRead);
    return packet(env, 0, &values);
}

#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "dng_compatibility_projection_v0_2.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::dng_compatibility_projection::v0_2::ISequentialByteSink;
using truthraw::dng_compatibility_projection::v0_2::ProjectionMetadata;
using truthraw::dng_compatibility_projection::v0_2::ProjectionRole;
using truthraw::dng_compatibility_projection::v0_2::Result;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jlong kDngExportMagic = 0x54524447LL; // TRDG
constexpr std::size_t kPacketLongs = 16u;

class PosixFdSequentialSink final : public ISequentialByteSink {
public:
    explicit PosixFdSequentialSink(int fd) : fd_(fd) {}

    bool write(const void* data, std::size_t count) override {
        if (fd_ < 0 || (data == nullptr && count != 0u)) return false;
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        std::size_t written = 0u;
        while (written < count) {
            const ssize_t rc = ::write(fd_, bytes + written, count - written);
            if (rc < 0) {
                if (errno == EINTR) continue;
                return false;
            }
            if (rc == 0) return false;
            written += static_cast<std::size_t>(rc);
        }
        return true;
    }

    std::size_t residentBytesUpperBound() const override { return sizeof(*this); }

private:
    int fd_ = -1;
};

jlongArray packet(JNIEnv* env,
                  jlong status,
                  jlong role = 0,
                  const Result* result = nullptr,
                  const truthraw::tile_dng_v0_1::SourceAudit* audit = nullptr,
                  std::size_t gaugePasses = 0u) {
    std::array<jlong, kPacketLongs> out{};
    out[0] = kDngExportMagic;
    out[1] = status;
    out[2] = role;
    if (result != nullptr) {
        const auto& writer = result->writer;
        out[3] = static_cast<jlong>(writer.bytesWritten);
        out[4] = static_cast<jlong>(writer.tilesWritten);
        out[5] = writer.scientificMasterMatched ? 1 : 0;
        out[6] = writer.fullFrameMaterialized ? 1 : 0;
        out[7] = result->projectionIsEvidence ? 1 : 0;
        out[8] = result->colorAuthorityPromoted ? 1 : 0;
        out[9] = static_cast<jlong>(writer.physicalFrameCount);
        out[10] = static_cast<jlong>(writer.independentEvidenceCount);
        out[11] = static_cast<jlong>(std::min<std::size_t>(
            writer.logicalResidentUpperBound,
            static_cast<std::size_t>(std::numeric_limits<jlong>::max())));
        out[15] = static_cast<jlong>(writer.colorAuthority);
    }
    if (audit != nullptr) {
        out[12] = static_cast<jlong>(std::min<std::uint64_t>(
            audit->tileReadCalls,
            static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
        out[13] = static_cast<jlong>(std::min<std::uint64_t>(
            audit->rawPayloadBytesRead,
            static_cast<std::uint64_t>(std::numeric_limits<jlong>::max())));
    }
    out[14] = static_cast<jlong>(gaugePasses);

    jlongArray array = env->NewLongArray(static_cast<jsize>(out.size()));
    if (array != nullptr) {
        env->SetLongArrayRegion(array, 0, static_cast<jsize>(out.size()), out.data());
    }
    return array;
}

jlong binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000LL + static_cast<jlong>(status.code);
}

jlong producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100LL + static_cast<jlong>(status.code);
}

jlong dng_source_status(const truthraw::tile_dng_v0_1::DngSourceStatus& status) {
    return 3000LL + static_cast<jlong>(status.code);
}

jlong scientific_status(
    const truthraw::scientific_master_streaming_binding::v0_2::Status& status) {
    return 6000LL + static_cast<jlong>(status.code);
}

jlong phase2_status(const truthraw::technical_backplane_phase2::v0_1::Status& status) {
    return 6100LL + static_cast<jlong>(status.code);
}

jlong projection_status(const truthraw::dng_compatibility_projection::v0_2::Status& status) {
    return 7000LL + static_cast<jlong>(status.code);
}

bool admission_matches_prepared(
    const PreparedScientificPreviewSource& prepared,
    const truthraw::technical_backplane_phase2::v0_1::Phase2Result& phase2) {
    const auto& admission = phase2.admission;
    return admission.claimScope != ColorClaimScope::None &&
        admission.sourceSeal.sha256 == prepared.source.sha256 &&
        admission.sourceSeal.byteLength == prepared.source.byteLength &&
        admission.sourceSeal.sourceEvidenceId == prepared.source.sourceEvidenceId &&
        admission.tileNativeOptions.sourceEvidenceId == prepared.source.sourceEvidenceId &&
        admission.tileNativeOptions.color.valid &&
        admission.tileNativeOptions.color.bindingId == prepared.color.bindingId &&
        admission.tileNativeOptions.color.cameraToXyzD50 == prepared.color.cameraToXyzD50;
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_NativeDngProjectionBridge_exportFinalizedProjection(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint destinationFd,
    jint projectionRole,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 || destinationFd < 0 ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0 ||
        (projectionRole != 0 && projectionRole != 1)) {
        return packet(env, -1, projectionRole);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));

    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(
        *bytes, sourceSeal);
    if (!sealed) return packet(env, binding_status(sealed), projectionRole);

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producer_status(colorStatus), projectionRole);

    // v0.2 compatibility projection requires an explicit resolved source white
    // as provenance. Dual-illuminant v0.2 supplies it. Delegated single-
    // illuminant export remains fail-closed until that exact white is surfaced.
    if (!(produced.audit.resolvedWhiteX > 0.0) ||
        !(produced.audit.resolvedWhiteY > 0.0) ||
        produced.audit.resolvedWhiteX + produced.audit.resolvedWhiteY >= 1.0) {
        return packet(env, -2, projectionRole);
    }

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, binding_status(preparedStatus), projectionRole);

    if (!prepared.mainHouseComputeAllowed ||
        !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed ||
        prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return packet(env, -3, projectionRole);
    }

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);
    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, openOptions, source);
    if (!opened) return packet(env, dng_source_status(opened), projectionRole);

    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes = static_cast<std::size_t>(maxLogicalResidentBytes);
    truthraw::scientific_master_streaming_binding::v0_2::Result scientificIdentity;
    const auto scientific =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientificIdentity);
    if (!scientific) return packet(env, scientific_status(scientific), projectionRole);

    if (scientificIdentity.physicalFrameCount != 1u ||
        scientificIdentity.independentEvidenceCount != 1u ||
        scientificIdentity.stage2GaugeScanPasses != 2u) {
        return packet(env, -4, projectionRole, nullptr, &source->audit(),
                      scientificIdentity.stage2GaugeScanPasses);
    }

    std::array<truthraw::technical_backplane::v0_1::RoomStatus,
               truthraw::technical_backplane::v0_1::kRoomCount> roomStatus{};
    roomStatus.fill(truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phase2Input{};
    phase2Input.prepared = prepared;
    phase2Input.scientificMasterHash = scientificIdentity.scientificMasterHash;
    phase2Input.zeroLineGauge = scientificIdentity.zeroLineGauge;
    phase2Input.sceneBinding = scientificIdentity.sceneBinding;
    phase2Input.roomStatus = roomStatus;
    phase2Input.claimStatus = truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto phase2Status =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(phase2Input, phase2);
    if (!phase2Status) {
        return packet(env, phase2_status(phase2Status), projectionRole,
                      nullptr, &source->audit(), scientificIdentity.stage2GaugeScanPasses);
    }
    if (!admission_matches_prepared(prepared, phase2)) {
        return packet(env, -5, projectionRole, nullptr, &source->audit(),
                      scientificIdentity.stage2GaugeScanPasses);
    }

    ProjectionMetadata projectionMetadata{};
    projectionMetadata.sourceSeal = sourceSeal;
    projectionMetadata.color = produced.color;
    projectionMetadata.expectedScientificMasterHash = scientificIdentity.scientificMasterHash;
    projectionMetadata.sourceResolvedWhiteX = produced.audit.resolvedWhiteX;
    projectionMetadata.sourceResolvedWhiteY = produced.audit.resolvedWhiteY;
    projectionMetadata.sourceResolvedWhiteTemperatureK =
        produced.audit.resolvedWhiteTemperatureK;

    PosixFdSequentialSink sink(static_cast<int>(destinationFd));
    Result result{};
    truthraw::dng_compatibility_projection::v0_2::Status projected;
    if (projectionRole == 0) {
        projected =
            truthraw::dng_compatibility_projection::v0_2::write_linear_scientific_master_dng(
                *source, *reconstruction, projectionMetadata, sink, result);
    } else {
        projected =
            truthraw::dng_compatibility_projection::v0_2::write_reconstructed_cfa_dng(
                *source, *reconstruction, projectionMetadata, sink, result);
    }
    if (!projected) {
        return packet(env, projection_status(projected), projectionRole,
                      &result, &source->audit(), scientificIdentity.stage2GaugeScanPasses);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!postVerified) {
        return packet(env, binding_status(postVerified), projectionRole,
                      &result, &source->audit(), scientificIdentity.stage2GaugeScanPasses);
    }

    const auto& writer = result.writer;
    if (!writer.scientificMasterMatched || !result.fixedD50CompatibilityWhite ||
        writer.fullFrameMaterialized || result.projectionIsEvidence ||
        result.colorAuthorityPromoted || writer.physicalFrameCount != 1u ||
        writer.independentEvidenceCount != 1u ||
        writer.logicalResidentUpperBound == 0u ||
        writer.logicalResidentUpperBound > static_cast<std::size_t>(maxLogicalResidentBytes) ||
        source->audit().fullRawMaterialized || source->audit().fullFileMaterialized) {
        return packet(env, -6, projectionRole,
                      &result, &source->audit(), scientificIdentity.stage2GaugeScanPasses);
    }

    return packet(env, 0, projectionRole,
                  &result, &source->audit(), scientificIdentity.stage2GaugeScanPasses);
}

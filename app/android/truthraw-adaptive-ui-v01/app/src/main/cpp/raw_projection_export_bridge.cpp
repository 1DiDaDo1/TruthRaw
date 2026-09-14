#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "raw_projection_export_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw_certificate_v0_1.h"
#include "truthraw_dng_certificate_embed_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <unistd.h>
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
namespace float_dng = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace certificate = truthraw::certificate::v0_1;
namespace certificate_embed = truthraw::dng_certificate_embed::v0_1;

constexpr jint kExportMagic = 0x54525831; // TRX1
constexpr std::size_t kHeaderInts = 19u;
constexpr jint kFloat32ScientificDngKind = 4;

// Public pipeline identity, not a secret signing key. SHA-256 of:
// "TruthRaw Android TRUTHRAW PURE float32 certificate pipeline v0.1".
constexpr certificate::Hash256 kPurePipelineIdentity = {
    0xe8u,0xedu,0x38u,0xccu,0x9bu,0x92u,0x37u,0x87u,
    0x60u,0xf6u,0x0au,0xe1u,0x7du,0x90u,0xdcu,0x6bu,
    0x31u,0x49u,0xf7u,0xe4u,0x40u,0x5cu,0xc9u,0x7eu,
    0xb7u,0xa9u,0xf3u,0x61u,0x66u,0xccu,0x8eu,0x55u,
};

class FdTransactionalByteSink final : public float_dng::ITransactionalByteSink {
public:
    explicit FdTransactionalByteSink(int fd) noexcept : fd_(fd) {}

    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    bool begin(std::uint64_t expectedBytes) noexcept override {
        if (fd_ < 0 || expectedBytes == 0u) return false;
        expectedBytes_ = expectedBytes;
        writtenBytes_ = 0u;
        active_ = false;
        if (::ftruncate(fd_, 0) != 0 || ::lseek(fd_, 0, SEEK_SET) < 0) return false;
        active_ = true;
        return true;
    }

    bool write(const std::uint8_t* data, std::size_t size) noexcept override {
        if (!active_ || data == nullptr || size == 0u) return false;
        if (writtenBytes_ > expectedBytes_ ||
            static_cast<std::uint64_t>(size) > expectedBytes_ - writtenBytes_) {
            return false;
        }
        std::size_t offset = 0u;
        while (offset < size) {
            const ssize_t n = ::write(fd_, data + offset, size - offset);
            if (n <= 0) return false;
            offset += static_cast<std::size_t>(n);
        }
        writtenBytes_ += static_cast<std::uint64_t>(size);
        return true;
    }

    bool commit() noexcept override {
        if (!active_ || writtenBytes_ != expectedBytes_) return false;
        if (::fsync(fd_) != 0) return false;
        active_ = false;
        return true;
    }

    void abort() noexcept override {
        if (fd_ >= 0) {
            (void)::ftruncate(fd_, 0);
            (void)::lseek(fd_, 0, SEEK_SET);
        }
        writtenBytes_ = 0u;
        active_ = false;
    }

private:
    int fd_ = -1;
    std::uint64_t expectedBytes_ = 0u;
    std::uint64_t writtenBytes_ = 0u;
    bool active_ = false;
};

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

jint float_dng_status(const float_dng::Status& status) {
    return 9000 + static_cast<jint>(status.code);
}

jint certificate_status(certificate::Status status) {
    return 12000 + static_cast<jint>(status);
}

jint certificate_embed_status(const certificate_embed::Status& status) {
    return 13000 + static_cast<jint>(status.code);
}

bool decode_kind(jint value, ProjectionKind& out) noexcept {
    switch (value) {
        case 1: out = ProjectionKind::RawSensorCfa16; return true;
        case 2: out = ProjectionKind::ReconstructedCfaDng16; return true;
        case 3: out = ProjectionKind::LinearDng16; return true;
        default: return false;
    }
}

certificate::State make_unsigned_pure_certificate(
    const SourceSeal& sourceSeal,
    const truthraw::scientific_master_streaming_binding::v0_2::Result& scientific,
    const truthraw::technical_backplane_phase2::v0_1::Phase2Result& phase2) noexcept {
    certificate::State state{};
    state.projectionClass = certificate::ProjectionClass::TruthRawPureFloat32Dng;
    state.claimClass = certificate::ClaimClass::Reconstructed;
    state.signatureState = certificate::SignatureState::UnsignedDevelopment;
    state.signatureAlgorithm = certificate::SignatureAlgorithm::None;
    state.colorClaimScope = static_cast<std::uint8_t>(phase2.admission.claimScope);
    state.sourceEvidenceSha256 = sourceSeal.sha256;
    state.scientificMasterSha256 = scientific.scientificMasterHash;
    state.zeroLineSha256 = phase2.backplane.zeroLineHash;
    state.sceneScaleSha256 = phase2.backplane.sceneScaleHash;
    state.technicalBackplaneCrc32 = truthraw::technical_backplane::v0_1::crc32(
        std::span<const std::uint8_t>(
            phase2.serializedBackplane.data(), phase2.serializedBackplane.size()));
    state.physicalFrameCount = scientific.physicalFrameCount;
    state.independentEvidenceCount = scientific.independentEvidenceCount;
    state.buildIdentitySha256 = kPurePipelineIdentity;
    return state;
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
    const bool float32ScientificDng = projectionKind == kFloat32ScientificDngKind;
    ProjectionKind kind{};
    if (sourceFd < 0 || outputFd < 0 || maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0 ||
        (!float32ScientificDng && !decode_kind(projectionKind, kind))) {
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

    if (float32ScientificDng) {
        float_dng::StreamingScientificMasterTileSource masterSource(*source, *reconstruction);
        float_dng::ProjectionDescriptor descriptor{};
        descriptor.width = static_cast<std::uint32_t>(source->metadata().width);
        descriptor.height = static_cast<std::uint32_t>(source->metadata().height);
        descriptor.orientation = static_cast<std::uint16_t>(source->metadata().orientation);
        descriptor.sealedSourceSha256 = sourceSeal.sha256;
        descriptor.scientificMasterSha256 = scientific.scientificMasterHash;
        descriptor.sourceEvidenceId = sourceSeal.sourceEvidenceId;
        descriptor.colorBindingId = produced.color.bindingId;

        FdTransactionalByteSink sink(static_cast<int>(outputFd));
        float_dng::Result exported{};
        const auto exportedStatus = float_dng::write_xyz_d50_linear_dng_projection(
            masterSource, descriptor, produced.color.cameraToXyzD50, sink, exported);
        if (!exportedStatus) return packet(env, float_dng_status(exportedStatus));

        if (!exported.representationOnly || exported.scientificMasterModified ||
            exported.appearanceApplied || exported.counterfactualObservationCreated ||
            !exported.scientificMasterIdentityVerified || !exported.artifactCommitted ||
            exported.physicalFrameCount != 1u || exported.independentEvidenceCount != 1u) {
            sink.abort();
            return packet(env, -4);
        }

        const auto certificateState = make_unsigned_pure_certificate(sourceSeal, scientific, phase2);
        certificate::SerializedCertificate serializedCertificate{};
        const auto certificateStatus = certificate::serialize(certificateState, serializedCertificate);
        if (certificateStatus != certificate::Status::Ok) {
            sink.abort();
            return packet(env, certificate_status(certificateStatus));
        }
        if (certificate::verified_badge_allowed(certificateState, false)) {
            sink.abort();
            return packet(env, -5);
        }

        certificate_embed::Result embedded{};
        const auto embeddedStatus = certificate_embed::embed_certificate(
            static_cast<int>(outputFd),
            std::span<const std::uint8_t>(serializedCertificate.data(), serializedCertificate.size()),
            embedded);
        if (!embeddedStatus || !embedded.existingPrivateDataPreserved ||
            !embedded.ifdCommitApplied ||
            embedded.certificateBytes != serializedCertificate.size()) {
            sink.abort();
            if (!embeddedStatus) return packet(env, certificate_embed_status(embeddedStatus));
            return packet(env, -6);
        }

        const auto postVerified = truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
        if (!postVerified) {
            sink.abort();
            return packet(env, binding_status(postVerified));
        }

        std::vector<jint> out(kHeaderInts, 0);
        out[0] = kExportMagic;
        out[1] = 0;
        out[2] = kFloat32ScientificDngKind;
        out[3] = static_cast<jint>(descriptor.width);
        out[4] = static_cast<jint>(descriptor.height);
        out[5] = 3;
        out[6] = clamp_metric(exported.tilesWritten);
        out[7] = clamp_metric(embedded.outputBytes);
        out[8] = clamp_metric(exported.projectedPixels);
        out[9] = clamp_metric(exported.negativeComponentCount);
        out[10] = clamp_metric(exported.overOneComponentCount);
        out[11] = clamp_metric(exported.logicalResidentUpperBound);
        out[12] = 0;
        out[13] = 0;
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

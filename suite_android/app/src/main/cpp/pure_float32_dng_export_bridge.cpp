#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthraw/core.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

using truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace float_dng = truthraw::scientific_master_linear_dng_projection::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;

constexpr jlong kMagic = 0x54525046; // TRPF = TruthRaw PURE Float
constexpr std::size_t kPacketLongs = 18u;
constexpr const char* kPurePrecisionPolicyId =
    "EXACT_SOURCE__F64_BRANCH_SENSITIVE_REFERENCE_POLICY__"
    "F64_CAL_OPT_COV_REFERENCE_POLICY__CONTROLLED_F32_MASTER_STORAGE__"
    "F32_PURE_PROJECTION";

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
        active_ = false;
        writtenBytes_ = 0u;
    }

private:
    int fd_ = -1;
    std::uint64_t expectedBytes_ = 0u;
    std::uint64_t writtenBytes_ = 0u;
    bool active_ = false;
};

jlongArray packet(JNIEnv* env, jlong status) {
    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = status;
    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jlong bindingStatus(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jlong>(status.code);
}

jlong producerStatus(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jlong>(status.code);
}

jlong adapterStatus(const adapter::AdapterStatus& status) {
    return 7000 + static_cast<jlong>(status.code);
}

jlong scienceStatus(const truthraw::scientific_master_streaming_binding::v0_2::Status& status) {
    return 8000 + static_cast<jlong>(status.code);
}

jlong phase2Status(const truthraw::technical_backplane_phase2::v0_1::Status& status) {
    return 9000 + static_cast<jlong>(status.code);
}

jlong floatStatus(const float_dng::Status& status) {
    return 10000 + static_cast<jlong>(status.code);
}

jlong clampToJlong(std::uint64_t value) noexcept {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jlong>::max());
    return static_cast<jlong>(std::min(value, cap));
}

int orientationQuarterTurns(truthraw::Orientation orientation) noexcept {
    switch (orientation) {
        case truthraw::Orientation::Normal: return 0;
        case truthraw::Orientation::Rotate90CW: return 1;
        case truthraw::Orientation::Rotate180: return 2;
        case truthraw::Orientation::Rotate90CCW: return 3;
    }
    return -1;
}

truthraw::Orientation orientationFromQuarterTurns(int turns) noexcept {
    switch (((turns % 4) + 4) % 4) {
        case 0: return truthraw::Orientation::Normal;
        case 1: return truthraw::Orientation::Rotate90CW;
        case 2: return truthraw::Orientation::Rotate180;
        default: return truthraw::Orientation::Rotate90CCW;
    }
}

truthraw::Orientation composeOrientation(
    truthraw::Orientation sourceOrientation,
    int userQuarterTurns) noexcept {
    const int sourceTurns = orientationQuarterTurns(sourceOrientation);
    if (sourceTurns < 0) return sourceOrientation;
    return orientationFromQuarterTurns(sourceTurns + userQuarterTurns);
}

} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_PureFloat32DngNativeBridge_exportPureFloat32Dng(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint outputFd,
    jint userQuarterTurns,
    jint exportMode,
    jint advancedFlags,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    constexpr jint kPureMode = 0;
    constexpr jint kJpgLRawEditMode = 1;
    constexpr jint kAllowedAdvancedFlags = 0x0f;
    if (sourceFd < 0 || outputFd < 0 ||
        userQuarterTurns < 0 || userQuarterTurns > 3 ||
        (exportMode != kPureMode && exportMode != kJpgLRawEditMode) ||
        (advancedFlags & ~kAllowedAdvancedFlags) != 0 ||
        (exportMode == kPureMode && advancedFlags != 0) ||
        maxSourceResidentBytes <= 0 || maxLogicalResidentBytes <= 0) {
        return packet(env, -1);
    }

    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));

    SourceSeal sourceSeal;
    const auto sealed =
        truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, sourceSeal);
    if (!sealed) return packet(env, bindingStatus(sealed));

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
            *bytes, sourceSeal, produced);
    if (!colorStatus) return packet(env, producerStatus(colorStatus));

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::prepare_scientific_color_source(
            sourceSeal, produced.color, prepared);
    if (!preparedStatus) return packet(env, bindingStatus(preparedStatus));

    if (!prepared.mainHouseComputeAllowed ||
        !prepared.sourceBoundAppearanceReleaseAllowed ||
        prepared.scientificPreviewReleaseAllowed ||
        prepared.scientificClaimAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return packet(env, -2);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, sourceSeal);
    if (!preVerified) return packet(env, bindingStatus(preVerified));

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened = truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
        bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return packet(env, adapterStatus(opened));
    auto& source = openedSource.source;

    auto reconstruction =
        std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();

    truthraw::scientific_master_streaming_binding::v0_2::Options scientificOptions;
    scientificOptions.memoryBudgetBytes =
        static_cast<std::size_t>(maxLogicalResidentBytes);

    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::bind_scientific_master_streaming(
            *source, *reconstruction, scientificOptions, scientific);
    if (!scientificStatus) return packet(env, scienceStatus(scientificStatus));

    truthraw::technical_backplane_phase2::v0_1::Phase2Input phaseInput;
    phaseInput.prepared = prepared;
    phaseInput.scientificMasterHash = scientific.scientificMasterHash;
    phaseInput.zeroLineGauge = scientific.zeroLineGauge;
    phaseInput.sceneBinding = scientific.sceneBinding;
    phaseInput.roomStatus.fill(
        truthraw::technical_backplane::v0_1::RoomStatus::ResearchOnly);
    phaseInput.claimStatus =
        truthraw::technical_backplane::v0_1::ClaimStatus::Candidate;

    truthraw::technical_backplane_phase2::v0_1::Phase2Result phase2;
    const auto finalized =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(
            phaseInput, phase2);
    if (!finalized) return packet(env, phase2Status(finalized));

    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.admission.sourceSeal.sha256 != sourceSeal.sha256 ||
        phase2.admission.sourceSeal.byteLength != sourceSeal.byteLength ||
        phase2.backplane.scientificMasterHash != scientific.scientificMasterHash ||
        scientific.physicalFrameCount != 1u ||
        scientific.independentEvidenceCount != 1u) {
        return packet(env, -3);
    }

    const auto beforeProjection =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!beforeProjection) return packet(env, bindingStatus(beforeProjection));

    float_dng::StreamingScientificMasterTileSource masterSource(
        *source, *reconstruction);

    float_dng::ProjectionDescriptor descriptor{};
    descriptor.width = static_cast<std::uint32_t>(source->metadata().width);
    descriptor.height = static_cast<std::uint32_t>(source->metadata().height);
    descriptor.orientation = static_cast<std::uint16_t>(
        composeOrientation(source->metadata().orientation, userQuarterTurns));
    descriptor.sealedSourceSha256 = sourceSeal.sha256;
    descriptor.scientificMasterSha256 = scientific.scientificMasterHash;
    descriptor.zeroLineSha256 = phase2.zeroLineHash;
    descriptor.sceneScaleSha256 = phase2.sceneScaleHash;
    descriptor.zeroLineGauge = scientific.zeroLineGauge;
    descriptor.sceneBinding = scientific.sceneBinding;
    descriptor.serializedBackplane = phase2.serializedBackplane;
    descriptor.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    descriptor.colorBindingId = produced.color.bindingId;
    descriptor.precisionPolicyId = kPurePrecisionPolicyId;
    descriptor.runtimeReconstructionBackendId = reconstruction->name();

    if (exportMode == kJpgLRawEditMode) {
        descriptor.projectionRole =
            "TRUTHRAW_JPGL_RAW_EDIT_FLOAT32_XYZ_D50_LINEAR_DNG";
        descriptor.downstreamEditManifest =
            std::string("schema=TruthRawJpgLRawEditRecipe/0.3\n") +
            "primary_image_role=FLOAT32_XYZ_D50_LINEAR_EDIT_MASTER\n" +
            "source_scientific_master_unchanged=1\n" +
            "appearance_baked_into_primary=0\n" +
            "advanced_recipe_flags=" + std::to_string(advancedFlags) + "\n" +
            "flag_open_world_light=" + std::to_string((advancedFlags & 0x01) ? 1 : 0) + "\n" +
            "flag_natural_hdr=" + std::to_string((advancedFlags & 0x02) ? 1 : 0) + "\n" +
            "flag_adaptive_detail=" + std::to_string((advancedFlags & 0x04) ? 1 : 0) + "\n" +
            "flag_restoration=" + std::to_string((advancedFlags & 0x08) ? 1 : 0) + "\n" +
            "hdr_recipe_authority=APPEARANCE_ONLY_UNTIL_OUTPUT_CHANNEL_AUTHORITY\n" +
            "restoration_recipe_role=AESTHETIC_REINTEGRATION_ONLY\n" +
            "lightroom_editable_primary=1\n" +
            "scientific_writeback_allowed=0\n" +
            "creates_new_evidence=0";
    }

    FdTransactionalByteSink sink(static_cast<int>(outputFd));
    float_dng::Result exported{};
    const auto exportedStatus =
        float_dng::write_xyz_d50_linear_dng_projection(
            masterSource,
            descriptor,
            produced.color.cameraToXyzD50,
            sink,
            exported);
    if (!exportedStatus) return packet(env, floatStatus(exportedStatus));

    if (!exported.representationOnly ||
        exported.scientificMasterModified ||
        exported.appearanceApplied ||
        exported.counterfactualObservationCreated ||
        !exported.scientificMasterIdentityVerified ||
        !exported.artifactCommitted ||
        exported.physicalFrameCount != 1u ||
        exported.independentEvidenceCount != 1u) {
        sink.abort();
        return packet(env, -4);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!postVerified) {
        sink.abort();
        return packet(env, bindingStatus(postVerified));
    }

    std::array<jlong, kPacketLongs> values{};
    values[0] = kMagic;
    values[1] = 0;
    values[2] = descriptor.width;
    values[3] = descriptor.height;
    values[4] = 3;   // XYZ-D50 LinearRaw channels
    values[5] = 32;  // IEEE float32 bits/component
    values[6] = clampToJlong(exported.bytesWritten);
    values[7] = clampToJlong(exported.projectedPixels);
    values[8] = clampToJlong(exported.negativeComponentCount);
    values[9] = clampToJlong(exported.overOneComponentCount);
    values[10] = exported.tilesWritten;
    values[11] = static_cast<jlong>(exported.logicalResidentUpperBound);
    values[12] = exported.scientificMasterIdentityVerified ? 1 : 0;
    values[13] = exported.appearanceApplied ? 1 : 0;
    values[14] = exported.counterfactualObservationCreated ? 1 : 0;
    values[15] = exported.physicalFrameCount;
    values[16] = exported.independentEvidenceCount;
    values[17] = static_cast<jlong>(phase2.admission.claimScope);

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(
            out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

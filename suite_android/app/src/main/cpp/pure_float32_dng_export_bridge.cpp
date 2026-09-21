#include <jni.h>

#include "advanced_render_edit_tile_source_v0_1.h"
#include "dng_color_binding_producer_v0_2.h"
#include "open_scene_canonical_v0_70.h"
#include "open_scene_channel_authority_v0_78.h"
#include "bound_uncertainty_admission_v0_79.h"
#include "full_frame_streaming_v0_1_internal.h"
#include "output_channel_authority_v0_84.h"
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
#include <sys/stat.h>
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
namespace render_edit = truthraw::advanced_render_edit::v0_1;
namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace canonical_scene = truthraw::open_scene_canonical::v0_70;
namespace channel_authority = truthraw::open_scene_channel_authority::v0_78;
namespace uncertainty_admission = truthraw::bound_uncertainty_admission::v0_79;
namespace output_channel_authority = truthraw::output_channel_authority::v0_84;

constexpr jlong kMagic = 0x54525046; // TRPF = TruthRaw PURE Float
constexpr std::size_t kPacketLongs = 34u;
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

std::string hexDigest(const std::array<std::uint8_t,32>& digest) {
    static constexpr char kHex[]="0123456789abcdef";
    std::string out(64u,'0');
    for(std::size_t i=0;i<digest.size();++i){
        out[2u*i]=kHex[(digest[i]>>4u)&0x0fu];
        out[2u*i+1u]=kHex[digest[i]&0x0fu];
    }
    return out;
}

bool readPreviewJpeg(int fd, std::vector<std::uint8_t>& out) noexcept {
    out.clear();
    if (fd < 0) return true;
    struct stat st {};
    if (::fstat(fd, &st) != 0 || st.st_size <= 0 ||
        st.st_size > static_cast<off_t>(128u * 1024u * 1024u)) {
        return false;
    }
    out.resize(static_cast<std::size_t>(st.st_size));
    std::size_t done = 0u;
    while (done < out.size()) {
        const ssize_t n = ::pread(
            fd,
            out.data() + done,
            out.size() - done,
            static_cast<off_t>(done));
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return out.size() >= 4u &&
           out[0] == 0xffu && out[1] == 0xd8u &&
           out[out.size()-2u] == 0xffu && out[out.size()-1u] == 0xd9u;
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
    jint sourceRouteCode,
    jint advancedFlags,
    jint previewFd,
    jint previewWidth,
    jint previewHeight,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    constexpr jint kPureMode = 0;
    constexpr jint kJpgLRawEditMode = 1;
    constexpr jint kAdvancedRenderEditMode = 2;
    constexpr jint kAllowedAdvancedFlags = static_cast<jint>(render_edit::kAllowedFlags);
    if (sourceFd < 0 || outputFd < 0 ||
        userQuarterTurns < 0 || userQuarterTurns > 3 ||
        (exportMode != kPureMode &&
         exportMode != kJpgLRawEditMode &&
         exportMode != kAdvancedRenderEditMode) ||
        (sourceRouteCode != 0 && sourceRouteCode != 1) ||
        (advancedFlags & ~kAllowedAdvancedFlags) != 0 ||
        (exportMode == kPureMode && advancedFlags != 0) ||
        ((previewFd < 0) != (previewWidth == 0 && previewHeight == 0)) ||
        previewWidth < 0 || previewHeight < 0 ||
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

    canonical_scene::Binding openSceneBinding{};
    openSceneBinding.sourceEvidenceSha256=sourceSeal.sha256;
    openSceneBinding.scientificMasterSha256=scientific.scientificMasterHash;
    openSceneBinding.width=static_cast<std::uint32_t>(source->metadata().width);
    openSceneBinding.height=static_cast<std::uint32_t>(source->metadata().height);
    openSceneBinding.physicalFrameCount=scientific.physicalFrameCount;
    openSceneBinding.independentEvidenceCount=scientific.independentEvidenceCount;
    openSceneBinding.colourBindingId=produced.color.bindingId;

    canonical_scene::Summary openSceneSummary{};
    if(!canonical_scene::build_from_source(*source,openSceneBinding,openSceneSummary) ||
       openSceneSummary.counterfactualPixelCount!=0u ||
       openSceneSummary.scientificWritebackPixelCount!=0u ||
       openSceneSummary.createsNewEvidence ||
       openSceneSummary.chunkingChangesScientificIdentity) {
        return packet(env,-6);
    }

    uncertainty_admission::Candidate uncertaintyCandidate{};
    if(sourceRouteCode==1) {
        uncertaintyCandidate=
            uncertainty_admission::make_current_camera5_derived_blocked_candidate(
                sourceSeal.sha256,
                static_cast<std::uint32_t>(source->metadata().width),
                static_cast<std::uint32_t>(source->metadata().height),
                static_cast<std::uint32_t>(source->metadata().cfa),
                source->metadata().whiteLevel,
                reconstruction->name());
    } else {
        uncertaintyCandidate.sourceDomain=uncertainty_admission::SourceDomain::Unattested;
        uncertaintyCandidate.sourceEvidenceSha256=sourceSeal.sha256;
        uncertaintyCandidate.width=static_cast<std::uint32_t>(source->metadata().width);
        uncertaintyCandidate.height=static_cast<std::uint32_t>(source->metadata().height);
        uncertaintyCandidate.cfaCode=static_cast<std::uint32_t>(source->metadata().cfa);
        uncertaintyCandidate.whiteLevel=source->metadata().whiteLevel;
        uncertaintyCandidate.reconstructionBackendId=reconstruction->name();
    }
    const auto uncertaintyDecision=uncertainty_admission::evaluate(uncertaintyCandidate);
    if(uncertaintyDecision.reconstructedAuthorityAllowed ||
       uncertaintyDecision.code==uncertainty_admission::DecisionCode::Admitted) {
        return packet(env,-7);
    }

    channel_authority::Binding channelBinding{};
    channelBinding.sourceEvidenceSha256=sourceSeal.sha256;
    channelBinding.scientificMasterSha256=scientific.scientificMasterHash;
    channelBinding.zeroLineSha256=phase2.zeroLineHash;
    channelBinding.sceneScaleSha256=phase2.sceneScaleHash;
    channelBinding.parentOpenSceneV070Sha256=openSceneSummary.artifactSha256;
    channelBinding.width=static_cast<std::uint32_t>(source->metadata().width);
    channelBinding.height=static_cast<std::uint32_t>(source->metadata().height);
    channelBinding.physicalFrameCount=scientific.physicalFrameCount;
    channelBinding.independentEvidenceCount=scientific.independentEvidenceCount;
    channelBinding.reconstructionBackendId=reconstruction->name();
    channelBinding.reconstructedAuthorityAllowed=false;

    channel_authority::Summary channelSummary{};
    if(!channel_authority::build_generic_fail_closed_from_source(
            *source,channelBinding,channelSummary) ||
       channelSummary.authorityCounts[1]!=0u ||
       channelSummary.p95KnownCount!=0u ||
       channelSummary.createsNewEvidence ||
       channelSummary.scientificWritebackAllowed) {
        return packet(env,-8);
    }

    output_channel_authority::Binding outputAuthorityBinding{};
    outputAuthorityBinding.sourceEvidenceSha256=sourceSeal.sha256;
    outputAuthorityBinding.scientificMasterSha256=scientific.scientificMasterHash;
    outputAuthorityBinding.canonicalOpenSceneSha256=openSceneSummary.artifactSha256;
    outputAuthorityBinding.sourceChannelAuthoritySha256=channelSummary.artifactSha256;
    outputAuthorityBinding.uncertaintyDecisionSha256=uncertaintyDecision.decisionSha256;
    outputAuthorityBinding.sourceWidth=static_cast<std::uint32_t>(source->metadata().width);
    outputAuthorityBinding.sourceHeight=static_cast<std::uint32_t>(source->metadata().height);
    outputAuthorityBinding.outputWidth=outputAuthorityBinding.sourceWidth;
    outputAuthorityBinding.outputHeight=outputAuthorityBinding.sourceHeight;
    outputAuthorityBinding.reconstructionSupportRadius=
        static_cast<std::uint32_t>(std::max(0,reconstruction->requiredHalo()));
    outputAuthorityBinding.reconstructedUncertaintyAdmitted=false;
    outputAuthorityBinding.physicalFrameCount=scientific.physicalFrameCount;
    outputAuthorityBinding.independentEvidenceCount=scientific.independentEvidenceCount;
    outputAuthorityBinding.reconstructionBackendId=reconstruction->name();

    output_channel_authority::Summary outputAuthoritySummary{};
    if(!output_channel_authority::build_conservative(
            *source,outputAuthorityBinding,outputAuthoritySummary) ||
       !outputAuthoritySummary.perOutputChannelAuthorityAvailable ||
       outputAuthoritySummary.authorityCounts[1]!=0u ||
       outputAuthoritySummary.authorityCounts[3]==0u ||
       outputAuthoritySummary.recordCount!=
           static_cast<std::uint64_t>(source->metadata().width)*
           static_cast<std::uint64_t>(source->metadata().height)*3u ||
       outputAuthoritySummary.createsNewEvidence ||
       outputAuthoritySummary.scientificWritebackAllowed) {
        return packet(env,-9);
    }

    const auto beforeProjection =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!beforeProjection) return packet(env, bindingStatus(beforeProjection));

    truthraw::ExposurePlan renderEditExposure{};
    if (exportMode == kAdvancedRenderEditMode) {
        const auto exposureTiles = truthraw::make_tiles(
            source->metadata().width,
            source->metadata().height,
            truthraw::TilePolicy{128, 16});
        truthraw::streaming_v0_1::detail::Workspace exposureWorkspace{};
        truthraw::streaming_v0_1::detail::Pass1Stats pass1{};
        const auto exposureStatus =
            truthraw::streaming_v0_1::detail::run_pass1(
                *source,
                exposureTiles,
                *reconstruction,
                exposureWorkspace,
                pass1);
        if (!exposureStatus) {
            return packet(env, -10);
        }
        const std::size_t pixelCount =
            static_cast<std::size_t>(source->metadata().width) *
            static_cast<std::size_t>(source->metadata().height);
        const float clipFraction =
            pixelCount > 0u
                ? static_cast<float>(pass1.totalClipped) /
                    static_cast<float>(pixelCount)
                : 0.0f;
        renderEditExposure = truthraw::choose_exposure_plan_from_histograms(
            pass1.display.bins,
            pass1.scene.bins,
            4.0f,
            truthraw::streaming_v0_1::detail::noise_sigma_2pct(
                source->metadata()),
            clipFraction,
            pass1.totalOver1);
    }

    float_dng::StreamingScientificMasterTileSource masterSource(
        *source, *reconstruction);

    std::vector<std::uint8_t> previewJpeg;
    if (!readPreviewJpeg(previewFd, previewJpeg)) {
        return packet(env, -5);
    }

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
    descriptor.jpegPreviewBytes = previewJpeg;
    descriptor.jpegPreviewWidth = static_cast<std::uint32_t>(previewWidth);
    descriptor.jpegPreviewHeight = static_cast<std::uint32_t>(previewHeight);
    descriptor.outputAuthorityManifest =
        std::string("schema=TruthRawOutputChannelAuthority/0.84\n") +
        "artifact_sha256=" + hexDigest(outputAuthoritySummary.artifactSha256) + "\n" +
        "mapping_mode=" +
            output_channel_authority::mapping_mode_name(outputAuthoritySummary.mappingMode) + "\n" +
        "calibrated_estimate_channels=" +
            std::to_string(outputAuthoritySummary.authorityCounts[0]) + "\n" +
        "reconstructed_channels=" +
            std::to_string(outputAuthoritySummary.authorityCounts[1]) + "\n" +
        "censored_channels=" +
            std::to_string(outputAuthoritySummary.authorityCounts[2]) + "\n" +
        "unknown_channels=" +
            std::to_string(outputAuthoritySummary.authorityCounts[3]) + "\n" +
        "censored_support_pixels=" +
            std::to_string(outputAuthoritySummary.censoredSupportPixels) + "\n" +
        "uncertainty_decision_code=" +
            std::to_string(static_cast<std::uint32_t>(uncertaintyDecision.code)) + "\n" +
        "orientation_transform_changes_authority=0\n" +
        "scientific_writeback_allowed=0\n" +
        "creates_new_evidence=0";

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
            "hdr_recipe_authority=APPEARANCE_ONLY_OUTPUT_CHANNEL_MAP_HAS_UNKNOWN\n" +
            "restoration_recipe_role=AESTHETIC_REINTEGRATION_ONLY\n" +
            "lightroom_editable_primary=1\n" +
            "scientific_writeback_allowed=0\n" +
            "creates_new_evidence=0";
    }

    std::unique_ptr<render_edit::ExtendedLinearSrgbTileSource>
        renderEditSource;
    if (exportMode == kAdvancedRenderEditMode) {
        renderEditSource =
            std::make_unique<render_edit::ExtendedLinearSrgbTileSource>(
                *source,
                *reconstruction,
                produced.color.cameraToXyzD50,
                static_cast<std::uint32_t>(advancedFlags),
                renderEditExposure);

        float_dng::Hash256 projectedHash{};
        const auto projectedStatus =
            render_edit::compute_projected_raster_sha256(
                *renderEditSource,
                descriptor.width,
                descriptor.height,
                projectedHash);
        if (!projectedStatus) {
            return packet(env, floatStatus(projectedStatus));
        }

        descriptor.projectedRasterSha256 = projectedHash;
        descriptor.openSceneStateSha256 = openSceneSummary.artifactSha256;
        descriptor.projectionRole =
            "TRUTHRAW_ADVANCED_RENDER_EDIT_FLOAT32_XYZ_D50_LINEAR_DNG_V0_1";
        descriptor.projectedAppearanceApplied =
            renderEditSource->appearanceBakedIntoPrimary();
        descriptor.projectedCounterfactualObservationCreated = false;
        descriptor.restorationDerivative = false;
        descriptor.downstreamEditManifest =
            std::string("schema=TruthRawAdvancedRenderEdit/0.1\n") +
            "derivative_identity_space=EXTENDED_LINEAR_SRGB_FLOAT32\n" +
            "stored_primary_space=XYZ_D50_LINEAR_FLOAT32\n" +
            "storage_transform=LINEAR_SRGB_TO_XYZ_D50\n" +
            "source_scientific_master_unchanged=1\n" +
            "negative_components_preserved=1\n" +
            "over_one_components_preserved=1\n" +
            "advanced_flags=" + std::to_string(advancedFlags) + "\n" +
            "detail_strength_percent=" +
                std::to_string(
                    render_edit::controls::detail_strength_percent(
                        static_cast<std::uint32_t>(advancedFlags))) + "\n" +
            "color_fullness=" +
                std::to_string(
                    render_edit::controls::color_fullness(
                        static_cast<std::uint32_t>(advancedFlags))) + "\n" +
            "color_fullness_role=APPEARANCE_ONLY_LUMINANCE_PRESERVING\n" +
            "detail_baked_into_primary=" +
                std::to_string(
                    (advancedFlags & static_cast<jint>(render_edit::kFlagDetail))
                        ? 1 : 0) + "\n" +
            "light_baked_into_primary=" +
                std::to_string(
                    (advancedFlags & static_cast<jint>(render_edit::kFlagLight))
                        ? 1 : 0) + "\n" +
            "restoration_baked_into_primary=" +
                std::to_string(
                    (advancedFlags & static_cast<jint>(render_edit::kFlagRestoration))
                        ? 1 : 0) + "\n" +
            "restoration_role=AESTHETIC_REINTEGRATION_ONLY\n" +
            "natural_hdr_baked_into_primary=0\n" +
            "natural_hdr_recipe_only=" +
                std::to_string(
                    (advancedFlags & static_cast<jint>(render_edit::kFlagHdr))
                        ? 1 : 0) + "\n" +
            "hdr_authority=APPEARANCE_ONLY_OUTPUT_CHANNEL_MAP_HAS_UNKNOWN\n" +
            "output_acutance_baked_into_primary=0\n" +
            "lightroom_editable_primary=1\n" +
            "scientific_writeback_allowed=0\n" +
            "creates_new_evidence=0";
    }

    FdTransactionalByteSink sink(static_cast<int>(outputFd));
    float_dng::Result exported{};
    const auto exportedStatus =
        exportMode == kAdvancedRenderEditMode
            ? float_dng::write_xyz_d50_linear_dng_projection(
                  *renderEditSource,
                  descriptor,
                  render_edit::linear_srgb_to_xyz_d50_matrix(),
                  sink,
                  exported)
            : float_dng::write_xyz_d50_linear_dng_projection(
                  masterSource,
                  descriptor,
                  produced.color.cameraToXyzD50,
                  sink,
                  exported);
    if (!exportedStatus) return packet(env, floatStatus(exportedStatus));

    const bool commonInvariant =
        exported.representationOnly &&
        !exported.scientificMasterModified &&
        !exported.counterfactualObservationCreated &&
        exported.artifactCommitted &&
        exported.physicalFrameCount == 1u &&
        exported.independentEvidenceCount == 1u;

    const bool flavorInvariant =
        exportMode == kAdvancedRenderEditMode
            ? (exported.projectedRasterIdentityVerified &&
               !exported.scientificMasterIdentityVerified &&
               exported.appearanceApplied ==
                   renderEditSource->appearanceBakedIntoPrimary())
            : (exported.projectedRasterIdentityVerified &&
               exported.scientificMasterIdentityVerified &&
               !exported.appearanceApplied);

    if (!commonInvariant || !flavorInvariant) {
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
    values[18] = outputAuthoritySummary.perOutputChannelAuthorityAvailable ? 1 : 0;
    values[19] = static_cast<jlong>(outputAuthoritySummary.mappingMode);
    values[20] = clampToJlong(outputAuthoritySummary.authorityCounts[0]);
    values[21] = clampToJlong(outputAuthoritySummary.authorityCounts[1]);
    values[22] = clampToJlong(outputAuthoritySummary.authorityCounts[2]);
    values[23] = clampToJlong(outputAuthoritySummary.authorityCounts[3]);
    values[24] = clampToJlong(outputAuthoritySummary.censoredSupportPixels);
    values[25] = clampToJlong(outputAuthoritySummary.outputPixelCount);
    for(std::size_t word=0;word<8u;++word){
        const std::size_t i=word*4u;
        const auto& d=outputAuthoritySummary.artifactSha256;
        const std::uint32_t value=
            static_cast<std::uint32_t>(d[i]) |
            (static_cast<std::uint32_t>(d[i+1u])<<8u) |
            (static_cast<std::uint32_t>(d[i+2u])<<16u) |
            (static_cast<std::uint32_t>(d[i+3u])<<24u);
        values[26u+word]=static_cast<jlong>(value);
    }

    auto out = env->NewLongArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetLongArrayRegion(
            out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

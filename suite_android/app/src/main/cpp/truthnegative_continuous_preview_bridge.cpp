#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "open_world_appearance_corridor_v03.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_master_f64_reconstruction_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "scientific_preview_source_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"
#include "truthnegative_continuous_v0_5.h"
#include "drawnegative_v0_1.h"
#include "truthnegative_deep_scene_bridge_v0_8.h"
#include "truthnegative_dense_local_field_adapter_v0_4.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthraw/core.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

using truthraw::scientific_master_f64_reconstruction_v0_1::
    ResearchEdgeAwareMeasuredPreservingReconstructionF64;
using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::ColorClaimScope;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::scientific_preview_binding_v0_2::PreparedScientificPreviewSource;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
namespace appearance = truthraw::free_world_appearance_resolve::v0_7;
namespace binding =
    truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace open_world_host = truthraw::open_world::v0_3;
namespace master_projection =
    truthraw::scientific_master_linear_dng_projection::v0_1;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace drawnegative = truthraw::drawnegative::v0_1;
namespace tn_deep = truthraw::truthnegative_deep_scene_bridge::v0_8;
namespace tn_field =
    truthraw::truthnegative_dense_local_field_adapter::v0_4;
namespace n2_cfa =
    truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace sha = truthraw::sha256_v0_69;

constexpr jint kMagic = 0x35434e54; // TNC5 in little-endian byte view.
constexpr std::size_t kHeaderInts = 192u;
constexpr jint kMaxEdgeHardLimit = 256;

jint clamp_metric(std::uint64_t value) noexcept {
    const auto cap =
        static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jint scaled_metric(double value, double scale) noexcept {
    if (!std::isfinite(value) || value <= 0.0 || !std::isfinite(scale) ||
        scale <= 0.0) {
        return 0;
    }
    const double cap =
        static_cast<double>(std::numeric_limits<jint>::max());
    return static_cast<jint>(
        std::llround(std::min(value * scale, cap)));
}

jintArray status_packet(JNIEnv* env, jint status) {
    std::array<jint, kHeaderInts> values{};
    values[0] = kMagic;
    values[1] = status;
    jintArray out =
        env->NewIntArray(static_cast<jsize>(values.size()));
    if (out != nullptr) {
        env->SetIntArrayRegion(
            out, 0, static_cast<jsize>(values.size()), values.data());
    }
    return out;
}

jint binding_status(
    const truthraw::scientific_preview_binding_v0_1::BindingStatus& s) {
    return 2000 + static_cast<jint>(s.code);
}

jint producer_status(
    const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& s) {
    return 2100 + static_cast<jint>(s.code);
}

jint adapter_status(const adapter::AdapterStatus& s) {
    return 7000 + static_cast<jint>(s.code);
}

jint science_status(
    const truthraw::scientific_master_streaming_binding::v0_2::Status& s) {
    return 8000 + static_cast<jint>(s.code);
}

jint phase2_status(
    const truthraw::technical_backplane_phase2::v0_1::Status& s) {
    return 9000 + static_cast<jint>(s.code);
}

sha::Digest labeled_digest(
    const char* label,
    const sha::Digest* parent = nullptr,
    const std::string* text = nullptr) noexcept {
    sha::Hasher h;
    const std::size_t n = std::char_traits<char>::length(label);
    h.update(
        reinterpret_cast<const std::uint8_t*>(label), n);
    if (parent != nullptr) h.update(*parent);
    if (text != nullptr) {
        h.update(
            reinterpret_cast<const std::uint8_t*>(text->data()),
            text->size());
    }
    return h.finalize();
}

void begin_stage_digest(
    sha::Hasher& hasher,
    const char* domain) noexcept {
    const std::size_t n = std::char_traits<char>::length(domain);
    hasher.update(
        reinterpret_cast<const std::uint8_t*>(domain), n);
}

sha::Digest n2_candidate_scene_digest(
    const sha::Digest& scientificScene,
    const sha::Digest& gridSha,
    std::size_t pixelIndex,
    const std::array<double,3u>& rgb) noexcept {
    sha::Hasher h;
    constexpr char domain[] =
        "D_RAW_TN_N2_APPEARANCE_CANDIDATE_SCENE_V0_1";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain)-1u);
    h.update(scientificScene);
    h.update(gridSha);
    const std::uint64_t index64 =
        static_cast<std::uint64_t>(pixelIndex);
    std::array<std::uint8_t,8u> indexBytes{};
    for(std::size_t i=0u;i<8u;++i){
        indexBytes[i]=static_cast<std::uint8_t>(index64>>(8u*i));
    }
    h.update(indexBytes);
    for(double v:rgb){
        const auto bits=std::bit_cast<std::uint64_t>(v);
        std::array<std::uint8_t,8u> b{};
        for(std::size_t i=0u;i<8u;++i){
            b[i]=static_cast<std::uint8_t>(bits>>(8u*i));
        }
        h.update(b);
    }
    return h.finalize();
}

appearance::Matrix3 matrix_from_f32(
    const std::array<float, 9u>& source) noexcept {
    appearance::Matrix3 out{};
    for (std::size_t i = 0u; i < source.size(); ++i) {
        out.m[i] = static_cast<double>(source[i]);
    }
    return out;
}

appearance::Matrix3 xyz_d50_to_linear_srgb() noexcept {
    appearance::Matrix3 out{};
    out.m = {
         3.1338561, -1.6168667, -0.4906146,
        -0.9787684,  1.9161415,  0.0334540,
         0.0719453, -0.2289914,  1.4052427,
    };
    return out;
}

std::uint32_t quantize_u8(double encoded) noexcept {
    if (!std::isfinite(encoded)) return 0u;
    const long q = std::lround(
        std::clamp(encoded, 0.0, 1.0) * 255.0);
    return static_cast<std::uint32_t>(
        std::clamp<long>(q, 0l, 255l));
}

void digest_to_words(
    const sha::Digest& digest,
    jint* out) noexcept {
    for (std::size_t word = 0u; word < 8u; ++word) {
        const std::size_t i = word * 4u;
        const std::uint32_t v =
            static_cast<std::uint32_t>(digest[i]) |
            (static_cast<std::uint32_t>(digest[i + 1u]) << 8u) |
            (static_cast<std::uint32_t>(digest[i + 2u]) << 16u) |
            (static_cast<std::uint32_t>(digest[i + 3u]) << 24u);
        out[word] = static_cast<jint>(v);
    }
}

bool target_geometry(
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    std::uint32_t maxEdge,
    std::uint32_t& targetWidth,
    std::uint32_t& targetHeight) noexcept {
    if (sourceWidth == 0u || sourceHeight == 0u || maxEdge == 0u) {
        return false;
    }
    if (sourceWidth >= sourceHeight) {
        targetWidth = maxEdge;
        targetHeight = std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(std::llround(
                static_cast<double>(maxEdge) *
                static_cast<double>(sourceHeight) /
                static_cast<double>(sourceWidth))));
    } else {
        targetHeight = maxEdge;
        targetWidth = std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(std::llround(
                static_cast<double>(maxEdge) *
                static_cast<double>(sourceWidth) /
                static_cast<double>(sourceHeight))));
    }
    return targetWidth > 0u && targetHeight > 0u;
}

}  // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_TruthNegativeContinuousNativeBridge_buildProContinuousPreview(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    if (sourceFd < 0 ||
        requestedMaxEdge < 32 ||
        requestedMaxEdge > kMaxEdgeHardLimit ||
        maxSourceResidentBytes <= 0 ||
        maxLogicalResidentBytes <= 0) {
        return status_packet(env, -1);
    }

    auto bytes =
        std::make_shared<PosixFdByteSource>(static_cast<int>(sourceFd));

    SourceSeal sourceSeal;
    const auto sealed =
        truthraw::scientific_preview_binding_v0_1::seal_source_sha256(
            *bytes, sourceSeal);
    if (!sealed) return status_packet(env, binding_status(sealed));

    ProducerResult produced;
    const auto colorStatus =
        truthraw::dng_color_binding_producer_v0_2::
            produce_source_metadata_color_binding(
                *bytes, sourceSeal, produced);
    if (!colorStatus) {
        return status_packet(env, producer_status(colorStatus));
    }

    PreparedScientificPreviewSource prepared;
    const auto preparedStatus =
        truthraw::scientific_preview_binding_v0_2::
            prepare_scientific_color_source(
                sourceSeal, produced.color, prepared);
    if (!preparedStatus) {
        return status_packet(env, binding_status(preparedStatus));
    }
    if (!prepared.mainHouseComputeAllowed ||
        prepared.physicalFrameCount != 1u ||
        prepared.independentEvidenceCount != 1u) {
        return status_packet(env, -2);
    }

    const auto preVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!preVerified) {
        return status_packet(env, binding_status(preVerified));
    }

    auto openOptions = prepared.tileNativeOptions;
    openOptions.maxResidentBytes =
        static_cast<std::size_t>(maxSourceResidentBytes);

    truthraw::android_raw_adapter_bridge::v0_1::OpenedDngSource openedSource;
    const auto opened =
        truthraw::android_raw_adapter_bridge::v0_1::openDngViaAdapter(
            bytes, sourceSeal, openOptions, openedSource);
    if (!opened) return status_packet(env, adapter_status(opened));
    auto& source = openedSource.source;

    auto reconstruction =
        std::make_shared<
            ResearchEdgeAwareMeasuredPreservingReconstructionF64>();

    truthraw::scientific_master_streaming_binding::v0_2::Options
        scientificOptions;
    scientificOptions.memoryBudgetBytes =
        static_cast<std::size_t>(maxLogicalResidentBytes);

    truthraw::scientific_master_streaming_binding::v0_2::Result scientific;
    const auto scientificStatus =
        truthraw::scientific_master_streaming_binding::v0_2::
            bind_scientific_master_streaming(
                *source,
                *reconstruction,
                scientificOptions,
                scientific);
    if (!scientificStatus) {
        return status_packet(env, science_status(scientificStatus));
    }

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
    const auto phaseStatus =
        truthraw::technical_backplane_phase2::v0_1::finalize_phase2(
            phaseInput, phase2);
    if (!phaseStatus) {
        return status_packet(env, phase2_status(phaseStatus));
    }
    if (phase2.admission.claimScope == ColorClaimScope::None ||
        phase2.backplane.sourceEvidenceHash != sourceSeal.sha256 ||
        phase2.backplane.scientificMasterHash !=
            scientific.scientificMasterHash ||
        phase2.backplane.forbiddenFlags != 0u ||
        phase2.backplane.physicalFrameCount != 1u ||
        phase2.backplane.independentEvidenceCount != 1u ||
        scientific.physicalFrameCount != 1u ||
        scientific.independentEvidenceCount != 1u) {
        return status_packet(env, -3);
    }

    const int sourceWidthInt = source->metadata().width;
    const int sourceHeightInt = source->metadata().height;
    if (sourceWidthInt <= 0 || sourceHeightInt <= 0) {
        return status_packet(env, -4);
    }
    const auto sourceWidth =
        static_cast<std::uint32_t>(sourceWidthInt);
    const auto sourceHeight =
        static_cast<std::uint32_t>(sourceHeightInt);

    master_projection::StreamingScientificMasterTileSource masterSource(
        *source, *reconstruction);
    tn_field::SourceFieldAdapter fieldSource(*source, masterSource);

    tn::AuthorityFieldSummary authorityField{};
    if (!tn::summarizeAuthorityField(fieldSource, authorityField) ||
        authorityField.createsNewEvidence ||
        authorityField.scientificWritebackAllowed) {
        return status_packet(env, -5);
    }

    tn::StateInput tnInput{};
    tnInput.sourceEvidenceSha256 = sourceSeal.sha256;
    tnInput.scientificMasterSha256 = scientific.scientificMasterHash;
    tnInput.authorityFieldSha256 = authorityField.contentSha256;
    tnInput.width = sourceWidth;
    tnInput.height = sourceHeight;
    tnInput.reconstructionBackendId = reconstruction->name();
    tnInput.colourBindingId = produced.color.bindingId;
    tnInput.physicalFrameCount = 1u;
    tnInput.independentEvidenceCount = 1u;

    tn::State tnState{};
    if (!tn::finalizeState(tnInput, tnState) ||
        !tnState.isRasterIndependent ||
        tnState.createsNewEvidence ||
        tnState.scientificWritebackAllowed) {
        return status_packet(env, -6);
    }

    const std::string sourceHex = sha::hex(sourceSeal.sha256);
    drawnegative::Input drawNegativeInput{};
    drawNegativeInput.truthNegativeState = tnState;
    drawNegativeInput.observationId =
        std::string("DRAW_OBS_") + sourceHex;
    drawNegativeInput.scaleGaugeId =
        std::string("DRAW_SOURCE_LOCAL_GAUGE_") + sourceHex;
    drawNegativeInput.gaugeRelation =
        drawnegative::GaugeRelation::SourceLocalOnly;
    drawNegativeInput.canonicalStorage =
        drawnegative::CanonicalStorage::Float32Validated;
    drawNegativeInput.truthRangeCoordinateFamilyDeclared = true;
    drawNegativeInput.perSampleTruthRangeMaterialized = false;

    drawnegative::State drawNegativeState{};
    if (!drawnegative::finalize(
            drawNegativeInput, drawNegativeState) ||
        !drawNegativeState.finalized ||
        !drawNegativeState.isRasterIndependent ||
        !drawNegativeState.isPerObservationLineage ||
        drawNegativeState.commonGaugeAdmitted ||
        drawNegativeState.crossObservationRadiometricEqualityAllowed ||
        drawNegativeState.crossObservationRadiometricFusionAllowed ||
        drawNegativeState.createsNewEvidence ||
        drawNegativeState.scientificWritebackAllowed ||
        drawNegativeState.parentTruthNegativeStateSha256 !=
            tnState.stateSha256) {
        return status_packet(env, -19);
    }

    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
    if (!target_geometry(
            sourceWidth,
            sourceHeight,
            static_cast<std::uint32_t>(requestedMaxEdge),
            targetWidth,
            targetHeight)) {
        return status_packet(env, -8);
    }

    // N2 remains a side-car validation path. It reads the same admitted RAW
    // source and computes candidate corrections on a balanced CFA sample set,
    // but its values are never supplied to Scientific Master, TruthNegative,
    // Deep Scene, Appearance or the visible preview below.
    n2_cfa::Binding n2Binding{};
    n2Binding.sourceEvidenceSha256 = sourceSeal.sha256;
    n2Binding.truthNegativeStateSha256 = tnState.stateSha256;
    n2_cfa::Options n2Options{};
    n2Options.tileEdge = 64u;
    n2Options.samplingPeriod = 8u;
    n2Options.appearanceGridWidth = targetWidth;
    n2Options.appearanceGridHeight = targetHeight;
    n2_cfa::Result n2Audit{};
    if (!n2_cfa::run(*source, n2Binding, n2Options, n2Audit) ||
        n2Audit.sourceValuesModified ||
        n2Audit.truthNegativeModified ||
        n2Audit.createsNewEvidence ||
        n2Audit.scientificWritebackAllowed ||
        !n2Audit.appearanceGridDerived ||
        n2Audit.appearanceGridWidth != targetWidth ||
        n2Audit.appearanceGridHeight != targetHeight ||
        n2Audit.appearanceGrid.size() !=
            static_cast<std::size_t>(targetWidth) * targetHeight) {
        return status_packet(env, -17);
    }

    binding::BoundScenePlane scene(
        masterSource, fieldSource, sourceWidth, sourceHeight);
    if (!scene.valid()) return status_packet(env, -7);

    tn::RasterResolver resolver(
        scene, tnState, targetWidth, targetHeight);
    if (!resolver.valid()) return status_packet(env, -9);

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz =
        matrix_from_f32(prepared.color.cameraToXyzD50);
    sceneColor.referenceWhiteXyz = {0.96422, 1.0, 0.82521};
    sceneColor.sceneReferenceWhiteNits = 100.0;
    sceneColor.identitySha256 = labeled_digest(
        "D_RAW_TN_CONT_V05_SCENE_COLORIMETRY",
        &tnState.stateSha256,
        &prepared.color.bindingId);

    appearance::ViewingConditions viewing{};
    viewing.adaptingWhiteXyz = {0.95047, 1.0, 1.08883};
    viewing.adaptingLuminanceNits = 20.0;
    viewing.backgroundLuminanceNits = 20.0;
    viewing.surround = appearance::Surround::Average;
    viewing.viewingDistanceMeters = 0.5;
    viewing.identitySha256 = labeled_digest(
        "D_RAW_TN_CONT_V05_PRO_VIEW_AVERAGE_20_NIT");

    appearance::DisplayTarget display{};
    display.xyzToRgb = xyz_d50_to_linear_srgb();
    display.whitePointXyz = {0.95047, 1.0, 1.08883};
    display.referenceWhiteNits = 100.0;
    display.peakLuminanceNits = 100.0;
    display.blackLuminanceNits = 0.0;
    display.transfer = appearance::TransferFunction::Srgb;
    display.identitySha256 = labeled_digest(
        "D_RAW_TN_CONT_V05_SRGB_100_NIT_DISPLAY");

    appearance::AppearancePolicy policy{};
    policy.exposureEv = 0.0;
    policy.colorfulnessScale = 1.0;
    policy.highlightCompression = 1.0;
    policy.identitySha256 = labeled_digest(
        "D_RAW_TN_CONT_V05_NEUTRAL_APPEARANCE_POLICY");

    // The scientific baseline now runs the existing extensible Appearance
    // corridor with a real authority-bound v0.6 LightTransportAttachment and
    // the existing Room Capsule stage. Until room geometry/illumination is
    // actually admitted for this observation, the sample is explicitly
    // outside-room, so the stage executes and takes an exact-preserving bypass.
    // The separate N2 appearance-only candidate remains an empty-chain branch.
    constexpr std::array<open_world_host::AppearanceStage, 0u>
        kCandidateAppearanceStages{};

    const std::uint64_t pixelCount64 =
        static_cast<std::uint64_t>(targetWidth) * targetHeight;
    if (pixelCount64 >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
        return status_packet(env, -10);
    }
    const std::size_t pixelCount =
        static_cast<std::size_t>(pixelCount64);

    std::vector<jint> packet(
        kHeaderInts + pixelCount + pixelCount, 0);
    packet[0] = kMagic;
    packet[1] = 0;
    packet[2] = static_cast<jint>(targetWidth);
    packet[3] = static_cast<jint>(targetHeight);
    packet[4] = sourceWidthInt;
    packet[5] = sourceHeightInt;
    packet[6] = static_cast<jint>(source->metadata().orientation);
    packet[7] = clamp_metric(authorityField.recordCount);
    for (std::size_t i = 0u; i < 4u; ++i) {
        packet[8u + i] =
            clamp_metric(authorityField.authorityCounts[i]);
    }
    packet[12] = clamp_metric(pixelCount64);
    packet[19] = 1;
    packet[20] = 1;
    packet[21] = 0;
    packet[22] = 0;
    packet[23] = 1;
    packet[24] = 1;
    packet[25] = 0;
    packet[28] = produced.audit.usedForwardMatrix ? 1 : 0;
    packet[29] =
        produced.audit.cameraCalibrationApplied ? 1 : 0;

    std::uint64_t reconstructedChannels = 0u;
    std::uint64_t censoredChannels = 0u;
    std::uint64_t unknownChannels = 0u;
    std::uint64_t p95KnownChannels = 0u;
    std::uint64_t boundKnownChannels = 0u;
    std::uint64_t footprintLinks = 0u;
    std::uint64_t displayClampPixels = 0u;
    std::uint64_t n2CandidateChangedPixels = 0u;
    std::uint64_t n2CandidateAdjustedChannels = 0u;
    std::uint64_t n2CandidateDisplayClampPixels = 0u;
    std::uint64_t v04ObservedCount = 0u;
    std::uint64_t v05ObservedCount = 0u;
    std::uint64_t lightTransportSeedCount = 0u;
    std::uint64_t roomCapsuleAppliedCount = 0u;
    std::uint64_t roomCapsuleExactBypassCount = 0u;
    std::uint64_t v07ObservedCount = 0u;

    sha::Hasher v04LineageHasher;
    sha::Hasher v05LineageHasher;
    sha::Hasher v06LineageHasher;
    sha::Hasher roomCapsuleLineageHasher;
    sha::Hasher v07LineageHasher;
    begin_stage_digest(
        v04LineageHasher, "D_RAW_T5_V04_RUNTIME_LINEAGE_V0_1");
    begin_stage_digest(
        v05LineageHasher, "D_RAW_T5_V05_RUNTIME_LINEAGE_V0_1");
    begin_stage_digest(
        v06LineageHasher, "D_RAW_T5_V06_RUNTIME_LINEAGE_V0_1");
    begin_stage_digest(
        roomCapsuleLineageHasher,
        "D_RAW_T5_ROOM_CAPSULE_RUNTIME_LINEAGE_V0_1");
    begin_stage_digest(
        v07LineageHasher, "D_RAW_T5_V07_RUNTIME_LINEAGE_V0_1");

    for (std::uint32_t y = 0u; y < targetHeight; ++y) {
        for (std::uint32_t x = 0u; x < targetWidth; ++x) {
            tn::QueryResult query{};
            if (!resolver.resolvePixel(x, y, query) ||
                query.stateSha256 != tnState.stateSha256 ||
                query.stateIdentityChangedByTargetRaster ||
                query.createsNewEvidence ||
                query.scientificWritebackAllowed) {
                return status_packet(env, -11);
            }

            // Bind every resolved TruthNegative footprint through the
            // authority-preserving Deep Scene bridge before Appearance. This
            // turns the camera-plane observation into a first-class Free-World
            // contribution without promoting image-plane geometry, inferred
            // material, lighting, or future temporal/multi-view hypotheses to
            // measured sensor evidence.
            tn_deep::CameraPlaneObjectInput cameraPlane{};
            cameraPlane.provenanceId =
                1u + static_cast<std::uint64_t>(y) * targetWidth + x;
            cameraPlane.regionId = 1u;
            cameraPlane.objectId = 1u;
            cameraPlane.depth = 0.0;
            cameraPlane.geometryAuthority =
                truthraw::free_world_deep_scene_binding::v0_5::
                    GeometryAuthority::ImagePlaneBound;
            cameraPlane.parentAncestrySha256 = query.querySha256;

            tn_deep::ScenePacket scenePacket{};
            if (!tn_deep::buildCameraPlaneObject(
                    tnState, query, cameraPlane, scenePacket) ||
                !scenePacket.radiometryBoundToTruthNegative ||
                !scenePacket.geometryAuthoritySeparate ||
                scenePacket.createsNewEvidence ||
                scenePacket.scientificWritebackAllowed ||
                !scenePacket.deepPacket.finalized ||
                scenePacket.deepPacket.createsNewEvidence ||
                scenePacket.deepPacket.scientificWritebackAllowed ||
                !scenePacket.boundPacket.finalized ||
                !scenePacket.boundPacket.geometryAndRadiometrySeparated ||
                scenePacket.boundPacket.createsNewEvidence ||
                scenePacket.boundPacket.scientificWritebackAllowed ||
                scenePacket.boundPacket.contributions.empty()) {
                return status_packet(env, -14);
            }
            for (const auto& contribution :
                 scenePacket.boundPacket.contributions) {
                if (contribution.metadata.geometryAuthority !=
                    truthraw::free_world_deep_scene_binding::v0_5::
                        GeometryAuthority::ImagePlaneBound) {
                    return status_packet(env, -14);
                }
            }
            ++v04ObservedCount;
            ++v05ObservedCount;
            v04LineageHasher.update(scenePacket.deepPacket.packetSha256);
            v05LineageHasher.update(scenePacket.boundPacket.boundPacketSha256);

            deep::DeepResolvedPixel scientificView{};
            if (!deep::resolve(
                    scenePacket.deepPacket,
                    deep::ResolveView::ScientificView,
                    scientificView) ||
                scientificView.createsNewEvidence ||
                scientificView.scientificWritebackAllowed ||
                scientificView.physicalFrameCount != 1u ||
                scientificView.independentEvidenceCount != 1u) {
                return status_packet(env, -15);
            }

            for (std::size_t c = 0u; c < 3u; ++c) {
                const auto& support = query.pixel.support[c];
                if (scientificView.channelAuthority[c] !=
                        support.authority ||
                    scientificView.uncertaintyKnown[c] !=
                        support.uncertaintyKnown) {
                    return status_packet(env, -16);
                }

                switch (support.authority) {
                    case free_world::ResolvedAuthority::Reconstructed:
                        ++reconstructedChannels;
                        break;
                    case free_world::ResolvedAuthority::Censored:
                        ++censoredChannels;
                        break;
                    case free_world::ResolvedAuthority::Unknown:
                        ++unknownChannels;
                        break;
                }
                if (support.uncertaintyKnown) ++p95KnownChannels;
                if (support.boundKnown) ++boundKnownChannels;
            }

            footprintLinks +=
                static_cast<std::uint64_t>(
                    query.pixel.footprint.size());

            tn_deep::InferredLambertianSeedInput seedInput{};
            seedInput.scene = scenePacket;
            seedInput.provenanceId = cameraPlane.provenanceId;
            seedInput.regionId = cameraPlane.regionId;
            seedInput.objectId = cameraPlane.objectId;
            seedInput.incomingDirection = {0.0, 0.0, 1.0};
            seedInput.outgoingDirection = {0.0, 0.0, 1.0};
            seedInput.surfaceNormal = {0.0, 0.0, 1.0};
            seedInput.materialIdentitySha256 = labeled_digest(
                "D_RAW_TN_CONT_V05_INFERRED_NEUTRAL_MATERIAL",
                &scenePacket.scenePacketSha256);
            seedInput.illuminationIdentitySha256 = labeled_digest(
                "D_RAW_TN_CONT_V05_INFERRED_NEUTRAL_ILLUMINATION",
                &scenePacket.scenePacketSha256);
            seedInput.materialSpectralHypothesisSha256 = labeled_digest(
                "D_RAW_TN_CONT_V05_INFERRED_MATERIAL_SPECTRAL_HYPOTHESIS",
                &scenePacket.scenePacketSha256);
            seedInput.illuminationSpectralHypothesisSha256 = labeled_digest(
                "D_RAW_TN_CONT_V05_INFERRED_ILLUMINATION_SPECTRAL_HYPOTHESIS",
                &scenePacket.scenePacketSha256);
            seedInput.diffuseReflectanceRgb = {1.0, 1.0, 1.0};
            seedInput.visibility = 1.0;

            tn_deep::InferredLambertianSeedResult seed{};
            if (!tn_deep::buildInferredLambertianSeed(seedInput, seed) ||
                seed.inheritedScientificRadiometryAsMeasurement ||
                seed.createsNewEvidence ||
                seed.scientificWritebackAllowed ||
                !seed.state.finalized ||
                seed.state.parentBoundDeepPacketSha256 !=
                    scenePacket.boundPacket.boundPacketSha256 ||
                seed.state.surface.normalAuthority !=
                    truthraw::free_world_deep_scene_binding::v0_5::
                        GeometryAuthority::Inferred ||
                seed.state.material.authority !=
                    truthraw::free_world_light_transport_state::v0_6::
                        ParameterAuthority::Inferred ||
                seed.state.material.spectral.authority !=
                    truthraw::free_world_light_transport_state::v0_6::
                        ParameterAuthority::Inferred ||
                seed.state.material.spectral.spectralMeasurementAdmitted ||
                seed.state.material.spectral.fullSpectrumRecovered ||
                seed.state.illumination.authority !=
                    truthraw::free_world_light_transport_state::v0_6::
                        ParameterAuthority::Inferred ||
                seed.state.illumination.spectral.authority !=
                    truthraw::free_world_light_transport_state::v0_6::
                        ParameterAuthority::Inferred ||
                seed.state.illumination.spectral.spectralMeasurementAdmitted ||
                seed.state.illumination.spectral.fullSpectrumRecovered) {
                return status_packet(env, -20);
            }
            ++lightTransportSeedCount;
            v06LineageHasher.update(seed.state.stateSha256);

            open_world_host::LightTransportAttachment lightAttachment{};
            lightAttachment.state = &seed.state;
            lightAttachment.expectedParentBoundDeepPacketSha256 =
                scenePacket.boundPacket.boundPacketSha256;

            open_world_host::RoomCapsuleAppearanceStageContext roomContext{};
            roomContext.sample.position = {0.0F, 0.0F, 0.0F};
            roomContext.sample.normal = {0.0F, 0.0F, 1.0F};
            roomContext.sample.visibility = 1.0F;
            roomContext.sample.confidence = 0.0F;
            roomContext.sample.insideRoom = false;
            roomContext.illumination.present = true;
            roomContext.illumination.authority =
                open_world_host::IlluminationAuthority::Inferred;
            roomContext.illumination.recordId =
                "D_RAW_TN_CONT_V05_INFERRED_NEUTRAL_ROOM_LIGHT";
            roomContext.illumination.spatialScope =
                "CAMERA_PLANE_PIXEL_NO_ROOM_EVIDENCE";
            roomContext.illumination.provenanceSha256 =
                sha::hex(seed.seedSha256);
            roomContext.illumination.inferenceMethod =
                "TN_DEEP_V08_INFERRED_LAMBERTIAN_EXACT_BYPASS";

            std::array<open_world_host::AppearanceStage, 1u>
                baselineAppearanceStages{};
            baselineAppearanceStages[0].stageId =
                "ROOM_CAPSULE_V0_1_AUTHORITY_BOUND";
            baselineAppearanceStages[0].hook =
                &open_world_host::room_capsule_appearance_stage;
            baselineAppearanceStages[0].context = &roomContext;
            baselineAppearanceStages[0].required = false;

            appearance::AppearanceInput input{};
            input.scene = scientificView;
            input.sceneColorimetry = sceneColor;
            input.viewing = viewing;
            input.display = display;
            input.policy = policy;

            open_world_host::AppearanceCorridorEnvelope visibleEnvelope{};
            if (open_world_host::resolve_appearance_corridor(
                    input,
                    &lightAttachment,
                    baselineAppearanceStages,
                    visibleEnvelope) != open_world_host::Status::Ok ||
                !visibleEnvelope.valid ||
                visibleEnvelope.audit.configuredStages != 1u ||
                visibleEnvelope.audit.appliedStages != 0u ||
                visibleEnvelope.audit.bypassedStages != 1u ||
                !visibleEnvelope.audit.lightTransportAttached ||
                !visibleEnvelope.audit.exactPreservingBypass ||
                visibleEnvelope.audit.sourceSceneMutated ||
                visibleEnvelope.audit.createsNewEvidence ||
                visibleEnvelope.audit.scientificWritebackAllowed) {
                return status_packet(env, -12);
            }
            roomCapsuleAppliedCount +=
                visibleEnvelope.audit.appliedStages;
            ++roomCapsuleExactBypassCount;
            const auto roomAuditDigest = labeled_digest(
                "D_RAW_T5_ROOM_CAPSULE_EXACT_PRESERVING_BYPASS_V0_1",
                &seed.seedSha256);
            roomCapsuleLineageHasher.update(roomAuditDigest);

            const auto& visible = visibleEnvelope.result;
            if (visible.sourceSceneSha256 != scientificView.sourcePacketSha256 ||
                visible.sourceSceneMutated ||
                visible.createsNewEvidence ||
                visible.scientificWritebackAllowed ||
                !visible.appearanceApplied ||
                !visible.displayEncoded) {
                return status_packet(env, -12);
            }
            if (visible.exposureApplicationCount != 1u) {
                return status_packet(env, -21);
            }
            ++v07ObservedCount;
            v07LineageHasher.update(visible.outputSha256);
            if (visible.gamutOrDisplayClampApplied) {
                ++displayClampPixels;
            }

            const std::uint32_t r =
                quantize_u8(visible.encodedRgb[0]);
            const std::uint32_t g =
                quantize_u8(visible.encodedRgb[1]);
            const std::uint32_t b =
                quantize_u8(visible.encodedRgb[2]);
            const std::uint32_t argb =
                0xff000000u | (r << 16u) | (g << 8u) | b;
            const std::size_t index =
                static_cast<std::size_t>(y) * targetWidth + x;
            packet[kHeaderInts + index] =
                static_cast<jint>(argb);

            // Separate appearance-only N2 A/B candidate. The Scientific
            // Master, TruthNegative state, Deep Scene packet and baseline
            // Appearance input above remain untouched. We only copy the
            // resolved scene into an explicitly non-scientific candidate and
            // add the area-averaged sampled-CFA correction for this target
            // footprint.
            const auto& correctionBin =
                n2Audit.appearanceGrid[index];
            auto candidateScene = scientificView;
            bool candidateChanged = false;
            for(std::size_t cc=0u;cc<3u;++cc){
                if(correctionBin.sampled[cc]==0u)continue;
                const double correction =
                    correctionBin.correctionSum[cc] /
                    static_cast<double>(correctionBin.sampled[cc]);
                if(!std::isfinite(correction)) {
                    return status_packet(env, -18);
                }
                if(correction!=0.0){
                    candidateScene.sceneLinearRgb[cc] += correction;
                    candidateChanged = true;
                    ++n2CandidateAdjustedChannels;
                }
            }
            if(candidateChanged)++n2CandidateChangedPixels;

            // This alternate display hypothesis must never inherit measured
            // authority merely because it started from the scientific view.
            candidateScene.channelAuthority.fill(
                free_world::ResolvedAuthority::Unknown);
            candidateScene.uncertaintyKnown.fill(false);
            candidateScene.p95Uncertainty.fill(0.0);
            candidateScene.visibility = {};
            candidateScene.appearanceApplied = false;
            candidateScene.displayEncoded = false;
            candidateScene.sourcePacketSha256 =
                n2_candidate_scene_digest(
                    scientificView.sourcePacketSha256,
                    n2Audit.appearanceGridSha256,
                    index,
                    candidateScene.sceneLinearRgb);
            candidateScene.createsNewEvidence = false;
            candidateScene.scientificWritebackAllowed = false;

            appearance::AppearanceInput candidateInput{};
            candidateInput.scene = candidateScene;
            candidateInput.sceneColorimetry = sceneColor;
            candidateInput.viewing = viewing;
            candidateInput.display = display;
            candidateInput.policy = policy;

            open_world_host::AppearanceCorridorEnvelope candidateEnvelope{};
            if (open_world_host::resolve_appearance_corridor(
                    candidateInput,
                    nullptr,
                    kCandidateAppearanceStages,
                    candidateEnvelope) != open_world_host::Status::Ok ||
                !candidateEnvelope.valid ||
                !candidateEnvelope.audit.exactPreservingBypass ||
                candidateEnvelope.audit.sourceSceneMutated ||
                candidateEnvelope.audit.createsNewEvidence ||
                candidateEnvelope.audit.scientificWritebackAllowed) {
                return status_packet(env, -18);
            }
            const auto& candidateVisible = candidateEnvelope.result;
            if(candidateVisible.sourceSceneMutated ||
               candidateVisible.createsNewEvidence ||
               candidateVisible.scientificWritebackAllowed ||
               !candidateVisible.appearanceApplied ||
               !candidateVisible.displayEncoded ||
               candidateVisible.exposureApplicationCount != 1u){
                return status_packet(env, -18);
            }
            if(candidateVisible.gamutOrDisplayClampApplied){
                ++n2CandidateDisplayClampPixels;
            }

            const std::uint32_t cr =
                quantize_u8(candidateVisible.encodedRgb[0]);
            const std::uint32_t cg =
                quantize_u8(candidateVisible.encodedRgb[1]);
            const std::uint32_t cb =
                quantize_u8(candidateVisible.encodedRgb[2]);
            const std::uint32_t candidateArgb =
                0xff000000u | (cr << 16u) | (cg << 8u) | cb;
            packet[kHeaderInts + pixelCount + index] =
                static_cast<jint>(candidateArgb);
        }
    }

    const auto report = scene.report();
    if (!report.fieldSchemaValidated ||
        !report.valueIdentityVerifiedForLoadedRecords ||
        report.masterFieldValueBitMismatches != 0u ||
        report.createsNewEvidence ||
        report.scientificWritebackAllowed ||
        report.physicalFrameCount != 1u ||
        report.independentEvidenceCount != 1u) {
        return status_packet(env, -13);
    }

    const auto postVerified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(
            *bytes, sourceSeal);
    if (!postVerified) {
        return status_packet(env, binding_status(postVerified));
    }

    if (v04ObservedCount != pixelCount64 ||
        v05ObservedCount != pixelCount64 ||
        lightTransportSeedCount != pixelCount64 ||
        roomCapsuleAppliedCount != 0u ||
        roomCapsuleExactBypassCount != pixelCount64 ||
        v07ObservedCount != pixelCount64) {
        return status_packet(env, -22);
    }

    const auto v04LineageSha256 = v04LineageHasher.finalize();
    const auto v05LineageSha256 = v05LineageHasher.finalize();
    const auto v06LineageSha256 = v06LineageHasher.finalize();
    const auto roomCapsuleLineageSha256 =
        roomCapsuleLineageHasher.finalize();
    const auto v07LineageSha256 = v07LineageHasher.finalize();

    packet[13] = clamp_metric(reconstructedChannels);
    packet[14] = clamp_metric(censoredChannels);
    packet[15] = clamp_metric(unknownChannels);
    packet[16] = clamp_metric(p95KnownChannels);
    packet[17] = clamp_metric(boundKnownChannels);
    packet[18] = clamp_metric(footprintLinks);
    packet[26] =
        clamp_metric(report.masterFieldValueBitMismatches);
    packet[27] =
        clamp_metric(report.masterFieldValueBitMatches);
    packet[30] = clamp_metric(displayClampPixels);
    packet[31] = clamp_metric(report.tileLoads);
    digest_to_words(tnState.stateSha256, packet.data() + 32u);
    digest_to_words(
        authorityField.contentSha256, packet.data() + 40u);

    packet[48] = 1;
    packet[49] = n2Audit.noiseProfileAvailable ? 1 : 0;
    packet[50] = clamp_metric(n2Audit.sampled);
    packet[51] = clamp_metric(n2Audit.audit.eligible);
    packet[52] = clamp_metric(n2Audit.audit.corrected);
    packet[53] = clamp_metric(n2Audit.audit.preserved);
    packet[54] = clamp_metric(n2Audit.audit.censoredProtected);
    packet[55] = clamp_metric(n2Audit.audit.censorBoundaryProtected);
    packet[56] = clamp_metric(n2Audit.audit.structureProtected);
    packet[57] = clamp_metric(n2Audit.audit.unknownNoiseProtected);
    packet[58] = clamp_metric(n2Audit.audit.noNeighborhoodProtected);
    packet[59] = clamp_metric(n2Audit.audit.residualOutlierProtected);
    packet[60] = clamp_metric(n2Audit.borderProtected);
    const double removedEnergyFraction =
        n2Audit.audit.totalResidualEnergy > 0.0
            ? n2Audit.audit.removedResidualEnergy /
                n2Audit.audit.totalResidualEnergy
            : 0.0;
    packet[61] = scaled_metric(
        std::clamp(removedEnergyFraction, 0.0, 1.0), 1000000.0);
    packet[62] = scaled_metric(
        n2Audit.audit.maxAbsCorrection, 1000000000.0);
    packet[63] = static_cast<jint>(n2Audit.samplingPeriod);
    digest_to_words(n2Audit.candidateSha256, packet.data() + 64u);
    digest_to_words(n2Audit.auditSha256, packet.data() + 72u);
    for (std::size_t i = 0u; i < 4u; ++i) {
        packet[80u + i] = clamp_metric(n2Audit.cfaPhaseSamples[i]);
    }
    packet[84] = n2Audit.sourceValuesModified ? 1 : 0;
    packet[85] = n2Audit.truthNegativeModified ? 1 : 0;
    packet[86] = n2Audit.createsNewEvidence ? 1 : 0;
    packet[87] = n2Audit.scientificWritebackAllowed ? 1 : 0;
    packet[88] = 0; // baseline candidateAppliedToAppearance = false
    packet[89] = 1; // scientific audit-only = true
    packet[90] = 1; // measuredCfaDomain = true
    packet[91] =
        (lightTransportSeedCount == pixelCount64 &&
         roomCapsuleExactBypassCount == pixelCount64)
            ? 15
            : 0; // bits 0..3: v0.6 built, parent-bound, Room Capsule ran, exact bypass
    packet[92] = 1; // N2 appearance A/B candidate available
    packet[93] = 1; // appearance-only candidate
    packet[94] = 1; // candidate rendered in separate B bitmap
    packet[95] = 0; // candidate createsNewEvidence
    packet[96] = 0; // candidate scientificWritebackAllowed
    packet[97] = 0; // baseline/source scene mutated
    packet[98] = static_cast<jint>(n2Audit.appearanceGridWidth);
    packet[99] = static_cast<jint>(n2Audit.appearanceGridHeight);
    packet[100] = clamp_metric(n2CandidateChangedPixels);
    packet[101] = clamp_metric(n2CandidateAdjustedChannels);
    packet[102] = clamp_metric(n2CandidateDisplayClampPixels);
    packet[103] =
        roomCapsuleExactBypassCount == pixelCount64 ? 1 : 0;
        // Open-World corridor active: v0.6 -> Room Capsule -> v0.7, exact bypass.
    digest_to_words(
        n2Audit.appearanceGridSha256, packet.data() + 104u);
    digest_to_words(
        drawNegativeState.stateSha256, packet.data() + 112u);

    // T5 Corridor Audit v0.1: diagnostic-only runtime telemetry. These slots
    // observe the already executed corridor and never feed values back into
    // Scientific Master, TruthNegative, Deep Scene, Room Capsule or v0.7.
    digest_to_words(sourceSeal.sha256, packet.data() + 120u);
    digest_to_words(scientific.scientificMasterHash, packet.data() + 128u);
    packet[136] = 1; // source <-> Scientific Master phase-2 binding verified
    packet[137] = 1; // v0.7 logical exposureApplicationCount
    packet[138] = clamp_metric(v04ObservedCount);
    packet[139] = clamp_metric(v05ObservedCount);
    packet[140] = clamp_metric(lightTransportSeedCount);
    packet[141] = clamp_metric(roomCapsuleAppliedCount);
    packet[142] = clamp_metric(roomCapsuleExactBypassCount);
    packet[143] = clamp_metric(v07ObservedCount);
    digest_to_words(v04LineageSha256, packet.data() + 144u);
    digest_to_words(v05LineageSha256, packet.data() + 152u);
    digest_to_words(v06LineageSha256, packet.data() + 160u);
    digest_to_words(roomCapsuleLineageSha256, packet.data() + 168u);
    digest_to_words(v07LineageSha256, packet.data() + 176u);
    packet[184] = 0xff; // all T5 mutation/evidence/writeback/binding gates closed
    packet[185] = 1;    // physicalFrameCount observed by T5
    packet[186] = 1;    // independentEvidenceCount observed by T5
    packet[187] = 1;    // T5 telemetry schema version
    packet[188] = 1;    // v0.5 geometry authority == IMAGE_PLANE_BOUND
    packet[189] = 1;    // v0.6 geometry/material/light authority == INFERRED
    packet[190] = 0;    // T5 candidateApplied / scientific writeback remains false
    packet[191] = 0;    // reserved

    jintArray out =
        env->NewIntArray(static_cast<jsize>(packet.size()));
    if (out == nullptr) return nullptr;
    env->SetIntArrayRegion(
        out,
        0,
        static_cast<jsize>(packet.size()),
        packet.data());
    return out;
}

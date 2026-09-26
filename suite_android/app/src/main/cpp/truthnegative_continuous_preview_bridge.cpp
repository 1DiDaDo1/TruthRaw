#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
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
constexpr std::size_t kHeaderInts = 120u;
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
                scenePacket.scientificWritebackAllowed) {
                return status_packet(env, -14);
            }

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

            appearance::AppearanceInput input{};
            input.scene = scientificView;
            input.sceneColorimetry = sceneColor;
            input.viewing = viewing;
            input.display = display;
            input.policy = policy;

            appearance::AppearanceResolvedPixel visible{};
            if (!appearance::resolveAppearance(input, visible) ||
                visible.sourceSceneSha256 != scientificView.sourcePacketSha256 ||
                visible.sourceSceneMutated ||
                visible.createsNewEvidence ||
                visible.scientificWritebackAllowed ||
                !visible.appearanceApplied ||
                !visible.displayEncoded) {
                return status_packet(env, -12);
            }
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

            appearance::AppearanceResolvedPixel candidateVisible{};
            if(!appearance::resolveAppearance(
                    candidateInput,candidateVisible) ||
               candidateVisible.sourceSceneMutated ||
               candidateVisible.createsNewEvidence ||
               candidateVisible.scientificWritebackAllowed ||
               !candidateVisible.appearanceApplied ||
               !candidateVisible.displayEncoded){
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
    packet[91] = 0; // reserved
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
    packet[103] = 0; // reserved
    digest_to_words(
        n2Audit.appearanceGridSha256, packet.data() + 104u);
    digest_to_words(
        drawNegativeState.stateSha256, packet.data() + 112u);

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

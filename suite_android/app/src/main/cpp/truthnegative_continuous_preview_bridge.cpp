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
#include "truthnegative_dense_local_field_adapter_v0_4.h"
#include "truthraw/core.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
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
namespace tn_field =
    truthraw::truthnegative_dense_local_field_adapter::v0_4;
namespace sha = truthraw::sha256_v0_69;

constexpr jint kMagic = 0x35434e54; // TNC5 in little-endian byte view.
constexpr std::size_t kHeaderInts = 48u;
constexpr jint kMaxEdgeHardLimit = 256;

jint clamp_metric(std::uint64_t value) noexcept {
    const auto cap =
        static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
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

    binding::BoundScenePlane scene(
        masterSource, fieldSource, sourceWidth, sourceHeight);
    if (!scene.valid()) return status_packet(env, -7);

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

    std::vector<jint> packet(kHeaderInts + pixelCount, 0);
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

            deep::DeepResolvedPixel scientificView{};
            scientificView.sceneLinearRgb =
                query.pixel.sceneLinear;
            scientificView.sourcePacketSha256 =
                tnState.stateSha256;
            scientificView.view = deep::ResolveView::ScientificView;
            scientificView.appearanceApplied = false;
            scientificView.displayEncoded = false;
            scientificView.createsNewEvidence = false;
            scientificView.scientificWritebackAllowed = false;
            scientificView.physicalFrameCount = 1u;
            scientificView.independentEvidenceCount = 1u;

            for (std::size_t c = 0u; c < 3u; ++c) {
                const auto& support = query.pixel.support[c];
                scientificView.channelAuthority[c] =
                    support.authority;
                scientificView.uncertaintyKnown[c] =
                    support.uncertaintyKnown;
                scientificView.p95Uncertainty[c] =
                    support.uncertaintyKnown
                        ? support.p95Uncertainty
                        : 0.0;

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
                visible.sourceSceneSha256 != tnState.stateSha256 ||
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

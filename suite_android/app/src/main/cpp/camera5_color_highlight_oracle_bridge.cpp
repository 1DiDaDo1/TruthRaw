#include <jni.h>

#include "free_world_appearance_resolve_v0_7.h"
#include "free_world_scientific_open_scene_binding_v0_3.h"
#include "truthnegative_camera5_color_highlight_oracle_v0_1.h"
#include "truthnegative_pipeline_bridge_common.h"
#include "truthraw_sha256_v0_69.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

namespace pipeline = truthraw::android_truthnegative_pipeline::v0_1;
namespace binding = truthraw::free_world_scientific_open_scene_binding::v0_3;
namespace appearance = truthraw::free_world_appearance_resolve::v0_7;
namespace deep = truthraw::free_world_deep_scene_contribution::v0_4;
namespace oracle =
    truthraw::truthnegative_camera5_color_highlight_oracle::v0_1;
namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace sha = truthraw::sha256_v0_69;

sha::Digest labeledDigest(
    const char* label,
    const sha::Digest* parent = nullptr,
    const std::string* text = nullptr) noexcept {
    sha::Hasher h;
    h.update(
        reinterpret_cast<const std::uint8_t*>(label),
        std::char_traits<char>::length(label));
    if (parent != nullptr) h.update(*parent);
    if (text != nullptr) {
        h.update(
            reinterpret_cast<const std::uint8_t*>(text->data()),
            text->size());
    }
    return h.finalize();
}

appearance::Matrix3 matrixFromF32(
    const std::array<float, 9u>& source) noexcept {
    appearance::Matrix3 out{};
    for (std::size_t i = 0u; i < 9u; ++i) out.m[i] = source[i];
    return out;
}

appearance::Matrix3 xyzD50ToSrgb() noexcept {
    appearance::Matrix3 out{};
    out.m = {
         3.1338561, -1.6168667, -0.4906146,
        -0.9787684,  1.9161415,  0.0334540,
         0.0719453, -0.2289914,  1.4052427};
    return out;
}

std::array<double, 3u> mul(
    const appearance::Matrix3& m,
    const std::array<double, 3u>& v) noexcept {
    return {
        m.m[0]*v[0]+m.m[1]*v[1]+m.m[2]*v[2],
        m.m[3]*v[0]+m.m[4]*v[1]+m.m[5]*v[2],
        m.m[6]*v[0]+m.m[7]*v[1]+m.m[8]*v[2]};
}

bool targetGeometry(
    std::uint32_t sw,
    std::uint32_t sh,
    std::uint32_t edge,
    std::uint32_t& tw,
    std::uint32_t& th) noexcept {
    if (sw == 0u || sh == 0u || edge == 0u) return false;
    if (sw >= sh) {
        tw = edge;
        th = std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(
                std::llround(
                    static_cast<double>(edge) *
                    static_cast<double>(sh) /
                    static_cast<double>(sw))));
    } else {
        th = edge;
        tw = std::max<std::uint32_t>(
            1u,
            static_cast<std::uint32_t>(
                std::llround(
                    static_cast<double>(edge) *
                    static_cast<double>(sw) /
                    static_cast<double>(sh))));
    }
    return true;
}

jstring jsonStatus(
    JNIEnv* env,
    int status,
    const std::string& message) {
    std::ostringstream o;
    o << "{\"status\":" << status << ",\"message\":\"";
    for (char c : message) {
        if (c == '"' || c == '\\') o << '\\';
        if (c == '\n' || c == '\r') o << ' ';
        else o << c;
    }
    o << "\"}";
    return env->NewStringUTF(o.str().c_str());
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_truthraw_adaptiveui_Camera5ColorHighlightNativeBridge_runOracle(
    JNIEnv* env,
    jobject,
    jint sourceFd,
    jdouble asShotR,
    jdouble asShotG,
    jdouble asShotB,
    jboolean asShotKnown,
    jint maxSourceResidentBytes,
    jint maxLogicalResidentBytes) {
    pipeline::Context ctx{};
    const auto prepared = pipeline::prepare(
        sourceFd,
        static_cast<std::size_t>(
            std::max(0, maxSourceResidentBytes)),
        static_cast<std::size_t>(
            std::max(0, maxLogicalResidentBytes)),
        ctx);
    if (!prepared) {
        return jsonStatus(env, prepared.code, prepared.message);
    }

    binding::BoundScenePlane scene(
        *ctx.masterSource,
        *ctx.fieldSource,
        ctx.width,
        ctx.height);
    if (!scene.valid()) {
        return jsonStatus(env, -20, scene.error());
    }

    std::uint32_t tw = 0u;
    std::uint32_t th = 0u;
    if (!targetGeometry(ctx.width, ctx.height, 96u, tw, th)) {
        return jsonStatus(env, -21, "oracle target geometry failed");
    }
    tn::RasterResolver resolver(
        scene, ctx.truthNegativeState, tw, th);
    if (!resolver.valid()) {
        return jsonStatus(env, -22, resolver.error());
    }

    appearance::SceneColorimetry sceneColor{};
    sceneColor.rgbToXyz =
        matrixFromF32(ctx.prepared.color.cameraToXyzD50);
    sceneColor.referenceWhiteXyz = {0.96422, 1.0, 0.82521};
    sceneColor.sceneReferenceWhiteNits = 100.0;
    sceneColor.identitySha256 = labeledDigest(
        "D_RAW_CAMERA5_ORACLE_SCENE_COLOR",
        &ctx.truthNegativeState.stateSha256,
        &ctx.prepared.color.bindingId);

    appearance::ViewingConditions viewing{};
    viewing.adaptingWhiteXyz = {0.95047, 1.0, 1.08883};
    viewing.adaptingLuminanceNits = 20.0;
    viewing.backgroundLuminanceNits = 20.0;
    viewing.surround = appearance::Surround::Average;
    viewing.viewingDistanceMeters = 0.5;
    viewing.identitySha256 =
        labeledDigest("D_RAW_CAMERA5_ORACLE_VIEW");

    appearance::DisplayTarget display{};
    display.xyzToRgb = xyzD50ToSrgb();
    display.whitePointXyz = {0.95047, 1.0, 1.08883};
    display.referenceWhiteNits = 100.0;
    display.peakLuminanceNits = 100.0;
    display.blackLuminanceNits = 0.0;
    display.transfer = appearance::TransferFunction::Srgb;
    display.identitySha256 =
        labeledDigest("D_RAW_CAMERA5_ORACLE_SRGB100");

    oracle::Input input{};
    input.sourceEvidenceSha256 = ctx.sourceSeal.sha256;
    input.scientificMasterSha256 =
        ctx.scientific.scientificMasterHash;
    input.truthNegativeStateSha256 =
        ctx.truthNegativeState.stateSha256;
    input.colorBindingSha256 = labeledDigest(
        "D_RAW_CAMERA5_SOURCE_COLOR_BINDING",
        &ctx.sourceSeal.sha256,
        &ctx.produced.color.bindingId);
    input.physicalCameraId = 5u;

    const auto& meta = ctx.openedSource.source->metadata();
    input.whiteLevel = meta.whiteLevel;
    for (std::size_t i = 0u; i < 4u; ++i) {
        input.blackPhase[i] = meta.blackPhase[i];
    }
    input.asShotNeutral = {
        static_cast<double>(asShotR),
        static_cast<double>(asShotG),
        static_cast<double>(asShotB)};
    input.asShotNeutralKnown = asShotKnown == JNI_TRUE;
    input.remosaicState = oracle::RemosaicState::Unknown;
    input.samples.reserve(static_cast<std::size_t>(tw) * th);

    for (std::uint32_t y = 0u; y < th; ++y) {
        for (std::uint32_t x = 0u; x < tw; ++x) {
            tn::QueryResult q{};
            if (!resolver.resolvePixel(x, y, q)) {
                return jsonStatus(env, -23, "TN oracle query failed");
            }

            oracle::Sample sample{};
            sample.cameraNativeRgb = q.pixel.sceneLinear;
            sample.xyzD50 =
                mul(sceneColor.rgbToXyz, q.pixel.sceneLinear);
            for (std::size_t c = 0u; c < 3u; ++c) {
                sample.authority[c] =
                    q.pixel.support[c].authority;
                sample.censoredWeight[c] =
                    q.pixel.support[c].censoredWeight;
            }

            deep::DeepResolvedPixel scientific{};
            scientific.sceneLinearRgb = q.pixel.sceneLinear;
            scientific.sourcePacketSha256 =
                ctx.truthNegativeState.stateSha256;
            scientific.channelAuthority = {
                q.pixel.support[0].authority,
                q.pixel.support[1].authority,
                q.pixel.support[2].authority};
            scientific.uncertaintyKnown = {
                q.pixel.support[0].uncertaintyKnown,
                q.pixel.support[1].uncertaintyKnown,
                q.pixel.support[2].uncertaintyKnown};
            scientific.p95Uncertainty = {
                q.pixel.support[0].p95Uncertainty,
                q.pixel.support[1].p95Uncertainty,
                q.pixel.support[2].p95Uncertainty};
            scientific.view = deep::ResolveView::ScientificView;
            scientific.physicalFrameCount = 1u;
            scientific.independentEvidenceCount = 1u;

            for (std::size_t e = 0u;
                 e < oracle::kExposureEv.size();
                 ++e) {
                appearance::AppearancePolicy policy{};
                policy.exposureEv = oracle::kExposureEv[e];
                policy.colorfulnessScale = 1.0;
                policy.highlightCompression = 1.0;
                policy.identitySha256 =
                    labeledDigest("D_RAW_CAMERA5_ORACLE_NEUTRAL_POLICY");

                appearance::AppearanceInput ai{};
                ai.scene = scientific;
                ai.sceneColorimetry = sceneColor;
                ai.viewing = viewing;
                ai.display = display;
                ai.policy = policy;

                appearance::AppearanceResolvedPixel ao{};
                if (!appearance::resolveAppearance(ai, ao)) {
                    return jsonStatus(
                        env, -24, "appearance EV sweep failed");
                }
                sample.encodedRgbByExposure[e] = ao.encodedRgb;
                sample.displayClampByExposure[e] =
                    ao.gamutOrDisplayClampApplied;
            }
            input.samples.push_back(sample);
        }
    }

    oracle::Report report{};
    if (!oracle::run(input, report)) {
        return jsonStatus(env, -25, "Camera5 oracle failed");
    }
    if (!pipeline::reverify(ctx)) {
        return jsonStatus(env, -26, "source changed during oracle");
    }

    std::ostringstream o;
    o << std::setprecision(10);
    o << "{\"status\":0";
    o << ",\"firstFailureStage\":\""
      << oracle::toString(report.firstFailureStage) << "\"";
    o << ",\"sampleCount\":" << report.sampleCount;
    o << ",\"apparentWhiteHighlightCandidates\":"
      << report.apparentWhiteHighlightCandidates;
    o << ",\"censoredChannelCount\":"
      << report.censoredChannelCount;
    o << ",\"censoredChannelFraction\":"
      << report.censoredChannelFraction;
    o << ",\"metadataNeutralMismatch\":"
      << (report.metadataNeutralMismatch ? "true" : "false");
    o << ",\"metadataNeutralLogError\":"
      << report.metadataNeutralLogError;
    o << ",\"lowExposureGreenBias\":"
      << report.lowExposureGreenBias;
    o << ",\"exposureGreenDrift\":"
      << report.exposureGreenDrift;
    o << ",\"appearanceDisplayDrift\":"
      << (report.appearanceDisplayDriftDetected ? "true" : "false");
    o << ",\"unclampedExposureHueDrift\":"
      << report.unclampedExposureHueDrift;
    o << ",\"sourceCensoringDominant\":"
      << (report.sourceCensoringDominant ? "true" : "false");
    o << ",\"colorBindingBiasDetected\":"
      << (report.colorBindingBiasDetected ? "true" : "false");
    o << ",\"remosaicState\":\""
      << oracle::toString(input.remosaicState) << "\"";
    o << ",\"remosaicScientificallyResolved\":"
      << (report.remosaicScientificallyResolved ? "true" : "false");
    o << ",\"asShotNeutralKnown\":"
      << (input.asShotNeutralKnown ? "true" : "false");
    o << ",\"asShotNeutral\":["
      << input.asShotNeutral[0] << ","
      << input.asShotNeutral[1] << ","
      << input.asShotNeutral[2] << "]";
    o << ",\"empiricalNeutralKnown\":"
      << (report.empiricalCameraNeutralKnown ? "true" : "false");
    o << ",\"empiricalCameraNeutral\":["
      << report.empiricalCameraNeutral[0] << ","
      << report.empiricalCameraNeutral[1] << ","
      << report.empiricalCameraNeutral[2] << "]";
    o << ",\"whiteLevel\":" << input.whiteLevel;
    o << ",\"usedForwardMatrix\":"
      << (ctx.produced.audit.usedForwardMatrix ? "true" : "false");
    o << ",\"cameraCalibrationApplied\":"
      << (ctx.produced.audit.cameraCalibrationApplied ? "true" : "false");
    o << ",\"createsNewEvidence\":false";
    o << ",\"scientificWritebackAllowed\":false";
    o << ",\"oracleSha256\":\""
      << sha::hex(report.oracleSha256) << "\"}";
    return env->NewStringUTF(o.str().c_str());
}

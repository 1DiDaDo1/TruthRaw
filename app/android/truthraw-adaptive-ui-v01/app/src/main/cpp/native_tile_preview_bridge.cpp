#include <jni.h>

#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {

using truthraw::TileRect;
using truthraw::tile_dng_v0_1::ColorBinding;
using truthraw::tile_dng_v0_1::DngSourceCode;
using truthraw::tile_dng_v0_1::OpenOptions;
using truthraw::tile_dng_v0_1::PosixFdByteSource;
using truthraw::tile_dng_v0_1::TileNativeDngSource;

constexpr jint kMagic = 0x54525032; // TRP2
constexpr std::size_t kHeaderInts = 13;
constexpr int kChunkSamples = 1024;
constexpr int kAbsoluteMaxPreviewEdge = 512;

jint clamp_metric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jintArray packet(JNIEnv* env, jint status) {
    jint header[kHeaderInts] = {};
    header[0] = kMagic;
    header[1] = status;
    jintArray out = env->NewIntArray(static_cast<jsize>(kHeaderInts));
    if (out != nullptr) env->SetIntArrayRegion(out, 0, static_cast<jsize>(kHeaderInts), header);
    return out;
}

int phase_index(int y, int x) {
    return ((y & 1) << 1) | (x & 1);
}

int sample_x(int previewX, int previewWidth, int sourceWidth) {
    const double x = (static_cast<double>(previewX) + 0.5) * static_cast<double>(sourceWidth) /
        static_cast<double>(previewWidth);
    return std::clamp(static_cast<int>(x), 0, sourceWidth - 1);
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeTilePreviewBridge_buildCfaPreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes) {
    if (fd < 0 || requestedMaxEdge < 32 || maxSourceResidentBytes <= 0) return packet(env, -1);

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    OpenOptions options;
    // Preview-only parser sentinel. This call never exports TileNativeDngSource,
    // never enters the scientific pipeline and never claims calibrated color.
    // A real scientific handoff must replace both bindings with evidence-bound IDs.
    options.sourceEvidenceId = "ui-preview-ephemeral-not-evidence-v0.2";
    options.color = ColorBinding{};
    options.color.valid = true;
    options.color.bindingId = "ui-preview-parser-sentinel-not-scientific-v0.2";
    options.color.cameraToXyzD50 = {1,0,0, 0,1,0, 0,0,1};
    options.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    std::unique_ptr<TileNativeDngSource> source;
    const auto opened = TileNativeDngSource::open(bytes, options, source);
    if (!opened) return packet(env, static_cast<jint>(opened.code));

    const auto& meta = source->metadata();
    if (meta.width <= 0 || meta.height <= 0) return packet(env, -2);

    int previewWidth = maxEdge;
    int previewHeight = maxEdge;
    if (meta.width >= meta.height) {
        previewHeight = std::max(1, static_cast<int>(std::lround(
            static_cast<double>(maxEdge) * static_cast<double>(meta.height) / static_cast<double>(meta.width))));
    } else {
        previewWidth = std::max(1, static_cast<int>(std::lround(
            static_cast<double>(maxEdge) * static_cast<double>(meta.width) / static_cast<double>(meta.height))));
    }

    const std::size_t pixelCount = static_cast<std::size_t>(previewWidth) * static_cast<std::size_t>(previewHeight);
    if (pixelCount > static_cast<std::size_t>(kAbsoluteMaxPreviewEdge) * kAbsoluteMaxPreviewEdge) return packet(env, -3);

    std::vector<jint> out(kHeaderInts + pixelCount, 0);
    std::vector<std::uint16_t> raw(kChunkSamples, 0);
    std::vector<float> gain(kChunkSamples, 1.0f);

    for (int py = 0; py < previewHeight; ++py) {
        const double fy = (static_cast<double>(py) + 0.5) * static_cast<double>(meta.height) /
            static_cast<double>(previewHeight);
        const int sy = std::clamp(static_cast<int>(fy), 0, meta.height - 1);
        int nextPx = 0;

        for (int x0 = 0; x0 < meta.width && nextPx < previewWidth; x0 += kChunkSamples) {
            const int x1 = std::min(meta.width, x0 + kChunkSamples);
            const std::size_t count = static_cast<std::size_t>(x1 - x0);
            TileRect rect{x0, sy, x1, sy + 1, x0, sy, x1, sy + 1};
            const auto read = source->readRawTile(
                rect,
                raw.data(),
                count,
                meta.hasGainField ? gain.data() : nullptr,
                meta.hasGainField ? count : 0);
            if (!read) return packet(env, 1000 + static_cast<jint>(read.code));

            while (nextPx < previewWidth) {
                const int sx = sample_x(nextPx, previewWidth, meta.width);
                if (sx >= x1) break;
                if (sx >= x0) {
                    const int phase = phase_index(sy, sx);
                    const float black = meta.blackPhase[static_cast<std::size_t>(phase)];
                    const float denom = std::max(1.0e-6f, meta.whiteLevel - black);
                    const float linear = std::clamp((static_cast<float>(raw[static_cast<std::size_t>(sx - x0)]) - black) / denom, 0.0f, 1.0f);
                    const float display = std::sqrt(linear); // presentation-only visibility curve
                    const int gray = std::clamp(static_cast<int>(std::lround(display * 255.0f)), 0, 255);
                    const std::uint32_t argb = 0xff000000u |
                        (static_cast<std::uint32_t>(gray) << 16u) |
                        (static_cast<std::uint32_t>(gray) << 8u) |
                        static_cast<std::uint32_t>(gray);
                    out[kHeaderInts + static_cast<std::size_t>(py) * previewWidth + static_cast<std::size_t>(nextPx)] =
                        static_cast<jint>(argb);
                }
                ++nextPx;
            }
        }
        if (nextPx != previewWidth) return packet(env, -4);
    }

    const auto& audit = source->audit();
    out[0] = kMagic;
    out[1] = 0;
    out[2] = previewWidth;
    out[3] = previewHeight;
    out[4] = meta.width;
    out[5] = meta.height;
    out[6] = clamp_metric(source->residentBytesUpperBound());
    out[7] = clamp_metric(audit.rawPayloadBytesRead);
    out[8] = clamp_metric(audit.metadataBytesRead);
    out[9] = clamp_metric(audit.tileReadCalls);
    out[10] = audit.fullRawMaterialized ? 1 : 0;
    out[11] = meta.hasGainField ? 1 : 0;
    out[12] = static_cast<jint>(meta.orientation);

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result == nullptr) return nullptr;
    env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    return result;
}

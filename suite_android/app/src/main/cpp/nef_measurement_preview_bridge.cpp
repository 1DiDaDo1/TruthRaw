#include <jni.h>

#include "multivendor_raw_source_adapter_v0_1.h"
#include "raw_source_adapter_bridge_common.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {

namespace adapter = truthraw::multivendor_raw_source_adapter::v0_1;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

constexpr jint kMagic = 0x54524e4d; // TRNM = TruthRaw NEF Measurement
constexpr std::size_t kHeaderInts = 26u;
constexpr int kAbsoluteMaxPreviewEdge = 512;

jintArray statusPacket(JNIEnv* env, jint status) {
    jint header[kHeaderInts] = {};
    header[0] = kMagic;
    header[1] = status;
    auto out = env->NewIntArray(static_cast<jsize>(kHeaderInts));
    if (out != nullptr) env->SetIntArrayRegion(out, 0, static_cast<jsize>(kHeaderInts), header);
    return out;
}

jint clampMetric(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jint bindingStatus(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jint>(status.code);
}

jint adapterStatus(const adapter::AdapterStatus& status) {
    return 7000 + static_cast<jint>(status.code);
}

int previewDimension(int sourcePrimary, int sourceSecondary, int maxEdge) {
    if (sourcePrimary >= sourceSecondary) return maxEdge;
    return std::max(1, static_cast<int>(std::lround(
        static_cast<double>(maxEdge) * static_cast<double>(sourcePrimary) /
        static_cast<double>(sourceSecondary))));
}

int sampleCoordinate(int previewCoordinate, int previewExtent, int sourceExtent) {
    const double v = (static_cast<double>(previewCoordinate) + 0.5) *
        static_cast<double>(sourceExtent) / static_cast<double>(previewExtent);
    return std::clamp(static_cast<int>(v), 0, sourceExtent - 1);
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NefMeasurementNativeBridge_buildMeasurementCfaPreview(
    JNIEnv* env,
    jobject,
    jint fd,
    jint requestedMaxEdge,
    jint maxSourceResidentBytes) {
    if (fd < 0 || requestedMaxEdge < 32 || maxSourceResidentBytes <= 0) {
        return statusPacket(env, -1);
    }

    const int maxEdge = std::min(static_cast<int>(requestedMaxEdge), kAbsoluteMaxPreviewEdge);
    auto bytes = std::make_shared<PosixFdByteSource>(static_cast<int>(fd));

    // Seal the exact source before format parsing. The NEF adapter accepts this
    // identity but does not independently recompute SHA-256.
    SourceSeal seal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(*bytes, seal);
    if (!sealed) return statusPacket(env, bindingStatus(sealed));

    adapter::RawSourceOpenRequest request;
    request.declaredFormat = adapter::RawFormatFamily::NikonNef;
    request.sourceSeal.valid = true;
    request.sourceSeal.sha256 = seal.sha256;
    request.sourceSeal.byteLength = seal.byteLength;
    request.sourceSeal.sourceEvidenceId = seal.sourceEvidenceId;
    request.color.valid = false;
    request.maxResidentBytes = static_cast<std::size_t>(maxSourceResidentBytes);

    adapter::RawSourceAdapterRegistry registry;
    const auto registered = registry.registerAdapter(adapter::makeNikonNefUncompressedAdapter());
    if (!registered) return statusPacket(env, adapterStatus(registered));

    auto genericBytes = std::make_shared<truthraw::android_raw_adapter_bridge::v0_1::RawByteSourceView>(bytes);
    std::unique_ptr<truthraw::streaming_v0_1::IRawTileSource> source;
    adapter::RawSourceDescriptor descriptor;
    const auto opened = registry.open(genericBytes, request, source, descriptor);
    if (!opened) return statusPacket(env, adapterStatus(opened));

    if (!source ||
        !descriptor.sourceSealAcceptedAtBoundary ||
        !descriptor.exactCfaSamplesAvailable ||
        !descriptor.measurementAdmissionReady ||
        descriptor.scientificAdmissionReady ||
        descriptor.directSensorAdcClaimAllowed ||
        descriptor.fullRawFrameMaterialized) {
        return statusPacket(env, -2);
    }

    const auto& metadata = source->metadata();
    if (metadata.width <= 0 || metadata.height <= 0) return statusPacket(env, -3);

    int previewWidth;
    int previewHeight;
    if (metadata.width >= metadata.height) {
        previewWidth = maxEdge;
        previewHeight = previewDimension(metadata.height, metadata.width, maxEdge);
    } else {
        previewHeight = maxEdge;
        previewWidth = previewDimension(metadata.width, metadata.height, maxEdge);
    }

    const std::size_t pixelCount =
        static_cast<std::size_t>(previewWidth) * static_cast<std::size_t>(previewHeight);
    if (pixelCount > static_cast<std::size_t>(kAbsoluteMaxPreviewEdge) * kAbsoluteMaxPreviewEdge) {
        return statusPacket(env, -4);
    }

    std::vector<jint> packet(kHeaderInts + pixelCount, 0);
    std::vector<std::uint16_t> row(static_cast<std::size_t>(metadata.width), 0u);
    std::uint16_t minCode = std::numeric_limits<std::uint16_t>::max();
    std::uint16_t maxCode = 0u;
    std::uint64_t samplesReadForPreview = 0u;

    for (int py = 0; py < previewHeight; ++py) {
        const int sy = sampleCoordinate(py, previewHeight, metadata.height);

        truthraw::TileRect rect{};
        rect.x0 = 0;
        rect.y0 = sy;
        rect.x1 = metadata.width;
        rect.y1 = sy + 1;
        rect.hx0 = rect.x0;
        rect.hy0 = rect.y0;
        rect.hx1 = rect.x1;
        rect.hy1 = rect.y1;

        const auto read = source->readRawTile(
            rect, row.data(), row.size(), nullptr, 0u);
        if (!read) return statusPacket(env, 4000 + static_cast<jint>(read.code));

        for (int px = 0; px < previewWidth; ++px) {
            const int sx = sampleCoordinate(px, previewWidth, metadata.width);
            const std::uint16_t code = row[static_cast<std::size_t>(sx)];
            minCode = std::min(minCode, code);
            maxCode = std::max(maxCode, code);
            ++samplesReadForPreview;

            // Visibility-only proxy from exact source code. This is not black
            // subtraction, white normalization, demosaic, color science, or a
            // Scientific Master. Use the 16-bit storage ceiling only.
            const float unit = static_cast<float>(code) / 65535.0f;
            const float visible = std::sqrt(std::clamp(unit, 0.0f, 1.0f));
            const int gray = std::clamp(
                static_cast<int>(std::lround(visible * 255.0f)), 0, 255);
            const std::uint32_t argb = 0xff000000u |
                (static_cast<std::uint32_t>(gray) << 16u) |
                (static_cast<std::uint32_t>(gray) << 8u) |
                static_cast<std::uint32_t>(gray);
            packet[kHeaderInts +
                static_cast<std::size_t>(py) * previewWidth +
                static_cast<std::size_t>(px)] = static_cast<jint>(argb);
        }
    }

    const auto reverified =
        truthraw::scientific_preview_binding_v0_1::reverify_source_sha256(*bytes, seal);
    if (!reverified) return statusPacket(env, bindingStatus(reverified));

    packet[0] = kMagic;
    packet[1] = 0;
    packet[2] = previewWidth;
    packet[3] = previewHeight;
    packet[4] = metadata.width;
    packet[5] = metadata.height;
    packet[6] = static_cast<jint>(metadata.cfa);
    packet[7] = clampMetric(source->residentBytesUpperBound());
    packet[8] = static_cast<jint>(minCode);
    packet[9] = static_cast<jint>(maxCode);
    packet[10] = clampMetric(samplesReadForPreview);
    packet[11] = descriptor.measurementAdmissionReady ? 1 : 0;
    packet[12] = descriptor.scientificAdmissionReady ? 1 : 0;
    packet[13] = descriptor.exactCfaSamplesAvailable ? 1 : 0;
    packet[14] = descriptor.directSensorAdcClaimAllowed ? 1 : 0;
    packet[15] = descriptor.fullRawFrameMaterialized ? 1 : 0;
    packet[16] = static_cast<jint>(seal.byteLength & 0xffffffffull);
    packet[17] = static_cast<jint>((seal.byteLength >> 32u) & 0xffffffffull);
    for (std::size_t word = 0; word < 8u; ++word) {
        const std::size_t base = word * 4u;
        const std::uint32_t packed =
            static_cast<std::uint32_t>(seal.sha256[base]) |
            (static_cast<std::uint32_t>(seal.sha256[base + 1u]) << 8u) |
            (static_cast<std::uint32_t>(seal.sha256[base + 2u]) << 16u) |
            (static_cast<std::uint32_t>(seal.sha256[base + 3u]) << 24u);
        packet[18u + word] = static_cast<jint>(packed);
    }

    auto out = env->NewIntArray(static_cast<jsize>(packet.size()));
    if (out == nullptr) return nullptr;
    env->SetIntArrayRegion(out, 0, static_cast<jsize>(packet.size()), packet.data());
    return out;
}

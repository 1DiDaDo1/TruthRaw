#include <jni.h>

#include "dng_color_binding_producer_v0_2.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {

using truthraw::dng_color_binding_producer_v0_2::ProducerResult;
using truthraw::scientific_preview_binding_v0_1::SourceSeal;
using truthraw::tile_dng_v0_1::PosixFdByteSource;

constexpr jint kEmpiricalMagic = 0x54524531; // TRE1
constexpr std::size_t kEmpiricalHeaderInts = 32;

jint clamp_u64(std::uint64_t value) {
    const auto cap = static_cast<std::uint64_t>(std::numeric_limits<jint>::max());
    return static_cast<jint>(std::min(value, cap));
}

jint scaled_double(double value, double scale) {
    if (!std::isfinite(value)) return 0;
    const double scaled = value * scale;
    if (scaled >= static_cast<double>(std::numeric_limits<jint>::max())) {
        return std::numeric_limits<jint>::max();
    }
    if (scaled <= static_cast<double>(std::numeric_limits<jint>::min())) {
        return std::numeric_limits<jint>::min();
    }
    return static_cast<jint>(std::llround(scaled));
}

void write_source_seal(std::vector<jint>& out, const SourceSeal& seal) {
    for (std::size_t wordIndex = 0; wordIndex < 8; ++wordIndex) {
        const std::size_t byteIndex = wordIndex * 4;
        const std::uint32_t word =
            (static_cast<std::uint32_t>(seal.sha256[byteIndex]) << 24u) |
            (static_cast<std::uint32_t>(seal.sha256[byteIndex + 1]) << 16u) |
            (static_cast<std::uint32_t>(seal.sha256[byteIndex + 2]) << 8u) |
            static_cast<std::uint32_t>(seal.sha256[byteIndex + 3]);
        out[2 + wordIndex] = static_cast<jint>(word);
    }
    out[10] = static_cast<jint>((seal.byteLength >> 32u) & 0xffffffffu);
    out[11] = static_cast<jint>(seal.byteLength & 0xffffffffu);
    out[31] = clamp_u64(static_cast<std::uint64_t>(seal.hashWorkspacePeakBytes));
}

void write_producer_audit(std::vector<jint>& out, const ProducerResult& produced) {
    const auto& audit = produced.audit;
    out[12] = clamp_u64(audit.metadataBytesRead);
    out[13] = static_cast<jint>(audit.ifdEntriesVisited);
    out[14] = clamp_u64(static_cast<std::uint64_t>(audit.parserWorkspacePeakBytes));
    out[15] = audit.delegatedSingleIlluminantV01 ? 1 : 0;
    out[16] = audit.dualIlluminantUsed ? 1 : 0;
    out[17] = audit.thirdCalibrationSeen ? 1 : 0;
    out[18] = audit.usedForwardMatrix ? 1 : 0;
    out[19] = audit.usedSingleForwardMatrixAcrossTemperatures ? 1 : 0;
    out[20] = audit.cameraCalibrationPresent ? 1 : 0;
    out[21] = audit.cameraCalibrationSignatureMatched ? 1 : 0;
    out[22] = audit.cameraCalibrationApplied ? 1 : 0;
    out[23] = static_cast<jint>(audit.calibrationIlluminant1);
    out[24] = static_cast<jint>(audit.calibrationIlluminant2);
    out[25] = scaled_double(audit.resolvedWhiteTemperatureK, 1000.0);
    out[26] = scaled_double(audit.interpolationWeightLow, 1000000000.0);
    out[27] = static_cast<jint>(audit.neutralSolveIterations);
    out[28] = static_cast<jint>(produced.color.authority);
    out[29] = static_cast<jint>(produced.color.physicalFrameCount);
    out[30] = static_cast<jint>(produced.color.independentEvidenceCount);
}

jintArray emit_packet(
    JNIEnv* env,
    jint status,
    const SourceSeal* seal,
    const ProducerResult* produced) {
    std::vector<jint> out(kEmpiricalHeaderInts, 0);
    out[0] = kEmpiricalMagic;
    out[1] = status;
    if (seal != nullptr) write_source_seal(out, *seal);
    if (produced != nullptr) write_producer_audit(out, *produced);

    jintArray result = env->NewIntArray(static_cast<jsize>(out.size()));
    if (result != nullptr) {
        env->SetIntArrayRegion(result, 0, static_cast<jsize>(out.size()), out.data());
    }
    return result;
}

jint binding_status(const truthraw::scientific_preview_binding_v0_1::BindingStatus& status) {
    return 2000 + static_cast<jint>(status.code);
}

jint producer_status(const truthraw::dng_color_binding_producer_v0_2::ProducerStatus& status) {
    return 2100 + static_cast<jint>(status.code);
}

} // namespace

extern "C" JNIEXPORT jintArray JNICALL
Java_com_truthraw_adaptiveui_NativeEmpiricalBridge_probeSourceColor(
    JNIEnv* env,
    jobject,
    jint fd) {
    if (fd < 0) return emit_packet(env, -1, nullptr, nullptr);

    PosixFdByteSource bytes(static_cast<int>(fd));
    SourceSeal sourceSeal;
    const auto sealed = truthraw::scientific_preview_binding_v0_1::seal_source_sha256(bytes, sourceSeal);
    if (!sealed) return emit_packet(env, binding_status(sealed), nullptr, nullptr);

    ProducerResult produced;
    const auto colorStatus = truthraw::dng_color_binding_producer_v0_2::produce_source_metadata_color_binding(
        bytes, sourceSeal, produced);
    if (!colorStatus) {
        return emit_packet(env, producer_status(colorStatus), &sourceSeal, &produced);
    }

    return emit_packet(env, 0, &sourceSeal, &produced);
}

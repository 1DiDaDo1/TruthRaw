#include <jni.h>

#include "draw_jpeg444_q100_encoder.h"
#include "presentation_gamut_fit_v0_1.h"
#include "presentation_highlight_chroma_rolloff_v0_1.h"
#include "presentation_illuminant_warmth_retention_v0_1.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr jlong kMagic = 0x44524a34; // DRJ4
constexpr jint kRequiredQuality = 100;

namespace presentation_gamut = truthraw::presentation_gamut_fit::v0_1;
namespace presentation_highlight = truthraw::presentation_highlight_chroma_rolloff::v0_1;
namespace presentation_warmth = truthraw::presentation_illuminant_warmth_retention::v0_1;

struct WriterContext {
    int fd = -1;
    bool ok = true;
    bool canonicalSosWritten = false;
    bool diagnosticApp15Written = false;
    std::uint64_t bytes = 0u;
    std::string diagnosticApp15;
};

bool write_all(WriterContext *context, const std::uint8_t *bytes, std::size_t size) {
    if (context == nullptr || !context->ok || context->fd < 0 || bytes == nullptr) {
        if (context != nullptr) context->ok = false;
        return false;
    }
    std::size_t remaining = size;
    while (remaining > 0u) {
        const ssize_t written = ::write(context->fd, bytes, remaining);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            context->ok = false;
            return false;
        }
        bytes += static_cast<std::size_t>(written);
        remaining -= static_cast<std::size_t>(written);
        context->bytes += static_cast<std::uint64_t>(written);
    }
    return true;
}

std::string build_diagnostic_app15(int width, int height) {
    const auto gamut = presentation_gamut::take_diagnostics_snapshot_and_reset();
    const auto highlight = presentation_highlight::take_diagnostics_snapshot_and_reset();
    const auto warmth = presentation_warmth::take_diagnostics_snapshot_and_reset();

    const std::uint64_t pixels =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    const bool frameScoped = pixels > 0u && gamut.unitCalls == pixels;

    std::string payload = "D.RAW_GAMUT_DIAG_V01\n";
    const auto add = [&](const char *key, std::uint64_t value) {
        payload += key;
        payload += '=';
        payload += std::to_string(value);
        payload += '\n';
    };
    add("frame_scoped", frameScoped ? 1u : 0u);
    add("pixels", pixels);
    add("early_negative_any", gamut.nonnegativeNegativeInput);
    add("early_g_negative_rb_positive", gamut.nonnegativeGreenNegativeRbPositive);
    add("early_g_boundary_input_rb_positive", gamut.nonnegativeGreenBoundaryInputRbPositive);
    add("early_g_boundary_input_rb_high", gamut.nonnegativeGreenBoundaryInputRbHigh);
    add("early_g_boundary_after_fit", gamut.nonnegativeGreenBoundaryAfterFit);
    add("final_unit_calls", gamut.unitCalls);
    add("final_out_of_range", gamut.unitOutOfRangeInput);
    add("final_g_negative_rb_positive", gamut.unitGreenNegativeRbPositive);
    add("warm_bright_inputs", warmth.brightInputs);
    add("warm_bright_green_min_in", warmth.brightGreenStrictMinInput);
    add("warm_bright_green_min_out", warmth.brightGreenStrictMinOutput);
    add("warm_bright_green_negative_out", warmth.brightGreenNegativeOutput);
    add("highlight_censored_calls", highlight.censoredCalls);
    add("highlight_censored_green_min_in", highlight.censoredGreenStrictMinInput);
    add("highlight_white_candidates", highlight.censoredWhiteCandidates);
    add("highlight_severe_applied", highlight.censoredSevereApplied);
    add("highlight_neutral_applied", highlight.censoredNearNeutralApplied);
    payload += "sealed_streaming_module_modified=0\n";
    payload += "pixel_mutation_by_diagnostics=0\n";
    payload += "scientific_writeback=0\n";

    // JPEG segment length includes its own two-byte length word but not marker.
    if (payload.size() + 2u > 65535u) return {};
    const std::uint16_t segmentLength =
        static_cast<std::uint16_t>(payload.size() + 2u);

    std::string segment;
    segment.reserve(payload.size() + 4u);
    segment.push_back(static_cast<char>(0xff));
    segment.push_back(static_cast<char>(0xef)); // APP15
    segment.push_back(static_cast<char>((segmentLength >> 8u) & 0xffu));
    segment.push_back(static_cast<char>(segmentLength & 0xffu));
    segment += payload;
    return segment;
}

void fd_writer(void *opaque, const void *data, int size) {
    auto *context = static_cast<WriterContext *>(opaque);
    if (context == nullptr || !context->ok || context->fd < 0 || data == nullptr || size < 0) {
        if (context != nullptr) context->ok = false;
        return;
    }

    const auto *bytes = static_cast<const std::uint8_t *>(data);

    // Embed the temporary real-device diagnostic record immediately after SOI.
    // APP15 is metadata only: RGB pixels, sampling and quantization are untouched.
    if (!context->diagnosticApp15Written && size >= 2 &&
        bytes[0] == 0xff && bytes[1] == 0xd8) {
        if (!write_all(context, bytes, 2u)) return;
        if (context->diagnosticApp15.empty() ||
            !write_all(
                context,
                reinterpret_cast<const std::uint8_t *>(context->diagnosticApp15.data()),
                context->diagnosticApp15.size())) {
            context->ok = false;
            return;
        }
        context->diagnosticApp15Written = true;
        if (size > 2) {
            (void)write_all(
                context,
                bytes + 2u,
                static_cast<std::size_t>(size - 2));
        }
        return;
    }

    // The first generated-Huffman encoder revision accidentally emitted one
    // extra 0x00 byte inside the SOS payload while still declaring length 12.
    // Tolerant decoders could display the image, but strict JPEG decoders
    // correctly rejected the stream. Canonicalize exactly that known header at
    // the downstream presentation boundary and fail closed if the encoder's SOS
    // shape changes unexpectedly. This does not touch RGB pixels or science.
    static constexpr std::uint8_t kMalformedSos[15] = {
        0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x00,0x3f,0x00,
    };
    static constexpr std::uint8_t kCanonicalSos[14] = {
        0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,
    };

    if (size == static_cast<int>(sizeof(kMalformedSos)) &&
        std::memcmp(bytes, kMalformedSos, sizeof(kMalformedSos)) == 0) {
        if (context->canonicalSosWritten) {
            context->ok = false;
            return;
        }
        context->canonicalSosWritten = true;
        (void)write_all(context, kCanonicalSos, sizeof(kCanonicalSos));
        return;
    }

    if (size == static_cast<int>(sizeof(kCanonicalSos)) &&
        std::memcmp(bytes, kCanonicalSos, sizeof(kCanonicalSos)) == 0) {
        if (context->canonicalSosWritten) {
            context->ok = false;
            return;
        }
        context->canonicalSosWritten = true;
    }

    (void)write_all(context, bytes, static_cast<std::size_t>(size));
}

jlongArray result(JNIEnv *env, jlong status, jlong bytes = 0) {
    const jlong values[4] = {kMagic, status, bytes, kRequiredQuality};
    jlongArray out = env->NewLongArray(4);
    if (out != nullptr) env->SetLongArrayRegion(out, 0, 4, values);
    return out;
}
} // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_truthraw_adaptiveui_HighFidelityJpegNativeBridge_encodeRgb24Jpeg444Q100(
    JNIEnv *env,
    jobject,
    jint inputFd,
    jint outputFd,
    jint width,
    jint height,
    jint quality) {
    if (inputFd < 0 || outputFd < 0 || width <= 0 || height <= 0 ||
        width > 65535 || height > 65535 || quality != kRequiredQuality) {
        return result(env, -1);
    }

    const std::uint64_t expectedBytes =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) * 3u;
    if (expectedBytes == 0u ||
        expectedBytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return result(env, -2);
    }

    struct stat sourceStat {};
    if (::fstat(inputFd, &sourceStat) != 0 || sourceStat.st_size < 0 ||
        static_cast<std::uint64_t>(sourceStat.st_size) != expectedBytes) {
        return result(env, -3);
    }

    if (::ftruncate(outputFd, 0) != 0 || ::lseek(outputFd, 0, SEEK_SET) < 0) {
        return result(env, -4);
    }

    void *mapped = ::mmap(nullptr, static_cast<std::size_t>(expectedBytes), PROT_READ, MAP_PRIVATE, inputFd, 0);
    if (mapped == MAP_FAILED) return result(env, -5);

    WriterContext writer;
    writer.fd = outputFd;
    writer.diagnosticApp15 = build_diagnostic_app15(width, height);
    if (writer.diagnosticApp15.empty()) {
        (void)::munmap(mapped, static_cast<std::size_t>(expectedBytes));
        (void)::ftruncate(outputFd, 0);
        return result(env, -8);
    }

    const bool encoded = draw_jpeg444_q100::encodeRgb24(
        fd_writer,
        &writer,
        static_cast<const std::uint8_t *>(mapped),
        width,
        height);
    const int unmapStatus = ::munmap(mapped, static_cast<std::size_t>(expectedBytes));

    if (!encoded || !writer.ok || !writer.canonicalSosWritten ||
        !writer.diagnosticApp15Written || writer.bytes <= 4u ||
        unmapStatus != 0 || ::fsync(outputFd) != 0) {
        (void)::ftruncate(outputFd, 0);
        return result(env, -6);
    }

    struct stat outputStat {};
    if (::fstat(outputFd, &outputStat) != 0 || outputStat.st_size < 0 ||
        static_cast<std::uint64_t>(outputStat.st_size) != writer.bytes) {
        (void)::ftruncate(outputFd, 0);
        return result(env, -7);
    }

    return result(env, 0, static_cast<jlong>(writer.bytes));
}

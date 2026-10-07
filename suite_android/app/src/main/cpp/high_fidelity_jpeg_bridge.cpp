#include <jni.h>

#include "draw_jpeg444_q100_encoder.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
constexpr jlong kMagic = 0x44524a34; // DRJ4
constexpr jint kRequiredQuality = 100;

struct WriterContext {
    int fd = -1;
    bool ok = true;
    bool canonicalSosWritten = false;
    std::uint64_t bytes = 0u;
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

void fd_writer(void *opaque, const void *data, int size) {
    auto *context = static_cast<WriterContext *>(opaque);
    if (context == nullptr || !context->ok || context->fd < 0 || data == nullptr || size < 0) {
        if (context != nullptr) context->ok = false;
        return;
    }

    const auto *bytes = static_cast<const std::uint8_t *>(data);

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
    const bool encoded = draw_jpeg444_q100::encodeRgb24(
        fd_writer,
        &writer,
        static_cast<const std::uint8_t *>(mapped),
        width,
        height);
    const int unmapStatus = ::munmap(mapped, static_cast<std::size_t>(expectedBytes));

    if (!encoded || !writer.ok || !writer.canonicalSosWritten ||
        writer.bytes <= 4u || unmapStatus != 0 || ::fsync(outputFd) != 0) {
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

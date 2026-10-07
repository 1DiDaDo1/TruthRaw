#include <jni.h>

#include <cerrno>
#include <cstdint>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using jo_write_func = void(void *context, const void *data, int size);
extern bool jo_write_jpg_to_func(
    jo_write_func *func,
    void *context,
    const void *data,
    int width,
    int height,
    int comp,
    int quality);

namespace {
constexpr jlong kMagic = 0x44524a34; // DRJ4
constexpr jint kRequiredQuality = 100;

struct WriterContext {
    int fd = -1;
    bool ok = true;
    std::uint64_t bytes = 0u;
};

void fd_writer(void *opaque, const void *data, int size) {
    auto *context = static_cast<WriterContext *>(opaque);
    if (context == nullptr || !context->ok || context->fd < 0 || data == nullptr || size < 0) {
        if (context != nullptr) context->ok = false;
        return;
    }
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    std::size_t remaining = static_cast<std::size_t>(size);
    while (remaining > 0u) {
        const ssize_t written = ::write(context->fd, bytes, remaining);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) {
            context->ok = false;
            return;
        }
        bytes += static_cast<std::size_t>(written);
        remaining -= static_cast<std::size_t>(written);
        context->bytes += static_cast<std::uint64_t>(written);
    }
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
    const bool encoded = jo_write_jpg_to_func(
        fd_writer,
        &writer,
        mapped,
        width,
        height,
        3,
        kRequiredQuality);
    const int unmapStatus = ::munmap(mapped, static_cast<std::size_t>(expectedBytes));

    if (!encoded || !writer.ok || writer.bytes <= 4u || unmapStatus != 0 || ::fsync(outputFd) != 0) {
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

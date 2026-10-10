#include "draw_jpeg444_q100_encoder.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
struct FileWriter {
    std::FILE* file = nullptr;
    bool ok = true;
    bool canonicalSosWritten = false;
};

bool write_all(FileWriter* writer, const void* data, std::size_t size) {
    if (writer == nullptr || writer->file == nullptr || !writer->ok || data == nullptr) {
        if (writer != nullptr) writer->ok = false;
        return false;
    }
    if (size == 0u) return true;
    if (std::fwrite(data, 1u, size, writer->file) != size) {
        writer->ok = false;
        return false;
    }
    return true;
}

void write_file(void* opaque, const void* data, int size) {
    auto* writer = static_cast<FileWriter*>(opaque);
    if (writer == nullptr || writer->file == nullptr || !writer->ok || data == nullptr || size < 0) {
        if (writer != nullptr) writer->ok = false;
        return;
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    static constexpr std::uint8_t kMalformedSos[15] = {
        0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x00,0x3f,0x00,
    };
    static constexpr std::uint8_t kCanonicalSos[14] = {
        0xff,0xda,0x00,0x0c,0x03,0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,
    };

    if (size == static_cast<int>(sizeof(kMalformedSos)) &&
        std::memcmp(bytes, kMalformedSos, sizeof(kMalformedSos)) == 0) {
        if (writer->canonicalSosWritten) {
            writer->ok = false;
            return;
        }
        writer->canonicalSosWritten = true;
        (void)write_all(writer, kCanonicalSos, sizeof(kCanonicalSos));
        return;
    }

    if (size == static_cast<int>(sizeof(kCanonicalSos)) &&
        std::memcmp(bytes, kCanonicalSos, sizeof(kCanonicalSos)) == 0) {
        if (writer->canonicalSosWritten) {
            writer->ok = false;
            return;
        }
        writer->canonicalSosWritten = true;
    }

    (void)write_all(writer, data, static_cast<std::size_t>(size));
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    constexpr int width = 256;
    constexpr int height = 192;
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3u);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t p = (static_cast<std::size_t>(y) * width + x) * 3u;
            const int checker = ((x / 8) ^ (y / 8)) & 1;
            rgb[p] = static_cast<std::uint8_t>((x * 255) / (width - 1));
            rgb[p + 1u] = static_cast<std::uint8_t>((y * 255) / (height - 1));
            rgb[p + 2u] = static_cast<std::uint8_t>(checker ? 245 : ((x + 3 * y) & 0xff));
        }
    }

    std::FILE* file = std::fopen(argv[1], "wb");
    if (file == nullptr) return 3;
    FileWriter writer{file, true, false};
    const bool encoded = draw_jpeg444_q100::encodeRgb24(write_file, &writer, rgb.data(), width, height);
    const int closeStatus = std::fclose(file);
    if (!encoded || !writer.ok || !writer.canonicalSosWritten || closeStatus != 0) return 4;
    return 0;
}

#include "truthraw_dng_certificate_embed_v0_1.h"

#include "truthraw_certificate_v0_1.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <unistd.h>
#include <vector>

namespace truthraw::dng_certificate_embed::v0_1 {
namespace {

constexpr std::uint16_t kTiffByte = 1u;
constexpr std::uint16_t kTagDngPrivateData = 50740u;
constexpr std::uint64_t kClassicTiffLimit = 0xffffffffull;

bool pread_all(int fd, std::uint64_t offset, std::uint8_t* data, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const auto off = static_cast<off_t>(offset + done);
        const ssize_t n = ::pread(fd, data + done, size - done, off);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

bool pwrite_all(int fd, std::uint64_t offset, const std::uint8_t* data, std::size_t size) noexcept {
    std::size_t done = 0u;
    while (done < size) {
        const auto off = static_cast<off_t>(offset + done);
        const ssize_t n = ::pwrite(fd, data + done, size - done, off);
        if (n <= 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

std::uint16_t u16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t u32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}

void put_u32(std::uint8_t* p, std::uint32_t value) noexcept {
    p[0] = static_cast<std::uint8_t>(value & 0xffu);
    p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
    p[2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
    p[3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
}

std::uint64_t align4(std::uint64_t value) noexcept {
    return (value + 3ull) & ~3ull;
}

bool certificate_magic_ok(std::span<const std::uint8_t> certificate) noexcept {
    static constexpr std::array<std::uint8_t, 8> magic{'T','R','C','E','R','T','0','1'};
    return certificate.size() == certificate::v0_1::kSerializedBytes &&
           std::equal(magic.begin(), magic.end(), certificate.begin());
}

}  // namespace

Status embed_certificate(
    int fd,
    std::span<const std::uint8_t> certificate,
    Result& out) noexcept {
    out = {};
    try {
        if (fd < 0 || !certificate_magic_ok(certificate)) {
            return Status::error(StatusCode::InvalidArgument,
                                 "invalid fd or non-canonical TruthRaw Certificate record");
        }

        const off_t end = ::lseek(fd, 0, SEEK_END);
        if (end < 0) return Status::error(StatusCode::ReadFailed, "output fd is not seekable");
        const std::uint64_t originalSize = static_cast<std::uint64_t>(end);
        if (originalSize < 8u || originalSize > kClassicTiffLimit) {
            return Status::error(StatusCode::NotClassicLittleEndianTiff,
                                 "DNG is outside classic TIFF range");
        }

        std::array<std::uint8_t, 8> tiff{};
        if (!pread_all(fd, 0u, tiff.data(), tiff.size())) {
            return Status::error(StatusCode::ReadFailed, "failed reading TIFF header");
        }
        if (tiff[0] != 'I' || tiff[1] != 'I' || u16(tiff.data() + 2u) != 42u) {
            return Status::error(StatusCode::NotClassicLittleEndianTiff,
                                 "expected little-endian classic TIFF/DNG");
        }
        const std::uint32_t ifdOffset = u32(tiff.data() + 4u);
        if (ifdOffset < 8u || static_cast<std::uint64_t>(ifdOffset) + 2u > originalSize) {
            return Status::error(StatusCode::CorruptLayout, "invalid first IFD offset");
        }

        std::array<std::uint8_t, 2> countBytes{};
        if (!pread_all(fd, ifdOffset, countBytes.data(), countBytes.size())) {
            return Status::error(StatusCode::ReadFailed, "failed reading IFD entry count");
        }
        const std::uint16_t entryCount = u16(countBytes.data());
        const std::uint64_t entriesStart = static_cast<std::uint64_t>(ifdOffset) + 2u;
        const std::uint64_t entriesBytes = static_cast<std::uint64_t>(entryCount) * 12u;
        if (entriesStart + entriesBytes + 4u > originalSize) {
            return Status::error(StatusCode::CorruptLayout, "IFD entries exceed file bounds");
        }

        std::uint64_t privateEntryOffset = 0u;
        std::array<std::uint8_t, 12> entry{};
        std::uint32_t oldCount = 0u;
        std::uint32_t oldOffset = 0u;
        for (std::uint16_t i = 0u; i < entryCount; ++i) {
            const std::uint64_t off = entriesStart + static_cast<std::uint64_t>(i) * 12u;
            if (!pread_all(fd, off, entry.data(), entry.size())) {
                return Status::error(StatusCode::ReadFailed, "failed reading IFD entry");
            }
            if (u16(entry.data()) != kTagDngPrivateData) continue;
            if (u16(entry.data() + 2u) != kTiffByte) {
                return Status::error(StatusCode::CorruptLayout,
                                     "DNGPrivateData is not BYTE data");
            }
            oldCount = u32(entry.data() + 4u);
            oldOffset = u32(entry.data() + 8u);
            privateEntryOffset = off;
            break;
        }
        if (privateEntryOffset == 0u) {
            return Status::error(StatusCode::MissingDngPrivateData,
                                 "TruthRaw DNGPrivateData tag is missing");
        }
        if (oldCount <= 4u || oldOffset == 0u ||
            static_cast<std::uint64_t>(oldOffset) + oldCount > originalSize) {
            return Status::error(StatusCode::CorruptLayout,
                                 "existing DNGPrivateData payload is invalid");
        }

        const std::uint64_t newCount64 =
            static_cast<std::uint64_t>(oldCount) + certificate.size();
        if (newCount64 > std::numeric_limits<std::uint32_t>::max()) {
            return Status::error(StatusCode::SizeOverflow, "certificate payload count overflow");
        }
        const std::uint64_t appendOffset = align4(originalSize);
        const std::uint64_t newEnd = appendOffset + newCount64;
        if (newEnd > kClassicTiffLimit) {
            return Status::error(StatusCode::SizeOverflow,
                                 "certified DNG would exceed classic TIFF range");
        }

        std::vector<std::uint8_t> combined(static_cast<std::size_t>(newCount64));
        if (!pread_all(fd, oldOffset, combined.data(), oldCount)) {
            return Status::error(StatusCode::ReadFailed,
                                 "failed reading existing DNGPrivateData payload");
        }
        std::copy(certificate.begin(), certificate.end(),
                  combined.begin() + static_cast<std::ptrdiff_t>(oldCount));

        if (appendOffset > originalSize) {
            const std::array<std::uint8_t, 3> zeroPad{};
            const std::size_t padding = static_cast<std::size_t>(appendOffset - originalSize);
            if (!pwrite_all(fd, originalSize, zeroPad.data(), padding)) {
                return Status::error(StatusCode::WriteFailed, "failed writing DNG alignment padding");
            }
        }
        if (!pwrite_all(fd, appendOffset, combined.data(), combined.size())) {
            (void)::ftruncate(fd, static_cast<off_t>(originalSize));
            return Status::error(StatusCode::WriteFailed,
                                 "failed staging expanded DNGPrivateData payload");
        }

        std::array<std::uint8_t, 8> patch{};
        put_u32(patch.data(), static_cast<std::uint32_t>(newCount64));
        put_u32(patch.data() + 4u, static_cast<std::uint32_t>(appendOffset));
        if (!pwrite_all(fd, privateEntryOffset + 4u, patch.data(), patch.size())) {
            (void)::ftruncate(fd, static_cast<off_t>(originalSize));
            return Status::error(StatusCode::WriteFailed, "failed committing DNGPrivateData IFD patch");
        }

        if (::fsync(fd) != 0 && errno != EINVAL && errno != EROFS) {
            std::array<std::uint8_t, 8> restore{};
            put_u32(restore.data(), oldCount);
            put_u32(restore.data() + 4u, oldOffset);
            (void)pwrite_all(fd, privateEntryOffset + 4u, restore.data(), restore.size());
            (void)::ftruncate(fd, static_cast<off_t>(originalSize));
            (void)::fsync(fd);
            return Status::error(StatusCode::WriteFailed,
                                 "certificate commit fsync failed and was rolled back");
        }

        out.oldPrivateDataBytes = oldCount;
        out.newPrivateDataBytes = static_cast<std::uint32_t>(newCount64);
        out.certificateBytes = static_cast<std::uint32_t>(certificate.size());
        out.certificateBlockOffset = appendOffset + oldCount;
        out.outputBytes = newEnd;
        out.existingPrivateDataPreserved = true;
        out.ifdCommitApplied = true;
        return Status::ok();
    } catch (...) {
        return Status::error(StatusCode::InvalidArgument,
                             "unexpected exception while embedding TruthRaw Certificate");
    }
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::NotClassicLittleEndianTiff: return "NOT_CLASSIC_LITTLE_ENDIAN_TIFF";
        case StatusCode::MissingDngPrivateData: return "MISSING_DNG_PRIVATE_DATA";
        case StatusCode::CorruptLayout: return "CORRUPT_LAYOUT";
        case StatusCode::ReadFailed: return "READ_FAILED";
        case StatusCode::WriteFailed: return "WRITE_FAILED";
        case StatusCode::SizeOverflow: return "SIZE_OVERFLOW";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::dng_certificate_embed::v0_1

#include "decoded_measurement_handoff_v0_1.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>

namespace truthraw::decoded_measurement_handoff::v0_1 {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{'T','R','D','M','H','0','0','1'};
constexpr std::uint32_t kFlagSealed = 1u;
constexpr std::size_t kHeaderCrcOffset = 252;
constexpr std::size_t kIoChunkBytes = 4096;
constexpr std::size_t kSamplesPerChunk = kIoChunkBytes / sizeof(std::uint16_t);
using Header = std::array<std::uint8_t, kHeaderBytes>;

[[nodiscard]] bool hash_nonzero(const Hash256& hash) noexcept {
    return std::any_of(hash.begin(), hash.end(), [](std::uint8_t b) { return b != 0; });
}

[[nodiscard]] bool valid_color(CfaColor c) noexcept {
    return c == CfaColor::Red || c == CfaColor::Green || c == CfaColor::Blue;
}

[[nodiscard]] bool valid_bayer_pattern(const std::array<CfaColor, 4>& cfa) noexcept {
    std::uint32_t r = 0;
    std::uint32_t g = 0;
    std::uint32_t b = 0;
    for (const auto c : cfa) {
        if (!valid_color(c)) return false;
        r += c == CfaColor::Red ? 1u : 0u;
        g += c == CfaColor::Green ? 1u : 0u;
        b += c == CfaColor::Blue ? 1u : 0u;
    }
    return r == 1u && g == 2u && b == 1u;
}

[[nodiscard]] bool mul_u64(std::uint64_t a,
                           std::uint64_t b,
                           std::uint64_t& out) noexcept {
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) return false;
    out = a * b;
    return true;
}

void put_u16(Header& h, std::size_t o, std::uint16_t v) noexcept {
    h[o] = static_cast<std::uint8_t>(v & 0xffu);
    h[o + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put_u32(Header& h, std::size_t o, std::uint32_t v) noexcept {
    for (std::size_t i = 0; i < 4; ++i) {
        h[o + i] = static_cast<std::uint8_t>((v >> (8u * i)) & 0xffu);
    }
}

void put_u64(Header& h, std::size_t o, std::uint64_t v) noexcept {
    for (std::size_t i = 0; i < 8; ++i) {
        h[o + i] = static_cast<std::uint8_t>((v >> (8u * i)) & 0xffu);
    }
}

[[nodiscard]] std::uint16_t get_u16(std::span<const std::uint8_t> h,
                                    std::size_t o) noexcept {
    return static_cast<std::uint16_t>(h[o]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(h[o + 1]) << 8u);
}

[[nodiscard]] std::uint32_t get_u32(std::span<const std::uint8_t> h,
                                    std::size_t o) noexcept {
    std::uint32_t v = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        v |= static_cast<std::uint32_t>(h[o + i]) << (8u * i);
    }
    return v;
}

[[nodiscard]] std::uint64_t get_u64(std::span<const std::uint8_t> h,
                                    std::size_t o) noexcept {
    std::uint64_t v = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        v |= static_cast<std::uint64_t>(h[o + i]) << (8u * i);
    }
    return v;
}

[[nodiscard]] std::uint32_t crc32_update(std::uint32_t state,
                                         std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t crc = state;
    for (const auto byte : bytes) {
        crc ^= static_cast<std::uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return crc;
}

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept {
    return ~crc32_update(0xffffffffu, bytes);
}

[[nodiscard]] bool write_all_at(int fd,
                                std::span<const std::uint8_t> bytes,
                                std::uint64_t offset) noexcept {
    std::size_t done = 0;
    while (done < bytes.size()) {
        const auto off = offset + static_cast<std::uint64_t>(done);
        if (off > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
        const ssize_t n = ::pwrite(fd,
                                   bytes.data() + done,
                                   bytes.size() - done,
                                   static_cast<off_t>(off));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

[[nodiscard]] bool read_all_at(int fd,
                               std::span<std::uint8_t> bytes,
                               std::uint64_t offset) noexcept {
    std::size_t done = 0;
    while (done < bytes.size()) {
        const auto off = offset + static_cast<std::uint64_t>(done);
        if (off > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) return false;
        const ssize_t n = ::pread(fd,
                                  bytes.data() + done,
                                  bytes.size() - done,
                                  static_cast<off_t>(off));
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        done += static_cast<std::size_t>(n);
    }
    return true;
}

[[nodiscard]] bool compute_layout(const Descriptor& d,
                                  std::uint32_t& rowStride,
                                  std::uint64_t& samples,
                                  std::uint64_t& payloadBytes) noexcept {
    std::uint64_t row = 0;
    if (!mul_u64(d.width, sizeof(std::uint16_t), row) ||
        row > std::numeric_limits<std::uint32_t>::max()) return false;
    rowStride = static_cast<std::uint32_t>(row);
    if (!mul_u64(d.width, d.height, samples)) return false;
    if (!mul_u64(rowStride, d.height, payloadBytes)) return false;
    return true;
}

[[nodiscard]] Header serialize_header(const Descriptor& d,
                                      std::uint32_t rowStride,
                                      std::uint64_t payloadBytes,
                                      std::uint32_t payloadCrc,
                                      bool sealed) noexcept {
    Header h{};
    std::copy(kMagic.begin(), kMagic.end(), h.begin());
    put_u16(h, 8, kVersion);
    put_u16(h, 10, static_cast<std::uint16_t>(kHeaderBytes));
    put_u32(h, 12, sealed ? kFlagSealed : 0u);
    put_u32(h, 16, d.width);
    put_u32(h, 20, d.height);
    put_u32(h, 24, rowStride);
    put_u16(h, 28, d.sourceBitDepth);
    h[30] = static_cast<std::uint8_t>(d.topology);
    h[31] = static_cast<std::uint8_t>(d.evidenceClass);
    h[32] = static_cast<std::uint8_t>(d.sourceCompression);
    for (std::size_t i = 0; i < 4; ++i) h[33 + i] = static_cast<std::uint8_t>(d.cfa2x2[i]);
    put_u32(h, 40, d.physicalFrameCount);
    put_u32(h, 44, d.independentEvidenceCount);
    put_u64(h, 48, kHeaderBytes);
    put_u64(h, 56, payloadBytes);
    std::copy(d.sourceEvidenceHash.begin(), d.sourceEvidenceHash.end(), h.begin() + 64);
    std::copy(d.decoderAuditHash.begin(), d.decoderAuditHash.end(), h.begin() + 96);
    put_u32(h, 128, payloadCrc);
    put_u32(h, kHeaderCrcOffset,
            crc32(std::span<const std::uint8_t>(h.data(), kHeaderCrcOffset)));
    return h;
}

[[nodiscard]] Status parse_header(const Header& h, StoreInfo& out) noexcept {
    out = {};
    const std::span<const std::uint8_t> bytes(h.data(), h.size());
    if (!std::equal(kMagic.begin(), kMagic.end(), h.begin())) return Status::CorruptHeader;
    if (get_u16(bytes, 8) != kVersion || get_u16(bytes, 10) != kHeaderBytes) {
        return Status::Unsupported;
    }
    const auto flags = get_u32(bytes, 12);
    if ((flags & ~kFlagSealed) != 0u || (flags & kFlagSealed) == 0u) {
        return Status::CorruptHeader;
    }
    if (get_u32(bytes, kHeaderCrcOffset) !=
        crc32(bytes.first(kHeaderCrcOffset))) return Status::CorruptHeader;
    for (std::size_t i = 37; i < 40; ++i) if (h[i] != 0u) return Status::CorruptHeader;
    for (std::size_t i = 132; i < kHeaderCrcOffset; ++i) if (h[i] != 0u) return Status::CorruptHeader;

    Descriptor d{};
    d.width = get_u32(bytes, 16);
    d.height = get_u32(bytes, 20);
    d.sourceBitDepth = get_u16(bytes, 28);
    d.topology = static_cast<ingress::MeasurementTopology>(h[30]);
    d.evidenceClass = static_cast<ingress::EvidenceClass>(h[31]);
    d.sourceCompression = static_cast<ingress::CompressionSemantics>(h[32]);
    for (std::size_t i = 0; i < 4; ++i) d.cfa2x2[i] = static_cast<CfaColor>(h[33 + i]);
    d.physicalFrameCount = get_u32(bytes, 40);
    d.independentEvidenceCount = get_u32(bytes, 44);
    std::copy_n(h.begin() + 64, kHashBytes, d.sourceEvidenceHash.begin());
    std::copy_n(h.begin() + 96, kHashBytes, d.decoderAuditHash.begin());
    if (!valid_descriptor(d)) return Status::EvidenceInvariantViolation;

    std::uint32_t expectedStride = 0;
    std::uint64_t expectedSamples = 0;
    std::uint64_t expectedPayload = 0;
    if (!compute_layout(d, expectedStride, expectedSamples, expectedPayload)) {
        return Status::CorruptHeader;
    }
    (void)expectedSamples;
    const auto payloadOffset = get_u64(bytes, 48);
    const auto payloadBytes = get_u64(bytes, 56);
    if (payloadOffset != kHeaderBytes ||
        payloadBytes != expectedPayload ||
        get_u32(bytes, 24) != expectedStride) return Status::CorruptHeader;

    out.descriptor = d;
    out.payloadOffset = payloadOffset;
    out.payloadBytes = payloadBytes;
    out.rowStrideBytes = expectedStride;
    out.payloadCrc32 = get_u32(bytes, 128);
    out.sealed = true;
    out.payloadIntegrityVerified = false;
    return Status::Ok;
}

} // namespace

bool valid_descriptor(const Descriptor& d) noexcept {
    if (d.width == 0 || d.height == 0 || d.sourceBitDepth == 0 || d.sourceBitDepth > 16) {
        return false;
    }
    if (d.topology != ingress::MeasurementTopology::Bayer2x2 ||
        d.evidenceClass != ingress::EvidenceClass::LosslessDecodedCertified) {
        return false;
    }
    if (d.sourceCompression != ingress::CompressionSemantics::Uncompressed &&
        d.sourceCompression != ingress::CompressionSemantics::LosslessVerified) {
        return false;
    }
    if (!valid_bayer_pattern(d.cfa2x2) ||
        d.physicalFrameCount != 1u || d.independentEvidenceCount != 1u ||
        !hash_nonzero(d.sourceEvidenceHash) || !hash_nonzero(d.decoderAuditHash)) {
        return false;
    }
    std::uint32_t rowStride = 0;
    std::uint64_t samples = 0;
    std::uint64_t payloadBytes = 0;
    return compute_layout(d, rowStride, samples, payloadBytes) &&
           payloadBytes <= static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()) - kHeaderBytes;
}

Status Writer::begin(int borrowedFd, const Descriptor& descriptor) noexcept {
    if (begun_ || borrowedFd < 0 || !valid_descriptor(descriptor)) return Status::InvalidArgument;
    std::uint32_t rowStride = 0;
    std::uint64_t payloadBytes = 0;
    if (!compute_layout(descriptor, rowStride, samplesExpected_, payloadBytes)) {
        return Status::InvalidArgument;
    }
    const std::uint64_t totalBytes = kHeaderBytes + payloadBytes;
    if (totalBytes > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) {
        return Status::InvalidArgument;
    }
    if (::ftruncate(borrowedFd, static_cast<off_t>(totalBytes)) != 0) return Status::IoError;

    const Header provisional = serialize_header(descriptor, rowStride, payloadBytes, 0u, false);
    if (!write_all_at(borrowedFd,
                      std::span<const std::uint8_t>(provisional.data(), provisional.size()),
                      0)) return Status::IoError;

    fd_ = borrowedFd;
    descriptor_ = descriptor;
    samplesWritten_ = 0;
    crcState_ = 0xffffffffu;
    begun_ = true;
    sealed_ = false;
    return Status::Ok;
}

Status Writer::append_samples(std::span<const std::uint16_t> samples) noexcept {
    if (!begun_ || sealed_) return Status::InvalidState;
    if (samples.size() > samplesExpected_ - samplesWritten_) return Status::BoundsError;

    std::array<std::uint8_t, kIoChunkBytes> encoded{};
    std::size_t consumed = 0;
    while (consumed < samples.size()) {
        const std::size_t count = std::min(kSamplesPerChunk, samples.size() - consumed);
        for (std::size_t i = 0; i < count; ++i) {
            const std::uint16_t v = samples[consumed + i];
            encoded[2 * i] = static_cast<std::uint8_t>(v & 0xffu);
            encoded[2 * i + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
        }
        const std::size_t byteCount = count * sizeof(std::uint16_t);
        const std::span<const std::uint8_t> bytes(encoded.data(), byteCount);
        const std::uint64_t offset = kHeaderBytes +
            samplesWritten_ * sizeof(std::uint16_t);
        if (!write_all_at(fd_, bytes, offset)) return Status::IoError;
        crcState_ = crc32_update(crcState_, bytes);
        samplesWritten_ += count;
        consumed += count;
    }
    return Status::Ok;
}

Status Writer::seal() noexcept {
    if (!begun_ || sealed_ || samplesWritten_ != samplesExpected_) return Status::InvalidState;
    std::uint32_t rowStride = 0;
    std::uint64_t samples = 0;
    std::uint64_t payloadBytes = 0;
    if (!compute_layout(descriptor_, rowStride, samples, payloadBytes) || samples != samplesExpected_) {
        return Status::InvalidState;
    }
    const Header finalHeader = serialize_header(
        descriptor_, rowStride, payloadBytes, ~crcState_, true);
    if (!write_all_at(fd_,
                      std::span<const std::uint8_t>(finalHeader.data(), finalHeader.size()),
                      0)) return Status::IoError;
    if (::fsync(fd_) != 0) return Status::IoError;
    sealed_ = true;
    return Status::Ok;
}

std::size_t Writer::resident_bytes_upper_bound() const noexcept {
    return sizeof(Writer) + kIoChunkBytes + kHeaderBytes;
}

Status Reader::open(int borrowedFd) noexcept {
    if (open_ || borrowedFd < 0) return Status::InvalidArgument;
    Header header{};
    if (!read_all_at(borrowedFd, std::span<std::uint8_t>(header.data(), header.size()), 0)) {
        return Status::IoError;
    }
    StoreInfo parsed{};
    const Status parsedStatus = parse_header(header, parsed);
    if (parsedStatus != Status::Ok) return parsedStatus;

    struct stat st {};
    if (::fstat(borrowedFd, &st) != 0 || st.st_size < 0) return Status::IoError;
    const std::uint64_t expected = parsed.payloadOffset + parsed.payloadBytes;
    if (static_cast<std::uint64_t>(st.st_size) != expected) return Status::CorruptHeader;

    fd_ = borrowedFd;
    info_ = parsed;
    open_ = true;
    return Status::Ok;
}

Status Reader::verify_payload_integrity() noexcept {
    if (!open_) return Status::InvalidState;
    std::array<std::uint8_t, kIoChunkBytes> buffer{};
    std::uint64_t offset = 0;
    std::uint32_t state = 0xffffffffu;
    while (offset < info_.payloadBytes) {
        const std::size_t count = static_cast<std::size_t>(
            std::min<std::uint64_t>(kIoChunkBytes, info_.payloadBytes - offset));
        std::span<std::uint8_t> chunk(buffer.data(), count);
        if (!read_all_at(fd_, chunk, info_.payloadOffset + offset)) return Status::IoError;
        state = crc32_update(state, std::span<const std::uint8_t>(chunk.data(), chunk.size()));
        offset += count;
    }
    if ((~state) != info_.payloadCrc32) return Status::CorruptPayload;
    info_.payloadIntegrityVerified = true;
    return Status::Ok;
}

Status Reader::read_rect(std::uint32_t x,
                         std::uint32_t y,
                         std::uint32_t width,
                         std::uint32_t height,
                         std::span<std::uint16_t> out) const noexcept {
    if (!open_ || !info_.payloadIntegrityVerified) return Status::InvalidState;
    if (width == 0 || height == 0 ||
        x > info_.descriptor.width || y > info_.descriptor.height ||
        width > info_.descriptor.width - x ||
        height > info_.descriptor.height - y) return Status::BoundsError;

    std::uint64_t outputSamples = 0;
    if (!mul_u64(width, height, outputSamples) || out.size() < outputSamples) {
        return Status::BoundsError;
    }

    std::array<std::uint8_t, kIoChunkBytes> bytes{};
    std::size_t dst = 0;
    for (std::uint32_t row = 0; row < height; ++row) {
        std::uint32_t remaining = width;
        std::uint32_t col = 0;
        while (remaining != 0) {
            const std::uint32_t sampleCount = std::min<std::uint32_t>(
                remaining, static_cast<std::uint32_t>(kSamplesPerChunk));
            const std::size_t byteCount = static_cast<std::size_t>(sampleCount) * 2u;
            const std::uint64_t fileOffset = info_.payloadOffset +
                static_cast<std::uint64_t>(y + row) * info_.rowStrideBytes +
                static_cast<std::uint64_t>(x + col) * 2u;
            std::span<std::uint8_t> chunk(bytes.data(), byteCount);
            if (!read_all_at(fd_, chunk, fileOffset)) return Status::IoError;
            for (std::uint32_t i = 0; i < sampleCount; ++i) {
                out[dst++] = static_cast<std::uint16_t>(bytes[2u * i]) |
                    static_cast<std::uint16_t>(
                        static_cast<std::uint16_t>(bytes[2u * i + 1u]) << 8u);
            }
            col += sampleCount;
            remaining -= sampleCount;
        }
    }
    return Status::Ok;
}

std::size_t Reader::resident_bytes_upper_bound() const noexcept {
    return sizeof(Reader) + kIoChunkBytes + kHeaderBytes;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidArgument: return "INVALID_ARGUMENT";
        case Status::InvalidState: return "INVALID_STATE";
        case Status::Unsupported: return "UNSUPPORTED";
        case Status::IoError: return "IO_ERROR";
        case Status::CorruptHeader: return "CORRUPT_HEADER";
        case Status::CorruptPayload: return "CORRUPT_PAYLOAD";
        case Status::BoundsError: return "BOUNDS_ERROR";
        case Status::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
    }
    return "UNKNOWN";
}

} // namespace truthraw::decoded_measurement_handoff::v0_1

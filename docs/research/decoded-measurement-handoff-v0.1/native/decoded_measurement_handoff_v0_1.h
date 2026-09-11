#pragma once

#include "professional_raw_ingress_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace truthraw::decoded_measurement_handoff::v0_1 {

namespace ingress = truthraw::professional_raw_ingress::v0_1;

constexpr std::size_t kHashBytes = 32;
constexpr std::size_t kHeaderBytes = 256;
constexpr std::uint16_t kVersion = 1;
using Hash256 = std::array<std::uint8_t, kHashBytes>;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    InvalidState,
    Unsupported,
    IoError,
    CorruptHeader,
    CorruptPayload,
    BoundsError,
    EvidenceInvariantViolation,
};

enum class CfaColor : std::uint8_t {
    Red = 0,
    Green = 1,
    Blue = 2,
};

struct Descriptor {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint16_t sourceBitDepth = 0;
    ingress::MeasurementTopology topology = ingress::MeasurementTopology::Unknown;
    ingress::EvidenceClass evidenceClass = ingress::EvidenceClass::FailClosedUnsupported;
    ingress::CompressionSemantics sourceCompression = ingress::CompressionSemantics::Unknown;
    std::array<CfaColor, 4> cfa2x2{
        CfaColor::Red, CfaColor::Green,
        CfaColor::Green, CfaColor::Blue};
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    Hash256 sourceEvidenceHash{};
    Hash256 decoderAuditHash{};
};

struct StoreInfo {
    Descriptor descriptor{};
    std::uint64_t payloadOffset = kHeaderBytes;
    std::uint64_t payloadBytes = 0;
    std::uint32_t rowStrideBytes = 0;
    std::uint32_t payloadCrc32 = 0;
    bool sealed = false;
    bool payloadIntegrityVerified = false;
};

class Writer final {
public:
    Writer() = default;
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    [[nodiscard]] Status begin(int borrowedFd, const Descriptor& descriptor) noexcept;
    [[nodiscard]] Status append_samples(std::span<const std::uint16_t> samples) noexcept;
    [[nodiscard]] Status seal() noexcept;

    [[nodiscard]] std::uint64_t samples_written() const noexcept { return samplesWritten_; }
    [[nodiscard]] std::uint64_t samples_expected() const noexcept { return samplesExpected_; }
    [[nodiscard]] bool sealed() const noexcept { return sealed_; }
    [[nodiscard]] std::size_t resident_bytes_upper_bound() const noexcept;

private:
    int fd_ = -1;
    Descriptor descriptor_{};
    std::uint64_t samplesWritten_ = 0;
    std::uint64_t samplesExpected_ = 0;
    std::uint32_t crcState_ = 0xffffffffu;
    bool begun_ = false;
    bool sealed_ = false;
};

class Reader final {
public:
    Reader() = default;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    [[nodiscard]] Status open(int borrowedFd) noexcept;
    [[nodiscard]] Status verify_payload_integrity() noexcept;
    [[nodiscard]] Status read_rect(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        std::span<std::uint16_t> out) const noexcept;

    [[nodiscard]] const StoreInfo& info() const noexcept { return info_; }
    [[nodiscard]] bool open_ok() const noexcept { return open_; }
    [[nodiscard]] std::size_t resident_bytes_upper_bound() const noexcept;

private:
    int fd_ = -1;
    StoreInfo info_{};
    bool open_ = false;
};

[[nodiscard]] bool valid_descriptor(const Descriptor& descriptor) noexcept;
[[nodiscard]] const char* status_name(Status status) noexcept;

} // namespace truthraw::decoded_measurement_handoff::v0_1

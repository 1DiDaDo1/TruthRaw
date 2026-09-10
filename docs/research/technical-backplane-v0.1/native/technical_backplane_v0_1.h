#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace truthraw::technical_backplane::v0_1 {

constexpr std::size_t kRoomCount = 12;
constexpr std::size_t kHashBytes = 32;
constexpr std::size_t kSerializedBytes = 180;
constexpr std::uint16_t kVersion = 1;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    EvidenceInvariantViolation,
    MutableScientificState,
    InvalidStatus,
    CorruptRecord,
    UnsupportedVersion
};

enum class RoomStatus : std::uint8_t {
    Available = 0,
    ResearchOnly = 1,
    BlockedMissingEvidence = 2,
    IdentityFallback = 3,
    Rejected = 4
};

enum class ClaimStatus : std::uint8_t {
    Open = 0,
    Candidate = 1,
    Rejected = 2,
    Promoted = 3
};

enum ForbiddenFlag : std::uint32_t {
    ScientificMasterModified = 1u << 0,
    ZeroLineModified = 1u << 1,
    AppearanceCountedAsEvidence = 1u << 2,
    CounterfactualCountedAsEvidence = 1u << 3
};

using Hash256 = std::array<std::uint8_t, kHashBytes>;
using SerializedBackplane = std::array<std::uint8_t, kSerializedBytes>;

struct State {
    Hash256 sourceEvidenceHash{};
    Hash256 scientificMasterHash{};
    Hash256 zeroLineHash{};
    Hash256 sceneScaleHash{};
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
    std::array<RoomStatus, kRoomCount> roomStatus{};
    ClaimStatus claimStatus = ClaimStatus::Open;
    std::uint32_t forbiddenFlags = 0;
};

Status validate(const State& state) noexcept;
Status serialize(const State& state, SerializedBackplane& out) noexcept;
Status deserialize(std::span<const std::uint8_t> bytes, State& out) noexcept;
std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept;
const char* status_name(Status status) noexcept;

} // namespace truthraw::technical_backplane::v0_1

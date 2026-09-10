#include "technical_backplane_v0_1.h"

#include <algorithm>

namespace truthraw::technical_backplane::v0_1 {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{'T','R','B','A','C','K','0','1'};
constexpr std::size_t kCrcOffset = 176;

bool hash_nonzero(const Hash256& h) noexcept {
    return std::any_of(h.begin(), h.end(), [](std::uint8_t b) { return b != 0; });
}

bool valid_room_status(RoomStatus s) noexcept {
    return static_cast<std::uint8_t>(s) <= static_cast<std::uint8_t>(RoomStatus::Rejected);
}

bool valid_claim_status(ClaimStatus s) noexcept {
    return static_cast<std::uint8_t>(s) <= static_cast<std::uint8_t>(ClaimStatus::Promoted);
}

void put_u16_le(SerializedBackplane& b, std::size_t o, std::uint16_t v) noexcept {
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put_u32_le(SerializedBackplane& b, std::size_t o, std::uint32_t v) noexcept {
    b[o] = static_cast<std::uint8_t>(v & 0xffu);
    b[o + 1] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    b[o + 2] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    b[o + 3] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::uint16_t get_u16_le(std::span<const std::uint8_t> b, std::size_t o) noexcept {
    return static_cast<std::uint16_t>(b[o]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[o + 1]) << 8u);
}

std::uint32_t get_u32_le(std::span<const std::uint8_t> b, std::size_t o) noexcept {
    return static_cast<std::uint32_t>(b[o]) |
           (static_cast<std::uint32_t>(b[o + 1]) << 8u) |
           (static_cast<std::uint32_t>(b[o + 2]) << 16u) |
           (static_cast<std::uint32_t>(b[o + 3]) << 24u);
}

void copy_hash_out(SerializedBackplane& b, std::size_t o, const Hash256& h) noexcept {
    std::copy(h.begin(), h.end(), b.begin() + static_cast<std::ptrdiff_t>(o));
}

void copy_hash_in(std::span<const std::uint8_t> b, std::size_t o, Hash256& h) noexcept {
    std::copy_n(b.begin() + static_cast<std::ptrdiff_t>(o), kHashBytes, h.begin());
}

} // namespace

Status validate(const State& state) noexcept {
    if (!hash_nonzero(state.sourceEvidenceHash) || !hash_nonzero(state.scientificMasterHash) ||
        !hash_nonzero(state.zeroLineHash) || !hash_nonzero(state.sceneScaleHash)) {
        return Status::InvalidInput;
    }
    if (state.physicalFrameCount != 1u || state.independentEvidenceCount != 1u) {
        return Status::EvidenceInvariantViolation;
    }
    if (state.forbiddenFlags != 0u) return Status::MutableScientificState;
    if (!valid_claim_status(state.claimStatus)) return Status::InvalidStatus;
    for (const auto s : state.roomStatus) {
        if (!valid_room_status(s)) return Status::InvalidStatus;
    }
    return Status::Ok;
}

std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t crc = 0xffffffffu;
    for (const auto byte : bytes) {
        crc ^= static_cast<std::uint32_t>(byte);
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xedb88320u & mask);
        }
    }
    return ~crc;
}

Status serialize(const State& state, SerializedBackplane& out) noexcept {
    const Status valid = validate(state);
    if (valid != Status::Ok) return valid;

    out.fill(0);
    std::copy(kMagic.begin(), kMagic.end(), out.begin());
    put_u16_le(out, 8, kVersion);
    put_u16_le(out, 10, static_cast<std::uint16_t>(kSerializedBytes));
    put_u32_le(out, 12, state.forbiddenFlags);
    copy_hash_out(out, 16, state.sourceEvidenceHash);
    copy_hash_out(out, 48, state.scientificMasterHash);
    copy_hash_out(out, 80, state.zeroLineHash);
    copy_hash_out(out, 112, state.sceneScaleHash);
    put_u32_le(out, 144, state.physicalFrameCount);
    put_u32_le(out, 148, state.independentEvidenceCount);
    for (std::size_t i = 0; i < kRoomCount; ++i) {
        out[152 + i] = static_cast<std::uint8_t>(state.roomStatus[i]);
    }
    out[164] = static_cast<std::uint8_t>(state.claimStatus);
    const std::uint32_t crc = crc32(std::span<const std::uint8_t>(out.data(), kCrcOffset));
    put_u32_le(out, kCrcOffset, crc);
    return Status::Ok;
}

Status deserialize(std::span<const std::uint8_t> bytes, State& out) noexcept {
    out = {};
    if (bytes.size() != kSerializedBytes) return Status::InvalidInput;
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) return Status::CorruptRecord;
    if (get_u16_le(bytes, 8) != kVersion || get_u16_le(bytes, 10) != kSerializedBytes) {
        return Status::UnsupportedVersion;
    }
    if (get_u32_le(bytes, kCrcOffset) != crc32(bytes.first(kCrcOffset))) return Status::CorruptRecord;
    for (std::size_t i = 165; i < kCrcOffset; ++i) {
        if (bytes[i] != 0u) return Status::CorruptRecord;
    }

    copy_hash_in(bytes, 16, out.sourceEvidenceHash);
    copy_hash_in(bytes, 48, out.scientificMasterHash);
    copy_hash_in(bytes, 80, out.zeroLineHash);
    copy_hash_in(bytes, 112, out.sceneScaleHash);
    out.physicalFrameCount = get_u32_le(bytes, 144);
    out.independentEvidenceCount = get_u32_le(bytes, 148);
    for (std::size_t i = 0; i < kRoomCount; ++i) {
        out.roomStatus[i] = static_cast<RoomStatus>(bytes[152 + i]);
    }
    out.claimStatus = static_cast<ClaimStatus>(bytes[164]);
    out.forbiddenFlags = get_u32_le(bytes, 12);
    return validate(out);
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
        case Status::MutableScientificState: return "MUTABLE_SCIENTIFIC_STATE";
        case Status::InvalidStatus: return "INVALID_STATUS";
        case Status::CorruptRecord: return "CORRUPT_RECORD";
        case Status::UnsupportedVersion: return "UNSUPPORTED_VERSION";
    }
    return "UNKNOWN";
}

} // namespace truthraw::technical_backplane::v0_1

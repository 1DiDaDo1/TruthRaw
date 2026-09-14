#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace truthraw::dng_certificate_embed::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    NotClassicLittleEndianTiff,
    MissingDngPrivateData,
    CorruptLayout,
    ReadFailed,
    WriteFailed,
    SizeOverflow,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

struct Result final {
    std::uint32_t oldPrivateDataBytes = 0u;
    std::uint32_t newPrivateDataBytes = 0u;
    std::uint32_t certificateBytes = 0u;
    std::uint64_t certificateBlockOffset = 0u;
    std::uint64_t outputBytes = 0u;
    bool existingPrivateDataPreserved = false;
    bool ifdCommitApplied = false;
};

// Extends the existing DNGPrivateData payload of the TruthRaw float32 DNG and
// appends a canonical TruthRaw Certificate record inside that tag. Pixel data
// is never rewritten. The new payload is written first; the IFD count/offset
// patch is the commit point. On a post-commit fsync failure the original IFD
// values and file length are restored best-effort before returning failure.
Status embed_certificate(
    int fd,
    std::span<const std::uint8_t> certificate,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::dng_certificate_embed::v0_1

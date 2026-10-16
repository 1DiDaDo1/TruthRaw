#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>

namespace truthraw::raw_ifd_authority::v0_1 {

using SourceSha256 = std::array<std::uint8_t, 32>;

struct Authority final {
    SourceSha256 sourceSha256{};
    std::uint32_t rawIfdOffset = 0;
};

inline std::mutex& registryMutex() noexcept {
    static std::mutex mutex;
    return mutex;
}

inline std::map<SourceSha256, std::uint32_t>& registry() noexcept {
    static std::map<SourceSha256, std::uint32_t> values;
    return values;
}

// Idempotent for the same source/offset pair. A conflicting offset for an
// already-bound sealed source is rejected instead of silently replacing
// authority. Classic-TIFF/DNG raw-IFD offsets are uint32 by construction; a
// wider value is invalid rather than silently narrowed. This registry carries
// identity only; it never grants radiometric or sample-domain authority.
inline bool registerValidated(
    const SourceSha256& sourceSha256,
    std::uint64_t rawIfdOffset) noexcept {
    if (rawIfdOffset == 0u ||
        rawIfdOffset > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    const auto boundedOffset = static_cast<std::uint32_t>(rawIfdOffset);
    std::lock_guard<std::mutex> lock(registryMutex());
    auto& values = registry();
    const auto found = values.find(sourceSha256);
    if (found == values.end()) {
        values.emplace(sourceSha256, boundedOffset);
        return true;
    }
    return found->second == boundedOffset;
}

inline bool lookupValidated(
    const SourceSha256& sourceSha256,
    Authority& out) noexcept {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = registry().find(sourceSha256);
    if (found == registry().end() || found->second == 0u) return false;
    out.sourceSha256 = sourceSha256;
    out.rawIfdOffset = found->second;
    return true;
}

} // namespace truthraw::raw_ifd_authority::v0_1

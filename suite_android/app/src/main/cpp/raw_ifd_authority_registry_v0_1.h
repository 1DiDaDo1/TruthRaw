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

// Read-only observation of what the native DNG open boundary actually did for
// one sealed source. This state never participates in source selection or
// scientific admission; it exists only so the already-enforced runtime checks
// can be exported for real-device diagnostics.
struct RuntimeObservation final {
    std::uint32_t validatedRawIfdOffset = 0;
    std::uint32_t requestedRawIfdOffset = 0;
    std::uint32_t decoderSourceAuditRawIfdOffset = 0;
    // -1 = no explicit authority comparison available, 0 = mismatch, 1 = match.
    std::int32_t authorityMatchState = -1;
    bool decoderOpenObserved = false;
    bool sourceOpenSucceeded = false;
    std::int32_t sourceOpenResultCode = -1;
};

inline std::mutex& registryMutex() noexcept {
    static std::mutex mutex;
    return mutex;
}

inline std::map<SourceSha256, std::uint32_t>& registry() noexcept {
    static std::map<SourceSha256, std::uint32_t> values;
    return values;
}

inline std::map<SourceSha256, RuntimeObservation>& runtimeObservationRegistry() noexcept {
    static std::map<SourceSha256, RuntimeObservation> values;
    return values;
}

// Last recorded adapter-open observation is diagnostic process state, not
// source authority. A later open for another source may replace this snapshot.
// It never participates in source selection or scientific validation.
struct LastOpenSnapshot final {
    bool present = false;
    SourceSha256 sourceSha256{};
    RuntimeObservation observation{};
};

inline LastOpenSnapshot& lastOpenSnapshot() noexcept {
    static LastOpenSnapshot state;
    return state;
}

struct RegistryProbe final {
    bool validatedAuthorityRegistered = false;
    std::uint32_t validatedRawIfdOffset = 0;
    bool requestedSourceObservationPresent = false;
    LastOpenSnapshot lastOpen{};
};

inline void lookupRegistryProbe(
    const SourceSha256& sourceSha256,
    RegistryProbe& out) noexcept {
    std::lock_guard<std::mutex> lock(registryMutex());
    out = {};
    const auto authority = registry().find(sourceSha256);
    if (authority != registry().end() && authority->second != 0u) {
        out.validatedAuthorityRegistered = true;
        out.validatedRawIfdOffset = authority->second;
    }
    out.requestedSourceObservationPresent =
        runtimeObservationRegistry().find(sourceSha256) !=
        runtimeObservationRegistry().end();
    out.lastOpen = lastOpenSnapshot();
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

inline void recordRuntimeObservation(
    const SourceSha256& sourceSha256,
    const RuntimeObservation& observation) noexcept {
    std::lock_guard<std::mutex> lock(registryMutex());
    runtimeObservationRegistry()[sourceSha256] = observation;
    // Mirror only the already-recorded adapter result for troubleshooting.
    // This last-open marker is never read by scientific selection.
    auto& last = lastOpenSnapshot();
    last.present = true;
    last.sourceSha256 = sourceSha256;
    last.observation = observation;
}

inline bool lookupRuntimeObservation(
    const SourceSha256& sourceSha256,
    RuntimeObservation& out) noexcept {
    std::lock_guard<std::mutex> lock(registryMutex());
    const auto found = runtimeObservationRegistry().find(sourceSha256);
    if (found == runtimeObservationRegistry().end()) return false;
    out = found->second;
    return true;
}

} // namespace truthraw::raw_ifd_authority::v0_1

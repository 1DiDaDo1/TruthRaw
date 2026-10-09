#pragma once

#include <atomic>
#include <cstdint>

namespace truthraw::streaming_v0_1::presentation_diagnostics_v0_1 {

// Temporary diagnostics for the downstream SDR presentation path. These
// counters observe values only; they never participate in reconstruction,
// colour transforms, exposure, authority, or pixel output decisions.
struct Snapshot final {
    std::uint64_t preClampNegativeAny = 0u;
    std::uint64_t preClampGreenNegativeRbPositive = 0u;
    std::uint64_t preClampGreenStrictMin = 0u;
    std::uint64_t postClampGreenBoundaryRbPositive = 0u;
};

namespace detail {
inline std::atomic<std::uint64_t> preClampNegativeAny{0u};
inline std::atomic<std::uint64_t> preClampGreenNegativeRbPositive{0u};
inline std::atomic<std::uint64_t> preClampGreenStrictMin{0u};
inline std::atomic<std::uint64_t> postClampGreenBoundaryRbPositive{0u};
} // namespace detail

inline void observe_pre_clamp(float r, float g, float b) noexcept {
    if (r < 0.0f || g < 0.0f || b < 0.0f) {
        detail::preClampNegativeAny.fetch_add(1u, std::memory_order_relaxed);
    }
    if (g < 0.0f && r > 0.0f && b > 0.0f) {
        detail::preClampGreenNegativeRbPositive.fetch_add(1u, std::memory_order_relaxed);
    }
    if (g < r && g < b) {
        detail::preClampGreenStrictMin.fetch_add(1u, std::memory_order_relaxed);
    }
}

inline void observe_post_clamp(float r, float g, float b) noexcept {
    if (g <= 1.0e-7f && r > 0.0f && b > 0.0f) {
        detail::postClampGreenBoundaryRbPositive.fetch_add(1u, std::memory_order_relaxed);
    }
}

inline Snapshot take_snapshot_and_reset() noexcept {
    Snapshot out{};
    out.preClampNegativeAny =
        detail::preClampNegativeAny.exchange(0u, std::memory_order_relaxed);
    out.preClampGreenNegativeRbPositive =
        detail::preClampGreenNegativeRbPositive.exchange(0u, std::memory_order_relaxed);
    out.preClampGreenStrictMin =
        detail::preClampGreenStrictMin.exchange(0u, std::memory_order_relaxed);
    out.postClampGreenBoundaryRbPositive =
        detail::postClampGreenBoundaryRbPositive.exchange(0u, std::memory_order_relaxed);
    return out;
}

} // namespace truthraw::streaming_v0_1::presentation_diagnostics_v0_1

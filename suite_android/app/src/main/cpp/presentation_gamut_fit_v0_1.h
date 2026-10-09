#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace truthraw::presentation_gamut_fit::v0_1 {

// Display/presentation-only gamut fit for linear sRGB / Rec.709 primaries.
//
// Evidence boundary:
// - does not change sealed source data;
// - does not change Scientific Master;
// - does not create or promote scientific authority;
// - is only permitted when producing DERIVED_PRESENTATION_OUTPUT.
//
// Design rule:
// - values already inside the admitted target interval are returned unchanged;
// - out-of-gamut chroma is compressed along the line to the neutral axis;
// - when the source luminance itself is representable, Rec.709 luminance is
//   preserved by construction while chroma is reduced only as much as needed.

constexpr double kLumaR = 0.2126;
constexpr double kLumaG = 0.7152;
constexpr double kLumaB = 0.0722;
constexpr double kEpsilon = 1e-12;

// Temporary presentation diagnostics for the real-device colour/highlight
// investigation. They are observation-only counters: no value below is used by
// either gamut transform and no scientific state or authority is modified.
struct DiagnosticsSnapshot final {
    std::uint64_t nonnegativeNegativeInput = 0u;
    std::uint64_t nonnegativeGreenNegativeRbPositive = 0u;
    std::uint64_t nonnegativeGreenBoundaryAfterFit = 0u;
    std::uint64_t unitCalls = 0u;
    std::uint64_t unitOutOfRangeInput = 0u;
    std::uint64_t unitGreenNegativeRbPositive = 0u;
};

namespace diagnostics_detail {
inline std::atomic<std::uint64_t> nonnegativeNegativeInput{0u};
inline std::atomic<std::uint64_t> nonnegativeGreenNegativeRbPositive{0u};
inline std::atomic<std::uint64_t> nonnegativeGreenBoundaryAfterFit{0u};
inline std::atomic<std::uint64_t> unitCalls{0u};
inline std::atomic<std::uint64_t> unitOutOfRangeInput{0u};
inline std::atomic<std::uint64_t> unitGreenNegativeRbPositive{0u};
} // namespace diagnostics_detail

inline DiagnosticsSnapshot take_diagnostics_snapshot_and_reset() noexcept {
    DiagnosticsSnapshot out{};
    out.nonnegativeNegativeInput =
        diagnostics_detail::nonnegativeNegativeInput.exchange(0u, std::memory_order_relaxed);
    out.nonnegativeGreenNegativeRbPositive =
        diagnostics_detail::nonnegativeGreenNegativeRbPositive.exchange(0u, std::memory_order_relaxed);
    out.nonnegativeGreenBoundaryAfterFit =
        diagnostics_detail::nonnegativeGreenBoundaryAfterFit.exchange(0u, std::memory_order_relaxed);
    out.unitCalls =
        diagnostics_detail::unitCalls.exchange(0u, std::memory_order_relaxed);
    out.unitOutOfRangeInput =
        diagnostics_detail::unitOutOfRangeInput.exchange(0u, std::memory_order_relaxed);
    out.unitGreenNegativeRbPositive =
        diagnostics_detail::unitGreenNegativeRbPositive.exchange(0u, std::memory_order_relaxed);
    return out;
}

inline bool finite_rgb(float r, float g, float b) noexcept {
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

inline double luminance709(float r, float g, float b) noexcept {
    return kLumaR * static_cast<double>(r) +
           kLumaG * static_cast<double>(g) +
           kLumaB * static_cast<double>(b);
}

inline bool fit_nonnegative_preserve_luminance(
    float& r,
    float& g,
    float& b) noexcept {
    if (!finite_rgb(r, g, b)) return false;

    const bool negativeInput = r < 0.0f || g < 0.0f || b < 0.0f;
    const bool greenNegativeRbPositive = g < 0.0f && r > 0.0f && b > 0.0f;
    if (negativeInput) {
        diagnostics_detail::nonnegativeNegativeInput.fetch_add(1u, std::memory_order_relaxed);
    }
    if (greenNegativeRbPositive) {
        diagnostics_detail::nonnegativeGreenNegativeRbPositive.fetch_add(
            1u, std::memory_order_relaxed);
    }

    if (!negativeInput) return true;

    const double y = luminance709(r, g, b);
    if (!std::isfinite(y)) return false;

    // No positive display luminance exists to preserve. Black is the unique
    // neutral non-negative result at/under the display black boundary.
    if (y <= kEpsilon) {
        r = g = b = 0.0f;
        return true;
    }

    double t = 1.0;
    const auto constrain_lower = [&](float c) noexcept {
        if (c < 0.0f) {
            const double cd = static_cast<double>(c);
            const double denom = y - cd;
            if (denom > kEpsilon) {
                t = std::min(t, y / denom);
            }
        }
    };
    constrain_lower(r);
    constrain_lower(g);
    constrain_lower(b);
    t = std::clamp(t, 0.0, 1.0);

    const auto fit = [&](float c) noexcept -> float {
        const double out = y + t * (static_cast<double>(c) - y);
        return static_cast<float>(std::max(0.0, out));
    };
    r = fit(r);
    g = fit(g);
    b = fit(b);
    if (greenNegativeRbPositive && g <= 1.0e-7f && r > 0.0f && b > 0.0f) {
        diagnostics_detail::nonnegativeGreenBoundaryAfterFit.fetch_add(
            1u, std::memory_order_relaxed);
    }
    return finite_rgb(r, g, b);
}

inline bool fit_unit_rgb_preserve_luminance(
    float& r,
    float& g,
    float& b) noexcept {
    if (!finite_rgb(r, g, b)) return false;

    diagnostics_detail::unitCalls.fetch_add(1u, std::memory_order_relaxed);
    const bool outOfRange =
        r < 0.0f || r > 1.0f ||
        g < 0.0f || g > 1.0f ||
        b < 0.0f || b > 1.0f;
    if (outOfRange) {
        diagnostics_detail::unitOutOfRangeInput.fetch_add(1u, std::memory_order_relaxed);
    }
    if (g < 0.0f && r > 0.0f && b > 0.0f) {
        diagnostics_detail::unitGreenNegativeRbPositive.fetch_add(
            1u, std::memory_order_relaxed);
    }

    if (!outOfRange) return true;

    const double y = luminance709(r, g, b);
    if (!std::isfinite(y)) return false;

    if (y <= kEpsilon) {
        r = g = b = 0.0f;
        return true;
    }
    if (y >= 1.0 - kEpsilon) {
        r = g = b = 1.0f;
        return true;
    }

    double t = 1.0;
    const auto constrain = [&](float c) noexcept {
        const double cd = static_cast<double>(c);
        const double d = cd - y;
        if (cd < 0.0 && d < -kEpsilon) {
            t = std::min(t, y / (-d));
        } else if (cd > 1.0 && d > kEpsilon) {
            t = std::min(t, (1.0 - y) / d);
        }
    };
    constrain(r);
    constrain(g);
    constrain(b);
    t = std::clamp(t, 0.0, 1.0);

    const auto fit = [&](float c) noexcept -> float {
        const double out = y + t * (static_cast<double>(c) - y);
        return static_cast<float>(std::clamp(out, 0.0, 1.0));
    };
    r = fit(r);
    g = fit(g);
    b = fit(b);
    return finite_rgb(r, g, b);
}

} // namespace truthraw::presentation_gamut_fit::v0_1

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace truthraw::presentation_highlight_chroma_rolloff::v0_1 {

// Presentation-only support-aware highlight authority observer.
//
// IMPORTANT 2026-10-09 real-device correction:
// the previous candidate contracted RGB chroma toward the Rec.709 luminance
// axis for selected near-white/censored highlights. Real-device tele evidence
// showed that this did not solve the broad purple highlight failure and could
// remove colour information that may still be valid. Therefore this stage is
// now pixel-preserving pass-through.
//
// The useful architecture is retained:
// - `censored` is the stricter reconstruction-support highlight authority;
// - it is consumed only at this final highlight-colour boundary;
// - HDR, detail/acutance, restoration and Natural Light Local Field keep using
//   their historical centre-sample censor state;
// - no source/Scientific-Master mutation, evidence creation or promotion.
//
// We continue to count the old candidate signatures diagnostically so a real
// device can show where a hypothetical contraction would have triggered, while
// guaranteeing that diagnostics never modify RGB.

constexpr double kLumaR = 0.2126;
constexpr double kLumaG = 0.7152;
constexpr double kLumaB = 0.0722;

struct DiagnosticsSnapshot final {
    std::uint64_t censoredCalls = 0u;
    std::uint64_t censoredGreenStrictMinInput = 0u;
    std::uint64_t censoredWhiteCandidates = 0u;
    std::uint64_t censoredSevereCandidates = 0u;
    std::uint64_t censoredNearNeutralCandidates = 0u;
};

namespace diagnostics_detail {
inline std::atomic<std::uint64_t> censoredCalls{0u};
inline std::atomic<std::uint64_t> censoredGreenStrictMinInput{0u};
inline std::atomic<std::uint64_t> censoredWhiteCandidates{0u};
inline std::atomic<std::uint64_t> censoredSevereCandidates{0u};
inline std::atomic<std::uint64_t> censoredNearNeutralCandidates{0u};
} // namespace diagnostics_detail

inline DiagnosticsSnapshot take_diagnostics_snapshot_and_reset() noexcept {
    DiagnosticsSnapshot out{};
    out.censoredCalls =
        diagnostics_detail::censoredCalls.exchange(0u, std::memory_order_relaxed);
    out.censoredGreenStrictMinInput =
        diagnostics_detail::censoredGreenStrictMinInput.exchange(0u, std::memory_order_relaxed);
    out.censoredWhiteCandidates =
        diagnostics_detail::censoredWhiteCandidates.exchange(0u, std::memory_order_relaxed);
    out.censoredSevereCandidates =
        diagnostics_detail::censoredSevereCandidates.exchange(0u, std::memory_order_relaxed);
    out.censoredNearNeutralCandidates =
        diagnostics_detail::censoredNearNeutralCandidates.exchange(0u, std::memory_order_relaxed);
    return out;
}

inline float smoothstep01(float x) noexcept {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

inline double luminance709(float r, float g, float b) noexcept {
    return kLumaR * static_cast<double>(r) +
           kLumaG * static_cast<double>(g) +
           kLumaB * static_cast<double>(b);
}

inline bool apply_near_neutral_rolloff(
    float& r,
    float& g,
    float& b,
    bool censored) noexcept {
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) {
        return false;
    }

    const float r0 = r;
    const float g0 = g;
    const float b0 = b;
    const float mx = std::max(r, std::max(g, b));
    const float mn = std::min(r, std::min(g, b));
    const double yd = luminance709(r, g, b);
    if (!std::isfinite(yd)) return false;
    const float y = static_cast<float>(yd);

    if (censored) {
        diagnostics_detail::censoredCalls.fetch_add(1u, std::memory_order_relaxed);
        if (g < r && g < b) {
            diagnostics_detail::censoredGreenStrictMinInput.fetch_add(
                1u, std::memory_order_relaxed);
        }
    }

    // Retain detection of the former severe R/B-high, G-low censored-white
    // signature as telemetry only. No chroma contraction is applied.
    if (censored && mx > 0.92f && y > 0.45f) {
        diagnostics_detail::censoredWhiteCandidates.fetch_add(
            1u, std::memory_order_relaxed);
        const float rbFloor = std::min(r, b);
        const float rbBalance = std::abs(r - b);
        const float greenDeficit = std::max(0.0f, 0.5f * (r + b) - g);
        const float whiteGate = smoothstep01((mx - 0.92f) / 0.08f);
        const float rbHighGate = smoothstep01((rbFloor - 0.86f) / 0.14f);
        const float rbBalanceGate = 1.0f - smoothstep01(rbBalance / 0.28f);
        const float deficitGate = smoothstep01((greenDeficit - 0.10f) / 0.30f);
        const float lowGreenGate = smoothstep01((0.70f - g) / 0.22f);
        const float lowLumaGuard = smoothstep01((y - 0.45f) / 0.18f);
        const float severeAmount = std::clamp(
            0.95f * whiteGate * rbHighGate * rbBalanceGate *
                deficitGate * lowGreenGate * lowLumaGuard,
            0.0f,
            0.95f);
        if (severeAmount > 1.0e-7f) {
            diagnostics_detail::censoredSevereCandidates.fetch_add(
                1u, std::memory_order_relaxed);
        }
    }

    // Retain detection of the former near-neutral roll-off as telemetry only.
    if (mx > 0.92f && y > 0.78f && mn > 0.65f) {
        const float maxGate = smoothstep01((mx - 0.92f) / 0.08f);
        const float lumaGate = smoothstep01((y - 0.78f) / 0.18f);
        const float minGate = smoothstep01((mn - 0.65f) / 0.25f);
        const float authorityStrength = censored ? 1.0f : 0.60f;
        const float amount = std::clamp(
            maxGate * lumaGate * minGate * authorityStrength,
            0.0f,
            1.0f);
        if (censored && amount > 1.0e-7f) {
            diagnostics_detail::censoredNearNeutralCandidates.fetch_add(
                1u, std::memory_order_relaxed);
        }
    }

    // Hard regression invariant: this authority/diagnostic boundary must not
    // alter colour or luminance. Any future colour treatment needs new evidence
    // and a separate reviewed candidate.
    r = r0;
    g = g0;
    b = b0;
    return true;
}

} // namespace truthraw::presentation_highlight_chroma_rolloff::v0_1

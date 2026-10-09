#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace truthraw::presentation_illuminant_warmth_retention::v0_1 {

// Presentation-only source-white context. These coordinates may only come from
// the already authority-checked, source-metadata-bound illumination state.
// They are not SPD proof and do not classify the light as artificial/daylight.
struct SourceWhitePoint final {
    bool known = false;
    double x = 0.0;
    double y = 0.0;
    double correlatedColorTemperatureK = 0.0;
};

// Temporary presentation diagnostics. Only bright inputs are counted so the
// real-device highlight investigation can locate a green-channel collapse
// without turning this appearance operation into a scientific inference.
struct DiagnosticsSnapshot final {
    std::uint64_t brightInputs = 0u;
    std::uint64_t brightGreenStrictMinInput = 0u;
    std::uint64_t brightGreenStrictMinOutput = 0u;
    std::uint64_t brightGreenNegativeOutput = 0u;
};

namespace diagnostics_detail {
inline std::atomic<std::uint64_t> brightInputs{0u};
inline std::atomic<std::uint64_t> brightGreenStrictMinInput{0u};
inline std::atomic<std::uint64_t> brightGreenStrictMinOutput{0u};
inline std::atomic<std::uint64_t> brightGreenNegativeOutput{0u};
} // namespace diagnostics_detail

inline DiagnosticsSnapshot take_diagnostics_snapshot_and_reset() noexcept {
    DiagnosticsSnapshot out{};
    out.brightInputs =
        diagnostics_detail::brightInputs.exchange(0u, std::memory_order_relaxed);
    out.brightGreenStrictMinInput =
        diagnostics_detail::brightGreenStrictMinInput.exchange(0u, std::memory_order_relaxed);
    out.brightGreenStrictMinOutput =
        diagnostics_detail::brightGreenStrictMinOutput.exchange(0u, std::memory_order_relaxed);
    out.brightGreenNegativeOutput =
        diagnostics_detail::brightGreenNegativeOutput.exchange(0u, std::memory_order_relaxed);
    return out;
}

inline float smoothstep01(float x) noexcept {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// Very warm source whites retain at most 18% of the source-illuminant
// appearance. The effect falls smoothly to zero at 5000 K and above.
// CCT is used only as a bounded strength gate; the actual chromatic direction
// comes from the source-bound x/y white point.
inline float retention_strength(double cctK) noexcept {
    if (!std::isfinite(cctK) || !(cctK > 0.0)) return 0.0f;
    if (cctK >= 5000.0) return 0.0f;
    if (cctK <= 3200.0) return 0.18f;
    const float warm = static_cast<float>((5000.0 - cctK) / 1800.0);
    return 0.18f * smoothstep01(warm);
}

inline bool valid_white(const SourceWhitePoint& white) noexcept {
    return white.known &&
           std::isfinite(white.x) && std::isfinite(white.y) &&
           std::isfinite(white.correlatedColorTemperatureK) &&
           white.x > 0.0 && white.y > 0.0 &&
           white.x < 1.0 && white.y < 1.0 &&
           white.x + white.y < 1.0 &&
           white.correlatedColorTemperatureK > 0.0;
}

// Re-introduces a bounded fraction of the source-white appearance after the
// DNG colour path has established its D50 colourimetric basis. This is a
// partial inverse Bradford adaptation D50 -> source white in linear light.
// It is strictly APPEARANCE_ONLY / DERIVED_PRESENTATION_OUTPUT:
// - no source/Scientific-Master mutation;
// - no evidence creation or authority promotion;
// - no image-content/gray-world/semantic inference;
// - no spectrum or light-kind claim.
//
// Linear Rec.709 luminance is preserved so this colour operation cannot become
// an accidental exposure/headroom control. Existing highlight and gamut stages
// remain authoritative downstream of this function.
inline bool apply(
    float& r,
    float& g,
    float& b,
    const SourceWhitePoint& white,
    bool enabled) noexcept {
    if (!enabled || !valid_white(white)) return true;
    if (!std::isfinite(r) || !std::isfinite(g) || !std::isfinite(b)) return false;

    const float strength = retention_strength(white.correlatedColorTemperatureK);
    if (!(strength > 0.0f)) return true;

    const bool diagnoseBright = std::max(r, std::max(g, b)) > 0.90f;
    if (diagnoseBright) {
        diagnostics_detail::brightInputs.fetch_add(1u, std::memory_order_relaxed);
        if (g < r && g < b) {
            diagnostics_detail::brightGreenStrictMinInput.fetch_add(
                1u, std::memory_order_relaxed);
        }
    }

    // Current D.RAW presentation RGB is linear sRGB relative to D50. This is
    // the inverse of the D50->linear-sRGB matrix already used by the renderer.
    constexpr std::array<double, 9> kLinearSrgbToXyzD50{
        0.43607472, 0.38506492, 0.14308038,
        0.22250448, 0.71687860, 0.06061692,
        0.01393217, 0.09710452, 0.71417328,
    };
    constexpr std::array<double, 9> kXyzD50ToLinearSrgb{
         3.1338561, -1.6168667, -0.4906146,
        -0.9787684,  1.9161415,  0.0334540,
         0.0719453, -0.2289914,  1.4052427,
    };
    constexpr std::array<double, 9> kBradford{
         0.8951,  0.2664, -0.1614,
        -0.7502,  1.7135,  0.0367,
         0.0389, -0.0685,  1.0296,
    };
    constexpr std::array<double, 9> kBradfordInv{
         0.9869929, -0.1470543, 0.1599627,
         0.4323053,  0.5183603, 0.0492912,
        -0.0085287,  0.0400428, 0.9684867,
    };
    constexpr std::array<double, 3> kD50{0.96422, 1.0, 0.82521};

    auto mul3 = [](const std::array<double, 9>& m,
                   const std::array<double, 3>& v) noexcept {
        return std::array<double, 3>{
            m[0] * v[0] + m[1] * v[1] + m[2] * v[2],
            m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
            m[6] * v[0] + m[7] * v[1] + m[8] * v[2],
        };
    };

    const std::array<double, 3> sourceWhite{
        white.x / white.y,
        1.0,
        (1.0 - white.x - white.y) / white.y,
    };
    const auto d50Cone = mul3(kBradford, kD50);
    const auto sourceCone = mul3(kBradford, sourceWhite);

    std::array<double, 3> partialConeGain{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(d50Cone[i]) || !std::isfinite(sourceCone[i]) ||
            !(d50Cone[i] > 1.0e-12) || !(sourceCone[i] > 1.0e-12)) {
            return true; // invalid source-white geometry => appearance no-op
        }
        const double ratio = sourceCone[i] / d50Cone[i];
        partialConeGain[i] = std::pow(ratio, static_cast<double>(strength));
        if (!std::isfinite(partialConeGain[i]) || !(partialConeGain[i] > 0.0)) {
            return true;
        }
    }

    const std::array<double, 3> rgb{
        static_cast<double>(r), static_cast<double>(g), static_cast<double>(b)};
    const auto xyz = mul3(kLinearSrgbToXyzD50, rgb);
    auto cone = mul3(kBradford, xyz);
    for (std::size_t i = 0; i < 3; ++i) cone[i] *= partialConeGain[i];
    const auto adaptedXyz = mul3(kBradfordInv, cone);
    auto adaptedRgb = mul3(kXyzD50ToLinearSrgb, adaptedXyz);

    constexpr double kWr = 0.2126;
    constexpr double kWg = 0.7152;
    constexpr double kWb = 0.0722;
    const double inputY = kWr * rgb[0] + kWg * rgb[1] + kWb * rgb[2];
    const double outputY =
        kWr * adaptedRgb[0] + kWg * adaptedRgb[1] + kWb * adaptedRgb[2];
    if (std::isfinite(inputY) && std::isfinite(outputY) &&
        inputY > 1.0e-10 && outputY > 1.0e-10) {
        const double scale = inputY / outputY;
        adaptedRgb[0] *= scale;
        adaptedRgb[1] *= scale;
        adaptedRgb[2] *= scale;
    }

    if (!std::isfinite(adaptedRgb[0]) ||
        !std::isfinite(adaptedRgb[1]) ||
        !std::isfinite(adaptedRgb[2])) {
        return false;
    }

    r = static_cast<float>(adaptedRgb[0]);
    g = static_cast<float>(adaptedRgb[1]);
    b = static_cast<float>(adaptedRgb[2]);
    if (diagnoseBright) {
        if (g < r && g < b) {
            diagnostics_detail::brightGreenStrictMinOutput.fetch_add(
                1u, std::memory_order_relaxed);
        }
        if (g < 0.0f) {
            diagnostics_detail::brightGreenNegativeOutput.fetch_add(
                1u, std::memory_order_relaxed);
        }
    }
    return std::isfinite(r) && std::isfinite(g) && std::isfinite(b);
}

} // namespace truthraw::presentation_illuminant_warmth_retention::v0_1

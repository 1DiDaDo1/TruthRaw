#include "free_world_appearance_resolve_v0_7.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace truthraw::free_world_appearance_resolve::v0_7 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

bool finite(double v) noexcept {
    return std::isfinite(v);
}

bool finitePositive(double v) noexcept {
    return std::isfinite(v) && v > 0.0;
}

bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v) && v >= 0.0;
}

bool validSurround(Surround s) noexcept {
    const auto v = static_cast<std::uint8_t>(s);
    return v >= 1u && v <= 3u;
}

bool validTransfer(TransferFunction t) noexcept {
    const auto v = static_cast<std::uint8_t>(t);
    return v >= 1u && v <= 3u;
}

std::array<double, 3u> mul(
    const Matrix3& matrix,
    const std::array<double, 3u>& v) noexcept {
    return {
        matrix.m[0] * v[0] + matrix.m[1] * v[1] + matrix.m[2] * v[2],
        matrix.m[3] * v[0] + matrix.m[4] * v[1] + matrix.m[5] * v[2],
        matrix.m[6] * v[0] + matrix.m[7] * v[1] + matrix.m[8] * v[2],
    };
}

double surroundExponent(Surround surround) noexcept {
    switch (surround) {
        case Surround::Dark: return 0.90;
        case Surround::Dim: return 0.95;
        case Surround::Average: return 1.00;
    }
    return 1.00;
}

double mapLuminance(
    double sourceY,
    const AppearanceInput& input) noexcept {
    if (!finite(sourceY) || sourceY <= 0.0) {
        return input.display.blackLuminanceNits;
    }

    const double exposure =
        std::exp2(input.policy.exposureEv);
    const double sourceNits =
        sourceY *
        input.sceneColorimetry.sceneReferenceWhiteNits *
        exposure;

    const double referenceWhite =
        input.display.referenceWhiteNits;
    const double peak =
        input.display.peakLuminanceNits;
    const double black =
        input.display.blackLuminanceNits;

    const double adaptationRatio = std::clamp(
        input.viewing.adaptingLuminanceNits /
            std::max(input.viewing.backgroundLuminanceNits, 1.0e-6),
        0.25,
        4.0);
    const double adaptationGain =
        std::pow(adaptationRatio, 0.08);

    const double surroundGamma =
        surroundExponent(input.viewing.surround);
    const double relative =
        std::max(0.0, sourceNits / referenceWhite);
    const double adapted =
        referenceWhite *
        std::pow(relative, surroundGamma) *
        adaptationGain;

    if (adapted <= referenceWhite || peak <= referenceWhite) {
        return std::clamp(adapted, black, peak);
    }

    const double headroom = peak - referenceWhite;
    const double strength =
        std::max(1.0e-6, input.policy.highlightCompression);
    const double compressed =
        referenceWhite +
        headroom *
            (1.0 - std::exp(
                -(adapted - referenceWhite) /
                (headroom * strength)));

    return std::clamp(compressed, black, peak);
}

void hashU64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t, 8u> b{};
    for (unsigned i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hashF64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hashU64(h, std::bit_cast<std::uint64_t>(v));
}

void hashMatrix(
    truthraw::sha256_v0_69::Hasher& h,
    const Matrix3& matrix) noexcept {
    for (double v : matrix.m) hashF64(h, v);
}

Digest appearanceStateDigest(
    const AppearanceInput& input) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "D_RAW_FREE_WORLD_APPEARANCE_STATE_V0_7";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);

    h.update(input.sceneColorimetry.identitySha256);
    h.update(input.viewing.identitySha256);
    h.update(input.display.identitySha256);
    h.update(input.policy.identitySha256);

    hashMatrix(h, input.sceneColorimetry.rgbToXyz);
    hashMatrix(h, input.display.xyzToRgb);

    for (double v : input.sceneColorimetry.referenceWhiteXyz) hashF64(h, v);
    for (double v : input.viewing.adaptingWhiteXyz) hashF64(h, v);
    for (double v : input.display.whitePointXyz) hashF64(h, v);

    hashF64(h, input.sceneColorimetry.sceneReferenceWhiteNits);
    hashF64(h, input.viewing.adaptingLuminanceNits);
    hashF64(h, input.viewing.backgroundLuminanceNits);
    hashF64(h, input.viewing.viewingDistanceMeters);
    hashF64(h, input.display.referenceWhiteNits);
    hashF64(h, input.display.peakLuminanceNits);
    hashF64(h, input.display.blackLuminanceNits);
    hashF64(h, input.policy.exposureEv);
    hashF64(h, input.policy.colorfulnessScale);
    hashF64(h, input.policy.highlightCompression);

    const std::uint8_t surround =
        static_cast<std::uint8_t>(input.viewing.surround);
    const std::uint8_t transfer =
        static_cast<std::uint8_t>(input.display.transfer);
    h.update(&surround, 1u);
    h.update(&transfer, 1u);

    return h.finalize();
}

Digest outputDigest(
    const AppearanceInput& input,
    const AppearanceResolvedPixel& out) noexcept {
    truthraw::sha256_v0_69::Hasher h;
    constexpr char domain[] =
        "D_RAW_FREE_WORLD_APPEARANCE_OUTPUT_V0_7";
    h.update(
        reinterpret_cast<const std::uint8_t*>(domain),
        sizeof(domain) - 1u);
    h.update(out.sourceSceneSha256);
    h.update(out.appearanceStateSha256);
    for (double v : out.encodedRgb) hashF64(h, v);
    for (double v : out.displayLinearNits) hashF64(h, v);
    for (double v : out.mappedXyzNits) hashF64(h, v);
    hashF64(h, out.sourceLuminanceNits);
    hashF64(h, out.mappedLuminanceNits);
    const std::uint8_t clamp =
        out.gamutOrDisplayClampApplied ? 1u : 0u;
    h.update(&clamp, 1u);
    for (auto a : out.channelAuthority) {
        const std::uint8_t av =
            static_cast<std::uint8_t>(a);
        h.update(&av, 1u);
    }
    return h.finalize();
}

}  // namespace

bool validateMatrix(const Matrix3& matrix) noexcept {
    bool any = false;
    for (double v : matrix.m) {
        if (!finite(v)) return false;
        any = any || v != 0.0;
    }
    return any;
}

bool validateInput(const AppearanceInput& input) noexcept {
    if (!input.scene.sourcePacketSha256.empty()) {
        // std::array::empty() is always false for Digest; retained only as
        // a readability guard. Actual non-zero check follows below.
    }

    if (!nonzero(input.scene.sourcePacketSha256) ||
        input.scene.createsNewEvidence ||
        input.scene.scientificWritebackAllowed ||
        input.scene.physicalFrameCount != 1u ||
        input.scene.independentEvidenceCount != 1u ||
        !validateMatrix(input.sceneColorimetry.rgbToXyz) ||
        !validateMatrix(input.display.xyzToRgb) ||
        !nonzero(input.sceneColorimetry.identitySha256) ||
        !nonzero(input.viewing.identitySha256) ||
        !nonzero(input.display.identitySha256) ||
        !nonzero(input.policy.identitySha256) ||
        !finitePositive(input.sceneColorimetry.sceneReferenceWhiteNits) ||
        !finitePositive(input.viewing.adaptingLuminanceNits) ||
        !finiteNonNegative(input.viewing.backgroundLuminanceNits) ||
        !finitePositive(input.viewing.viewingDistanceMeters) ||
        !validSurround(input.viewing.surround) ||
        !finitePositive(input.display.referenceWhiteNits) ||
        !finitePositive(input.display.peakLuminanceNits) ||
        !finiteNonNegative(input.display.blackLuminanceNits) ||
        input.display.blackLuminanceNits >= input.display.peakLuminanceNits ||
        input.display.referenceWhiteNits > input.display.peakLuminanceNits ||
        !validTransfer(input.display.transfer) ||
        !finite(input.policy.exposureEv) ||
        input.policy.exposureEv < -10.0 ||
        input.policy.exposureEv > 10.0 ||
        !finiteNonNegative(input.policy.colorfulnessScale) ||
        input.policy.colorfulnessScale > 2.0 ||
        !finitePositive(input.policy.highlightCompression) ||
        input.policy.highlightCompression > 8.0) {
        return false;
    }

    for (double v : input.scene.sceneLinearRgb) {
        if (!finite(v)) return false;
    }
    for (double v : input.sceneColorimetry.referenceWhiteXyz) {
        if (!finitePositive(v)) return false;
    }
    for (double v : input.viewing.adaptingWhiteXyz) {
        if (!finitePositive(v)) return false;
    }
    for (double v : input.display.whitePointXyz) {
        if (!finitePositive(v)) return false;
    }
    return true;
}

double encodeSrgb(double linear) noexcept {
    if (!finite(linear)) return 0.0;
    const double x = std::clamp(linear, 0.0, 1.0);
    if (x <= 0.0031308) return 12.92 * x;
    return 1.055 * std::pow(x, 1.0 / 2.4) - 0.055;
}

double encodePqSt2084(double luminanceNits) noexcept {
    if (!finite(luminanceNits)) return 0.0;
    const double l =
        std::clamp(luminanceNits / 10000.0, 0.0, 1.0);

    constexpr double m1 = 2610.0 / 16384.0;
    constexpr double m2 = 2523.0 / 32.0;
    constexpr double c1 = 3424.0 / 4096.0;
    constexpr double c2 = 2413.0 / 128.0;
    constexpr double c3 = 2392.0 / 128.0;

    const double lm1 = std::pow(l, m1);
    const double numerator = c1 + c2 * lm1;
    const double denominator = 1.0 + c3 * lm1;
    return std::pow(numerator / denominator, m2);
}

bool resolveAppearance(
    const AppearanceInput& input,
    AppearanceResolvedPixel& out) noexcept {
    out = AppearanceResolvedPixel{};
    try {
        if (!validateInput(input)) return false;

        const double exposure =
            std::exp2(input.policy.exposureEv);
        std::array<double, 3u> exposedRgb{};
        for (std::size_t c = 0u; c < 3u; ++c) {
            exposedRgb[c] =
                input.scene.sceneLinearRgb[c] * exposure;
        }

        const auto xyzRelative =
            mul(input.sceneColorimetry.rgbToXyz, exposedRgb);
        const double sourceYRelative = xyzRelative[1];
        const double sourceY =
            sourceYRelative *
            input.sceneColorimetry.sceneReferenceWhiteNits;

        const double mappedY =
            mapLuminance(sourceYRelative, input);

        std::array<double, 3u> xyzNits{};
        if (sourceY > 0.0 && finite(sourceY)) {
            const double scale = mappedY / sourceY;
            for (std::size_t c = 0u; c < 3u; ++c) {
                xyzNits[c] =
                    xyzRelative[c] *
                    input.sceneColorimetry.sceneReferenceWhiteNits *
                    scale;
            }
        } else {
            xyzNits = {
                0.0,
                input.display.blackLuminanceNits,
                0.0};
        }

        auto rgbNits =
            mul(input.display.xyzToRgb, xyzNits);

        const double neutral = mappedY;
        for (std::size_t c = 0u; c < 3u; ++c) {
            rgbNits[c] =
                neutral +
                (rgbNits[c] - neutral) *
                    input.policy.colorfulnessScale;
        }

        bool clamped = false;
        for (std::size_t c = 0u; c < 3u; ++c) {
            if (!finite(rgbNits[c])) return false;
            const double bounded = std::clamp(
                rgbNits[c],
                input.display.blackLuminanceNits,
                input.display.peakLuminanceNits);
            if (bounded != rgbNits[c]) clamped = true;
            out.displayLinearNits[c] = bounded;

            switch (input.display.transfer) {
                case TransferFunction::LinearNormalized:
                    out.encodedRgb[c] =
                        (bounded - input.display.blackLuminanceNits) /
                        (input.display.peakLuminanceNits -
                         input.display.blackLuminanceNits);
                    break;
                case TransferFunction::Srgb:
                    out.encodedRgb[c] = encodeSrgb(
                        (bounded - input.display.blackLuminanceNits) /
                        (input.display.peakLuminanceNits -
                         input.display.blackLuminanceNits));
                    break;
                case TransferFunction::PqSt2084:
                    out.encodedRgb[c] =
                        encodePqSt2084(bounded);
                    break;
            }
        }

        out.mappedXyzNits = xyzNits;
        out.sourceSceneSha256 =
            input.scene.sourcePacketSha256;
        out.appearanceStateSha256 =
            appearanceStateDigest(input);
        out.sourceLuminanceNits =
            finite(sourceY) ? sourceY : 0.0;
        out.mappedLuminanceNits = mappedY;
        out.gamutOrDisplayClampApplied = clamped;

        out.channelAuthority =
            input.scene.channelAuthority;
        out.uncertaintyKnown =
            input.scene.uncertaintyKnown;
        out.p95Uncertainty =
            input.scene.p95Uncertainty;

        out.appearanceApplied = true;
        out.displayEncoded = true;
        out.sourceSceneMutated = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        out.outputSha256 =
            outputDigest(input, out);

        return nonzero(out.appearanceStateSha256) &&
               nonzero(out.outputSha256);
    } catch (...) {
        out = AppearanceResolvedPixel{};
        return false;
    }
}

const char* toString(Surround surround) noexcept {
    switch (surround) {
        case Surround::Dark: return "DARK";
        case Surround::Dim: return "DIM";
        case Surround::Average: return "AVERAGE";
    }
    return "INVALID";
}

const char* toString(TransferFunction transfer) noexcept {
    switch (transfer) {
        case TransferFunction::LinearNormalized:
            return "LINEAR_NORMALIZED";
        case TransferFunction::Srgb:
            return "SRGB";
        case TransferFunction::PqSt2084:
            return "PQ_ST2084";
    }
    return "INVALID";
}

}  // namespace truthraw::free_world_appearance_resolve::v0_7

#include "truthnegative_camera5_color_highlight_oracle_v0_1.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::truthnegative_camera5_color_highlight_oracle::v0_1 {
namespace {

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v) { return v != 0u; });
}

bool finite3(const std::array<double, 3u>& v) noexcept {
    return std::all_of(
        v.begin(), v.end(),
        [](double x) { return std::isfinite(x); });
}

double chromaSpread(const std::array<double, 3u>& rgb) noexcept {
    const double hi = std::max({rgb[0], rgb[1], rgb[2]});
    const double lo = std::min({rgb[0], rgb[1], rgb[2]});
    const double denom = std::max(1.0e-9, hi);
    return (hi - lo) / denom;
}

double greenBias(const std::array<double, 3u>& rgb) noexcept {
    const double sum = rgb[0] + rgb[1] + rgb[2];
    if (!(sum > 1.0e-12) || !std::isfinite(sum)) return 0.0;
    const double r = rgb[0] / sum;
    const double g = rgb[1] / sum;
    const double b = rgb[2] / sum;
    return g - 0.5 * (r + b);
}

double hueVectorDistance(
    const std::array<double, 3u>& a,
    const std::array<double, 3u>& b) noexcept {
    const double sa = a[0] + a[1] + a[2];
    const double sb = b[0] + b[1] + b[2];
    if (!(sa > 1.0e-12) || !(sb > 1.0e-12)) return 0.0;
    double sum = 0.0;
    for (std::size_t c = 0u; c < 3u; ++c) {
        const double d = a[c] / sa - b[c] / sb;
        sum += d * d;
    }
    return std::sqrt(sum);
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

}  // namespace

bool run(const Input& input, Report& out) noexcept {
    out = Report{};
    try {
        if (!nonzero(input.sourceEvidenceSha256) ||
            !nonzero(input.scientificMasterSha256) ||
            !nonzero(input.truthNegativeStateSha256) ||
            !nonzero(input.colorBindingSha256) ||
            input.physicalCameraId == 0u ||
            !std::isfinite(input.whiteLevel) ||
            input.whiteLevel <= 0.0 ||
            input.samples.empty()) {
            return false;
        }
        for (double b : input.blackPhase) {
            if (!std::isfinite(b) || b < 0.0 || b >= input.whiteLevel) {
                return false;
            }
        }
        if (input.asShotNeutralKnown) {
            for (double n : input.asShotNeutral) {
                if (!std::isfinite(n) || n <= 0.0) return false;
            }
        }

        out.sampleCount = input.samples.size();
        out.remosaicScientificallyResolved =
            input.remosaicState == RemosaicState::Enabled ||
            input.remosaicState == RemosaicState::Disabled;

        std::array<double, 3u> neutralSum{};
        std::uint64_t neutralCount = 0u;
        double lowGreenSum = 0.0;
        double greenDriftSum = 0.0;
        double unclampedHueDriftSum = 0.0;
        std::uint64_t driftCount = 0u;
        std::uint64_t unclampedDriftCount = 0u;

        for (const auto& s : input.samples) {
            if (!finite3(s.cameraNativeRgb) ||
                !finite3(s.xyzD50)) {
                return false;
            }
            for (const auto& rgb : s.encodedRgbByExposure) {
                if (!finite3(rgb)) return false;
            }
            for (auto a : s.authority) {
                if (a == fw::ResolvedAuthority::Censored) {
                    ++out.censoredChannelCount;
                }
            }

            // Candidate white highlight: appears low-chroma at EV0 and is
            // bright enough that display clipping/compression can hide bias.
            const auto& ev0 = s.encodedRgbByExposure[0];
            const double ev0Peak = std::max({ev0[0], ev0[1], ev0[2]});
            const bool candidate =
                ev0Peak >= 0.72 &&
                chromaSpread(ev0) <= 0.09;
            if (!candidate) continue;

            ++out.apparentWhiteHighlightCandidates;
            if (s.displayClampByExposure[0]) {
                ++out.clampedEv0CandidateCount;
            }

            const double g = s.cameraNativeRgb[1];
            if (g > 1.0e-9) {
                neutralSum[0] += s.cameraNativeRgb[0] / g;
                neutralSum[1] += 1.0;
                neutralSum[2] += s.cameraNativeRgb[2] / g;
                ++neutralCount;
            }

            const auto& low = s.encodedRgbByExposure[4];
            lowGreenSum += greenBias(low);
            greenDriftSum += greenBias(low) - greenBias(ev0);
            ++driftCount;

            // If neither endpoint is clamped, hue motion belongs to the
            // appearance/display stage rather than simple channel clipping.
            if (!s.displayClampByExposure[0] &&
                !s.displayClampByExposure[4]) {
                unclampedHueDriftSum += hueVectorDistance(ev0, low);
                ++unclampedDriftCount;
            }
        }

        const double totalChannels =
            static_cast<double>(input.samples.size()) * 3.0;
        out.censoredChannelFraction =
            static_cast<double>(out.censoredChannelCount) /
            totalChannels;

        if (neutralCount > 0u) {
            out.empiricalCameraNeutralKnown = true;
            for (std::size_t c = 0u; c < 3u; ++c) {
                out.empiricalCameraNeutral[c] =
                    neutralSum[c] /
                    static_cast<double>(neutralCount);
            }
        }

        if (driftCount > 0u) {
            out.lowExposureGreenBias =
                lowGreenSum / static_cast<double>(driftCount);
            out.exposureGreenDrift =
                greenDriftSum / static_cast<double>(driftCount);
        }
        if (unclampedDriftCount > 0u) {
            out.unclampedExposureHueDrift =
                unclampedHueDriftSum /
                static_cast<double>(unclampedDriftCount);
        }

        if (input.asShotNeutralKnown &&
            out.empiricalCameraNeutralKnown) {
            const double r =
                std::log(
                    out.empiricalCameraNeutral[0] /
                    input.asShotNeutral[0]);
            const double b =
                std::log(
                    out.empiricalCameraNeutral[2] /
                    input.asShotNeutral[2]);
            out.metadataNeutralLogError =
                std::sqrt(0.5 * (r * r + b * b));
        }

        out.sourceCensoringDominant =
            out.censoredChannelFraction >= 0.08;
        out.metadataNeutralMismatch =
            input.asShotNeutralKnown &&
            out.empiricalCameraNeutralKnown &&
            out.apparentWhiteHighlightCandidates >= 4u &&
            out.metadataNeutralLogError >= 0.16;

        out.colorBindingBiasDetected =
            out.apparentWhiteHighlightCandidates >= 4u &&
            out.lowExposureGreenBias >= 0.045 &&
            out.exposureGreenDrift >= 0.025;

        out.appearanceDisplayDriftDetected =
            out.apparentWhiteHighlightCandidates >= 4u &&
            out.unclampedExposureHueDrift >= 0.025;

        if (out.sourceCensoringDominant) {
            out.firstFailureStage = FirstFailureStage::SourceCensoring;
        } else if (out.metadataNeutralMismatch) {
            out.firstFailureStage =
                FirstFailureStage::MetadataNeutralMismatch;
        } else if (out.colorBindingBiasDetected) {
            out.firstFailureStage = FirstFailureStage::ColorBinding;
        } else if (out.appearanceDisplayDriftDetected) {
            out.firstFailureStage =
                FirstFailureStage::AppearanceDisplay;
        } else if (out.apparentWhiteHighlightCandidates > 0u) {
            out.firstFailureStage = FirstFailureStage::None;
        } else {
            out.firstFailureStage = FirstFailureStage::Unresolved;
        }

        truthraw::sha256_v0_69::Hasher h;
        constexpr char domain[] =
            "D_RAW_CAMERA5_COLOR_HIGHLIGHT_ORACLE_V0_1";
        h.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        h.update(input.sourceEvidenceSha256);
        h.update(input.scientificMasterSha256);
        h.update(input.truthNegativeStateSha256);
        h.update(input.colorBindingSha256);
        hashU64(h, input.physicalCameraId);
        hashF64(h, input.whiteLevel);
        for (double v : input.blackPhase) hashF64(h, v);
        for (double v : input.asShotNeutral) hashF64(h, v);
        hashU64(h, input.asShotNeutralKnown ? 1u : 0u);
        hashU64(h, static_cast<std::uint8_t>(input.remosaicState));
        hashU64(h, out.sampleCount);
        hashU64(h, out.apparentWhiteHighlightCandidates);
        hashU64(h, out.censoredChannelCount);
        for (double v : out.empiricalCameraNeutral) hashF64(h, v);
        hashF64(h, out.metadataNeutralLogError);
        hashF64(h, out.lowExposureGreenBias);
        hashF64(h, out.exposureGreenDrift);
        hashF64(h, out.unclampedExposureHueDrift);
        hashF64(h, out.censoredChannelFraction);
        hashU64(h, static_cast<std::uint8_t>(out.firstFailureStage));

        out.oracleSha256 = h.finalize();
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;
        return nonzero(out.oracleSha256);
    } catch (...) {
        out = Report{};
        return false;
    }
}

const char* toString(RemosaicState state) noexcept {
    switch (state) {
        case RemosaicState::Unknown: return "UNKNOWN";
        case RemosaicState::Disabled: return "DISABLED";
        case RemosaicState::Enabled: return "ENABLED";
        case RemosaicState::OemHintOnly: return "OEM_HINT_ONLY";
    }
    return "INVALID";
}

const char* toString(FirstFailureStage stage) noexcept {
    switch (stage) {
        case FirstFailureStage::None: return "NONE";
        case FirstFailureStage::SourceCensoring:
            return "SOURCE_CENSORING";
        case FirstFailureStage::MetadataNeutralMismatch:
            return "METADATA_NEUTRAL_MISMATCH";
        case FirstFailureStage::ColorBinding:
            return "COLOR_BINDING";
        case FirstFailureStage::AppearanceDisplay:
            return "APPEARANCE_DISPLAY";
        case FirstFailureStage::Unresolved:
            return "UNRESOLVED";
    }
    return "INVALID";
}

const char* schema_name() noexcept {
    return kSchemaName;
}

}  // namespace truthraw::truthnegative_camera5_color_highlight_oracle::v0_1

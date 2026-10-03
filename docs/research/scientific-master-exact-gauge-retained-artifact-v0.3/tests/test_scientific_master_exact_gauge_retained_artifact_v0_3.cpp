#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"
#include "scientific_master_streaming_binding_v0_2.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>

namespace smsb2 = truthraw::scientific_master_streaming_binding::v0_2;
namespace ega3 = truthraw::scientific_master_exact_gauge_retained_artifact::v0_3;

namespace {

void require_active(bool ok, const char* expression, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expression << '\n';
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(static_cast<bool>(expr), #expr, __LINE__)

enum class Pattern {
    PseudoRandom,
    Constant,
    SplitMedian,
    SparseEligible,
    SignedZeroGain,
    NonFiniteGain,
    ResidualBlackAndGain,
};

truthraw::DecodedDngFrame make_frame(int width, int height, Pattern pattern) {
    truthraw::DecodedDngFrame frame{};
    frame.meta.width = width;
    frame.meta.height = height;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Normal;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 64.0f, 64.0f, 64.0f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.noiseProfile = {0.0009f, 1.0e-6f, 0.0010f, 1.2e-6f,
                               0.0011f, 1.4e-6f};
    frame.meta.hasGainField =
        pattern == Pattern::SignedZeroGain ||
        pattern == Pattern::NonFiniteGain ||
        pattern == Pattern::ResidualBlackAndGain;
    frame.meta.hasResidualBlack = pattern == Pattern::ResidualBlackAndGain;
    frame.meta.sourceId = "synthetic_draw_exact_gauge_retained_v03";

    const std::size_t count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    frame.raw.resize(count);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint16_t value = 512u;
            switch (pattern) {
                case Pattern::PseudoRandom:
                case Pattern::SignedZeroGain:
                case Pattern::NonFiniteGain:
                case Pattern::ResidualBlackAndGain: {
                    const int v = 70 + ((x * 37 + y * 53 + x * y * 3) % 900);
                    value = static_cast<std::uint16_t>(v);
                    if ((x + y) % 97 == 0) value = 1023u;
                    break;
                }
                case Pattern::Constant:
                    value = 512u;
                    break;
                case Pattern::SplitMedian:
                    value = x < width / 2 ? 100u : 900u;
                    break;
                case Pattern::SparseEligible:
                    if ((x + 3 * y) % 11 == 0) value = 65u;
                    else if ((2 * x + y) % 17 == 0) value = 180u;
                    else if ((x + y) % 19 == 0) value = 1023u;
                    else value = 64u;
                    break;
            }
            frame.raw[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                      static_cast<std::size_t>(x)] = value;
        }
    }
    return frame;
}

class CountingFrameSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    CountingFrameSource(const truthraw::DecodedDngFrame& frame, Pattern pattern)
        : frame_(frame), pattern_(pattern) {}

    const truthraw::DngMetadata& metadata() const override { return frame_.meta; }

    std::size_t residentBytesUpperBound() const override {
        return frame_.raw.size() * sizeof(std::uint16_t);
    }

    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const int width = rect.hx1 - rect.hx0;
        const int height = rect.hy1 - rect.hy0;
        const std::size_t count =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (rawOut == nullptr || rawCount != count) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "raw tile count mismatch");
        }
        if (frame_.meta.hasGainField) {
            if (gainOut == nullptr || gainCount != count) {
                return truthraw::streaming_v0_1::StreamStatus::error(
                    truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                    "gain tile count mismatch");
            }
        } else if (gainOut != nullptr || gainCount != 0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected gain request");
        }

        ++rawTileCalls;
        rawSamplesRead += count;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int globalX = rect.hx0 + x;
                const int globalY = rect.hy0 + y;
                const std::size_t sourceIndex =
                    static_cast<std::size_t>(globalY) *
                        static_cast<std::size_t>(frame_.meta.width) +
                    static_cast<std::size_t>(globalX);
                const std::size_t destinationIndex =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(x);
                rawOut[destinationIndex] = frame_.raw[sourceIndex];

                if (gainOut != nullptr) {
                    float gain = 1.0f +
                        0.0005f * static_cast<float>((globalX + 2 * globalY) % 31);
                    if (pattern_ == Pattern::SignedZeroGain &&
                        (globalX + globalY) % 139 == 0) {
                        gain = -0.0f;
                    } else if (pattern_ == Pattern::NonFiniteGain) {
                        if ((globalX + 5 * globalY) % 131 == 0) {
                            gain = std::numeric_limits<float>::quiet_NaN();
                        } else if ((3 * globalX + globalY) % 137 == 0) {
                            gain = std::numeric_limits<float>::infinity();
                        }
                    }
                    gainOut[destinationIndex] = gain;
                }
            }
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readRowBias(
        int y0, int y1, float* out, std::size_t count) override {
        if (!frame_.meta.hasResidualBlack) {
            return count == 0u
                ? truthraw::streaming_v0_1::StreamStatus::ok()
                : truthraw::streaming_v0_1::StreamStatus::error(
                      truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                      "unexpected row-bias request");
        }
        if (out == nullptr || count != static_cast<std::size_t>(y1 - y0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "row-bias count mismatch");
        }
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = 0.25f * static_cast<float>((y0 + static_cast<int>(i)) % 5 - 2);
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int x0, int x1, float* out, std::size_t count) override {
        if (!frame_.meta.hasResidualBlack) {
            return count == 0u
                ? truthraw::streaming_v0_1::StreamStatus::ok()
                : truthraw::streaming_v0_1::StreamStatus::error(
                      truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                      "unexpected col-bias request");
        }
        if (out == nullptr || count != static_cast<std::size_t>(x1 - x0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "col-bias count mismatch");
        }
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = 0.125f * static_cast<float>((x0 + static_cast<int>(i)) % 7 - 3);
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    std::size_t rawTileCalls = 0u;
    std::size_t rawSamplesRead = 0u;

private:
    const truthraw::DecodedDngFrame& frame_;
    Pattern pattern_;
};

class CountingObserver final : public smsb2::ICanonicalTileObserver {
public:
    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }

    bool observeCanonicalTile(
        std::uint32_t,
        std::uint32_t,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint16_t* rawCore,
        std::size_t rawCount,
        const float* cameraNativeRgb,
        std::size_t floatCount) noexcept override {
        const std::size_t expected = static_cast<std::size_t>(width) * height;
        if (!rawCore || !cameraNativeRgb || rawCount != expected || floatCount != 3u * expected) {
            return false;
        }
        ++tiles;
        samples += rawCount;
        return true;
    }

    std::size_t tiles = 0u;
    std::size_t samples = 0u;
};

bool equal_double_bits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

std::uint32_t float_bits(float value) {
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void require_scientific_parity(const smsb2::Result& canonical,
                               const ega3::Result& candidate) {
    REQUIRE(canonical.scientificMasterHash == candidate.scientificMasterHash);
    REQUIRE(canonical.zeroLineGauge.mode == candidate.zeroLineGauge.mode);
    REQUIRE(equal_double_bits(canonical.zeroLineGauge.L0, candidate.zeroLineGauge.L0));
    REQUIRE(canonical.zeroLineGauge.gaugeId == candidate.zeroLineGauge.gaugeId);
    REQUIRE(canonical.zeroLineGauge.crossSceneComparable ==
            candidate.zeroLineGauge.crossSceneComparable);
    REQUIRE(canonical.zeroLineGauge.absolutePhysicalUnits ==
            candidate.zeroLineGauge.absolutePhysicalUnits);
    REQUIRE(canonical.sceneBinding.reconstructionBackend ==
            candidate.sceneBinding.reconstructionBackend);
    REQUIRE(canonical.sceneBinding.sceneScaleId == candidate.sceneBinding.sceneScaleId);
    REQUIRE(canonical.sceneBinding.gainMapAppliedExactlyOnce ==
            candidate.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(canonical.sceneBinding.exposureNormalizedToCommonScene ==
            candidate.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(canonical.sceneBinding.gainNormalizedToCommonScene ==
            candidate.sceneBinding.gainNormalizedToCommonScene);
    REQUIRE(canonical.selfGaugeEligibleSamples == candidate.selfGaugeEligibleSamples);
    REQUIRE(canonical.masterTilesProcessed == candidate.masterTilesProcessed);
    REQUIRE(canonical.physicalFrameCount == candidate.physicalFrameCount);
    REQUIRE(canonical.independentEvidenceCount == candidate.independentEvidenceCount);
}

std::size_t tile_count(int width, int height) {
    const std::size_t columns =
        (static_cast<std::size_t>(width) + smsb2::kCanonicalCore - 1u) /
        static_cast<std::size_t>(smsb2::kCanonicalCore);
    const std::size_t rows =
        (static_cast<std::size_t>(height) + smsb2::kCanonicalCore - 1u) /
        static_cast<std::size_t>(smsb2::kCanonicalCore);
    return columns * rows;
}

void compare_success_case(int width, int height, Pattern pattern, const char* label) {
    auto frame = make_frame(width, height, pattern);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction canonicalReconstruction;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction candidateReconstruction;
    CountingFrameSource canonicalSource(frame, pattern);
    CountingFrameSource candidateSource(frame, pattern);

    smsb2::Result canonical{};
    ega3::Result candidate{};
    ega3::Diagnostics diagnostics{};
    const smsb2::Options options{};
    const auto canonicalStatus = smsb2::bind_scientific_master_streaming(
        canonicalSource, canonicalReconstruction, options, canonical);
    const auto candidateStatus = ega3::bind_scientific_master_retained_exact_gauge(
        candidateSource, candidateReconstruction, options, candidate, diagnostics);
    if (!canonicalStatus) {
        std::cerr << label << " canonical v0.2 failed: " << canonicalStatus.message << '\n';
    }
    if (!candidateStatus) {
        std::cerr << label << " candidate v0.3 failed: " << candidateStatus.message << '\n';
    }
    REQUIRE(canonicalStatus);
    REQUIRE(candidateStatus);
    require_scientific_parity(canonical, candidate);

    const std::size_t tiles = tile_count(width, height);
    REQUIRE(canonical.stage2GaugeScanPasses == 2u);
    REQUIRE(candidate.stage2GaugeScanPasses == 1u);
    REQUIRE(canonicalSource.rawTileCalls == tiles * 2u);
    REQUIRE(candidateSource.rawTileCalls == tiles);
    REQUIRE(diagnostics.routeUsed == ega3::RouteUsed::RetainedExactFloat32Bits);
    REQUIRE(diagnostics.fallbackReason == ega3::FallbackReason::None);
    REQUIRE(diagnostics.optimizationApplied);
    REQUIRE(diagnostics.eligibleRetainedSamples == candidate.selfGaugeEligibleSamples);
    REQUIRE(diagnostics.retainedBytesUsed ==
            static_cast<std::size_t>(candidate.selfGaugeEligibleSamples) * sizeof(std::uint32_t));
    REQUIRE(diagnostics.retainedBytesReserved >= diagnostics.retainedBytesUsed);
    REQUIRE(diagnostics.retainedBytesReserved >= diagnostics.retainedBytesRequested);
    REQUIRE(diagnostics.stage2GaugeScanPassesActuallyUsed == 1u);
    REQUIRE(diagnostics.pass2Stage2TileReadsAvoided == tiles);
    REQUIRE(!diagnostics.sourceValuesModified);
    REQUIRE(!diagnostics.createsNewEvidence);
    REQUIRE(!diagnostics.scientificWritebackAllowed);

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction repeatedReconstruction;
    CountingFrameSource repeatedSource(frame, pattern);
    ega3::Result repeated{};
    ega3::Diagnostics repeatedDiagnostics{};
    const auto repeatedStatus = ega3::bind_scientific_master_retained_exact_gauge(
        repeatedSource, repeatedReconstruction, options, repeated, repeatedDiagnostics);
    REQUIRE(repeatedStatus);
    require_scientific_parity(candidate, repeated);
    REQUIRE(diagnostics.lowerMedianFloat32Bits == repeatedDiagnostics.lowerMedianFloat32Bits);
    REQUIRE(diagnostics.upperMedianFloat32Bits == repeatedDiagnostics.upperMedianFloat32Bits);
}

void test_equivalence_matrix() {
    compare_success_case(130, 70, Pattern::PseudoRandom, "pseudo-random");
    compare_success_case(128, 64, Pattern::Constant, "constant-even");
    compare_success_case(129, 65, Pattern::Constant, "constant-odd");
    compare_success_case(128, 64, Pattern::SplitMedian, "split-median");
    compare_success_case(129, 65, Pattern::SparseEligible, "sparse-eligible");
    compare_success_case(132, 72, Pattern::SignedZeroGain, "signed-zero-gain");
    compare_success_case(134, 74, Pattern::ResidualBlackAndGain, "residual-black-gain");
}

void test_constant_exact_median_bits() {
    auto frame = make_frame(129, 65, Pattern::Constant);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    CountingFrameSource source(frame, Pattern::Constant);
    ega3::Result result{};
    ega3::Diagnostics diagnostics{};
    const ega3::Options options{};
    const auto status = ega3::bind_scientific_master_retained_exact_gauge(
        source, reconstruction, options, result, diagnostics);
    REQUIRE(status);
    const float expectedStage2 = (512.0f - 64.0f) / (1023.0f - 64.0f);
    const std::uint32_t expectedBits = float_bits(expectedStage2);
    REQUIRE(diagnostics.lowerMedianFloat32Bits == expectedBits);
    REQUIRE(diagnostics.upperMedianFloat32Bits == expectedBits);
    REQUIRE(equal_double_bits(result.zeroLineGauge.L0,
                              static_cast<double>(expectedStage2)));
}

void test_nonfinite_stage2_is_rejected_before_gauge_in_both_routes() {
    auto frame = make_frame(132, 72, Pattern::NonFiniteGain);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction canonicalReconstruction;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction candidateReconstruction;
    CountingFrameSource canonicalSource(frame, Pattern::NonFiniteGain);
    CountingFrameSource candidateSource(frame, Pattern::NonFiniteGain);
    smsb2::Result canonical{};
    ega3::Result candidate{};
    ega3::Diagnostics diagnostics{};
    const smsb2::Options options{};
    const auto canonicalStatus = smsb2::bind_scientific_master_streaming(
        canonicalSource, canonicalReconstruction, options, canonical);
    const auto candidateStatus = ega3::bind_scientific_master_retained_exact_gauge(
        candidateSource, candidateReconstruction, options, candidate, diagnostics);
    REQUIRE(!canonicalStatus);
    REQUIRE(!candidateStatus);
    REQUIRE(canonicalStatus.code == smsb2::StatusCode::DigestFailed);
    REQUIRE(candidateStatus.code == ega3::StatusCode::DigestFailed);
    REQUIRE(canonicalStatus.message == candidateStatus.message);
    REQUIRE(!diagnostics.optimizationApplied);
}

void test_observed_route_parity() {
    constexpr int width = 130;
    constexpr int height = 70;
    auto frame = make_frame(width, height, Pattern::PseudoRandom);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction canonicalReconstruction;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction candidateReconstruction;
    CountingFrameSource canonicalSource(frame, Pattern::PseudoRandom);
    CountingFrameSource candidateSource(frame, Pattern::PseudoRandom);
    CountingObserver canonicalObserver;
    CountingObserver candidateObserver;
    smsb2::Result canonical{};
    ega3::Result candidate{};
    ega3::Diagnostics diagnostics{};
    const smsb2::Options options{};
    const auto canonicalStatus = smsb2::bind_scientific_master_streaming_observed(
        canonicalSource, canonicalReconstruction, options, canonicalObserver, canonical);
    const auto candidateStatus = ega3::bind_scientific_master_retained_exact_gauge_observed(
        candidateSource, candidateReconstruction, options, candidateObserver, candidate, diagnostics);
    REQUIRE(canonicalStatus);
    REQUIRE(candidateStatus);
    require_scientific_parity(canonical, candidate);
    const std::size_t tiles = tile_count(width, height);
    REQUIRE(canonicalObserver.tiles == tiles);
    REQUIRE(candidateObserver.tiles == tiles);
    REQUIRE(canonicalObserver.samples == static_cast<std::size_t>(width) * height);
    REQUIRE(candidateObserver.samples == static_cast<std::size_t>(width) * height);
    REQUIRE(canonicalSource.rawTileCalls == tiles * 2u);
    REQUIRE(candidateSource.rawTileCalls == tiles);
}

void test_geometric_bound_contract() {
    auto frame = make_frame(130, 70, Pattern::Constant);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    CountingFrameSource source(frame, Pattern::Constant);
    ega3::Result result{};
    ega3::Diagnostics diagnostics{};
    const ega3::Options options{};
    const auto status = ega3::bind_scientific_master_retained_exact_gauge(
        source, reconstruction, options, result, diagnostics);
    REQUIRE(status);
    const int borderX = static_cast<int>(std::floor(130.0 * smsb2::kSelfGaugeBorderFraction));
    const int borderY = static_cast<int>(std::floor(70.0 * smsb2::kSelfGaugeBorderFraction));
    const std::uint64_t expected =
        static_cast<std::uint64_t>(130 - 2 * borderX) *
        static_cast<std::uint64_t>(70 - 2 * borderY);
    REQUIRE(diagnostics.geometricEligibleUpperBound == expected);
    REQUIRE(diagnostics.retainedBytesRequested == expected * sizeof(std::uint32_t));
    REQUIRE(result.selfGaugeEligibleSamples == expected);
}

void test_budget_forces_complete_v0_2_fallback() {
    auto frame = make_frame(130, 70, Pattern::PseudoRandom);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction baselineReconstruction;
    CountingFrameSource baselineSource(frame, Pattern::PseudoRandom);
    smsb2::Result baseline{};
    smsb2::Options unlimited{};
    const auto baselineStatus = smsb2::bind_scientific_master_streaming(
        baselineSource, baselineReconstruction, unlimited, baseline);
    REQUIRE(baselineStatus);

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction fallbackReconstruction;
    CountingFrameSource fallbackSource(frame, Pattern::PseudoRandom);
    ega3::Result fallback{};
    ega3::Diagnostics diagnostics{};
    ega3::Options constrained{};
    constrained.memoryBudgetBytes = baseline.logicalResidentUpperBound;
    const auto fallbackStatus = ega3::bind_scientific_master_retained_exact_gauge(
        fallbackSource, fallbackReconstruction, constrained, fallback, diagnostics);
    REQUIRE(fallbackStatus);
    require_scientific_parity(baseline, fallback);
    REQUIRE(diagnostics.routeUsed == ega3::RouteUsed::CanonicalV02Fallback);
    REQUIRE(diagnostics.fallbackReason == ega3::FallbackReason::MemoryBudgetInsufficient ||
            diagnostics.fallbackReason == ega3::FallbackReason::CandidateResidentBudgetExceeded);
    REQUIRE(!diagnostics.optimizationApplied);
    REQUIRE(fallback.stage2GaugeScanPasses == 2u);
    REQUIRE(diagnostics.stage2GaugeScanPassesActuallyUsed == 2u);
    REQUIRE(diagnostics.pass2Stage2TileReadsAvoided == 0u);
    REQUIRE(fallbackSource.rawTileCalls ==
            tile_count(frame.meta.width, frame.meta.height) * 2u);
}

}  // namespace

int main() {
    test_equivalence_matrix();
    test_constant_exact_median_bits();
    test_nonfinite_stage2_is_rejected_before_gauge_in_both_routes();
    test_observed_route_parity();
    test_geometric_bound_contract();
    test_budget_forces_complete_v0_2_fallback();
    std::cout << "Scientific Master Exact Gauge Retained Artifact v0.3: PASS\n";
    return 0;
}

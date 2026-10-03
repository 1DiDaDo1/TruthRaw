#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"
#include "scientific_master_streaming_binding_v0_2.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
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
    SpecialFloatGain,
    ResidualBlackAndGain,
};

truthraw::DecodedDngFrame make_frame(int width, int height, Pattern pattern) {
    truthraw::DecodedDngFrame frame{};
    frame.meta.width = width;
    frame.meta.height = height;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Normal;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 65.0f, 63.0f, 66.0f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.noiseProfile = {0.0009f, 1.0e-6f, 0.0010f, 1.2e-6f,
                               0.0011f, 1.4e-6f};
    frame.meta.hasGainField =
        pattern == Pattern::SpecialFloatGain ||
        pattern == Pattern::ResidualBlackAndGain;
    frame.meta.hasResidualBlack = pattern == Pattern::ResidualBlackAndGain;
    frame.meta.sourceId = "synthetic_exact_gauge_retained_artifact_v03";

    const std::size_t count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    frame.raw.resize(count);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint16_t value = 512u;
            switch (pattern) {
                case Pattern::PseudoRandom:
                case Pattern::SpecialFloatGain:
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
                    if ((x + 3 * y) % 11 == 0) value = 67u;
                    else if ((2 * x + y) % 17 == 0) value = 180u;
                    else if ((x + y) % 19 == 0) value = 1023u;
                    else value = 63u;
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
    explicit CountingFrameSource(const truthraw::DecodedDngFrame& frame)
        : frame_(frame) {}

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
                    if (frame_.meta.sourceId == "synthetic_exact_gauge_retained_artifact_v03" &&
                        !frame_.meta.hasResidualBlack) {
                        if ((globalX + 5 * globalY) % 131 == 0) {
                            gain = std::numeric_limits<float>::quiet_NaN();
                        } else if ((3 * globalX + globalY) % 137 == 0) {
                            gain = std::numeric_limits<float>::infinity();
                        } else if ((globalX + globalY) % 139 == 0) {
                            gain = -0.0f;
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
};

bool equal_double_bits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
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
        (static_cast<std::size_t>(width) +
         static_cast<std::size_t>(smsb2::kCanonicalCore) - 1u) /
        static_cast<std::size_t>(smsb2::kCanonicalCore);
    const std::size_t rows =
        (static_cast<std::size_t>(height) +
         static_cast<std::size_t>(smsb2::kCanonicalCore) - 1u) /
        static_cast<std::size_t>(smsb2::kCanonicalCore);
    return columns * rows;
}

void compare_case(int width, int height, Pattern pattern, const char* label) {
    auto frame = make_frame(width, height, pattern);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction canonicalReconstruction;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction candidateReconstruction;
    CountingFrameSource canonicalSource(frame);
    CountingFrameSource candidateSource(frame);

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

    // Repeated-run determinism is part of the parity oracle.
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction repeatedReconstruction;
    CountingFrameSource repeatedSource(frame);
    ega3::Result repeated{};
    ega3::Diagnostics repeatedDiagnostics{};
    const auto repeatedStatus = ega3::bind_scientific_master_retained_exact_gauge(
        repeatedSource, repeatedReconstruction, options, repeated, repeatedDiagnostics);
    REQUIRE(repeatedStatus);
    require_scientific_parity(candidate, repeated);
    REQUIRE(equal_double_bits(candidate.zeroLineGauge.L0, repeated.zeroLineGauge.L0));
    REQUIRE(diagnostics.lowerMedianFloat32Bits == repeatedDiagnostics.lowerMedianFloat32Bits);
    REQUIRE(diagnostics.upperMedianFloat32Bits == repeatedDiagnostics.upperMedianFloat32Bits);
}

void test_equivalence_matrix() {
    compare_case(130, 70, Pattern::PseudoRandom, "pseudo-random");
    compare_case(128, 64, Pattern::Constant, "constant-even");
    compare_case(129, 65, Pattern::Constant, "constant-odd");
    compare_case(128, 64, Pattern::SplitMedian, "split-median");
    compare_case(129, 65, Pattern::SparseEligible, "sparse-eligible");
    compare_case(132, 72, Pattern::SpecialFloatGain, "nan-inf-signed-zero-gain");
    compare_case(134, 74, Pattern::ResidualBlackAndGain, "residual-black-gain");
}

void test_geometric_bound_contract() {
    auto frame = make_frame(130, 70, Pattern::Constant);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    CountingFrameSource source(frame);
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
    CountingFrameSource baselineSource(frame);
    smsb2::Result baseline{};
    smsb2::Options unlimited{};
    const auto baselineStatus = smsb2::bind_scientific_master_streaming(
        baselineSource, baselineReconstruction, unlimited, baseline);
    REQUIRE(baselineStatus);

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction fallbackReconstruction;
    CountingFrameSource fallbackSource(frame);
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
    REQUIRE(diagnostics.stage2GaugeScanPassesActuallyUsed == 2u);
    REQUIRE(diagnostics.pass2Stage2TileReadsAvoided == 0u);
    REQUIRE(fallback.stage2GaugeScanPasses == 2u);
    REQUIRE(fallbackSource.rawTileCalls == tile_count(frame.meta.width, frame.meta.height) * 2u);
}

}  // namespace

int main() {
    test_equivalence_matrix();
    test_geometric_bound_contract();
    test_budget_forces_complete_v0_2_fallback();
    std::cout << "Scientific Master Exact Gauge Retained Artifact v0.3: PASS\n";
    return 0;
}

#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"
#include "scientific_master_streaming_binding_v0_2.h"

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

void require_(bool ok, const char* expr, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expr << '\n';
        std::exit(2);
    }
}
#define REQUIRE(x) require_(static_cast<bool>(x), #x, __LINE__)

enum class Pattern {
    Pseudo,
    Constant,
    Split,
    Sparse,
    SignedZeroGain,
    NonFiniteGain,
    ResidualBlackGain,
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
        pattern == Pattern::ResidualBlackGain;
    frame.meta.hasResidualBlack = pattern == Pattern::ResidualBlackGain;
    frame.meta.sourceId = "draw_ega3_oracle";

    frame.raw.resize(static_cast<std::size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint16_t value = 512u;
            switch (pattern) {
                case Pattern::Pseudo:
                case Pattern::SignedZeroGain:
                case Pattern::NonFiniteGain:
                case Pattern::ResidualBlackGain:
                    value = static_cast<std::uint16_t>(
                        70 + ((x * 37 + y * 53 + x * y * 3) % 900));
                    if ((x + y) % 97 == 0) value = 1023u;
                    break;
                case Pattern::Constant:
                    value = 512u;
                    break;
                case Pattern::Split:
                    value = x < width / 2 ? 100u : 900u;
                    break;
                case Pattern::Sparse:
                    if ((x + 3 * y) % 11 == 0) value = 65u;
                    else if ((2 * x + y) % 17 == 0) value = 180u;
                    else if ((x + y) % 19 == 0) value = 1023u;
                    else value = 64u;
                    break;
            }
            frame.raw[static_cast<std::size_t>(y) * width + x] = value;
        }
    }
    return frame;
}

class Source final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    Source(const truthraw::DecodedDngFrame& frame, Pattern pattern)
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
        const std::size_t count = static_cast<std::size_t>(width) * height;
        if (!rawOut || rawCount != count) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "raw tile count mismatch");
        }
        if (frame_.meta.hasGainField) {
            if (!gainOut || gainCount != count) {
                return truthraw::streaming_v0_1::StreamStatus::error(
                    truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                    "gain tile count mismatch");
            }
        } else if (gainOut || gainCount != 0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected gain request");
        }

        ++tileReads;
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                const int x = rect.hx0 + xx;
                const int y = rect.hy0 + yy;
                const std::size_t src = static_cast<std::size_t>(y) * frame_.meta.width + x;
                const std::size_t dst = static_cast<std::size_t>(yy) * width + xx;
                rawOut[dst] = frame_.raw[src];
                if (gainOut) {
                    float gain = 1.0f + 0.0005f * static_cast<float>((x + 2 * y) % 31);
                    if (pattern_ == Pattern::SignedZeroGain && (x + y) % 139 == 0) {
                        gain = -0.0f;
                    } else if (pattern_ == Pattern::NonFiniteGain) {
                        if ((x + 5 * y) % 131 == 0) {
                            gain = std::numeric_limits<float>::quiet_NaN();
                        } else if ((3 * x + y) % 137 == 0) {
                            gain = std::numeric_limits<float>::infinity();
                        }
                    }
                    gainOut[dst] = gain;
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
                    "unexpected row bias");
        }
        if (!out || count != static_cast<std::size_t>(y1 - y0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "row bias count mismatch");
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
                    "unexpected col bias");
        }
        if (!out || count != static_cast<std::size_t>(x1 - x0)) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "col bias count mismatch");
        }
        for (std::size_t i = 0; i < count; ++i) {
            out[i] = 0.125f * static_cast<float>((x0 + static_cast<int>(i)) % 7 - 3);
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    std::size_t tileReads = 0u;

private:
    const truthraw::DecodedDngFrame& frame_;
    Pattern pattern_;
};

class Observer final : public smsb2::ICanonicalTileObserver {
public:
    std::size_t residentBytesUpperBound() const noexcept override { return 0u; }
    bool observeCanonicalTile(
        std::uint32_t,
        std::uint32_t,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint16_t* rawCore,
        std::size_t rawCount,
        const float* rgb,
        std::size_t floatCount) noexcept override {
        const std::size_t expected = static_cast<std::size_t>(width) * height;
        if (!rawCore || !rgb || rawCount != expected || floatCount != 3u * expected) return false;
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

std::size_t tiles_for(int width, int height) {
    const std::size_t core = static_cast<std::size_t>(smsb2::kCanonicalCore);
    return ((static_cast<std::size_t>(width) + core - 1u) / core) *
           ((static_cast<std::size_t>(height) + core - 1u) / core);
}

void parity(const smsb2::Result& a, const ega3::Result& b) {
    REQUIRE(a.scientificMasterHash == b.scientificMasterHash);
    REQUIRE(a.zeroLineGauge.mode == b.zeroLineGauge.mode);
    REQUIRE(equal_double_bits(a.zeroLineGauge.L0, b.zeroLineGauge.L0));
    REQUIRE(a.zeroLineGauge.gaugeId == b.zeroLineGauge.gaugeId);
    REQUIRE(a.zeroLineGauge.crossSceneComparable == b.zeroLineGauge.crossSceneComparable);
    REQUIRE(a.zeroLineGauge.absolutePhysicalUnits == b.zeroLineGauge.absolutePhysicalUnits);
    REQUIRE(a.sceneBinding.reconstructionBackend == b.sceneBinding.reconstructionBackend);
    REQUIRE(a.sceneBinding.sceneScaleId == b.sceneBinding.sceneScaleId);
    REQUIRE(a.sceneBinding.gainMapAppliedExactlyOnce == b.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(a.sceneBinding.exposureNormalizedToCommonScene == b.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(a.sceneBinding.gainNormalizedToCommonScene == b.sceneBinding.gainNormalizedToCommonScene);
    REQUIRE(a.selfGaugeEligibleSamples == b.selfGaugeEligibleSamples);
    REQUIRE(a.masterTilesProcessed == b.masterTilesProcessed);
    REQUIRE(a.physicalFrameCount == b.physicalFrameCount);
    REQUIRE(a.independentEvidenceCount == b.independentEvidenceCount);
}

void success_case(int width, int height, Pattern pattern) {
    auto frame = make_frame(width, height, pattern);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r2;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r3;
    Source s2(frame, pattern);
    Source s3(frame, pattern);
    smsb2::Result v2{};
    ega3::Result v3{};
    ega3::Diagnostics d{};
    const smsb2::Options options{};
    REQUIRE(smsb2::bind_scientific_master_streaming(s2, r2, options, v2));
    REQUIRE(ega3::bind_scientific_master_retained_exact_gauge(s3, r3, options, v3, d));
    parity(v2, v3);
    const auto tiles = tiles_for(width, height);
    REQUIRE(v2.stage2GaugeScanPasses == 2u);
    REQUIRE(v3.stage2GaugeScanPasses == 1u);
    REQUIRE(s2.tileReads == 2u * tiles);
    REQUIRE(s3.tileReads == tiles);
    REQUIRE(d.routeUsed == ega3::RouteUsed::RetainedExactFloat32Bits);
    REQUIRE(d.fallbackReason == ega3::FallbackReason::None);
    REQUIRE(d.optimizationApplied);
    REQUIRE(d.eligibleRetainedSamples == v3.selfGaugeEligibleSamples);
    REQUIRE(d.retainedBytesUsed == v3.selfGaugeEligibleSamples * sizeof(std::uint32_t));
    REQUIRE(d.pass2Stage2TileReadsAvoided == tiles);
    REQUIRE(!d.sourceValuesModified && !d.createsNewEvidence && !d.scientificWritebackAllowed);

    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction rr;
    Source sr(frame, pattern);
    ega3::Result repeat{};
    ega3::Diagnostics dr{};
    REQUIRE(ega3::bind_scientific_master_retained_exact_gauge(sr, rr, options, repeat, dr));
    parity(v3, repeat);
    REQUIRE(d.lowerMedianFloat32Bits == dr.lowerMedianFloat32Bits);
    REQUIRE(d.upperMedianFloat32Bits == dr.upperMedianFloat32Bits);
}

void test_matrix() {
    success_case(130, 70, Pattern::Pseudo);
    success_case(128, 64, Pattern::Constant);
    success_case(129, 65, Pattern::Constant);
    success_case(128, 64, Pattern::Split);
    success_case(129, 65, Pattern::Sparse);
    success_case(132, 72, Pattern::SignedZeroGain);
    success_case(134, 74, Pattern::ResidualBlackGain);
}

void test_exact_constant_bits_and_geometry() {
    auto frame = make_frame(129, 65, Pattern::Constant);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r;
    Source s(frame, Pattern::Constant);
    ega3::Result result{};
    ega3::Diagnostics d{};
    const ega3::Options options{};
    REQUIRE(ega3::bind_scientific_master_retained_exact_gauge(s, r, options, result, d));
    const float expected = (512.0f - 64.0f) / (1023.0f - 64.0f);
    REQUIRE(d.lowerMedianFloat32Bits == float_bits(expected));
    REQUIRE(d.upperMedianFloat32Bits == float_bits(expected));
    REQUIRE(equal_double_bits(result.zeroLineGauge.L0, static_cast<double>(expected)));
    const int bx = static_cast<int>(std::floor(129.0 * smsb2::kSelfGaugeBorderFraction));
    const int by = static_cast<int>(std::floor(65.0 * smsb2::kSelfGaugeBorderFraction));
    const std::uint64_t bound = static_cast<std::uint64_t>(129 - 2 * bx) *
                                static_cast<std::uint64_t>(65 - 2 * by);
    REQUIRE(d.geometricEligibleUpperBound == bound);
    REQUIRE(d.retainedBytesRequested == bound * sizeof(std::uint32_t));
    REQUIRE(result.selfGaugeEligibleSamples == bound);
}

void test_nonfinite_rejected_before_gauge() {
    auto frame = make_frame(132, 72, Pattern::NonFiniteGain);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r2;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r3;
    Source s2(frame, Pattern::NonFiniteGain);
    Source s3(frame, Pattern::NonFiniteGain);
    smsb2::Result v2{};
    ega3::Result v3{};
    ega3::Diagnostics d{};
    const smsb2::Options options{};
    const auto a = smsb2::bind_scientific_master_streaming(s2, r2, options, v2);
    const auto b = ega3::bind_scientific_master_retained_exact_gauge(s3, r3, options, v3, d);
    REQUIRE(!a && !b);
    REQUIRE(a.code == smsb2::StatusCode::DigestFailed);
    REQUIRE(b.code == ega3::StatusCode::DigestFailed);
    REQUIRE(a.message == b.message);
    REQUIRE(!d.optimizationApplied);
}

void test_observed_parity() {
    auto frame = make_frame(130, 70, Pattern::Pseudo);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r2;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r3;
    Source s2(frame, Pattern::Pseudo);
    Source s3(frame, Pattern::Pseudo);
    Observer o2;
    Observer o3;
    smsb2::Result v2{};
    ega3::Result v3{};
    ega3::Diagnostics d{};
    const smsb2::Options options{};
    REQUIRE(smsb2::bind_scientific_master_streaming_observed(s2, r2, options, o2, v2));
    REQUIRE(ega3::bind_scientific_master_retained_exact_gauge_observed(s3, r3, options, o3, v3, d));
    parity(v2, v3);
    const auto tiles = tiles_for(frame.meta.width, frame.meta.height);
    REQUIRE(o2.tiles == tiles && o3.tiles == tiles);
    REQUIRE(o2.samples == frame.raw.size() && o3.samples == frame.raw.size());
    REQUIRE(s2.tileReads == 2u * tiles);
    REQUIRE(s3.tileReads == tiles);
}

void test_explicit_artifact_budget_fallback() {
    auto frame = make_frame(130, 70, Pattern::Pseudo);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r2;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction r3;
    Source s2(frame, Pattern::Pseudo);
    Source s3(frame, Pattern::Pseudo);
    smsb2::Result v2{};
    ega3::Result v3{};
    ega3::Diagnostics d{};
    const smsb2::Options options{};
    REQUIRE(smsb2::bind_scientific_master_streaming(s2, r2, options, v2));

    ega3::ArtifactOptions artifactOptions{};
    artifactOptions.retainedArtifactBudgetBytes = 1u;
    REQUIRE(ega3::bind_scientific_master_retained_exact_gauge(
        s3, r3, options, artifactOptions, v3, d));
    parity(v2, v3);
    REQUIRE(d.routeUsed == ega3::RouteUsed::CanonicalV02Fallback);
    REQUIRE(d.fallbackReason == ega3::FallbackReason::RetainedArtifactBudgetInsufficient);
    REQUIRE(d.retainedArtifactBudgetBytes == 1u);
    REQUIRE(d.retainedBytesRequested > d.retainedArtifactBudgetBytes);
    REQUIRE(!d.optimizationApplied);
    REQUIRE(v3.stage2GaugeScanPasses == 2u);
    REQUIRE(d.stage2GaugeScanPassesActuallyUsed == 2u);
    REQUIRE(d.pass2Stage2TileReadsAvoided == 0u);
    REQUIRE(s3.tileReads == 2u * tiles_for(frame.meta.width, frame.meta.height));
}

}  // namespace

int main() {
    test_matrix();
    test_exact_constant_bits_and_geometry();
    test_nonfinite_rejected_before_gauge();
    test_observed_parity();
    test_explicit_artifact_budget_fallback();
    std::cout << "D.RAW Exact Gauge Retained Artifact v0.3 parity oracle: PASS\n";
    return 0;
}

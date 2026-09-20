#include "scientific_master_streaming_binding_v0_2.h"
#include "scientific_master_streaming_binding_v0_3.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace smsb2 = truthraw::scientific_master_streaming_binding::v0_2;
namespace smsb3 = truthraw::scientific_master_streaming_binding::v0_3;

namespace {

void require_active(bool ok, const char* expression, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expression << '\n';
        std::exit(2);
    }
}
#define REQUIRE(expr) require_active(static_cast<bool>(expr), #expr, __LINE__)

enum class Pattern { PseudoRandom, Constant, SplitMedian, SparseEligible };

truthraw::DecodedDngFrame make_frame(
    int width,
    int height,
    Pattern pattern,
    bool withGain,
    bool withResidualBlack) {
    truthraw::DecodedDngFrame frame{};
    frame.meta.width = width;
    frame.meta.height = height;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Normal;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 65.0f, 63.0f, 66.0f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.noiseProfile = {0.0009f, 1.0e-6f, 0.0010f, 1.2e-6f, 0.0011f, 1.4e-6f};
    frame.meta.hasGainField = withGain;
    frame.meta.hasResidualBlack = withResidualBlack;
    frame.meta.sourceId = "synthetic_stripe_equivalence";

    const std::size_t count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    frame.raw.resize(count);
    if (withGain) frame.gainField.resize(count);
    if (withResidualBlack) {
        frame.rowBias.resize(static_cast<std::size_t>(height));
        frame.colBias.resize(static_cast<std::size_t>(width));
    }

    for (int y = 0; y < height; ++y) {
        if (withResidualBlack) {
            frame.rowBias[static_cast<std::size_t>(y)] =
                static_cast<float>((y % 5) - 2) * 0.125f;
        }
        for (int x = 0; x < width; ++x) {
            if (withResidualBlack && y == 0) {
                frame.colBias[static_cast<std::size_t>(x)] =
                    static_cast<float>((x % 7) - 3) * 0.0625f;
            }
            std::uint16_t value = 512u;
            switch (pattern) {
                case Pattern::PseudoRandom:
                    value = static_cast<std::uint16_t>(70 + ((x * 37 + y * 53 + x * y * 3) % 900));
                    if ((x + y) % 97 == 0) value = 1023u;
                    break;
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
                    else value = 64u;
                    break;
            }
            const std::size_t i =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x);
            frame.raw[i] = value;
            if (withGain) {
                frame.gainField[i] =
                    0.92f + 0.01f * static_cast<float>((x + 2 * y) % 17);
            }
        }
    }
    return frame;
}

class CountingFrameSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit CountingFrameSource(const truthraw::DecodedDngFrame& frame) : frame_(frame) {}

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
                const std::size_t si =
                    static_cast<std::size_t>(rect.hy0 + y) *
                        static_cast<std::size_t>(frame_.meta.width) +
                    static_cast<std::size_t>(rect.hx0 + x);
                const std::size_t di =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(x);
                rawOut[di] = frame_.raw[si];
                if (frame_.meta.hasGainField) gainOut[di] = frame_.gainField[si];
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
        const std::size_t need = static_cast<std::size_t>(y1 - y0);
        if (out == nullptr || count != need) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "row-bias count mismatch");
        }
        std::copy(frame_.rowBias.begin() + y0, frame_.rowBias.begin() + y1, out);
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
        const std::size_t need = static_cast<std::size_t>(x1 - x0);
        if (out == nullptr || count != need) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "col-bias count mismatch");
        }
        std::copy(frame_.colBias.begin() + x0, frame_.colBias.begin() + x1, out);
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

std::size_t ceil_div(std::size_t n, std::size_t d) {
    return (n + d - 1u) / d;
}

void compare_case(
    int width,
    int height,
    Pattern pattern,
    bool gain,
    bool residual,
    const char* label) {
    auto frame = make_frame(width, height, pattern, gain, residual);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstructionV2;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstructionV3;
    CountingFrameSource sourceV2(frame);
    CountingFrameSource sourceV3(frame);

    smsb2::Result v2{};
    smsb3::Result v3{};
    const smsb2::Options options{};
    const auto s2 = smsb2::bind_scientific_master_streaming(
        sourceV2, reconstructionV2, options, v2);
    const auto s3 = smsb3::bind_scientific_master_streaming(
        sourceV3, reconstructionV3, options, v3);
    if (!s2) std::cerr << label << " v0.2 failed: " << s2.message << '\n';
    if (!s3) std::cerr << label << " v0.3 failed: " << s3.message << '\n';
    REQUIRE(s2);
    REQUIRE(s3);

    REQUIRE(v2.scientificMasterHash == v3.scientificMasterHash);
    REQUIRE(v2.zeroLineGauge.mode == v3.zeroLineGauge.mode);
    REQUIRE(v2.zeroLineGauge.gaugeId == v3.zeroLineGauge.gaugeId);
    REQUIRE(equal_double_bits(v2.zeroLineGauge.L0, v3.zeroLineGauge.L0));
    REQUIRE(v2.zeroLineGauge.crossSceneComparable == v3.zeroLineGauge.crossSceneComparable);
    REQUIRE(v2.zeroLineGauge.absolutePhysicalUnits == v3.zeroLineGauge.absolutePhysicalUnits);
    REQUIRE(v2.sceneBinding.reconstructionBackend == v3.sceneBinding.reconstructionBackend);
    REQUIRE(v2.sceneBinding.sceneScaleId == v3.sceneBinding.sceneScaleId);
    REQUIRE(v2.sceneBinding.gainMapAppliedExactlyOnce == v3.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(v2.sceneBinding.exposureNormalizedToCommonScene ==
            v3.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(v2.sceneBinding.gainNormalizedToCommonScene ==
            v3.sceneBinding.gainNormalizedToCommonScene);
    REQUIRE(v2.selfGaugeEligibleSamples == v3.selfGaugeEligibleSamples);
    REQUIRE(v2.masterTilesProcessed == v3.masterTilesProcessed);
    REQUIRE(v2.physicalFrameCount == 1u && v3.physicalFrameCount == 1u);
    REQUIRE(v2.independentEvidenceCount == 1u && v3.independentEvidenceCount == 1u);
    REQUIRE(v2.stage2GaugeScanPasses == 2u && v3.stage2GaugeScanPasses == 2u);

    const std::size_t canonicalTiles =
        ceil_div(static_cast<std::size_t>(width), static_cast<std::size_t>(smsb3::kCanonicalCore)) *
        ceil_div(static_cast<std::size_t>(height), static_cast<std::size_t>(smsb3::kCanonicalCore));
    REQUIRE(v3.masterTilesProcessed == canonicalTiles);

    const std::size_t digestCalls =
        ceil_div(static_cast<std::size_t>(width), static_cast<std::size_t>(smsb3::kDigestStripeCoreWidth)) *
        ceil_div(static_cast<std::size_t>(height), static_cast<std::size_t>(smsb3::kDigestStripeCoreHeight));
    const std::size_t gaugeCalls =
        ceil_div(static_cast<std::size_t>(width), static_cast<std::size_t>(smsb3::kGaugeStripeCoreWidth)) *
        ceil_div(static_cast<std::size_t>(height), static_cast<std::size_t>(smsb3::kGaugeStripeCoreHeight));
    REQUIRE(sourceV3.rawTileCalls == digestCalls + gaugeCalls);
    REQUIRE(sourceV3.rawTileCalls < sourceV2.rawTileCalls);
    REQUIRE(sourceV3.rawSamplesRead <= sourceV2.rawSamplesRead);
}

void test_equivalence_matrix() {
    compare_case(130, 70, Pattern::PseudoRandom, false, false, "pseudo-random");
    compare_case(128, 64, Pattern::Constant, true, false, "gain-field");
    compare_case(128, 64, Pattern::SplitMedian, false, true, "residual-black");
    compare_case(129, 65, Pattern::SparseEligible, true, true, "gain-plus-residual");
    compare_case(2113, 193, Pattern::PseudoRandom, true, true, "multi-stripe");
}

void test_budget_fails_before_source_reads() {
    auto frame = make_frame(130, 70, Pattern::PseudoRandom, true, true);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    CountingFrameSource source(frame);
    smsb3::Result result{};
    smsb3::Options options{};
    options.memoryBudgetBytes = smsb3::kMaximumRadixAuxiliaryBytes - 1u;
    const auto status = smsb3::bind_scientific_master_streaming(
        source, reconstruction, options, result);
    REQUIRE(!status);
    REQUIRE(status.code == smsb3::StatusCode::BudgetExceeded);
    REQUIRE(source.rawTileCalls == 0u);
}

}  // namespace

int main() {
    test_equivalence_matrix();
    test_budget_fails_before_source_reads();
    std::cout << "Scientific Master Streaming Binding v0.3 stripes: PASS\n";
    return 0;
}

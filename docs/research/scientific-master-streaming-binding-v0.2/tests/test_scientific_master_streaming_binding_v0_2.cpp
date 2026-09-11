#include "scientific_master_streaming_binding_v0_1.h"
#include "scientific_master_streaming_binding_v0_2.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace smsb1 = truthraw::scientific_master_streaming_binding::v0_1;
namespace smsb2 = truthraw::scientific_master_streaming_binding::v0_2;

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
    frame.meta.hasGainField = false;
    frame.meta.hasResidualBlack = false;
    frame.meta.sourceId = "synthetic_radix16_equivalence";

    const std::size_t count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    frame.raw.resize(count);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint16_t value = 512u;
            switch (pattern) {
                case Pattern::PseudoRandom: {
                    const int v = 70 + ((x * 37 + y * 53 + x * y * 3) % 900);
                    value = static_cast<std::uint16_t>(v);
                    if ((x + y) % 97 == 0) value = 1023u;
                    break;
                }
                case Pattern::Constant:
                    value = 512u;
                    break;
                case Pattern::SplitMedian:
                    // On an even-width, symmetric border crop this produces
                    // equal low/high populations. The lower and upper median
                    // therefore intentionally occupy distant float prefixes.
                    value = x < width / 2 ? 100u : 900u;
                    break;
                case Pattern::SparseEligible:
                    // Exercise exclusion of Stage-2 zero/below-black and
                    // source-censored WhiteLevel while retaining some signal.
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
        if (gainOut != nullptr || gainCount != 0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected gain request");
        }
        ++rawTileCalls;
        rawSamplesRead += count;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t sourceIndex =
                    static_cast<std::size_t>(rect.hy0 + y) *
                        static_cast<std::size_t>(frame_.meta.width) +
                    static_cast<std::size_t>(rect.hx0 + x);
                const std::size_t destinationIndex =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(x);
                rawOut[destinationIndex] = frame_.raw[sourceIndex];
            }
        }
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readRowBias(
        int, int, float*, std::size_t count) override {
        return count == 0u
            ? truthraw::streaming_v0_1::StreamStatus::ok()
            : truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected row-bias request");
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int, int, float*, std::size_t count) override {
        return count == 0u
            ? truthraw::streaming_v0_1::StreamStatus::ok()
            : truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected col-bias request");
    }

    std::size_t rawTileCalls = 0u;
    std::size_t rawSamplesRead = 0u;

private:
    const truthraw::DecodedDngFrame& frame_;
};

bool equal_double_bits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

void compare_case(int width, int height, Pattern pattern, const char* label) {
    auto frame = make_frame(width, height, pattern);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstructionV1;
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstructionV2;
    CountingFrameSource sourceV1(frame);
    CountingFrameSource sourceV2(frame);

    smsb1::Result v1{};
    smsb2::Result v2{};
    const smsb1::Options options{};
    const auto s1 = smsb1::bind_scientific_master_streaming(
        sourceV1, reconstructionV1, options, v1);
    const auto s2 = smsb2::bind_scientific_master_streaming(
        sourceV2, reconstructionV2, options, v2);
    if (!s1) std::cerr << label << " v0.1 failed: " << s1.message << '\n';
    if (!s2) std::cerr << label << " v0.2 failed: " << s2.message << '\n';
    REQUIRE(s1);
    REQUIRE(s2);

    REQUIRE(v1.scientificMasterHash == v2.scientificMasterHash);
    REQUIRE(v1.zeroLineGauge.mode == v2.zeroLineGauge.mode);
    REQUIRE(v1.zeroLineGauge.gaugeId == v2.zeroLineGauge.gaugeId);
    REQUIRE(equal_double_bits(v1.zeroLineGauge.L0, v2.zeroLineGauge.L0));
    REQUIRE(v1.zeroLineGauge.crossSceneComparable == v2.zeroLineGauge.crossSceneComparable);
    REQUIRE(v1.zeroLineGauge.absolutePhysicalUnits == v2.zeroLineGauge.absolutePhysicalUnits);

    REQUIRE(v1.sceneBinding.reconstructionBackend == v2.sceneBinding.reconstructionBackend);
    REQUIRE(v1.sceneBinding.sceneScaleId == v2.sceneBinding.sceneScaleId);
    REQUIRE(v1.sceneBinding.gainMapAppliedExactlyOnce == v2.sceneBinding.gainMapAppliedExactlyOnce);
    REQUIRE(v1.sceneBinding.exposureNormalizedToCommonScene ==
            v2.sceneBinding.exposureNormalizedToCommonScene);
    REQUIRE(v1.sceneBinding.gainNormalizedToCommonScene ==
            v2.sceneBinding.gainNormalizedToCommonScene);

    REQUIRE(v1.selfGaugeEligibleSamples == v2.selfGaugeEligibleSamples);
    REQUIRE(v1.masterTilesProcessed == v2.masterTilesProcessed);
    REQUIRE(v1.physicalFrameCount == 1u && v2.physicalFrameCount == 1u);
    REQUIRE(v1.independentEvidenceCount == 1u && v2.independentEvidenceCount == 1u);
    REQUIRE(v1.stage2GaugeScanPasses == 4u);
    REQUIRE(v2.stage2GaugeScanPasses == 2u);

    const std::size_t tileColumns =
        (static_cast<std::size_t>(width) + static_cast<std::size_t>(smsb1::kCanonicalCore) - 1u) /
        static_cast<std::size_t>(smsb1::kCanonicalCore);
    const std::size_t tileRows =
        (static_cast<std::size_t>(height) + static_cast<std::size_t>(smsb1::kCanonicalCore) - 1u) /
        static_cast<std::size_t>(smsb1::kCanonicalCore);
    const std::size_t tiles = tileColumns * tileRows;
    REQUIRE(v1.masterTilesProcessed == tiles);
    REQUIRE(sourceV1.rawTileCalls == tiles * 4u);
    REQUIRE(sourceV2.rawTileCalls == tiles * 2u);
    REQUIRE(sourceV1.rawSamplesRead == sourceV2.rawSamplesRead * 2u);

    // The optimization trades bounded memory for fewer full Stage-2 scans.
    REQUIRE(v2.logicalResidentUpperBound > v1.logicalResidentUpperBound);
}

void test_equivalence_matrix() {
    compare_case(130, 70, Pattern::PseudoRandom, "pseudo-random");
    compare_case(128, 64, Pattern::Constant, "constant");
    compare_case(128, 64, Pattern::SplitMedian, "split-median");
    compare_case(129, 65, Pattern::SparseEligible, "sparse-eligible");
}

void test_v0_2_budget_fails_before_source_reads() {
    auto frame = make_frame(130, 70, Pattern::PseudoRandom);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    CountingFrameSource source(frame);
    smsb2::Result result{};
    smsb2::Options options{};
    options.memoryBudgetBytes = smsb2::kMaximumRadixAuxiliaryBytes - 1u;
    const auto status = smsb2::bind_scientific_master_streaming(
        source, reconstruction, options, result);
    REQUIRE(!status);
    REQUIRE(status.code == smsb2::StatusCode::BudgetExceeded);
    REQUIRE(source.rawTileCalls == 0u);
}

}  // namespace

int main() {
    test_equivalence_matrix();
    test_v0_2_budget_fails_before_source_reads();
    std::cout << "Scientific Master Streaming Binding v0.2: PASS\n";
    return 0;
}

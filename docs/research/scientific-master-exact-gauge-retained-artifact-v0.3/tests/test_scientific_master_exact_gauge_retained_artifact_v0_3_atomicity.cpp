#include "scientific_master_exact_gauge_retained_artifact_v0_3.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace ega3 = truthraw::scientific_master_exact_gauge_retained_artifact::v0_3;
namespace smsb2 = truthraw::scientific_master_streaming_binding::v0_2;

namespace {

void require_(bool ok, const char* expression, int line) {
    if (!ok) {
        std::cerr << "REQUIRE_FAIL line=" << line << " expr=" << expression << '\n';
        std::exit(2);
    }
}
#define REQUIRE(x) require_(static_cast<bool>(x), #x, __LINE__)

truthraw::DecodedDngFrame make_frame(int width = 130, int height = 70) {
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
    frame.meta.sourceId = "draw_ega3_atomicity";
    frame.raw.assign(static_cast<std::size_t>(width) * height, 512u);
    return frame;
}

class CountingSource final : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit CountingSource(const truthraw::DecodedDngFrame& frame) : frame_(frame) {}

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
        if (!rawOut || rawCount != count || gainOut != nullptr || gainCount != 0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "atomicity source request mismatch");
        }
        ++rawTileCalls;
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                const int x = rect.hx0 + xx;
                const int y = rect.hy0 + yy;
                rawOut[static_cast<std::size_t>(yy) * width + xx] =
                    frame_.raw[static_cast<std::size_t>(y) * frame_.meta.width + x];
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
                  "unexpected row bias request");
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int, int, float*, std::size_t count) override {
        return count == 0u
            ? truthraw::streaming_v0_1::StreamStatus::ok()
            : truthraw::streaming_v0_1::StreamStatus::error(
                  truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                  "unexpected col bias request");
    }

    std::size_t rawTileCalls = 0u;

private:
    const truthraw::DecodedDngFrame& frame_;
};

class CountingObserver final : public smsb2::ICanonicalTileObserver {
public:
    enum class Mode { Accept, RejectFirst, GrowResidentAfterFirst };

    explicit CountingObserver(Mode mode) : mode_(mode) {}

    std::size_t residentBytesUpperBound() const noexcept override {
        return residentUpperBound_;
    }

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
        if (!rawCore || !cameraNativeRgb || rawCount != expected ||
            floatCount != 3u * expected) {
            return false;
        }
        ++calls;
        if (mode_ == Mode::GrowResidentAfterFirst && calls == 1u) {
            residentUpperBound_ = 8u * 1024u * 1024u;
        }
        if (mode_ == Mode::RejectFirst && calls == 1u) return false;
        return true;
    }

    std::size_t calls = 0u;

private:
    Mode mode_;
    std::size_t residentUpperBound_ = 0u;
};

std::size_t tile_count(int width, int height) {
    const std::size_t core = static_cast<std::size_t>(smsb2::kCanonicalCore);
    return ((static_cast<std::size_t>(width) + core - 1u) / core) *
           ((static_cast<std::size_t>(height) + core - 1u) / core);
}

void test_pre_admission_fallback_observes_exactly_once() {
    auto frame = make_frame();
    CountingSource source(frame);
    CountingObserver observer(CountingObserver::Mode::Accept);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    ega3::Options options{};
    ega3::ArtifactOptions artifactOptions{};
    artifactOptions.retainedArtifactBudgetBytes = 1u;
    ega3::Result result{};
    ega3::Diagnostics diagnostics{};

    const auto status = ega3::bind_scientific_master_retained_exact_gauge_observed(
        source, reconstruction, options, artifactOptions, observer, result, diagnostics);
    REQUIRE(status);
    const std::size_t tiles = tile_count(frame.meta.width, frame.meta.height);
    REQUIRE(diagnostics.routeUsed == ega3::RouteUsed::CanonicalV02Fallback);
    REQUIRE(!diagnostics.optimizationApplied);
    REQUIRE(observer.calls == tiles);
    REQUIRE(source.rawTileCalls == 2u * tiles);
    REQUIRE(result.stage2GaugeScanPasses == 2u);
}

void test_observer_rejection_after_start_never_replays() {
    auto frame = make_frame();
    CountingSource source(frame);
    CountingObserver observer(CountingObserver::Mode::RejectFirst);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    ega3::Options options{};
    ega3::Result result{};
    ega3::Diagnostics diagnostics{};

    const auto status = ega3::bind_scientific_master_retained_exact_gauge_observed(
        source, reconstruction, options, observer, result, diagnostics);
    REQUIRE(!status);
    REQUIRE(status.code == ega3::StatusCode::DigestFailed);
    REQUIRE(diagnostics.fallbackReason == ega3::FallbackReason::None);
    REQUIRE(!diagnostics.optimizationApplied);
    REQUIRE(observer.calls == 1u);
    REQUIRE(source.rawTileCalls == 1u);
}

void test_post_admission_resident_contract_breach_is_hard_failure() {
    auto frame = make_frame();
    CountingSource source(frame);
    CountingObserver observer(CountingObserver::Mode::GrowResidentAfterFirst);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    ega3::Options options{};
    options.memoryBudgetBytes = 4u * 1024u * 1024u;
    ega3::Result result{};
    ega3::Diagnostics diagnostics{};

    const auto status = ega3::bind_scientific_master_retained_exact_gauge_observed(
        source, reconstruction, options, observer, result, diagnostics);
    REQUIRE(!status);
    REQUIRE(status.code == ega3::StatusCode::BudgetExceeded);
    REQUIRE(diagnostics.fallbackReason == ega3::FallbackReason::None);
    REQUIRE(!diagnostics.optimizationApplied);
    REQUIRE(observer.calls == 1u);
    REQUIRE(source.rawTileCalls == 1u);
}

}  // namespace

int main() {
    test_pre_admission_fallback_observes_exactly_once();
    test_observer_rejection_after_start_never_replays();
    test_post_admission_resident_contract_breach_is_hard_failure();
    std::cout << "D.RAW Exact Gauge v0.3 atomic no-replay gate: PASS\n";
    return 0;
}

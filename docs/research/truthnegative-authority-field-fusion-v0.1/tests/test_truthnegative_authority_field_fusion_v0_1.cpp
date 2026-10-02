#include "scientific_master_streaming_binding_v0_2.h"
#include "streaming_scientific_master_tile_source_v0_1.h"
#include "truthnegative_continuous_v0_5.h"
#include "truthnegative_dense_local_field_adapter_v0_4.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <vector>

namespace smsb =
    truthraw::scientific_master_streaming_binding::v0_2;
namespace master =
    truthraw::scientific_master_linear_dng_projection::v0_1;
namespace tn =
    truthraw::truthnegative_continuous::v0_5;
namespace adapter =
    truthraw::truthnegative_dense_local_field_adapter::v0_4;

namespace {

void require_active(
    bool ok,
    const char* expression,
    int line) {
    if (!ok) {
        std::cerr
            << "REQUIRE_FAIL line=" << line
            << " expr=" << expression << "\n";
        std::exit(2);
    }
}
#define REQUIRE(expr) \
    require_active(static_cast<bool>(expr), #expr, __LINE__)

truthraw::DecodedDngFrame make_frame(
    int width,
    int height) {
    truthraw::DecodedDngFrame frame{};
    frame.meta.width = width;
    frame.meta.height = height;
    frame.meta.cfa = truthraw::CfaPattern::BGGR;
    frame.meta.orientation = truthraw::Orientation::Normal;
    frame.meta.whiteLevel = 1023.0f;
    frame.meta.blackPhase = {64.0f, 65.0f, 66.0f, 67.0f};
    frame.meta.hasNoiseProfile = true;
    frame.meta.noiseProfile = {
        0.0009f, 1.0e-6f,
        0.0010f, 1.2e-6f,
        0.0011f, 1.4e-6f,
    };
    frame.meta.hasGainField = false;
    frame.meta.hasResidualBlack = false;
    frame.meta.sourceId =
        "synthetic_authority_fusion_parity";

    const auto count =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height);
    frame.raw.resize(count);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint16_t value =
                static_cast<std::uint16_t>(
                    70 + ((x * 37 + y * 53 + x * y * 3) % 900));
            if ((x + 3 * y) % 97 == 0) {
                value = 1023u;
            }
            frame.raw[
                static_cast<std::size_t>(y) *
                    static_cast<std::size_t>(width) +
                static_cast<std::size_t>(x)] = value;
        }
    }
    return frame;
}

class CountingSource final
    : public truthraw::streaming_v0_1::IRawTileSource {
public:
    explicit CountingSource(
        const truthraw::DecodedDngFrame& frame)
        : frame_(frame) {}

    const truthraw::DngMetadata& metadata() const override {
        return frame_.meta;
    }

    std::size_t residentBytesUpperBound() const override {
        return frame_.raw.size() *
            sizeof(std::uint16_t);
    }

    truthraw::streaming_v0_1::StreamStatus readRawTile(
        const truthraw::TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const int width = rect.hx1 - rect.hx0;
        const int height = rect.hy1 - rect.hy0;
        if (width <= 0 || height <= 0 || !rawOut) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::InvalidArgument,
                "invalid synthetic tile");
        }
        const auto count =
            static_cast<std::size_t>(width) *
            static_cast<std::size_t>(height);
        if (rawCount != count ||
            gainOut != nullptr ||
            gainCount != 0u) {
            return truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::InvalidArgument,
                "synthetic tile count mismatch");
        }
        for (int yy = 0; yy < height; ++yy) {
            for (int xx = 0; xx < width; ++xx) {
                const auto src =
                    static_cast<std::size_t>(rect.hy0 + yy) *
                        static_cast<std::size_t>(frame_.meta.width) +
                    static_cast<std::size_t>(rect.hx0 + xx);
                const auto dst =
                    static_cast<std::size_t>(yy) *
                        static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(xx);
                rawOut[dst] = frame_.raw[src];
            }
        }
        ++rawTileCalls;
        rawSamplesRead += count;
        return truthraw::streaming_v0_1::StreamStatus::ok();
    }

    truthraw::streaming_v0_1::StreamStatus readRowBias(
        int,
        int,
        float*,
        std::size_t count) override {
        return count == 0u
            ? truthraw::streaming_v0_1::StreamStatus::ok()
            : truthraw::streaming_v0_1::StreamStatus::error(
                truthraw::streaming_v0_1::StreamStatusCode::SourceFailed,
                "unexpected row-bias request");
    }

    truthraw::streaming_v0_1::StreamStatus readColBias(
        int,
        int,
        float*,
        std::size_t count) override {
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

class AuthorityObserver final
    : public smsb::ICanonicalTileObserver {
public:
    explicit AuthorityObserver(
        const truthraw::DngMetadata& metadata)
        : cfa_(metadata.cfa),
          whiteLevel_(metadata.whiteLevel),
          accumulator_(
              static_cast<std::uint32_t>(metadata.width),
              static_cast<std::uint32_t>(metadata.height)) {}

    std::size_t residentBytesUpperBound() const noexcept override {
        return accumulator_.residentBytesUpperBound();
    }

    bool observeCanonicalTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        const std::uint16_t* rawCore,
        std::size_t rawCount,
        const float* cameraNativeRgb,
        std::size_t floatCount) noexcept override {
        if (!rawCore || !cameraNativeRgb) return false;
        ++tileCount;
        return accumulator_.appendSourceTile(
            cfa_,
            x,
            y,
            width,
            height,
            std::span<const std::uint16_t>(
                rawCore,
                rawCount),
            whiteLevel_,
            std::span<const float>(
                cameraNativeRgb,
                floatCount));
    }

    bool finalize(
        tn::AuthorityFieldSummary& out) noexcept {
        return accumulator_.finalize(out);
    }

    std::size_t tileCount = 0u;

private:
    truthraw::CfaPattern cfa_;
    float whiteLevel_;
    tn::AuthorityFieldAccumulator accumulator_;
};

bool same_double_bits(double a, double b) {
    return std::memcmp(&a, &b, sizeof(double)) == 0;
}

void require_same_summary(
    const tn::AuthorityFieldSummary& a,
    const tn::AuthorityFieldSummary& b) {
    REQUIRE(a.contentSha256 == b.contentSha256);
    REQUIRE(a.creationRoleCounts == b.creationRoleCounts);
    REQUIRE(a.authorityCounts == b.authorityCounts);
    REQUIRE(a.recordCount == b.recordCount);
    REQUIRE(a.p95KnownCount == b.p95KnownCount);
    REQUIRE(a.supportKnownCount == b.supportKnownCount);
    REQUIRE(a.boundKnownCount == b.boundKnownCount);
    REQUIRE(a.tileCount == b.tileCount);
    REQUIRE(a.createsNewEvidence == b.createsNewEvidence);
    REQUIRE(
        a.scientificWritebackAllowed ==
        b.scientificWritebackAllowed);
}

void test_fused_authority_summary_matches_replay_exactly() {
    auto frame = make_frame(130, 70);

    CountingSource fusedSource(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction
        fusedReconstruction;
    AuthorityObserver observer(frame.meta);

    smsb::Result fusedScientific{};
    smsb::Options options{};
    const auto fusedStatus =
        smsb::bind_scientific_master_streaming_observed(
            fusedSource,
            fusedReconstruction,
            options,
            observer,
            fusedScientific);
    if (!fusedStatus) {
        std::cerr
            << "fused bind failed: "
            << fusedStatus.message << "\n";
    }
    REQUIRE(fusedStatus);

    tn::AuthorityFieldSummary fusedAuthority{};
    REQUIRE(observer.finalize(fusedAuthority));

    const std::size_t tiles =
        ((static_cast<std::size_t>(frame.meta.width) + 63u) / 64u) *
        ((static_cast<std::size_t>(frame.meta.height) + 63u) / 64u);
    REQUIRE(observer.tileCount == tiles);
    REQUIRE(fusedSource.rawTileCalls == tiles * 2u);
    const auto readsAfterFusedScientific =
        fusedSource.rawTileCalls;

    master::StreamingScientificMasterTileSource replayMaster(
        fusedSource,
        fusedReconstruction);
    adapter::SourceFieldAdapter replayField(
        fusedSource,
        replayMaster);
    tn::AuthorityFieldSummary replayAuthority{};
    REQUIRE(tn::summarizeAuthorityField(
        replayField,
        replayAuthority));

    require_same_summary(
        fusedAuthority,
        replayAuthority);

    // The historical replay summary performs one Scientific-Master replay
    // source read plus one exact core RAW read per canonical tile.
    REQUIRE(
        fusedSource.rawTileCalls ==
        readsAfterFusedScientific + 2u * tiles);

    CountingSource directSource(frame);
    truthraw::ResearchEdgeAwareMeasuredPreservingReconstruction
        directReconstruction;
    smsb::Result directScientific{};
    const auto directStatus =
        smsb::bind_scientific_master_streaming(
            directSource,
            directReconstruction,
            options,
            directScientific);
    REQUIRE(directStatus);
    REQUIRE(
        directScientific.scientificMasterHash ==
        fusedScientific.scientificMasterHash);
    REQUIRE(
        same_double_bits(
            directScientific.zeroLineGauge.L0,
            fusedScientific.zeroLineGauge.L0));
    REQUIRE(
        directScientific.zeroLineGauge.gaugeId ==
        fusedScientific.zeroLineGauge.gaugeId);
    REQUIRE(
        directScientific.selfGaugeEligibleSamples ==
        fusedScientific.selfGaugeEligibleSamples);
    REQUIRE(
        directScientific.masterTilesProcessed ==
        fusedScientific.masterTilesProcessed);
    REQUIRE(
        directSource.rawTileCalls ==
        readsAfterFusedScientific);

    REQUIRE(!fusedAuthority.createsNewEvidence);
    REQUIRE(!fusedAuthority.scientificWritebackAllowed);
}

}  // namespace

int main() {
    test_fused_authority_summary_matches_replay_exactly();
    std::cout
        << "Authority-field fused Scientific Master parity PASS\n";
    return 0;
}

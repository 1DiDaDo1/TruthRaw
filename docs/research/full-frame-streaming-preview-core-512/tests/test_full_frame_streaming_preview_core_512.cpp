#include "streaming_test_support_v0_1.h"
#include "bounded_srgb_preview_sink_v0_1.h"

#include <array>
#include <cstring>
#include <memory>

namespace preview = truthraw::preview_surface_v0_1;

namespace {

class CountingSource final : public IRawTileSource {
public:
    explicit CountingSource(const DecodedDngFrame& frame) : inner_(frame) {}

    const DngMetadata& metadata() const override { return inner_.metadata(); }
    std::size_t residentBytesUpperBound() const override { return inner_.residentBytesUpperBound(); }

    StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* raw,
        std::size_t rawCount,
        float* gain,
        std::size_t gainCount) override {
        ++rawTileCalls;
        return inner_.readRawTile(rect, raw, rawCount, gain, gainCount);
    }

    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override {
        return inner_.readRowBias(y0, y1, out, count);
    }

    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override {
        return inner_.readColBias(x0, x1, out, count);
    }

    std::size_t rawTileCalls = 0u;

private:
    FrameSource inner_;
};

bool float_bits_equal(float a, float b) {
    std::uint32_t aa = 0u;
    std::uint32_t bb = 0u;
    static_assert(sizeof(aa) == sizeof(a));
    std::memcpy(&aa, &a, sizeof(aa));
    std::memcpy(&bb, &b, sizeof(bb));
    return aa == bb;
}

void require_exposure_bit_exact(const ExposurePlan& a, const ExposurePlan& b) {
    for (std::size_t i = 0; i < a.anchorsX.size(); ++i) {
        REQUIRE(float_bits_equal(a.anchorsX[i], b.anchorsX[i]));
        REQUIRE(float_bits_equal(a.anchorsY[i], b.anchorsY[i]));
    }
    REQUIRE(float_bits_equal(a.noiseSigmaAt2Pct, b.noiseSigmaAt2Pct));
    REQUIRE(float_bits_equal(a.clipFraction, b.clipFraction));
    REQUIRE(float_bits_equal(a.blackFactor, b.blackFactor));
    REQUIRE(float_bits_equal(a.midGain, b.midGain));
    REQUIRE(float_bits_equal(a.sdrHighlightGain, b.sdrHighlightGain));
    REQUIRE(float_bits_equal(a.evidenceConfidence, b.evidenceConfidence));
    REQUIRE(float_bits_equal(a.sceneToDisplayScalar, b.sceneToDisplayScalar));
    REQUIRE(float_bits_equal(a.hdrGateStartY, b.hdrGateStartY));
    REQUIRE(float_bits_equal(a.hdrGateFullY, b.hdrGateFullY));
    REQUIRE(float_bits_equal(a.hdrMaxGain, b.hdrMaxGain));
    REQUIRE(a.stage2Over1Count == b.stage2Over1Count);
}

void require_float_vectors_bit_exact(const std::vector<float>& a, const std::vector<float>& b) {
    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        REQUIRE(float_bits_equal(a[i], b[i]));
    }
}

StreamingOptions options_for_core(int core, bool diagnostics) {
    StreamingOptions o;
    o.tile = {core, 16};
    o.workers = 1;
    o.hdrEnabled = true;
    o.streamScientificDiagnostics = diagnostics;
    o.sdrLutSize = 4096;
    o.memoryBudgetBytes = 64u * 1024u * 1024u;
    return o;
}

void compare_collect_sink_case(int width, int height, bool diagnostics) {
    auto frame = make_frame(width, height);
    auto recon128 = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto recon512 = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance128 = std::make_shared<NeutralReferenceAppearance>();
    auto appearance512 = std::make_shared<NeutralReferenceAppearance>();

    CountingSource source128(frame);
    CountingSource source512(frame);
    CollectSink sink128(width, height, diagnostics);
    CollectSink sink512(width, height, diagnostics);
    StreamingTruthRawProcessor processor128(recon128, appearance128);
    StreamingTruthRawProcessor processor512(recon512, appearance512);
    StreamingResult result128{};
    StreamingResult result512{};

    const auto status128 = processor128.process(
        source128, sink128, options_for_core(128, diagnostics), result128);
    const auto status512 = processor512.process(
        source512, sink512, options_for_core(512, diagnostics), result512);
    REQUIRE(status128);
    REQUIRE(status512);
    REQUIRE(sink128.finished());
    REQUIRE(sink512.finished());

    require_exposure_bit_exact(result128.exposure, result512.exposure);
    require_float_vectors_bit_exact(sink128.sdr(), sink512.sdr());
    require_float_vectors_bit_exact(sink128.gain(), sink512.gain());
    if (diagnostics) {
        require_float_vectors_bit_exact(sink128.diagnostic(), sink512.diagnostic());
    }

    REQUIRE(result128.stage2Over1Count == result512.stage2Over1Count);
    REQUIRE(result128.clippedCount == result512.clippedCount);
    REQUIRE(result128.provenance.physicalFrameCount == 1u);
    REQUIRE(result512.provenance.physicalFrameCount == 1u);
    REQUIRE(result128.provenance.independentEvidenceCount == 1u);
    REQUIRE(result512.provenance.independentEvidenceCount == 1u);
    REQUIRE(!result128.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!result512.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(result128.provenance.gainMapAppliedExactlyOnce);
    REQUIRE(result512.provenance.gainMapAppliedExactlyOnce);

    REQUIRE(result128.tilesProcessedPass1 == result128.tilesProcessedPass2);
    REQUIRE(result512.tilesProcessedPass1 == result512.tilesProcessedPass2);
    REQUIRE(source512.rawTileCalls < source128.rawTileCalls);
    REQUIRE(source128.rawTileCalls == 2u * result128.tilesProcessedPass1);
    REQUIRE(source512.rawTileCalls == 2u * result512.tilesProcessedPass1);
}

void compare_bounded_preview_orientation(Orientation orientation) {
    auto frame = make_frame(1030, 770);
    frame.meta.orientation = orientation;

    CountingSource source128(frame);
    CountingSource source512(frame);
    auto recon128 = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto recon512 = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance128 = std::make_shared<NeutralReferenceAppearance>();
    auto appearance512 = std::make_shared<NeutralReferenceAppearance>();
    StreamingTruthRawProcessor processor128(recon128, appearance128);
    StreamingTruthRawProcessor processor512(recon512, appearance512);
    preview::BoundedSrgbPreviewSink sink128(384);
    preview::BoundedSrgbPreviewSink sink512(384);
    StreamingResult result128{};
    StreamingResult result512{};

    const auto status128 = processor128.process(
        source128, sink128, options_for_core(128, false), result128);
    const auto status512 = processor512.process(
        source512, sink512, options_for_core(512, false), result512);
    REQUIRE(status128);
    REQUIRE(status512);
    REQUIRE(sink128.finished());
    REQUIRE(sink512.finished());

    require_exposure_bit_exact(result128.exposure, result512.exposure);
    REQUIRE(sink128.width() == sink512.width());
    REQUIRE(sink128.height() == sink512.height());
    REQUIRE(sink128.writtenPixelCount() == sink512.writtenPixelCount());
    REQUIRE(sink128.argb8888() == sink512.argb8888());
    REQUIRE(source512.rawTileCalls < source128.rawTileCalls);
}

void verify_4080x3072_runtime_plan() {
    DngMetadata m{};
    m.width = 4080;
    m.height = 3072;
    m.cfa = CfaPattern::BGGR;
    m.orientation = Orientation::Normal;
    m.whiteLevel = 1023.0f;
    m.blackPhase = {64.0f, 64.0f, 64.0f, 64.0f};
    m.hasNoiseProfile = true;
    m.noiseProfile = {0.0009f, 1e-6f, 0.0010f, 1.2e-6f, 0.0011f, 1.4e-6f};
    m.hasGainField = true;
    m.hasResidualBlack = true;

    ResearchEdgeAwareMeasuredPreservingReconstruction recon;
    NeutralReferenceAppearance appearance;
    StreamingPlan p128{};
    StreamingPlan p512{};

    auto o128 = options_for_core(128, false);
    auto o512 = options_for_core(512, false);
    const std::size_t sourceResident = 8u * 1024u * 1024u;
    const std::size_t sinkResident = 2u * 1024u * 1024u;

    const auto s128 = plan_streaming_frame(
        m, o128, recon, appearance, sourceResident, sinkResident, p128);
    const auto s512 = plan_streaming_frame(
        m, o512, recon, appearance, sourceResident, sinkResident, p512);
    REQUIRE(s128);
    REQUIRE(s512);

    REQUIRE(p128.tileCount == 768u);
    REQUIRE(p512.tileCount == 48u);
    REQUIRE(p512.logicalResidentUpperBound < o512.memoryBudgetBytes);
    REQUIRE(p512.logicalResidentUpperBound > p128.logicalResidentUpperBound);
}

} // namespace

int main() {
    compare_collect_sink_case(130, 98, true);
    compare_collect_sink_case(1030, 770, false);
    compare_bounded_preview_orientation(Orientation::Normal);
    compare_bounded_preview_orientation(Orientation::Rotate180);
    compare_bounded_preview_orientation(Orientation::Rotate90CW);
    compare_bounded_preview_orientation(Orientation::Rotate90CCW);
    verify_4080x3072_runtime_plan();

    std::cout << "FULL_FRAME_STREAMING_PREVIEW_CORE_512_EQUIVALENCE_PASS\n";
    std::cout << "exposure_bit_exact=1\n";
    std::cout << "full_sdr_float_bit_exact=1\n";
    std::cout << "half_gain_float_bit_exact=1\n";
    std::cout << "bounded_preview_argb_exact=1\n";
    std::cout << "4080x3072_tiles_per_pass_128=768\n";
    std::cout << "4080x3072_tiles_per_pass_512=48\n";
    return 0;
}

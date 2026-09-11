#include "bounded_srgb_preview_sink_v0_1.h"
#include "../../full-frame-streaming-v0.1/tests/streaming_test_support_v0_1.h"

#include <cstdint>
#include <iostream>

using truthraw::Orientation;
using truthraw::preview_surface_v0_1::BoundedSrgbPreviewSink;
using truthraw::preview_surface_v0_1::linear_to_srgb_u8;
using truthraw::streaming_v0_1::StreamingOptions;
using truthraw::streaming_v0_1::StreamingResult;
using truthraw::streaming_v0_1::StreamingTruthRawProcessor;

int main() {
    REQUIRE(linear_to_srgb_u8(0.0f) == 0u);
    REQUIRE(linear_to_srgb_u8(1.0f) == 255u);
    REQUIRE(linear_to_srgb_u8(0.0031308f) >= 10u && linear_to_srgb_u8(0.0031308f) <= 11u);
    REQUIRE(linear_to_srgb_u8(0.18f) >= 117u && linear_to_srgb_u8(0.18f) <= 119u);

    auto frame = make_frame(130, 98);
    frame.meta.orientation = Orientation::Rotate90CW;

    auto recon = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<SkinSafeDetailedCrispAppearance>();
    StreamingTruthRawProcessor processor(recon, appearance);

    StreamingOptions options;
    options.tile = {32, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = false;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 0;

    FrameSource source64(frame);
    BoundedSrgbPreviewSink sink64(64);
    StreamingResult result64;
    auto status = processor.process(source64, sink64, options, result64);
    REQUIRE(status);
    REQUIRE(sink64.finished());
    REQUIRE(sink64.width() == 48);
    REQUIRE(sink64.height() == 64);
    REQUIRE(sink64.orientation() == Orientation::Rotate90CW);
    REQUIRE(sink64.writtenPixelCount() == sink64.argb8888().size());
    REQUIRE(sink64.halfGainSamplesObserved() > 0);
    REQUIRE(sink64.residentBytesUpperBound() <= 64u * 64u * 5u);
    REQUIRE(result64.provenance.physicalFrameCount == 1u);
    REQUIRE(result64.provenance.independentEvidenceCount == 1u);
    REQUIRE(!result64.provenance.scientificMasterModifiedByAppearance);
    REQUIRE(!result64.memory.adapterOwnsFullRawFrame);
    REQUIRE(!result64.memory.adapterOwnsFullSdrFrame);
    REQUIRE(!result64.memory.adapterOwnsFullHalfGainFrame);
    REQUIRE(!result64.memory.adapterOwnsFullDiagnosticFrame);

    bool sawNonGray = false;
    for (const std::uint32_t px : sink64.argb8888()) {
        REQUIRE((px >> 24u) == 0xffu);
        const std::uint32_t r = (px >> 16u) & 0xffu;
        const std::uint32_t g = (px >> 8u) & 0xffu;
        const std::uint32_t b = px & 0xffu;
        if (r != g || g != b) sawNonGray = true;
    }
    REQUIRE(sawNonGray);

    FrameSource source32(frame);
    BoundedSrgbPreviewSink sink32(32);
    StreamingResult result32;
    status = processor.process(source32, sink32, options, result32);
    REQUIRE(status);
    REQUIRE(sink32.finished());
    REQUIRE(sink32.width() == 24);
    REQUIRE(sink32.height() == 32);
    compare_exposure(result64.exposure, result32.exposure);
    REQUIRE(result64.stage2Over1Count == result32.stage2Over1Count);
    REQUIRE(result64.provenance.reconstructionBackend == result32.provenance.reconstructionBackend);
    REQUIRE(result64.provenance.appearanceBackend == result32.provenance.appearanceBackend);

    truthraw::ExposurePlan exposure{};
    BoundedSrgbPreviewSink lowMp(384);
    status = lowMp.beginFrame(4000, 3000, Orientation::Normal, exposure, true, false);
    REQUIRE(status);
    BoundedSrgbPreviewSink highMp(384);
    status = highMp.beginFrame(16000, 12000, Orientation::Normal, exposure, true, false);
    REQUIRE(status);
    REQUIRE(lowMp.width() == highMp.width());
    REQUIRE(lowMp.height() == highMp.height());
    REQUIRE(lowMp.residentBytesUpperBound() == highMp.residentBytesUpperBound());

    std::cout << "BOUNDED_SRGB_PREVIEW_SINK_V0_1_PASS\n";
    std::cout << "preview64=" << sink64.width() << "x" << sink64.height() << "\n";
    std::cout << "preview64_resident_bytes=" << sink64.residentBytesUpperBound() << "\n";
    std::cout << "preview32=" << sink32.width() << "x" << sink32.height() << "\n";
    std::cout << "megapixel_independent_surface_bytes=" << lowMp.residentBytesUpperBound() << "\n";
    std::cout << "frame_count=" << result64.provenance.physicalFrameCount << "\n";
    std::cout << "evidence_count=" << result64.provenance.independentEvidenceCount << "\n";
    return 0;
}

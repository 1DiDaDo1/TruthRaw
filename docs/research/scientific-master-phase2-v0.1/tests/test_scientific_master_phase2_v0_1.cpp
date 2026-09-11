#include "scientific_master_phase2_v0_1.h"
#include "full_frame_streaming_v0_1_internal.h"
#include "streaming_test_support_v0_1.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using truthraw::scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using truthraw::scientific_master_digest::v0_1::Sha256;
using truthraw::scientific_master_digest::v0_1::TileView;
using truthraw::scientific_master_digest::v0_1::to_hex;
using truthraw::scientific_master_phase2::v0_1::Phase2StreamingResult;
using truthraw::scientific_master_phase2::v0_1::StreamingScientificMasterProcessor;

namespace {

Sha256 full_frame_reference_digest(
    const DecodedDngFrame& frame,
    IReconstructionBackend& reconstruction) {
    FrameSource source(frame);
    TileRect full{};
    full.x0 = 0;
    full.y0 = 0;
    full.x1 = frame.meta.width;
    full.y1 = frame.meta.height;
    full.hx0 = 0;
    full.hy0 = 0;
    full.hx1 = frame.meta.width;
    full.hy1 = frame.meta.height;

    streaming_v0_1::detail::Workspace w;
    const auto fs = streaming_v0_1::detail::fill_stage2(source, full, w);
    REQUIRE(fs);

    const std::size_t n = std::size_t(frame.meta.width) * std::size_t(frame.meta.height);
    std::vector<float> cam(3u * n);
    const auto rs = reconstruction.reconstructTile(
        w.stage2.data(), frame.meta.width, frame.meta.height,
        0, 0, 0, 0, frame.meta.width, frame.meta.height,
        frame.meta.cfa, cam.data());
    REQUIRE(rs);

    ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(frame.meta.width),
        static_cast<std::uint32_t>(frame.meta.height));
    REQUIRE(digest.valid());
    for (int y = 0; y < frame.meta.height; y += 64) {
        for (int x = 0; x < frame.meta.width; x += 64) {
            const int cw = std::min(64, frame.meta.width - x);
            const int ch = std::min(64, frame.meta.height - y);
            TileView tile{};
            tile.x = static_cast<std::uint32_t>(x);
            tile.y = static_cast<std::uint32_t>(y);
            tile.width = static_cast<std::uint32_t>(cw);
            tile.height = static_cast<std::uint32_t>(ch);
            tile.rgb = cam.data() +
                (std::size_t(y) * std::size_t(frame.meta.width) + std::size_t(x)) * 3u;
            tile.rowStrideSamples = std::size_t(frame.meta.width) * 3u;
            REQUIRE(digest.add_tile(tile));
        }
    }
    Sha256 out{};
    REQUIRE(digest.finalize(out));
    return out;
}

Phase2StreamingResult run_stream(
    const DecodedDngFrame& frame,
    int core,
    const std::shared_ptr<IReconstructionBackend>& reconstruction,
    const std::shared_ptr<IAppearanceBackend>& appearance,
    bool hdr = true,
    bool diagnostics = true) {
    FrameSource source(frame);
    CollectSink sink(frame.meta.width, frame.meta.height, diagnostics);
    streaming_v0_1::StreamingOptions options;
    options.tile = {core, 7};
    options.workers = 1;
    options.hdrEnabled = hdr;
    options.streamScientificDiagnostics = diagnostics;
    options.sdrLutSize = 4096;
    options.memoryBudgetBytes = 0;

    Phase2StreamingResult result;
    StreamingScientificMasterProcessor processor(reconstruction, appearance);
    const auto st = processor.process(source, sink, options, result);
    REQUIRE(st);
    REQUIRE(sink.finished());
    REQUIRE(result.scientificMasterHashFinalized);
    REQUIRE(result.digestMetrics.cellsReceived == result.digestMetrics.cellCount);
    REQUIRE(result.streaming.provenance.physicalFrameCount == 1);
    REQUIRE(result.streaming.provenance.independentEvidenceCount == 1);
    REQUIRE(!result.streaming.provenance.scientificMasterModifiedByAppearance);
    return result;
}

void test_streaming_matches_full_frame_and_resource_partitions() {
    const auto frame = make_frame(258, 194);
    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto crisp = std::make_shared<SkinSafeDetailedCrispAppearance>();
    auto neutral = std::make_shared<NeutralReferenceAppearance>();

    const Sha256 reference = full_frame_reference_digest(frame, *reconstruction);
    const auto r64 = run_stream(frame, 64, reconstruction, crisp);
    const auto r128 = run_stream(frame, 128, reconstruction, crisp);
    const auto r128Neutral = run_stream(frame, 128, reconstruction, neutral);
    const auto r64NoHdr = run_stream(frame, 64, reconstruction, crisp, false, false);

    REQUIRE(r64.scientificMasterHash == reference);
    REQUIRE(r128.scientificMasterHash == reference);
    REQUIRE(r128Neutral.scientificMasterHash == reference);
    REQUIRE(r64NoHdr.scientificMasterHash == reference);
    REQUIRE(r64.scientificMasterHash == r128.scientificMasterHash);
    REQUIRE(r64.scientificMasterHash == r128Neutral.scientificMasterHash);
    REQUIRE(r64.scientificMasterHash == r64NoHdr.scientificMasterHash);

    std::cout << "reference_master_sha256=" << to_hex(reference) << "\n";
    std::cout << "phase2_tile64_sha256=" << to_hex(r64.scientificMasterHash) << "\n";
    std::cout << "phase2_tile128_sha256=" << to_hex(r128.scientificMasterHash) << "\n";
    std::cout << "phase2_neutral_sha256=" << to_hex(r128Neutral.scientificMasterHash) << "\n";
    std::cout << "phase2_nohdr_sha256=" << to_hex(r64NoHdr.scientificMasterHash) << "\n";
}

void test_noncanonical_runtime_core_fails_closed() {
    const auto frame = make_frame(130, 70);
    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    FrameSource source(frame);
    CollectSink sink(frame.meta.width, frame.meta.height, false);
    streaming_v0_1::StreamingOptions options;
    options.tile = {96, 7};
    options.workers = 1;
    options.hdrEnabled = false;
    options.streamScientificDiagnostics = false;

    Phase2StreamingResult result;
    StreamingScientificMasterProcessor processor(reconstruction, appearance);
    const auto st = processor.process(source, sink, options, result);
    REQUIRE(!st);
    REQUIRE(st.code == streaming_v0_1::StreamStatusCode::InvalidArgument);
    REQUIRE(!result.scientificMasterHashFinalized);
}

void test_digest_memory_is_accounted() {
    const auto frame = make_frame(128, 128);
    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<NeutralReferenceAppearance>();
    const auto result = run_stream(frame, 64, reconstruction, appearance, false, false);
    REQUIRE(result.digestResidentBytesUpperBound > 0);
    REQUIRE(result.streaming.memory.logicalResidentUpperBound >= result.digestResidentBytesUpperBound);
}

}  // namespace

int main() {
    test_streaming_matches_full_frame_and_resource_partitions();
    test_noncanonical_runtime_core_fails_closed();
    test_digest_memory_is_accounted();
    std::cout << "SCIENTIFIC_MASTER_PHASE2_V0_1_PASS\n";
    return 0;
}

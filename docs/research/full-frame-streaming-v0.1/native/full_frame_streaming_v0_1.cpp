#include "full_frame_streaming_v0_1.h"
#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>

namespace truthraw::streaming_v0_1 {
using namespace detail;

StreamingTruthRawProcessor::StreamingTruthRawProcessor(
    std::shared_ptr<IReconstructionBackend> r,
    std::shared_ptr<IAppearanceBackend> a)
    : reconstruction_(r ? std::move(r) : std::make_shared<ReferenceMeasuredPreservingReconstruction>()),
      appearance_(a ? std::move(a) : std::make_shared<NeutralReferenceAppearance>()) {}

StreamStatus StreamingTruthRawProcessor::process(
    IRawTileSource& source,
    IStreamingSink& sink,
    const StreamingOptions& o,
    StreamingResult& out) const {
    out = StreamingResult{};
    const auto& m = source.metadata();
    StreamingPlan plan;
    auto st = plan_streaming_frame(m, o, *reconstruction_, *appearance_,
                                   source.residentBytesUpperBound(),
                                   sink.residentBytesUpperBound(), plan);
    if (!st) { out.status = st; return st; }

    out.width = m.width;
    out.height = m.height;
    out.orientation = m.orientation;
    out.memory.sourceResidentUpperBound = source.residentBytesUpperBound();
    out.memory.sinkResidentUpperBound = sink.residentBytesUpperBound();
    out.memory.logicalWorkspacePeakBytes = plan.logicalWorkspaceUpperBound;
    out.memory.logicalResidentUpperBound = plan.logicalResidentUpperBound;
    out.provenance.reconstructionBackend = reconstruction_->name();
    out.provenance.appearanceBackend = appearance_->name();

    if (out.provenance.physicalFrameCount != 1 || out.provenance.independentEvidenceCount != 1)
        return out.status = StreamStatus::error(StreamStatusCode::BackendFailed, "single-frame evidence invariant broken");

    const auto tiles = make_tiles(m.width, m.height, o.tile);
    const int hw = plan.halfWidth, hh = plan.halfHeight;
    Workspace w;
    Pass1Stats pass1;
    st = run_pass1(source, tiles, *reconstruction_, w, pass1);
    if (!st) return out.status = st;

    const std::size_t N = std::size_t(m.width) * std::size_t(m.height);
    const float clipFraction = N ? float(pass1.totalClipped) / float(N) : 0.0f;
    out.exposure = choose_exposure_plan_from_histograms(
        pass1.display.bins, pass1.scene.bins, 4.0f, noise_sigma_2pct(m), clipFraction, pass1.totalOver1);
    const auto lut = build_monotone_lut(out.exposure, o.sdrLutSize);
    out.stage2Over1Count = pass1.totalOver1;
    out.clippedCount = pass1.totalClipped;
    out.tilesProcessedPass1 = pass1.tilesProcessed;

    st = sink.beginFrame(m.width, m.height, m.orientation, out.exposure, o.hdrEnabled, o.streamScientificDiagnostics);
    if (!st) return out.status = StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

    std::size_t pass2Peak = pass1.workspacePeak;
    std::size_t pass2Tiles = 0;
    st = run_pass2(source, sink, tiles, *reconstruction_, *appearance_, o, out.exposure, lut,
                   hw, hh, w, pass2Peak, pass2Tiles);
    if (!st) return out.status = st;
    out.tilesProcessedPass2 = pass2Tiles;

    st = sink.finishFrame();
    if (!st) return out.status = StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

    bool capOk = true;
    std::size_t observedLogical = pass2Peak;
    observedLogical = checked_add(observedLogical, 2*std::size_t(kHistBins)*sizeof(std::uint64_t), capOk);
    observedLogical = checked_add(observedLogical, lut.capacity()*sizeof(float), capOk);
    if (!capOk) return out.status = StreamStatus::error(StreamStatusCode::BudgetExceeded, "observed vector-capacity accounting overflow");
    std::size_t actualResident = observedLogical;
    actualResident = checked_add(actualResident, source.residentBytesUpperBound(), capOk);
    actualResident = checked_add(actualResident, sink.residentBytesUpperBound(), capOk);
    if (!capOk) return out.status = StreamStatus::error(StreamStatusCode::BudgetExceeded, "observed resident accounting overflow");
    if (o.memoryBudgetBytes != 0 && actualResident > o.memoryBudgetBytes)
        return out.status = StreamStatus::error(StreamStatusCode::BudgetExceeded, "observed vector-capacity resident bound exceeds budget");
    out.memory.logicalWorkspacePeakBytes = std::max(out.memory.logicalWorkspacePeakBytes, observedLogical);
    out.memory.logicalResidentUpperBound = std::max(out.memory.logicalResidentUpperBound, actualResident);
    out.status = StreamStatus::ok();
    return out.status;
}

} // namespace truthraw::streaming_v0_1

#include "scientific_master_phase2_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace truthraw::scientific_master_phase2::v0_1 {
namespace {

using namespace streaming_v0_1;
using namespace streaming_v0_1::detail;
using scientific_master_digest::v0_1::ScientificMasterDigestAccumulator;
using scientific_master_digest::v0_1::TileView;

StreamStatus run_pass2_with_master_digest(
    IRawTileSource& source,
    IStreamingSink& sink,
    const std::vector<TileRect>& tiles,
    IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    const StreamingOptions& o,
    const ExposurePlan& exposure,
    const std::vector<float>& lut,
    int hw,
    int hh,
    Workspace& w,
    ScientificMasterDigestAccumulator& digest,
    std::size_t& workspacePeak,
    std::size_t& tilesProcessed) {
    const auto& m = source.metadata();
    StreamStatus st;

    for (const auto& t : tiles) {
        st = fill_stage2(source, t, w);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);

        const int tw = t.hx1 - t.hx0;
        const int th = t.hy1 - t.hy0;
        const int cw = t.x1 - t.x0;
        const int ch = t.y1 - t.y0;
        const int ah = appearance.requiredHalo();
        const int ax0 = std::max(0, t.x0 - ah);
        const int ay0 = std::max(0, t.y0 - ah);
        const int ax1 = std::min(m.width, t.x1 + ah);
        const int ay1 = std::min(m.height, t.y1 + ah);
        const int aw = ax1 - ax0;
        const int ahh = ay1 - ay0;
        const std::size_t appN = std::size_t(aw) * std::size_t(ahh);
        const std::size_t coreN = std::size_t(cw) * std::size_t(ch);

        w.cam.resize(3u * appN);
        w.look.resize(3u * coreN);
        auto cs = reconstruction.reconstructTile(
            w.stage2.data(), tw, th, t.hx0, t.hy0,
            ax0, ay0, aw, ahh, m.cfa, w.cam.data());
        if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

        // Scientific Master capture point: exact camera-native reconstructed RGB,
        // before camera->XYZ and before any appearance/tone/output transform.
        const std::size_t coreOffset =
            (std::size_t(t.y0 - ay0) * std::size_t(aw) + std::size_t(t.x0 - ax0)) * 3u;
        TileView masterTile{};
        masterTile.x = static_cast<std::uint32_t>(t.x0);
        masterTile.y = static_cast<std::uint32_t>(t.y0);
        masterTile.width = static_cast<std::uint32_t>(cw);
        masterTile.height = static_cast<std::uint32_t>(ch);
        masterTile.rgb = w.cam.data() + coreOffset;
        masterTile.rowStrideSamples = std::size_t(aw) * 3u;
        if (!digest.add_tile(masterTile)) {
            return StreamStatus::error(
                StreamStatusCode::BackendFailed,
                "Scientific Master digest rejected reconstructed core: " + digest.error());
        }

        camera_to_xyz(w.cam.data(), w.cam.data(), int(appN), m.cameraToXyzD50);

        const HalfStateRect qr = half_core_rect(t);
        const int qw = qr.x1 - qr.x0;
        const int qh = qr.y1 - qr.y0;
        const std::size_t qn = std::size_t(qw) * std::size_t(qh);
        w.halfScene.assign(qn, 0.0f);
        for (int y = t.y0; y < t.y1; ++y) {
            for (int x = t.x0; x < t.x1; ++x) {
                const std::size_t ai = std::size_t(y - ay0) * std::size_t(aw) + std::size_t(x - ax0);
                const int qx = x / 2 - qr.x0;
                const int qy = y / 2 - qr.y0;
                const std::size_t qi = std::size_t(qy) * std::size_t(qw) + std::size_t(qx);
                w.halfScene[qi] = std::max(w.halfScene[qi], std::max(w.cam[3u * ai + 1u], 0.0f));
            }
        }

        xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(appN));
        cs = appearance.applyTile(
            w.cam.data(), aw, ahh,
            t.x0 - ax0, t.y0 - ay0, cw, ch, w.look.data());
        if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

        for (std::size_t i = 0; i < coreN; ++i) {
            float rr = w.look[3u * i];
            float gg = w.look[3u * i + 1u];
            float bb = w.look[3u * i + 2u];
            const float y = std::max(luminance709(rr, gg, bb), 0.0f);
            const float yo = lut_sample(lut, y);
            const float sc = y > 1e-8f ? yo / y : 0.0f;
            rr = std::max(rr * sc, 0.0f);
            gg = std::max(gg * sc, 0.0f);
            bb = std::max(bb * sc, 0.0f);
            const float mx = std::max(rr, std::max(gg, bb));
            if (mx > 1.0f) {
                rr /= mx;
                gg /= mx;
                bb /= mx;
            }
            w.look[3u * i] = rr;
            w.look[3u * i + 1u] = gg;
            w.look[3u * i + 2u] = bb;
        }

        st = sink.writeSdrTile(t, w.look.data(), w.look.size());
        if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

        if (o.streamScientificDiagnostics) {
            w.diagnosticCore.resize(coreN);
            for (int cy = 0; cy < ch; ++cy) {
                for (int cx = 0; cx < cw; ++cx) {
                    const int lx = t.x0 + cx - t.hx0;
                    const int ly = t.y0 + cy - t.hy0;
                    w.diagnosticCore[std::size_t(cy) * std::size_t(cw) + std::size_t(cx)] =
                        w.stage2[std::size_t(ly) * std::size_t(tw) + std::size_t(lx)];
                }
            }
            st = sink.writeStage2DiagnosticTile(t, w.diagnosticCore.data(), coreN);
            if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        }

        w.halfGain.resize(qn);
        for (int qy = qr.y0; qy < qr.y1; ++qy) {
            for (int qx = qr.x0; qx < qr.x1; ++qx) {
                const std::size_t qo =
                    std::size_t(qy - qr.y0) * std::size_t(qw) + std::size_t(qx - qr.x0);
                if (!o.hdrEnabled) {
                    w.halfGain[qo] = 0.0f;
                    continue;
                }
                bool censored = false;
                for (int nqy = std::max(0, qy - 1);
                     nqy <= std::min(hh - 1, qy + 1) && !censored; ++nqy) {
                    for (int nqx = std::max(0, qx - 1);
                         nqx <= std::min(hw - 1, qx + 1) && !censored; ++nqx) {
                        for (int py = 2 * nqy;
                             py < std::min(m.height, 2 * nqy + 2) && !censored; ++py) {
                            for (int px = 2 * nqx; px < std::min(m.width, 2 * nqx + 2); ++px) {
                                if (px < t.hx0 || px >= t.hx1 || py < t.hy0 || py >= t.hy1) {
                                    return StreamStatus::error(
                                        StreamStatusCode::BackendFailed,
                                        "RAW halo insufficient for half-censor dilation");
                                }
                                const std::size_t ri =
                                    std::size_t(py - t.hy0) * std::size_t(tw) + std::size_t(px - t.hx0);
                                if (float(w.raw[ri]) >= m.whiteLevel) {
                                    censored = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (censored) {
                    w.halfGain[qo] = 0.0f;
                    continue;
                }

                float sdrSum = 0.0f;
                int count = 0;
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        const int x = 2 * qx + dx;
                        const int y = 2 * qy + dy;
                        if (x < t.x0 || x >= t.x1 || y < t.y0 || y >= t.y1) continue;
                        const std::size_t li =
                            std::size_t(y - t.y0) * std::size_t(cw) + std::size_t(x - t.x0);
                        sdrSum += std::max(
                            luminance709(w.look[3u * li], w.look[3u * li + 1u], w.look[3u * li + 2u]),
                            0.0f);
                        ++count;
                    }
                }
                if (count == 0) {
                    return StreamStatus::error(
                        StreamStatusCode::BackendFailed,
                        "2x2 half-gain cell lost tile ownership");
                }
                const float yBase = sdrSum / float(count);
                const float target = w.halfScene[qo] * exposure.sceneToDisplayScalar;
                const float rawGain = yBase > 1e-7f ? target / yBase : 1.0f;
                const float tt = clamp01(
                    (yBase - exposure.hdrGateStartY) /
                    std::max(exposure.hdrGateFullY - exposure.hdrGateStartY, 1e-8f));
                const float gate = tt * tt * (3.0f - 2.0f * tt);
                const float gain = 1.0f + exposure.evidenceConfidence * gate *
                    (std::max(1.0f, std::min(exposure.hdrMaxGain, rawGain)) - 1.0f);
                w.halfGain[qo] = std::log2(std::max(gain, 1.0f));
            }
        }

        st = sink.writeHalfLogGainBlock(qr, w.halfGain.data(), w.halfGain.size());
        if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        workspacePeak = std::max(workspacePeak, vector_bytes(w));
        ++tilesProcessed;
    }
    return StreamStatus::ok();
}

}  // namespace

StreamingScientificMasterProcessor::StreamingScientificMasterProcessor(
    std::shared_ptr<IReconstructionBackend> reconstruction,
    std::shared_ptr<IAppearanceBackend> appearance)
    : reconstruction_(reconstruction ? std::move(reconstruction)
                                     : std::make_shared<ReferenceMeasuredPreservingReconstruction>()),
      appearance_(appearance ? std::move(appearance)
                             : std::make_shared<NeutralReferenceAppearance>()) {}

streaming_v0_1::StreamStatus StreamingScientificMasterProcessor::process(
    streaming_v0_1::IRawTileSource& source,
    streaming_v0_1::IStreamingSink& sink,
    const streaming_v0_1::StreamingOptions& o,
    Phase2StreamingResult& out) const {
    using namespace streaming_v0_1;
    using namespace streaming_v0_1::detail;

    out = Phase2StreamingResult{};
    const auto& m = source.metadata();
    if (o.tile.core <= 0 ||
        (o.tile.core % int(scientific_master_digest::v0_1::kCanonicalCellEdge)) != 0) {
        const auto st = StreamStatus::error(
            StreamStatusCode::InvalidArgument,
            "Scientific Master phase2 v0.1 requires runtime core tile size to be a multiple of 64");
        out.streaming.status = st;
        return st;
    }

    StreamingPlan plan;
    auto st = plan_streaming_frame(
        m, o, *reconstruction_, *appearance_,
        source.residentBytesUpperBound(), sink.residentBytesUpperBound(), plan);
    if (!st) {
        out.streaming.status = st;
        return st;
    }

    if (m.width <= 0 || m.height <= 0) {
        st = StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid frame dimensions");
        out.streaming.status = st;
        return st;
    }

    ScientificMasterDigestAccumulator digest(
        static_cast<std::uint32_t>(m.width), static_cast<std::uint32_t>(m.height));
    if (!digest.valid()) {
        st = StreamStatus::error(StreamStatusCode::BudgetExceeded, digest.error());
        out.streaming.status = st;
        return st;
    }
    out.digestMetrics = digest.metrics();
    out.digestResidentBytesUpperBound = out.digestMetrics.residentBytesUpperBound;

    bool capOk = true;
    const std::size_t plannedResident = checked_add(
        plan.logicalResidentUpperBound, out.digestResidentBytesUpperBound, capOk);
    if (!capOk) {
        st = StreamStatus::error(StreamStatusCode::BudgetExceeded, "phase2 planned resident accounting overflow");
        out.streaming.status = st;
        return st;
    }
    if (o.memoryBudgetBytes != 0 && plannedResident > o.memoryBudgetBytes) {
        st = StreamStatus::error(
            StreamStatusCode::BudgetExceeded,
            "Scientific Master digest state would exceed streaming memory budget");
        out.streaming.status = st;
        return st;
    }

    auto& sr = out.streaming;
    sr.width = m.width;
    sr.height = m.height;
    sr.orientation = m.orientation;
    sr.memory.sourceResidentUpperBound = source.residentBytesUpperBound();
    sr.memory.sinkResidentUpperBound = sink.residentBytesUpperBound();
    sr.memory.logicalWorkspacePeakBytes = plan.logicalWorkspaceUpperBound;
    sr.memory.logicalResidentUpperBound = plannedResident;
    sr.provenance.reconstructionBackend = reconstruction_->name();
    sr.provenance.appearanceBackend = appearance_->name();

    if (sr.provenance.physicalFrameCount != 1 || sr.provenance.independentEvidenceCount != 1) {
        st = StreamStatus::error(StreamStatusCode::BackendFailed, "single-frame evidence invariant broken");
        sr.status = st;
        return st;
    }

    const auto tiles = make_tiles(m.width, m.height, o.tile);
    const int hw = plan.halfWidth;
    const int hh = plan.halfHeight;
    Workspace workspace;
    Pass1Stats pass1;
    st = run_pass1(source, tiles, *reconstruction_, workspace, pass1);
    if (!st) {
        sr.status = st;
        return st;
    }

    const std::size_t n = std::size_t(m.width) * std::size_t(m.height);
    const float clipFraction = n ? float(pass1.totalClipped) / float(n) : 0.0f;
    sr.exposure = choose_exposure_plan_from_histograms(
        pass1.display.bins, pass1.scene.bins, 4.0f,
        noise_sigma_2pct(m), clipFraction, pass1.totalOver1);
    const auto lut = build_monotone_lut(sr.exposure, o.sdrLutSize);
    sr.stage2Over1Count = pass1.totalOver1;
    sr.clippedCount = pass1.totalClipped;
    sr.tilesProcessedPass1 = pass1.tilesProcessed;

    st = sink.beginFrame(
        m.width, m.height, m.orientation, sr.exposure,
        o.hdrEnabled, o.streamScientificDiagnostics);
    if (!st) {
        st = StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        sr.status = st;
        return st;
    }

    std::size_t pass2Peak = pass1.workspacePeak;
    std::size_t pass2Tiles = 0;
    st = run_pass2_with_master_digest(
        source, sink, tiles, *reconstruction_, *appearance_, o, sr.exposure, lut,
        hw, hh, workspace, digest, pass2Peak, pass2Tiles);
    if (!st) {
        sr.status = st;
        return st;
    }
    sr.tilesProcessedPass2 = pass2Tiles;

    if (!digest.finalize(out.scientificMasterHash)) {
        st = StreamStatus::error(
            StreamStatusCode::BackendFailed,
            "Scientific Master digest finalization failed: " + digest.error());
        sr.status = st;
        return st;
    }
    out.scientificMasterHashFinalized = true;
    out.digestMetrics = digest.metrics();

    st = sink.finishFrame();
    if (!st) {
        st = StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        sr.status = st;
        return st;
    }

    capOk = true;
    std::size_t observedLogical = pass2Peak;
    observedLogical = checked_add(
        observedLogical, 2u * std::size_t(kHistBins) * sizeof(std::uint64_t), capOk);
    observedLogical = checked_add(observedLogical, lut.capacity() * sizeof(float), capOk);
    if (!capOk) {
        st = StreamStatus::error(
            StreamStatusCode::BudgetExceeded,
            "observed vector-capacity accounting overflow");
        sr.status = st;
        return st;
    }
    std::size_t actualResident = observedLogical;
    actualResident = checked_add(actualResident, source.residentBytesUpperBound(), capOk);
    actualResident = checked_add(actualResident, sink.residentBytesUpperBound(), capOk);
    actualResident = checked_add(actualResident, out.digestResidentBytesUpperBound, capOk);
    if (!capOk) {
        st = StreamStatus::error(
            StreamStatusCode::BudgetExceeded,
            "observed phase2 resident accounting overflow");
        sr.status = st;
        return st;
    }
    if (o.memoryBudgetBytes != 0 && actualResident > o.memoryBudgetBytes) {
        st = StreamStatus::error(
            StreamStatusCode::BudgetExceeded,
            "observed phase2 resident bound exceeds budget");
        sr.status = st;
        return st;
    }

    sr.memory.logicalWorkspacePeakBytes = std::max(sr.memory.logicalWorkspacePeakBytes, observedLogical);
    sr.memory.logicalResidentUpperBound = std::max(sr.memory.logicalResidentUpperBound, actualResident);
    sr.status = StreamStatus::ok();
    return sr.status;
}

}  // namespace truthraw::scientific_master_phase2::v0_1

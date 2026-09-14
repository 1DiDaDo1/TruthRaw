#include "full_frame_streaming_v0_1_internal.h"
#include "streaming_test_support_v0_1.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

using namespace truthraw::streaming_v0_1::detail;

namespace {

struct CandidateTelemetry {
    std::size_t queueDepth = 0;
    std::size_t maxReadyPackets = 0;
    std::size_t sourceRawTileReads = 0;
    int sourceConcurrencyPeak = 0;
    bool orderedCommit = true;
};

class CountingSource final : public IRawTileSource {
public:
    explicit CountingSource(IRawTileSource& inner) : inner_(inner) {}

    const DngMetadata& metadata() const override { return inner_.metadata(); }
    std::size_t residentBytesUpperBound() const override { return inner_.residentBytesUpperBound(); }

    StreamStatus readRawTile(const TileRect& rect,
                             std::uint16_t* rawOut,
                             std::size_t rawCount,
                             float* gainOut,
                             std::size_t gainCount) override {
        enter();
        ++rawReads_;
        auto st = inner_.readRawTile(rect, rawOut, rawCount, gainOut, gainCount);
        leave();
        return st;
    }

    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override {
        enter();
        auto st = inner_.readRowBias(y0, y1, out, count);
        leave();
        return st;
    }

    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override {
        enter();
        auto st = inner_.readColBias(x0, x1, out, count);
        leave();
        return st;
    }

    std::size_t rawReads() const { return rawReads_.load(); }
    int concurrencyPeak() const { return peak_.load(); }

private:
    void enter() {
        const int now = active_.fetch_add(1) + 1;
        int old = peak_.load();
        while (now > old && !peak_.compare_exchange_weak(old, now)) {}
    }
    void leave() { active_.fetch_sub(1); }

    IRawTileSource& inner_;
    std::atomic<int> active_{0};
    std::atomic<int> peak_{0};
    std::atomic<std::size_t> rawReads_{0};
};

struct Pass1Worker {
    Workspace workspace;
    Pass1Stats stats;
    StreamStatus status = StreamStatus::ok();
};

StreamStatus process_pass1_tile(IRawTileSource& source,
                                std::mutex& sourceMutex,
                                const TileRect& t,
                                IReconstructionBackend& reconstruction,
                                Pass1Worker& worker) {
    const auto& m = source.metadata();
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        auto st = fill_stage2(source, t, worker.workspace);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
    }

    auto& w = worker.workspace;
    auto& stats = worker.stats;
    const int tw = t.hx1 - t.hx0;
    const int th = t.hy1 - t.hy0;
    const int cw = t.x1 - t.x0;
    const int ch = t.y1 - t.y0;
    const std::size_t coreN = std::size_t(cw) * std::size_t(ch);
    w.cam.resize(3 * coreN);

    auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, t.hx0, t.hy0,
                                             t.x0, t.y0, cw, ch, m.cfa, w.cam.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    camera_to_xyz(w.cam.data(), w.cam.data(), int(coreN), m.cameraToXyzD50);
    for (int cy = 0; cy < ch; ++cy) {
        for (int cx = 0; cx < cw; ++cx) {
            const int x = t.x0 + cx;
            const int y = t.y0 + cy;
            const std::size_t ci = std::size_t(cy) * std::size_t(cw) + std::size_t(cx);
            stats.scene.add(std::max(w.cam[3 * ci + 1], 0.0f));
            const int lx = x - t.hx0;
            const int ly = y - t.hy0;
            const std::size_t ri = std::size_t(ly) * std::size_t(tw) + std::size_t(lx);
            const float s2 = w.stage2[ri];
            if (s2 > 1.0f) stats.totalOver1++;
            if (float(w.raw[ri]) >= m.whiteLevel) stats.totalClipped++;
        }
    }

    xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(coreN));
    for (std::size_t ci = 0; ci < coreN; ++ci) {
        const float nY = std::max(luminance709(w.cam[3 * ci], w.cam[3 * ci + 1], w.cam[3 * ci + 2]), 0.0f);
        stats.display.add(nY);
    }

    stats.workspacePeak = std::max(stats.workspacePeak, vector_bytes(w));
    stats.tilesProcessed++;
    return StreamStatus::ok();
}

Pass1Stats reduce_pass1(const std::vector<std::unique_ptr<Pass1Worker>>& workers) {
    Pass1Stats total;
    for (std::size_t wi = 0; wi < workers.size(); ++wi) {
        const auto& s = workers[wi]->stats;
        for (int i = 0; i < kHistBins; ++i) {
            total.display.bins[std::size_t(i)] += s.display.bins[std::size_t(i)];
            total.scene.bins[std::size_t(i)] += s.scene.bins[std::size_t(i)];
        }
        total.display.total += s.display.total;
        total.scene.total += s.scene.total;
        total.totalOver1 += s.totalOver1;
        total.totalClipped += s.totalClipped;
        total.workspacePeak = std::max(total.workspacePeak, s.workspacePeak);
        total.tilesProcessed += s.tilesProcessed;
    }
    return total;
}

struct TilePacket {
    std::size_t tileIndex = 0;
    TileRect core{};
    HalfStateRect half{};
    std::vector<float> sdr;
    std::vector<float> gain;
    std::vector<float> diagnostic;
    StreamStatus status = StreamStatus::ok();
};

StreamStatus compute_pass2_packet(IRawTileSource& source,
                                  std::mutex& sourceMutex,
                                  const TileRect& t,
                                  std::size_t tileIndex,
                                  IReconstructionBackend& reconstruction,
                                  const IAppearanceBackend& appearance,
                                  const StreamingOptions& o,
                                  const ExposurePlan& exposure,
                                  const std::vector<float>& lut,
                                  int hw,
                                  int hh,
                                  Workspace& w,
                                  TilePacket& packet) {
    const auto& m = source.metadata();
    packet.tileIndex = tileIndex;
    packet.core = t;

    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        auto st = fill_stage2(source, t, w);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
    }

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

    w.cam.resize(3 * appN);
    w.look.resize(3 * coreN);
    auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, t.hx0, t.hy0,
                                             ax0, ay0, aw, ahh, m.cfa, w.cam.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    camera_to_xyz(w.cam.data(), w.cam.data(), int(appN), m.cameraToXyzD50);
    const HalfStateRect qr = half_core_rect(t);
    packet.half = qr;
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
            w.halfScene[qi] = std::max(w.halfScene[qi], std::max(w.cam[3 * ai + 1], 0.0f));
        }
    }

    xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(appN));
    cs = appearance.applyTile(w.cam.data(), aw, ahh, t.x0 - ax0, t.y0 - ay0,
                              cw, ch, w.look.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    for (std::size_t i = 0; i < coreN; ++i) {
        float rr = w.look[3 * i];
        float gg = w.look[3 * i + 1];
        float bb = w.look[3 * i + 2];
        const float Y = std::max(luminance709(rr, gg, bb), 0.0f);
        const float Yo = lut_sample(lut, Y);
        const float sc = Y > 1e-8f ? Yo / Y : 0.0f;
        rr = std::max(rr * sc, 0.0f);
        gg = std::max(gg * sc, 0.0f);
        bb = std::max(bb * sc, 0.0f);
        const float mx = std::max(rr, std::max(gg, bb));
        if (mx > 1.0f) { rr /= mx; gg /= mx; bb /= mx; }
        w.look[3 * i] = rr;
        w.look[3 * i + 1] = gg;
        w.look[3 * i + 2] = bb;
    }
    packet.sdr = w.look;

    if (o.streamScientificDiagnostics) {
        packet.diagnostic.resize(coreN);
        for (int cy = 0; cy < ch; ++cy) {
            for (int cx = 0; cx < cw; ++cx) {
                const int lx = t.x0 + cx - t.hx0;
                const int ly = t.y0 + cy - t.hy0;
                packet.diagnostic[std::size_t(cy) * std::size_t(cw) + std::size_t(cx)] =
                    w.stage2[std::size_t(ly) * std::size_t(tw) + std::size_t(lx)];
            }
        }
    }

    w.halfGain.resize(qn);
    for (int qy = qr.y0; qy < qr.y1; ++qy) {
        for (int qx = qr.x0; qx < qr.x1; ++qx) {
            const std::size_t qo = std::size_t(qy - qr.y0) * std::size_t(qw) + std::size_t(qx - qr.x0);
            if (!o.hdrEnabled) { w.halfGain[qo] = 0.0f; continue; }

            bool censored = false;
            for (int nqy = std::max(0, qy - 1); nqy <= std::min(hh - 1, qy + 1) && !censored; ++nqy) {
                for (int nqx = std::max(0, qx - 1); nqx <= std::min(hw - 1, qx + 1) && !censored; ++nqx) {
                    for (int py = 2 * nqy; py < std::min(m.height, 2 * nqy + 2) && !censored; ++py) {
                        for (int px = 2 * nqx; px < std::min(m.width, 2 * nqx + 2); ++px) {
                            if (px < t.hx0 || px >= t.hx1 || py < t.hy0 || py >= t.hy1)
                                return StreamStatus::error(StreamStatusCode::BackendFailed,
                                                           "RAW halo insufficient for half-censor dilation");
                            const std::size_t ri = std::size_t(py - t.hy0) * std::size_t(tw) + std::size_t(px - t.hx0);
                            if (float(w.raw[ri]) >= m.whiteLevel) { censored = true; break; }
                        }
                    }
                }
            }
            if (censored) { w.halfGain[qo] = 0.0f; continue; }

            float sdrSum = 0.0f;
            int cnt = 0;
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    const int x = 2 * qx + dx;
                    const int y = 2 * qy + dy;
                    if (x < t.x0 || x >= t.x1 || y < t.y0 || y >= t.y1) continue;
                    const std::size_t li = std::size_t(y - t.y0) * std::size_t(cw) + std::size_t(x - t.x0);
                    sdrSum += std::max(luminance709(w.look[3 * li], w.look[3 * li + 1], w.look[3 * li + 2]), 0.0f);
                    cnt++;
                }
            }
            if (cnt == 0)
                return StreamStatus::error(StreamStatusCode::BackendFailed, "2x2 half-gain cell lost tile ownership");

            const float Yb = sdrSum / float(cnt);
            const float target = w.halfScene[qo] * exposure.sceneToDisplayScalar;
            const float rawGain = Yb > 1e-7f ? target / Yb : 1.0f;
            const float tt = clamp01((Yb - exposure.hdrGateStartY) /
                                     std::max(exposure.hdrGateFullY - exposure.hdrGateStartY, 1e-8f));
            const float gate = tt * tt * (3.0f - 2.0f * tt);
            const float g = 1.0f + exposure.evidenceConfidence * gate *
                (std::max(1.0f, std::min(exposure.hdrMaxGain, rawGain)) - 1.0f);
            w.halfGain[qo] = std::log2(std::max(g, 1.0f));
        }
    }
    packet.gain = w.halfGain;
    return StreamStatus::ok();
}

StreamStatus run_candidate(IRawTileSource& source,
                           IStreamingSink& sink,
                           IReconstructionBackend& reconstruction,
                           const IAppearanceBackend& appearance,
                           StreamingOptions o,
                           StreamingResult& out,
                           CandidateTelemetry& telemetry) {
    out = StreamingResult{};
    const auto& m = source.metadata();
    if (m.width <= 1 || m.height <= 1) return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid dimensions");
    if (o.workers < 1) return StreamStatus::error(StreamStatusCode::InvalidArgument, "workers < 1");
    if ((o.tile.core & 1) != 0) return StreamStatus::error(StreamStatusCode::InvalidArgument, "even tile core required");
    const int requiredHalo = std::max(o.hdrEnabled ? 2 : 0,
                                     reconstruction.requiredHalo() + appearance.requiredHalo());
    if (o.tile.halo < requiredHalo) return StreamStatus::error(StreamStatusCode::InvalidArgument, "insufficient halo");
    if (o.sdrLutSize < 2) return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid LUT size");

    const auto tiles = make_tiles(m.width, m.height, o.tile);
    if (tiles.empty()) return StreamStatus::error(StreamStatusCode::InvalidArgument, "no tiles");
    const int workerCount = std::max(1, std::min(o.workers, int(tiles.size())));
    telemetry.queueDepth = std::min<std::size_t>(tiles.size(), std::size_t(2 * workerCount));

    StreamingOptions one = o;
    one.workers = 1;
    bool memoryOk = true;
    const std::size_t perWorker = estimate_workspace(m, one, appearance, memoryOk);
    if (!memoryOk) return StreamStatus::error(StreamStatusCode::InvalidArgument, "workspace overflow");
    const int coreW = std::min(m.width, o.tile.core);
    const int coreH = std::min(m.height, o.tile.core);
    const std::size_t coreN = std::size_t(coreW) * std::size_t(coreH);
    const std::size_t qN = std::size_t((coreW + 1) / 2) * std::size_t((coreH + 1) / 2);
    std::size_t packetBytes = 3 * coreN * sizeof(float) + qN * sizeof(float);
    if (o.streamScientificDiagnostics) packetBytes += coreN * sizeof(float);
    const std::size_t residentUpper = source.residentBytesUpperBound() + sink.residentBytesUpperBound()
        + std::size_t(workerCount) * perWorker + telemetry.queueDepth * packetBytes;
    if (o.memoryBudgetBytes != 0 && residentUpper > o.memoryBudgetBytes)
        return StreamStatus::error(StreamStatusCode::BudgetExceeded, "v0.2 conservative resident bound exceeds budget");

    out.width = m.width;
    out.height = m.height;
    out.orientation = m.orientation;
    out.memory.sourceResidentUpperBound = source.residentBytesUpperBound();
    out.memory.sinkResidentUpperBound = sink.residentBytesUpperBound();
    out.memory.logicalWorkspacePeakBytes = std::size_t(workerCount) * perWorker + telemetry.queueDepth * packetBytes;
    out.memory.logicalResidentUpperBound = residentUpper;
    out.provenance.reconstructionBackend = reconstruction.name();
    out.provenance.appearanceBackend = appearance.name();
    if (out.provenance.physicalFrameCount != 1 || out.provenance.independentEvidenceCount != 1)
        return StreamStatus::error(StreamStatusCode::BackendFailed, "single-frame evidence invariant broken");

    std::mutex sourceMutex;
    std::atomic<std::size_t> nextPass1{0};
    std::vector<std::unique_ptr<Pass1Worker>> p1Workers;
    p1Workers.reserve(std::size_t(workerCount));
    for (int i = 0; i < workerCount; ++i) p1Workers.emplace_back(std::make_unique<Pass1Worker>());

    std::vector<std::thread> threads;
    threads.reserve(std::size_t(workerCount));
    for (int wi = 0; wi < workerCount; ++wi) {
        threads.emplace_back([&, wi] {
            auto& worker = *p1Workers[std::size_t(wi)];
            while (true) {
                const std::size_t ti = nextPass1.fetch_add(1, std::memory_order_relaxed);
                if (ti >= tiles.size()) break;
                worker.status = process_pass1_tile(source, sourceMutex, tiles[ti], reconstruction, worker);
                if (!worker.status) break;
            }
        });
    }
    for (auto& th : threads) th.join();
    threads.clear();
    for (const auto& worker : p1Workers) if (!worker->status) return worker->status;

    const Pass1Stats pass1 = reduce_pass1(p1Workers);
    const std::size_t N = std::size_t(m.width) * std::size_t(m.height);
    const float clipFraction = N ? float(pass1.totalClipped) / float(N) : 0.0f;
    out.exposure = choose_exposure_plan_from_histograms(
        pass1.display.bins, pass1.scene.bins, 4.0f, noise_sigma_2pct(m),
        clipFraction, pass1.totalOver1);
    const auto lut = build_monotone_lut(out.exposure, o.sdrLutSize);
    out.stage2Over1Count = pass1.totalOver1;
    out.clippedCount = pass1.totalClipped;
    out.tilesProcessedPass1 = pass1.tilesProcessed;

    auto st = sink.beginFrame(m.width, m.height, m.orientation, out.exposure,
                              o.hdrEnabled, o.streamScientificDiagnostics);
    if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

    const int hw = (m.width + 1) / 2;
    const int hh = (m.height + 1) / 2;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::size_t nextClaim = 0;
    std::size_t nextCommit = 0;
    std::size_t readyCount = 0;
    bool cancel = false;
    std::vector<std::unique_ptr<TilePacket>> ready(tiles.size());
    std::vector<Workspace> p2Workspace(std::size_t(workerCount));

    for (int wi = 0; wi < workerCount; ++wi) {
        threads.emplace_back([&, wi] {
            while (true) {
                std::size_t ti = 0;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    queueCv.wait(lock, [&] {
                        return cancel || nextClaim >= tiles.size() ||
                               nextClaim < nextCommit + telemetry.queueDepth;
                    });
                    if (cancel || nextClaim >= tiles.size()) break;
                    ti = nextClaim++;
                }

                auto packet = std::make_unique<TilePacket>();
                packet->status = compute_pass2_packet(source, sourceMutex, tiles[ti], ti,
                                                      reconstruction, appearance, o, out.exposure,
                                                      lut, hw, hh, p2Workspace[std::size_t(wi)], *packet);
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    ready[ti] = std::move(packet);
                    ++readyCount;
                    telemetry.maxReadyPackets = std::max(telemetry.maxReadyPackets, readyCount);
                }
                queueCv.notify_all();
            }
        });
    }

    StreamStatus commitStatus = StreamStatus::ok();
    for (std::size_t ci = 0; ci < tiles.size(); ++ci) {
        std::unique_ptr<TilePacket> packet;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCv.wait(lock, [&] { return cancel || bool(ready[ci]); });
            if (cancel && !ready[ci]) break;
            packet = std::move(ready[ci]);
            --readyCount;
            nextCommit = ci + 1;
        }
        queueCv.notify_all();

        if (!packet || !packet->status) {
            commitStatus = packet ? packet->status
                                  : StreamStatus::error(StreamStatusCode::BackendFailed, "missing ordered packet");
            break;
        }
        if (packet->tileIndex != ci) {
            telemetry.orderedCommit = false;
            commitStatus = StreamStatus::error(StreamStatusCode::BackendFailed, "non-canonical tile commit order");
            break;
        }

        st = sink.writeSdrTile(packet->core, packet->sdr.data(), packet->sdr.size());
        if (!st) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, st.message); break; }
        if (o.streamScientificDiagnostics) {
            st = sink.writeStage2DiagnosticTile(packet->core, packet->diagnostic.data(), packet->diagnostic.size());
            if (!st) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, st.message); break; }
        }
        st = sink.writeHalfLogGainBlock(packet->half, packet->gain.data(), packet->gain.size());
        if (!st) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, st.message); break; }
        out.tilesProcessedPass2++;
    }

    if (!commitStatus) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            cancel = true;
        }
        queueCv.notify_all();
    }
    for (auto& th : threads) th.join();
    if (!commitStatus) return commitStatus;

    st = sink.finishFrame();
    if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
    out.status = StreamStatus::ok();
    return out.status;
}

void require_exact_float_vector(const std::vector<float>& a,
                                const std::vector<float>& b,
                                const char* what) {
    REQUIRE(a.size() == b.size());
    if (!a.empty() && std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) != 0) {
        std::cerr << "EXACT_FLOAT_MISMATCH " << what << " max_abs_diff=" << max_abs_diff(a, b) << "\n";
        std::exit(3);
    }
}

struct RunOutput {
    StreamingResult result;
    std::vector<float> sdr;
    std::vector<float> gain;
    std::vector<float> diagnostic;
    CandidateTelemetry telemetry;
};

RunOutput run_reference(const DecodedDngFrame& frame) {
    FrameSource source(frame);
    CollectSink sink(frame.meta.width, frame.meta.height, true);
    auto reconstruction = std::make_shared<ResearchEdgeAwareMeasuredPreservingReconstruction>();
    auto appearance = std::make_shared<SkinSafeDetailedCrispAppearance>();
    StreamingTruthRawProcessor processor(reconstruction, appearance);
    StreamingOptions options;
    options.tile = {32, 7};
    options.workers = 1;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    StreamingResult result;
    const auto st = processor.process(source, sink, options, result);
    REQUIRE(bool(st));
    REQUIRE(bool(result.status));
    REQUIRE(sink.finished());
    RunOutput out;
    out.result = result;
    out.sdr = sink.sdr();
    out.gain = sink.gain();
    out.diagnostic = sink.diagnostic();
    return out;
}

RunOutput run_multiworker(const DecodedDngFrame& frame, int workers) {
    FrameSource base(frame);
    CountingSource source(base);
    CollectSink sink(frame.meta.width, frame.meta.height, true);
    ResearchEdgeAwareMeasuredPreservingReconstruction reconstruction;
    SkinSafeDetailedCrispAppearance appearance;
    StreamingOptions options;
    options.tile = {32, 7};
    options.workers = workers;
    options.hdrEnabled = true;
    options.streamScientificDiagnostics = true;
    StreamingResult result;
    CandidateTelemetry telemetry;
    const auto st = run_candidate(source, sink, reconstruction, appearance, options, result, telemetry);
    REQUIRE(bool(st));
    REQUIRE(bool(result.status));
    REQUIRE(sink.finished());
    telemetry.sourceRawTileReads = source.rawReads();
    telemetry.sourceConcurrencyPeak = source.concurrencyPeak();
    RunOutput out;
    out.result = result;
    out.sdr = sink.sdr();
    out.gain = sink.gain();
    out.diagnostic = sink.diagnostic();
    out.telemetry = telemetry;
    return out;
}

void compare_run(const RunOutput& reference, const RunOutput& candidate, int workers) {
    compare_exposure(reference.result.exposure, candidate.result.exposure);
    REQUIRE(reference.result.stage2Over1Count == candidate.result.stage2Over1Count);
    REQUIRE(reference.result.clippedCount == candidate.result.clippedCount);
    REQUIRE(reference.result.tilesProcessedPass1 == candidate.result.tilesProcessedPass1);
    REQUIRE(reference.result.tilesProcessedPass2 == candidate.result.tilesProcessedPass2);
    REQUIRE(candidate.result.provenance.physicalFrameCount == 1);
    REQUIRE(candidate.result.provenance.independentEvidenceCount == 1);
    require_exact_float_vector(reference.sdr, candidate.sdr, "sdr");
    require_exact_float_vector(reference.gain, candidate.gain, "halfLogGain");
    require_exact_float_vector(reference.diagnostic, candidate.diagnostic, "stage2Diagnostic");
    REQUIRE(candidate.telemetry.sourceConcurrencyPeak == 1);
    REQUIRE(candidate.telemetry.orderedCommit);
    REQUIRE(candidate.telemetry.maxReadyPackets <= candidate.telemetry.queueDepth);
    REQUIRE(candidate.telemetry.queueDepth <= std::size_t(2 * workers));
    REQUIRE(candidate.telemetry.sourceRawTileReads == 2 * candidate.result.tilesProcessedPass1);
}

} // namespace

int main() {
    const auto frame = make_frame(258, 194);
    const auto reference = run_reference(frame);

    const auto two = run_multiworker(frame, 2);
    compare_run(reference, two, 2);

    const auto four = run_multiworker(frame, 4);
    compare_run(reference, four, 4);

    for (int repeat = 0; repeat < 3; ++repeat) {
        const auto again = run_multiworker(frame, 4);
        compare_run(reference, again, 4);
    }

    std::cout << "FULL_FRAME_STREAMING_V0_2_MULTIWORKER_PASS\n";
    std::cout << "workers_tested=2,4\n";
    std::cout << "four_worker_repeats=3\n";
    std::cout << "source_reader_concurrency=1\n";
    std::cout << "sink_commit_order=CANONICAL_TILE_INDEX\n";
    std::cout << "authoritative_output_equivalence=EXACT_FLOAT_BYTES\n";
    std::cout << "physical_frame_count=1 independent_evidence_count=1\n";
    return 0;
}

#include "multiworker_streaming_v0_2.h"
#include "full_frame_streaming_v0_1_internal.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace truthraw::streaming_v0_2 {
namespace {

using namespace streaming_v0_1;
using namespace streaming_v0_1::detail;

struct Pass1Worker {
    Workspace workspace;
    Pass1Stats stats;
    StreamStatus status = StreamStatus::ok();
};

StreamStatus process_pass1_tile(IRawTileSource& source,
                                std::mutex& sourceMutex,
                                const TileRect& tile,
                                IReconstructionBackend& reconstruction,
                                Pass1Worker& worker) {
    const auto& m = source.metadata();
    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        auto st = fill_stage2(source, tile, worker.workspace);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
    }

    auto& w = worker.workspace;
    auto& stats = worker.stats;
    const int tw = tile.hx1 - tile.hx0;
    const int th = tile.hy1 - tile.hy0;
    const int cw = tile.x1 - tile.x0;
    const int ch = tile.y1 - tile.y0;
    const std::size_t coreN = std::size_t(cw) * std::size_t(ch);
    w.cam.resize(3 * coreN);

    auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, tile.hx0, tile.hy0,
                                             tile.x0, tile.y0, cw, ch, m.cfa, w.cam.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    camera_to_xyz(w.cam.data(), w.cam.data(), int(coreN), m.cameraToXyzD50);
    for (int cy = 0; cy < ch; ++cy) {
        for (int cx = 0; cx < cw; ++cx) {
            const int x = tile.x0 + cx;
            const int y = tile.y0 + cy;
            const std::size_t ci = std::size_t(cy) * std::size_t(cw) + std::size_t(cx);
            stats.scene.add(std::max(w.cam[3 * ci + 1], 0.0f));
            const int lx = x - tile.hx0;
            const int ly = y - tile.hy0;
            const std::size_t ri = std::size_t(ly) * std::size_t(tw) + std::size_t(lx);
            const float stage2 = w.stage2[ri];
            if (stage2 > 1.0f) stats.totalOver1++;
            if (float(w.raw[ri]) >= m.whiteLevel) stats.totalClipped++;
        }
    }

    xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(coreN));
    for (std::size_t ci = 0; ci < coreN; ++ci) {
        const float y = std::max(luminance709(w.cam[3 * ci], w.cam[3 * ci + 1], w.cam[3 * ci + 2]), 0.0f);
        stats.display.add(y);
    }

    stats.workspacePeak = std::max(stats.workspacePeak, vector_bytes(w));
    stats.tilesProcessed++;
    return StreamStatus::ok();
}

Pass1Stats reduce_pass1(const std::vector<std::unique_ptr<Pass1Worker>>& workers) {
    Pass1Stats total;
    for (const auto& worker : workers) {
        const auto& stats = worker->stats;
        for (int i = 0; i < kHistBins; ++i) {
            total.display.bins[std::size_t(i)] += stats.display.bins[std::size_t(i)];
            total.scene.bins[std::size_t(i)] += stats.scene.bins[std::size_t(i)];
        }
        total.display.total += stats.display.total;
        total.scene.total += stats.scene.total;
        total.totalOver1 += stats.totalOver1;
        total.totalClipped += stats.totalClipped;
        total.workspacePeak = std::max(total.workspacePeak, stats.workspacePeak);
        total.tilesProcessed += stats.tilesProcessed;
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
                                  const TileRect& tile,
                                  std::size_t tileIndex,
                                  IReconstructionBackend& reconstruction,
                                  const IAppearanceBackend& appearance,
                                  const StreamingOptions& options,
                                  const ExposurePlan& exposure,
                                  const std::vector<float>& lut,
                                  int halfWidth,
                                  int halfHeight,
                                  Workspace& w,
                                  TilePacket& packet) {
    const auto& m = source.metadata();
    packet.tileIndex = tileIndex;
    packet.core = tile;

    {
        std::lock_guard<std::mutex> lock(sourceMutex);
        auto st = fill_stage2(source, tile, w);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
    }

    const int tw = tile.hx1 - tile.hx0;
    const int th = tile.hy1 - tile.hy0;
    const int cw = tile.x1 - tile.x0;
    const int ch = tile.y1 - tile.y0;
    const int appearanceHalo = appearance.requiredHalo();
    const int ax0 = std::max(0, tile.x0 - appearanceHalo);
    const int ay0 = std::max(0, tile.y0 - appearanceHalo);
    const int ax1 = std::min(m.width, tile.x1 + appearanceHalo);
    const int ay1 = std::min(m.height, tile.y1 + appearanceHalo);
    const int aw = ax1 - ax0;
    const int ah = ay1 - ay0;
    const std::size_t appN = std::size_t(aw) * std::size_t(ah);
    const std::size_t coreN = std::size_t(cw) * std::size_t(ch);

    w.cam.resize(3 * appN);
    w.look.resize(3 * coreN);
    auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, tile.hx0, tile.hy0,
                                             ax0, ay0, aw, ah, m.cfa, w.cam.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    camera_to_xyz(w.cam.data(), w.cam.data(), int(appN), m.cameraToXyzD50);
    const HalfStateRect half = half_core_rect(tile);
    packet.half = half;
    const int qw = half.x1 - half.x0;
    const int qh = half.y1 - half.y0;
    const std::size_t qn = std::size_t(qw) * std::size_t(qh);
    w.halfScene.assign(qn, 0.0f);

    for (int y = tile.y0; y < tile.y1; ++y) {
        for (int x = tile.x0; x < tile.x1; ++x) {
            const std::size_t ai = std::size_t(y - ay0) * std::size_t(aw) + std::size_t(x - ax0);
            const int qx = x / 2 - half.x0;
            const int qy = y / 2 - half.y0;
            const std::size_t qi = std::size_t(qy) * std::size_t(qw) + std::size_t(qx);
            w.halfScene[qi] = std::max(w.halfScene[qi], std::max(w.cam[3 * ai + 1], 0.0f));
        }
    }

    xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(appN));
    cs = appearance.applyTile(w.cam.data(), aw, ah, tile.x0 - ax0, tile.y0 - ay0,
                              cw, ch, w.look.data());
    if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

    for (std::size_t i = 0; i < coreN; ++i) {
        float r = w.look[3 * i];
        float g = w.look[3 * i + 1];
        float b = w.look[3 * i + 2];
        const float y = std::max(luminance709(r, g, b), 0.0f);
        const float yOut = lut_sample(lut, y);
        const float scale = y > 1e-8f ? yOut / y : 0.0f;
        r = std::max(r * scale, 0.0f);
        g = std::max(g * scale, 0.0f);
        b = std::max(b * scale, 0.0f);
        const float mx = std::max(r, std::max(g, b));
        if (mx > 1.0f) { r /= mx; g /= mx; b /= mx; }
        w.look[3 * i] = r;
        w.look[3 * i + 1] = g;
        w.look[3 * i + 2] = b;
    }
    packet.sdr = w.look;

    if (options.streamScientificDiagnostics) {
        packet.diagnostic.resize(coreN);
        for (int cy = 0; cy < ch; ++cy) {
            for (int cx = 0; cx < cw; ++cx) {
                const int lx = tile.x0 + cx - tile.hx0;
                const int ly = tile.y0 + cy - tile.hy0;
                packet.diagnostic[std::size_t(cy) * std::size_t(cw) + std::size_t(cx)] =
                    w.stage2[std::size_t(ly) * std::size_t(tw) + std::size_t(lx)];
            }
        }
    }

    w.halfGain.resize(qn);
    for (int qy = half.y0; qy < half.y1; ++qy) {
        for (int qx = half.x0; qx < half.x1; ++qx) {
            const std::size_t qo = std::size_t(qy - half.y0) * std::size_t(qw) + std::size_t(qx - half.x0);
            if (!options.hdrEnabled) { w.halfGain[qo] = 0.0f; continue; }

            bool censored = false;
            for (int nqy = std::max(0, qy - 1); nqy <= std::min(halfHeight - 1, qy + 1) && !censored; ++nqy) {
                for (int nqx = std::max(0, qx - 1); nqx <= std::min(halfWidth - 1, qx + 1) && !censored; ++nqx) {
                    for (int py = 2 * nqy; py < std::min(m.height, 2 * nqy + 2) && !censored; ++py) {
                        for (int px = 2 * nqx; px < std::min(m.width, 2 * nqx + 2); ++px) {
                            if (px < tile.hx0 || px >= tile.hx1 || py < tile.hy0 || py >= tile.hy1)
                                return StreamStatus::error(StreamStatusCode::BackendFailed,
                                                           "RAW halo insufficient for half-censor dilation");
                            const std::size_t ri = std::size_t(py - tile.hy0) * std::size_t(tw) + std::size_t(px - tile.hx0);
                            if (float(w.raw[ri]) >= m.whiteLevel) { censored = true; break; }
                        }
                    }
                }
            }
            if (censored) { w.halfGain[qo] = 0.0f; continue; }

            float sdrSum = 0.0f;
            int count = 0;
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    const int x = 2 * qx + dx;
                    const int y = 2 * qy + dy;
                    if (x < tile.x0 || x >= tile.x1 || y < tile.y0 || y >= tile.y1) continue;
                    const std::size_t li = std::size_t(y - tile.y0) * std::size_t(cw) + std::size_t(x - tile.x0);
                    sdrSum += std::max(luminance709(w.look[3 * li], w.look[3 * li + 1], w.look[3 * li + 2]), 0.0f);
                    ++count;
                }
            }
            if (count == 0)
                return StreamStatus::error(StreamStatusCode::BackendFailed, "2x2 half-gain cell lost tile ownership");

            const float yBase = sdrSum / float(count);
            const float target = w.halfScene[qo] * exposure.sceneToDisplayScalar;
            const float rawGain = yBase > 1e-7f ? target / yBase : 1.0f;
            const float t = clamp01((yBase - exposure.hdrGateStartY) /
                                    std::max(exposure.hdrGateFullY - exposure.hdrGateStartY, 1e-8f));
            const float gate = t * t * (3.0f - 2.0f * t);
            const float gain = 1.0f + exposure.evidenceConfidence * gate *
                (std::max(1.0f, std::min(exposure.hdrMaxGain, rawGain)) - 1.0f);
            w.halfGain[qo] = std::log2(std::max(gain, 1.0f));
        }
    }
    packet.gain = w.halfGain;
    return StreamStatus::ok();
}

} // namespace

streaming_v0_1::StreamStatus process_multiworker_streaming(
    streaming_v0_1::IRawTileSource& source,
    streaming_v0_1::IStreamingSink& sink,
    IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    streaming_v0_1::StreamingOptions options,
    streaming_v0_1::StreamingResult& out,
    MultiWorkerTelemetry& telemetry) {
    using namespace streaming_v0_1;
    using namespace streaming_v0_1::detail;

    out = StreamingResult{};
    telemetry = MultiWorkerTelemetry{};
    const auto& m = source.metadata();
    if (m.width <= 1 || m.height <= 1) return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid dimensions");
    if (options.workers < 1) return StreamStatus::error(StreamStatusCode::InvalidArgument, "workers < 1");
    if (options.tile.core <= 0 || options.tile.halo < 0)
        return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid tile policy");
    if ((options.tile.core & 1) != 0)
        return StreamStatus::error(StreamStatusCode::InvalidArgument, "even tile core required");
    const int requiredHalo = std::max(options.hdrEnabled ? 2 : 0,
                                      reconstruction.requiredHalo() + appearance.requiredHalo());
    if (options.tile.halo < requiredHalo)
        return StreamStatus::error(StreamStatusCode::InvalidArgument, "insufficient halo");
    if (options.sdrLutSize < 2)
        return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid LUT size");

    const auto tiles = make_tiles(m.width, m.height, options.tile);
    if (tiles.empty()) return StreamStatus::error(StreamStatusCode::InvalidArgument, "no tiles");
    const int workerCount = std::max(1, std::min(options.workers, int(tiles.size())));
    telemetry.effectiveWorkers = workerCount;
    telemetry.queueDepth = std::min<std::size_t>(tiles.size(), std::size_t(2 * workerCount));

    StreamingOptions one = options;
    one.workers = 1;
    bool memoryOk = true;
    const std::size_t perWorker = estimate_workspace(m, one, appearance, memoryOk);
    if (!memoryOk) return StreamStatus::error(StreamStatusCode::InvalidArgument, "workspace overflow");
    const int coreW = std::min(m.width, options.tile.core);
    const int coreH = std::min(m.height, options.tile.core);
    const std::size_t coreN = std::size_t(coreW) * std::size_t(coreH);
    const std::size_t halfN = std::size_t((coreW + 1) / 2) * std::size_t((coreH + 1) / 2);
    std::size_t packetBytes = 3 * coreN * sizeof(float) + halfN * sizeof(float);
    if (options.streamScientificDiagnostics) packetBytes += coreN * sizeof(float);
    const std::size_t residentUpper = source.residentBytesUpperBound() + sink.residentBytesUpperBound()
        + std::size_t(workerCount) * perWorker + telemetry.queueDepth * packetBytes;
    if (options.memoryBudgetBytes != 0 && residentUpper > options.memoryBudgetBytes)
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
    std::vector<std::unique_ptr<Pass1Worker>> pass1Workers;
    pass1Workers.reserve(std::size_t(workerCount));
    for (int i = 0; i < workerCount; ++i) pass1Workers.emplace_back(std::make_unique<Pass1Worker>());

    std::vector<std::thread> threads;
    threads.reserve(std::size_t(workerCount));
    for (int wi = 0; wi < workerCount; ++wi) {
        threads.emplace_back([&, wi] {
            auto& worker = *pass1Workers[std::size_t(wi)];
            while (true) {
                const std::size_t ti = nextPass1.fetch_add(1, std::memory_order_relaxed);
                if (ti >= tiles.size()) break;
                worker.status = process_pass1_tile(source, sourceMutex, tiles[ti], reconstruction, worker);
                if (!worker.status) break;
            }
        });
    }
    for (auto& thread : threads) thread.join();
    threads.clear();
    for (const auto& worker : pass1Workers) if (!worker->status) return worker->status;

    const Pass1Stats pass1 = reduce_pass1(pass1Workers);
    const std::size_t pixelCount = std::size_t(m.width) * std::size_t(m.height);
    const float clipFraction = pixelCount ? float(pass1.totalClipped) / float(pixelCount) : 0.0f;
    out.exposure = choose_exposure_plan_from_histograms(
        pass1.display.bins, pass1.scene.bins, 4.0f, noise_sigma_2pct(m),
        clipFraction, pass1.totalOver1);
    const auto lut = build_monotone_lut(out.exposure, options.sdrLutSize);
    out.stage2Over1Count = pass1.totalOver1;
    out.clippedCount = pass1.totalClipped;
    out.tilesProcessedPass1 = pass1.tilesProcessed;

    auto status = sink.beginFrame(m.width, m.height, m.orientation, out.exposure,
                                  options.hdrEnabled, options.streamScientificDiagnostics);
    if (!status) return StreamStatus::error(StreamStatusCode::SinkFailed, status.message);

    const int halfWidth = (m.width + 1) / 2;
    const int halfHeight = (m.height + 1) / 2;
    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::size_t nextClaim = 0;
    std::size_t nextCommit = 0;
    std::size_t readyCount = 0;
    bool cancel = false;
    std::vector<std::unique_ptr<TilePacket>> ready(tiles.size());
    std::vector<Workspace> pass2Workspace{std::size_t(workerCount)};

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
                                                      reconstruction, appearance, options, out.exposure,
                                                      lut, halfWidth, halfHeight,
                                                      pass2Workspace[std::size_t(wi)], *packet);
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

        status = sink.writeSdrTile(packet->core, packet->sdr.data(), packet->sdr.size());
        if (!status) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, status.message); break; }
        if (options.streamScientificDiagnostics) {
            status = sink.writeStage2DiagnosticTile(packet->core, packet->diagnostic.data(), packet->diagnostic.size());
            if (!status) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, status.message); break; }
        }
        status = sink.writeHalfLogGainBlock(packet->half, packet->gain.data(), packet->gain.size());
        if (!status) { commitStatus = StreamStatus::error(StreamStatusCode::SinkFailed, status.message); break; }
        out.tilesProcessedPass2++;
    }

    if (!commitStatus) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            cancel = true;
        }
        queueCv.notify_all();
    }
    for (auto& thread : threads) thread.join();
    if (!commitStatus) return commitStatus;

    status = sink.finishFrame();
    if (!status) return StreamStatus::error(StreamStatusCode::SinkFailed, status.message);
    out.status = StreamStatus::ok();
    return out.status;
}

} // namespace truthraw::streaming_v0_2

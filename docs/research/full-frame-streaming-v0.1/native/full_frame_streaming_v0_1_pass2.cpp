#include "full_frame_streaming_v0_1_internal.h"

namespace truthraw::streaming_v0_1::detail {

StreamStatus run_pass2(
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
    std::size_t& workspacePeak,
    std::size_t& tilesProcessed) {
    const auto& m = source.metadata();
    StreamStatus st;
    // PASS 2: recompute one tile, apply the now-known global exposure plan,
    // stream SDR/diagnostic output, compute its owned half-gain cells, release.
    for (const auto& t : tiles) {
        st = fill_stage2(source, t, w);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
        const int tw = t.hx1 - t.hx0;
        const int th = t.hy1 - t.hy0;
        const int cw = t.x1 - t.x0;
        const int ch = t.y1 - t.y0;
        const int ah = appearance.requiredHalo();
        const int ax0 = std::max(0, t.x0 - ah), ay0 = std::max(0, t.y0 - ah);
        const int ax1 = std::min(m.width, t.x1 + ah), ay1 = std::min(m.height, t.y1 + ah);
        const int aw = ax1 - ax0, ahh = ay1 - ay0;
        const std::size_t appN = std::size_t(aw) * std::size_t(ahh);
        const std::size_t coreN = std::size_t(cw) * std::size_t(ch);
        w.cam.resize(3*appN); w.look.resize(3*coreN);
        auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, t.hx0, t.hy0,
                                                   ax0, ay0, aw, ahh, m.cfa, w.cam.data());
        if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);
        camera_to_xyz(w.cam.data(), w.cam.data(), int(appN), m.cameraToXyzD50);
        const HalfStateRect qr = half_core_rect(t);
        const int qw = qr.x1-qr.x0, qh = qr.y1-qr.y0;
        const std::size_t qn = std::size_t(qw)*std::size_t(qh);
        w.halfScene.assign(qn, 0.0f);
        for (int y=t.y0; y<t.y1; ++y) for (int x=t.x0; x<t.x1; ++x) {
            const std::size_t ai = std::size_t(y-ay0)*std::size_t(aw)+std::size_t(x-ax0);
            const int qx=x/2-qr.x0, qy=y/2-qr.y0;
            const std::size_t qi=std::size_t(qy)*std::size_t(qw)+std::size_t(qx);
            w.halfScene[qi]=std::max(w.halfScene[qi],std::max(w.cam[3*ai+1],0.0f));
        }
        xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(appN));
        cs = appearance.applyTile(w.cam.data(), aw, ahh, t.x0-ax0, t.y0-ay0, cw, ch, w.look.data());
        if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);

        st = sink.writeExtendedLinearTile(t, w.look.data(), w.look.size());
        if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

        for (std::size_t i = 0; i < coreN; ++i) {
            float rr = w.look[3*i], gg = w.look[3*i+1], bb = w.look[3*i+2];
            const float Y = std::max(luminance709(rr,gg,bb), 0.0f);
            const float Yo = lut_sample(lut, Y);
            const float sc = Y > 1e-8f ? Yo/Y : 0.0f;
            rr = std::max(rr*sc, 0.0f); gg = std::max(gg*sc, 0.0f); bb = std::max(bb*sc, 0.0f);
            const float mx = std::max(rr, std::max(gg,bb));
            if (mx > 1.0f) { rr/=mx; gg/=mx; bb/=mx; }
            w.look[3*i]=rr; w.look[3*i+1]=gg; w.look[3*i+2]=bb;
        }
        st = sink.writeSdrTile(t, w.look.data(), w.look.size());
        if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);

        if (o.streamScientificDiagnostics) {
            w.diagnosticCore.resize(coreN);
            for (int cy=0; cy<ch; ++cy) for (int cx=0; cx<cw; ++cx) {
                const int lx = t.x0 + cx - t.hx0, ly = t.y0 + cy - t.hy0;
                w.diagnosticCore[std::size_t(cy)*std::size_t(cw)+std::size_t(cx)] =
                    w.stage2[std::size_t(ly)*std::size_t(tw)+std::size_t(lx)];
            }
            st = sink.writeStage2DiagnosticTile(t, w.diagnosticCore.data(), coreN);
            if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        }

        w.halfGain.resize(qn);
        for (int qy = qr.y0; qy < qr.y1; ++qy) {
            for (int qx = qr.x0; qx < qr.x1; ++qx) {
                const std::size_t qo = std::size_t(qy-qr.y0)*std::size_t(qw)+std::size_t(qx-qr.x0);
                if (!o.hdrEnabled) { w.halfGain[qo] = 0.0f; continue; }
                bool censored = false;
                for (int nqy=std::max(0,qy-1); nqy<=std::min(hh-1,qy+1) && !censored; ++nqy) {
                    for (int nqx=std::max(0,qx-1); nqx<=std::min(hw-1,qx+1) && !censored; ++nqx) {
                        for (int py=2*nqy; py<std::min(m.height,2*nqy+2) && !censored; ++py) {
                            for (int px=2*nqx; px<std::min(m.width,2*nqx+2); ++px) {
                                if (px<t.hx0 || px>=t.hx1 || py<t.hy0 || py>=t.hy1)
                                    return StreamStatus::error(StreamStatusCode::BackendFailed, "RAW halo insufficient for half-censor dilation");
                                const std::size_t ri=std::size_t(py-t.hy0)*std::size_t(tw)+std::size_t(px-t.hx0);
                                if (float(w.raw[ri])>=m.whiteLevel) { censored=true; break; }
                            }
                        }
                    }
                }
                if (censored) { w.halfGain[qo] = 0.0f; continue; }
                float sdrSum = 0.0f; int cnt = 0;
                for (int dy=0; dy<2; ++dy) for (int dx=0; dx<2; ++dx) {
                    const int x=2*qx+dx, y=2*qy+dy;
                    if (x<t.x0||x>=t.x1||y<t.y0||y>=t.y1) continue;
                    const std::size_t li = std::size_t(y-t.y0)*std::size_t(cw)+std::size_t(x-t.x0);
                    sdrSum += std::max(luminance709(w.look[3*li],w.look[3*li+1],w.look[3*li+2]),0.0f);
                    cnt++;
                }
                if (cnt == 0) return StreamStatus::error(StreamStatusCode::BackendFailed, "2x2 half-gain cell lost tile ownership");
                const float Yb = sdrSum / float(cnt);
                const float target = w.halfScene[qo] * exposure.sceneToDisplayScalar;
                const float rawGain = Yb > 1e-7f ? target/Yb : 1.0f;
                const float tt = clamp01((Yb-exposure.hdrGateStartY)/std::max(exposure.hdrGateFullY-exposure.hdrGateStartY,1e-8f));
                const float gate = tt*tt*(3.0f-2.0f*tt);
                const float g = 1.0f + exposure.evidenceConfidence*gate*(std::max(1.0f,std::min(exposure.hdrMaxGain,rawGain))-1.0f);
                w.halfGain[qo] = std::log2(std::max(g,1.0f));
            }
        }
        st = sink.writeHalfLogGainBlock(qr, w.halfGain.data(), w.halfGain.size());
        if (!st) return StreamStatus::error(StreamStatusCode::SinkFailed, st.message);
        workspacePeak = std::max(workspacePeak, vector_bytes(w));
        tilesProcessed++;
    }
    return StreamStatus::ok();
}

} // namespace truthraw::streaming_v0_1::detail

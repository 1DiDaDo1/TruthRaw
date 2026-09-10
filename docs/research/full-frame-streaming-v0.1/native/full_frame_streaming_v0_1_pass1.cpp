#include "full_frame_streaming_v0_1_internal.h"

namespace truthraw::streaming_v0_1::detail {

StreamStatus run_pass1(
    IRawTileSource& source,
    const std::vector<TileRect>& tiles,
    IReconstructionBackend& reconstruction,
    Workspace& w,
    Pass1Stats& stats) {
    const auto& m = source.metadata();
    StreamStatus st;
    // PASS 1: reconstruct each tile only long enough to accumulate global
    // histograms only. No full RGB frame or half-resolution scratch image is kept.
    for (const auto& t : tiles) {
        st = fill_stage2(source, t, w);
        if (!st) return StreamStatus::error(StreamStatusCode::SourceFailed, st.message);
        const int tw = t.hx1 - t.hx0;
        const int th = t.hy1 - t.hy0;
        const int cw = t.x1 - t.x0;
        const int ch = t.y1 - t.y0;
        const std::size_t coreN = std::size_t(cw) * std::size_t(ch);
        w.cam.resize(3*coreN);
        auto cs = reconstruction.reconstructTile(w.stage2.data(), tw, th, t.hx0, t.hy0,
                                                   t.x0, t.y0, cw, ch, m.cfa, w.cam.data());
        if (!cs) return StreamStatus::error(StreamStatusCode::BackendFailed, cs.message);
        camera_to_xyz(w.cam.data(), w.cam.data(), int(coreN), m.cameraToXyzD50);
        for (int cy = 0; cy < ch; ++cy) {
            for (int cx = 0; cx < cw; ++cx) {
                const int x = t.x0 + cx, y = t.y0 + cy;
                const std::size_t ci = std::size_t(cy)*std::size_t(cw)+std::size_t(cx);
                stats.scene.add(std::max(w.cam[3*ci+1], 0.0f));
                const int lx = x - t.hx0, ly = y - t.hy0;
                const float s2 = w.stage2[std::size_t(ly)*std::size_t(tw)+std::size_t(lx)];
                if (s2 > 1.0f) stats.totalOver1++;
                const std::size_t ri = std::size_t(ly)*std::size_t(tw)+std::size_t(lx);
                if (float(w.raw[ri]) >= m.whiteLevel) stats.totalClipped++;
            }
        }
        xyz_d50_to_linear_srgb(w.cam.data(), w.cam.data(), int(coreN));
        for (std::size_t ci = 0; ci < coreN; ++ci) {
            const float nY = std::max(luminance709(w.cam[3*ci], w.cam[3*ci+1], w.cam[3*ci+2]), 0.0f);
            stats.display.add(nY);
        }
        stats.workspacePeak = std::max(stats.workspacePeak, vector_bytes(w));
        stats.tilesProcessed++;
    }
    return StreamStatus::ok();
}

} // namespace truthraw::streaming_v0_1::detail

#pragma once

#include "full_frame_streaming_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace truthraw::streaming_v0_1::detail {

constexpr int kHistBins = 16384;

inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

inline std::size_t checked_add(std::size_t a, std::size_t b, bool& ok) {
    if (a > std::numeric_limits<std::size_t>::max() - b) { ok = false; return 0; }
    return a + b;
}

inline std::size_t checked_mul(std::size_t a, std::size_t b, bool& ok) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) { ok = false; return 0; }
    return a * b;
}

struct Histogram {
    std::vector<std::uint64_t> bins;
    float maxValue = 1.0f;
    std::uint64_t total = 0;
    explicit Histogram(float maxV) : bins(kHistBins, 0), maxValue(maxV) {}
    void add(float x) {
        x = std::max(0.0f, std::min(maxValue, x));
        int i = int((x / maxValue) * float(kHistBins - 1) + 0.5f);
        i = std::max(0, std::min(kHistBins - 1, i));
        bins[std::size_t(i)]++;
        total++;
    }
};

inline float noise_sigma_2pct(const DngMetadata& m) {
    if (!m.hasNoiseProfile) return 0.0f;
    float v = 0.0f;
    for (int c = 0; c < 3; ++c) {
        const float S = m.noiseProfile[2 * c];
        const float O = m.noiseProfile[2 * c + 1];
        v += std::max(S * 0.02f + O, 0.0f);
    }
    return std::sqrt(v / 3.0f);
}

inline void camera_to_xyz(const float* cam, float* xyz, int n, const std::array<float,9>& M) {
    for (int i = 0; i < n; ++i) {
        const float r = cam[3*i], g = cam[3*i+1], b = cam[3*i+2];
        xyz[3*i]   = M[0]*r + M[1]*g + M[2]*b;
        xyz[3*i+1] = M[3]*r + M[4]*g + M[5]*b;
        xyz[3*i+2] = M[6]*r + M[7]*g + M[8]*b;
    }
}

inline void xyz_d50_to_linear_srgb(const float* xyz, float* rgb, int n) {
    static const float M[9] = {
        3.1338561f,-1.6168667f,-0.4906146f,
       -0.9787684f, 1.9161415f, 0.0334540f,
        0.0719453f,-0.2289914f, 1.4052427f
    };
    for (int i = 0; i < n; ++i) {
        const float X = xyz[3*i], Y = xyz[3*i+1], Z = xyz[3*i+2];
        rgb[3*i]   = M[0]*X + M[1]*Y + M[2]*Z;
        rgb[3*i+1] = M[3]*X + M[4]*Y + M[5]*Z;
        rgb[3*i+2] = M[6]*X + M[7]*Y + M[8]*Z;
    }
}

inline float lut_sample(const std::vector<float>& lut, float x) {
    x = clamp01(x);
    const float u = x * float(lut.size() - 1);
    const int i = int(u);
    const int j = std::min(i + 1, int(lut.size() - 1));
    const float f = u - float(i);
    return lut[std::size_t(i)] + (lut[std::size_t(j)] - lut[std::size_t(i)]) * f;
}

struct Workspace {
    std::vector<std::uint16_t> raw;
    std::vector<float> gain;
    std::vector<float> rowBias;
    std::vector<float> colBias;
    std::vector<float> stage2;
    std::vector<float> cam;
    std::vector<float> look;
    std::vector<float> diagnosticCore;
    std::vector<float> halfScene;
    std::vector<float> halfGain;
};

inline std::size_t vector_bytes(const Workspace& w) {
    return w.raw.capacity()*sizeof(std::uint16_t)
        + (w.gain.capacity()+w.rowBias.capacity()+w.colBias.capacity()+w.stage2.capacity()
           +w.cam.capacity()+w.look.capacity()
           +w.diagnosticCore.capacity()+w.halfScene.capacity()+w.halfGain.capacity())*sizeof(float);
}

inline StreamStatus fill_stage2(IRawTileSource& source, const TileRect& t, Workspace& w) {
    const auto& m = source.metadata();
    const int tw = t.hx1 - t.hx0;
    const int th = t.hy1 - t.hy0;
    const std::size_t n = std::size_t(tw) * std::size_t(th);
    w.raw.resize(n);
    if (m.hasGainField) w.gain.resize(n); else w.gain.clear();
    if (m.hasResidualBlack) {
        w.rowBias.resize(std::size_t(th));
        w.colBias.resize(std::size_t(tw));
    } else {
        w.rowBias.clear(); w.colBias.clear();
    }
    auto s = source.readRawTile(t, w.raw.data(), n,
                                m.hasGainField ? w.gain.data() : nullptr,
                                m.hasGainField ? n : 0);
    if (!s) return s;
    if (m.hasResidualBlack) {
        s = source.readRowBias(t.hy0, t.hy1, w.rowBias.data(), w.rowBias.size());
        if (!s) return s;
        s = source.readColBias(t.hx0, t.hx1, w.colBias.data(), w.colBias.size());
        if (!s) return s;
    }
    w.stage2.resize(n);
    for (int yy = 0; yy < th; ++yy) {
        const int y = t.hy0 + yy;
        for (int xx = 0; xx < tw; ++xx) {
            const int x = t.hx0 + xx;
            const int ph = (y & 1) * 2 + (x & 1);
            float black = m.blackPhase[std::size_t(ph)];
            if (m.hasResidualBlack) black += w.rowBias[std::size_t(yy)] + w.colBias[std::size_t(xx)];
            const float denom = std::max(m.whiteLevel - black, 1.0f);
            const std::size_t i = std::size_t(yy) * std::size_t(tw) + std::size_t(xx);
            float v = (float(w.raw[i]) - black) / denom;
            if (m.hasGainField) v *= w.gain[i];
            w.stage2[i] = v;
        }
    }
    return StreamStatus::ok();
}

inline HalfStateRect half_core_rect(const TileRect& t) {
    return {t.x0/2, t.y0/2, (t.x1+1)/2, (t.y1+1)/2};
}

inline bool valid_options(const DngMetadata& m, const StreamingOptions& o,
                          const IReconstructionBackend& r, const IAppearanceBackend& a,
                          std::string& why) {
    if (m.width <= 1 || m.height <= 1) { why = "invalid frame dimensions"; return false; }
    if (o.tile.core <= 0 || o.tile.halo < 0) { why = "invalid tile policy"; return false; }
    if ((o.tile.core & 1) != 0) { why = "streaming v0.1 requires even tile core so each 2x2 HDR cell has one owner"; return false; }
    const int requiredHalo = std::max(o.hdrEnabled ? 2 : 0, r.requiredHalo() + a.requiredHalo());
    if (o.tile.halo < requiredHalo) { why = "tile halo smaller than reconstruction/appearance or HDR-censor streaming requirement"; return false; }
    if (o.workers != 1) { why = "v0.1 correctness path is single-worker; multi-worker streaming remains open"; return false; }
    if (o.sdrLutSize < 2) { why = "invalid LUT size"; return false; }
    return true;
}

inline std::size_t estimate_workspace(const DngMetadata& m, const StreamingOptions& o,
                                      const IAppearanceBackend& appearance, bool& ok) {
    ok = true;
    const int haloW = std::min(m.width, o.tile.core + 2*o.tile.halo);
    const int haloH = std::min(m.height, o.tile.core + 2*o.tile.halo);
    const int ah = appearance.requiredHalo();
    const int appW = std::min(m.width, o.tile.core + 2*ah);
    const int appH = std::min(m.height, o.tile.core + 2*ah);
    const int coreW = std::min(m.width, o.tile.core);
    const int coreH = std::min(m.height, o.tile.core);
    const int qW = (coreW + 1) / 2;
    const int qH = (coreH + 1) / 2;

    std::size_t total = 0;
    auto add = [&](std::size_t count, std::size_t elem) {
        const std::size_t b = checked_mul(count, elem, ok);
        if (!ok) return;
        total = checked_add(total, b, ok);
    };
    const std::size_t haloN = checked_mul(std::size_t(haloW), std::size_t(haloH), ok);
    const std::size_t appN = checked_mul(std::size_t(appW), std::size_t(appH), ok);
    const std::size_t coreN = checked_mul(std::size_t(coreW), std::size_t(coreH), ok);
    const std::size_t qN = checked_mul(std::size_t(qW), std::size_t(qH), ok);
    if (!ok) return 0;

    add(haloN, sizeof(std::uint16_t));                  // raw tile
    if (m.hasGainField) add(haloN, sizeof(float));      // gain tile
    if (m.hasResidualBlack) {
        add(std::size_t(haloH), sizeof(float));
        add(std::size_t(haloW), sizeof(float));
    }
    add(haloN, sizeof(float));                          // stage2
    add(3*appN, sizeof(float));                         // in-place camera -> XYZ -> neutral RGB
    add(3*coreN, sizeof(float));                        // in-place appearance -> SDR tile
    if (o.streamScientificDiagnostics) add(coreN, sizeof(float));
    add(qN, sizeof(float));                             // pass2 tile-local half scene max
    add(qN, sizeof(float));                             // half gain write
    add(2*std::size_t(kHistBins), sizeof(std::uint64_t));
    add(std::size_t(o.sdrLutSize), sizeof(float));
    return ok ? total : 0;
}


struct Pass1Stats {
    Histogram display{1.0f};
    Histogram scene{4.0f};
    std::uint64_t totalOver1 = 0;
    std::uint64_t totalClipped = 0;
    std::size_t workspacePeak = 0;
    std::size_t tilesProcessed = 0;
};

StreamStatus run_pass1(
    IRawTileSource& source,
    const std::vector<TileRect>& tiles,
    IReconstructionBackend& reconstruction,
    Workspace& workspace,
    Pass1Stats& stats);

StreamStatus run_pass2(
    IRawTileSource& source,
    IStreamingSink& sink,
    const std::vector<TileRect>& tiles,
    IReconstructionBackend& reconstruction,
    const IAppearanceBackend& appearance,
    const StreamingOptions& options,
    const ExposurePlan& exposure,
    const std::vector<float>& lut,
    int halfWidth,
    int halfHeight,
    Workspace& workspace,
    std::size_t& workspacePeak,
    std::size_t& tilesProcessed);

} // namespace truthraw::streaming_v0_1::detail

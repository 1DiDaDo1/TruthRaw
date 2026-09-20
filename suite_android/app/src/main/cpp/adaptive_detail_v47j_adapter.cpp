#include "adaptive_detail_v47j_adapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace truthraw::adaptive_detail_v47j_adapter {
namespace {

inline float clamp01(float x) noexcept {
    return std::max(0.0f, std::min(1.0f, x));
}

inline float clampf(float x, float a, float b) noexcept {
    return std::max(a, std::min(b, x));
}

inline float smoothstep01(float x) noexcept {
    x = clamp01(x);
    return x * x * (3.0f - 2.0f * x);
}

float integral_box_mean(
    const std::vector<float>& integral,
    int stride,
    int width,
    int height,
    int x,
    int y,
    int radius) noexcept {
    const int x0 = std::max(0, x - radius);
    const int x1 = std::min(width - 1, x + radius);
    const int y0 = std::max(0, y - radius);
    const int y1 = std::min(height - 1, y + radius);
    const int xa = x0;
    const int xb = x1 + 1;
    const int ya = y0;
    const int yb = y1 + 1;
    const float sum =
        integral[std::size_t(yb) * stride + xb] -
        integral[std::size_t(ya) * stride + xb] -
        integral[std::size_t(yb) * stride + xa] +
        integral[std::size_t(ya) * stride + xa];
    return sum / float((x1 - x0 + 1) * (y1 - y0 + 1));
}

inline float luminance709_local(float r, float g, float b) noexcept {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

}  // namespace

float noise_sigma_2pct_from_metadata(const DngMetadata& m) noexcept {
    if (!m.hasNoiseProfile) return 0.0f;
    float variance = 0.0f;
    for (int c = 0; c < 3; ++c) {
        const float slope = m.noiseProfile[2 * c];
        const float offset = m.noiseProfile[2 * c + 1];
        variance += std::max(slope * 0.02f + offset, 0.0f);
    }
    return std::sqrt(variance / 3.0f);
}

Status AdaptiveDetailedCrispAppearanceV47j::applyTile(
    const float* a,
    int w,
    int h,
    int cx0,
    int cy0,
    int cw,
    int ch,
    float* out) const {
    if (!a || !out || w <= 0 || h <= 0 || cx0 < 0 || cy0 < 0 ||
        cx0 + cw > w || cy0 + ch > h) {
        return Status::error(
            StatusCode::InvalidArgument,
            "invalid adaptive detailed appearance tile");
    }

    // This body is a compatibility-port of canonical/detail/v4.7j
    // AdaptiveDetailedCrispAppearance::applyTile. It intentionally preserves
    // the exact constants, operation ordering and luminance-only RGB scaling.
    const std::size_t N = std::size_t(w) * h;
    const std::size_t IN = std::size_t(w + 1) * (h + 1);
    const int stride = w + 1;

    thread_local std::vector<float> Y;
    thread_local std::vector<float> grad;
    thread_local std::vector<float> lap;
    thread_local std::vector<float> iY;
    thread_local std::vector<float> iGrad;
    thread_local std::vector<float> iLap;

    Y.resize(N);
    grad.resize(N);
    lap.resize(N);
    iY.assign(IN, 0.0f);
    iGrad.assign(IN, 0.0f);
    iLap.assign(IN, 0.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const std::size_t i = std::size_t(y) * w + x;
            Y[i] = std::max(
                luminance709_local(
                    std::max(a[3 * i], 0.0f),
                    std::max(a[3 * i + 1], 0.0f),
                    std::max(a[3 * i + 2], 0.0f)),
                0.0f);
        }
    }

    auto sy = [&](int x, int y) {
        x = std::max(0, std::min(w - 1, x));
        y = std::max(0, std::min(h - 1, y));
        return Y[std::size_t(y) * w + x];
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float c = sy(x, y);
            const float l = sy(x - 1, y);
            const float r = sy(x + 1, y);
            const float u = sy(x, y - 1);
            const float d = sy(x, y + 1);
            const float gx = 0.5f * (r - l);
            const float gy = 0.5f * (d - u);
            const std::size_t i = std::size_t(y) * w + x;
            grad[i] = std::sqrt(gx * gx + gy * gy);
            lap[i] = 0.25f * std::abs(4.0f * c - l - r - u - d);
        }
    }

    auto buildIntegral =
        [&](const std::vector<float>& src, std::vector<float>& dst) {
            for (int y = 0; y < h; ++y) {
                float row = 0.0f;
                for (int x = 0; x < w; ++x) {
                    row += src[std::size_t(y) * w + x];
                    dst[std::size_t(y + 1) * stride + (x + 1)] =
                        dst[std::size_t(y) * stride + (x + 1)] + row;
                }
            }
        };
    buildIntegral(Y, iY);
    buildIntegral(grad, iGrad);
    buildIntegral(lap, iLap);

    auto meanFrom = [&](const std::vector<float>& in, int x, int y, int radius) {
        return integral_box_mean(in, stride, w, h, x, y, radius);
    };

    const float n2 = noiseSigmaAt2Pct_;
    const float q =
        n2 > 0.0f
            ? 1.0f - smoothstep01(
                (n2 - 0.00125f) / (0.00180f - 0.00125f))
            : 0.65f;
    const float A = 0.25f + 0.45f * q;
    const float B = 0.18f + 0.26f * q;
    const float C = 0.10f + 0.12f * q;

    for (int y = 0; y < ch; ++y) {
        for (int x = 0; x < cw; ++x) {
            const int tx = cx0 + x;
            const int ty = cy0 + y;
            const std::size_t si = std::size_t(ty) * w + tx;
            const std::size_t oi = std::size_t(y) * cw + x;

            const float nr = std::max(a[3 * si], 0.0f);
            const float ng = std::max(a[3 * si + 1], 0.0f);
            const float nb = std::max(a[3 * si + 2], 0.0f);
            const float yy = Y[si];

            const float b1 = meanFrom(iY, tx, ty, 1);
            const float b2 = meanFrom(iY, tx, ty, 2);
            const float b5 = meanFrom(iY, tx, ty, 5);
            const float micro = yy - b1;
            const float fine = b1 - b2;
            const float texture = b2 - b5;

            const float low = smoothstep01((yy - 0.004f) / 0.055f);
            const float high =
                1.0f - smoothstep01((yy - 0.88f) / 0.30f);
            const float shadow = 0.32f + 0.68f * q;
            const float tone =
                high * (shadow + (1.0f - shadow) * low);

            const float sigmaProxy =
                (n2 > 0.0f ? n2 : 0.00145f) *
                std::sqrt(std::max(yy, 0.005f) / 0.02f);
            const float conf = smoothstep01(
                (std::abs(micro) / std::max(sigmaProxy, 1e-6f) - 0.80f) /
                1.60f);

            float microGate = 0.20f + 0.80f * conf;
            float fineGate = 0.42f + 0.58f * conf;
            float textureGate = 0.70f + 0.30f * conf;

            const float activity =
                (std::abs(fine) + 0.50f * std::abs(texture)) /
                std::max(b5, 0.03f);
            const float activityGate =
                0.15f +
                0.85f * smoothstep01((activity - 0.012f) / 0.043f);
            microGate *= activityGate;
            fineGate *= 0.50f + 0.50f * activityGate;
            textureGate *= 0.75f + 0.25f * activityGate;

            const float normalizedGrad =
                grad[si] / std::max(b5, 0.03f);
            const float meanGrad = meanFrom(iGrad, tx, ty, 2);
            const float meanLap = meanFrom(iLap, tx, ty, 2);
            const float oscillation =
                meanLap / std::max(meanGrad, 1e-6f);
            const float hardEdge =
                smoothstep01(
                    (normalizedGrad - 0.03f) / (0.20f - 0.03f)) *
                (1.0f -
                 smoothstep01(
                     (oscillation - 0.30f) / (1.00f - 0.30f)));

            const float edgeGate = 1.0f - hardEdge;
            microGate *= edgeGate;
            fineGate *= 0.40f + 0.60f * edgeGate;
            textureGate *= 0.75f + 0.25f * edgeGate;

            float yd =
                yy +
                (A * micro * microGate +
                 B * fine * fineGate +
                 C * texture * textureGate) *
                    tone;

            float lo = std::numeric_limits<float>::infinity();
            float hi = -std::numeric_limits<float>::infinity();
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const float v = sy(tx + dx, ty + dy);
                    lo = std::min(lo, v);
                    hi = std::max(hi, v);
                }
            }

            const float range = std::max(hi - lo, 1e-5f);
            const float margin =
                ((0.035f + 0.015f * q) *
                 (1.0f - 0.72f * hardEdge)) *
                    range +
                2e-5f;
            yd = clampf(yd, lo - margin, hi + margin);
            yd = std::max(yd, 0.0f);

            const float sc = yy > 1e-8f ? yd / yy : 1.0f;
            out[3 * oi] = nr * sc;
            out[3 * oi + 1] = ng * sc;
            out[3 * oi + 2] = nb * sc;
        }
    }

    return Status::ok();
}

}  // namespace truthraw::adaptive_detail_v47j_adapter

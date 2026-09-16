#include "bounded_srgb_preview_sink_v0_1.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::preview_surface_v0_1 {
namespace {

using streaming_v0_1::StreamStatus;
using streaming_v0_1::StreamStatusCode;

struct IntRect {
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
};

int sample_axis(int p, int previewSize, int displaySize) {
    const double v = (static_cast<double>(p) + 0.5) * static_cast<double>(displaySize) /
                     static_cast<double>(previewSize);
    return std::clamp(static_cast<int>(v), 0, displaySize - 1);
}

void preview_axis_bounds(int d0, int d1, int displaySize, int previewSize, int& p0, int& p1) {
    const double scale = static_cast<double>(previewSize) / static_cast<double>(displaySize);
    p0 = std::max(0, static_cast<int>(std::floor(static_cast<double>(d0) * scale)) - 2);
    p1 = std::min(previewSize, static_cast<int>(std::ceil(static_cast<double>(d1) * scale)) + 2);
}

IntRect display_rect_for_source(const TileRect& r, int width, int height, Orientation o) {
    switch (o) {
        case Orientation::Normal:
            return {r.x0, r.y0, r.x1, r.y1};
        case Orientation::Rotate180:
            return {width - r.x1, height - r.y1, width - r.x0, height - r.y0};
        case Orientation::Rotate90CW:
            return {height - r.y1, r.x0, height - r.y0, r.x1};
        case Orientation::Rotate90CCW:
            return {r.y0, width - r.x1, r.y1, width - r.x0};
    }
    return {};
}

void display_to_source(int dx, int dy, int width, int height, Orientation o, int& sx, int& sy) {
    switch (o) {
        case Orientation::Normal:
            sx = dx; sy = dy; return;
        case Orientation::Rotate180:
            sx = width - 1 - dx; sy = height - 1 - dy; return;
        case Orientation::Rotate90CW:
            sx = dy; sy = height - 1 - dx; return;
        case Orientation::Rotate90CCW:
            sx = width - 1 - dy; sy = dx; return;
    }
    sx = -1; sy = -1;
}

bool valid_orientation(Orientation o) {
    return o == Orientation::Normal || o == Orientation::Rotate180 ||
           o == Orientation::Rotate90CW || o == Orientation::Rotate90CCW;
}

} // namespace

std::uint8_t linear_to_srgb_u8(float linear) {
    if (!std::isfinite(linear)) return 0;
    const float x = std::clamp(linear, 0.0f, 1.0f);
    const float encoded = x <= 0.0031308f
        ? 12.92f * x
        : 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
    const long q = std::lround(std::clamp(encoded, 0.0f, 1.0f) * 255.0f);
    return static_cast<std::uint8_t>(std::clamp<long>(q, 0, 255));
}

BoundedSrgbPreviewSink::BoundedSrgbPreviewSink(int maxEdge) : maxEdge_(maxEdge) {}

std::size_t BoundedSrgbPreviewSink::residentBytesUpperBound() const {
    return argb_.capacity() * sizeof(std::uint32_t) +
           writeOwners_.capacity() * sizeof(std::uint8_t);
}

StreamStatus BoundedSrgbPreviewSink::beginFrame(
    int width,
    int height,
    Orientation orientation,
    const ExposurePlan&,
    bool hdrEnabled,
    bool diagnosticsEnabled) {
    if (begun_ || width <= 0 || height <= 0 || maxEdge_ < 16 || maxEdge_ > 4096 ||
        !valid_orientation(orientation)) {
        return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid preview begin-frame contract");
    }

    sourceWidth_ = width;
    sourceHeight_ = height;
    orientation_ = orientation;
    hdrEnabled_ = hdrEnabled;
    diagnosticsEnabled_ = diagnosticsEnabled;

    const bool rotated = orientation == Orientation::Rotate90CW || orientation == Orientation::Rotate90CCW;
    displayWidth_ = rotated ? height : width;
    displayHeight_ = rotated ? width : height;

    const int longEdge = std::max(displayWidth_, displayHeight_);
    const double scale = std::min(1.0, static_cast<double>(maxEdge_) / static_cast<double>(longEdge));
    previewWidth_ = std::max(1, static_cast<int>(std::lround(static_cast<double>(displayWidth_) * scale)));
    previewHeight_ = std::max(1, static_cast<int>(std::lround(static_cast<double>(displayHeight_) * scale)));

    const std::size_t pixels = static_cast<std::size_t>(previewWidth_) * static_cast<std::size_t>(previewHeight_);
    if (pixels == 0 || pixels > static_cast<std::size_t>(4096) * static_cast<std::size_t>(4096)) {
        return StreamStatus::error(StreamStatusCode::BudgetExceeded, "preview surface exceeds absolute pixel cap");
    }

    argb_.assign(pixels, 0xff000000u);
    writeOwners_.assign(pixels, 0u);
    writtenPixels_ = 0;
    halfGainSamplesObserved_ = 0;
    begun_ = true;
    finished_ = false;
    return StreamStatus::ok();
}

StreamStatus BoundedSrgbPreviewSink::writeSdrTile(
    const TileRect& r,
    const float* rgb,
    std::size_t floatCount) {
    if (!begun_ || finished_ || !rgb || r.x0 < 0 || r.y0 < 0 || r.x1 > sourceWidth_ ||
        r.y1 > sourceHeight_ || r.x0 >= r.x1 || r.y0 >= r.y1) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid SDR tile for preview");
    }
    const int coreW = r.x1 - r.x0;
    const int coreH = r.y1 - r.y0;
    const std::size_t corePixels = static_cast<std::size_t>(coreW) * static_cast<std::size_t>(coreH);
    if (floatCount != 3u * corePixels) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "preview SDR tile count mismatch");
    }

    const IntRect dr = display_rect_for_source(r, sourceWidth_, sourceHeight_, orientation_);
    int px0 = 0, px1 = 0, py0 = 0, py1 = 0;
    preview_axis_bounds(dr.x0, dr.x1, displayWidth_, previewWidth_, px0, px1);
    preview_axis_bounds(dr.y0, dr.y1, displayHeight_, previewHeight_, py0, py1);

    for (int py = py0; py < py1; ++py) {
        const int dy = sample_axis(py, previewHeight_, displayHeight_);
        if (dy < dr.y0 || dy >= dr.y1) continue;
        for (int px = px0; px < px1; ++px) {
            const int dx = sample_axis(px, previewWidth_, displayWidth_);
            if (dx < dr.x0 || dx >= dr.x1) continue;

            int sx = -1, sy = -1;
            display_to_source(dx, dy, sourceWidth_, sourceHeight_, orientation_, sx, sy);
            if (sx < r.x0 || sx >= r.x1 || sy < r.y0 || sy >= r.y1) continue;

            const std::size_t outIndex = static_cast<std::size_t>(py) * static_cast<std::size_t>(previewWidth_) +
                                         static_cast<std::size_t>(px);
            if (writeOwners_[outIndex] != 0u) {
                return StreamStatus::error(StreamStatusCode::SinkFailed, "preview pixel received overlapping tile ownership");
            }

            const std::size_t local = static_cast<std::size_t>(sy - r.y0) * static_cast<std::size_t>(coreW) +
                                      static_cast<std::size_t>(sx - r.x0);
            const std::uint32_t rr = linear_to_srgb_u8(rgb[3u * local]);
            const std::uint32_t gg = linear_to_srgb_u8(rgb[3u * local + 1u]);
            const std::uint32_t bb = linear_to_srgb_u8(rgb[3u * local + 2u]);
            argb_[outIndex] = 0xff000000u | (rr << 16u) | (gg << 8u) | bb;
            writeOwners_[outIndex] = 1u;
            ++writtenPixels_;
        }
    }

    return StreamStatus::ok();
}

StreamStatus BoundedSrgbPreviewSink::writeHalfLogGainBlock(
    const streaming_v0_1::HalfStateRect& rect,
    const float* halfLogGain,
    std::size_t count) {
    const int w = rect.x1 - rect.x0;
    const int h = rect.y1 - rect.y0;
    if (!begun_ || finished_ || w <= 0 || h <= 0 || !halfLogGain ||
        count != static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid half-gain block for preview");
    }
    halfGainSamplesObserved_ += count;
    return StreamStatus::ok();
}

StreamStatus BoundedSrgbPreviewSink::writeStage2DiagnosticTile(
    const TileRect& coreRect,
    const float* stage2,
    std::size_t count) {
    const int w = coreRect.x1 - coreRect.x0;
    const int h = coreRect.y1 - coreRect.y0;
    if (!begun_ || finished_ || !diagnosticsEnabled_ || w <= 0 || h <= 0 || !stage2 ||
        count != static_cast<std::size_t>(w) * static_cast<std::size_t>(h)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid diagnostic tile for preview");
    }
    return StreamStatus::ok();
}

StreamStatus BoundedSrgbPreviewSink::finishFrame() {
    if (!begun_ || finished_ || argb_.empty() || writtenPixels_ != argb_.size()) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "preview surface incomplete");
    }
    for (const auto owner : writeOwners_) {
        if (owner != 1u) {
            return StreamStatus::error(StreamStatusCode::SinkFailed, "preview pixel ownership incomplete");
        }
    }
    finished_ = true;
    return StreamStatus::ok();
}

} // namespace truthraw::preview_surface_v0_1

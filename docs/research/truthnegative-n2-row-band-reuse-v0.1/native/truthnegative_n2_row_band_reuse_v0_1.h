#pragma once

#include "full_frame_streaming_v0_1.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace truthraw::truthnegative_n2_row_band_reuse::v0_1 {

/**
 * Read-only row-band cache in front of an existing IRawTileSource.
 *
 * The scientific caller still requests the exact same tile/halo rectangles.
 * This adapter only changes transport: requests with the same vertical halo
 * band can be served from one bounded full-width raw/gain read. Sample values,
 * tile boundaries, sample order and downstream hashes are unchanged.
 *
 * The cache is process-local runtime state only. It is never evidence, never
 * persisted and never authorizes correction or Scientific Master writeback.
 */
class RowBandReuseTileSource final
    : public streaming_v0_1::IRawTileSource {
public:
    explicit RowBandReuseTileSource(
        streaming_v0_1::IRawTileSource& source,
        std::size_t maxCacheBytes = 8u * 1024u * 1024u) noexcept
        : source_(source),
          maxCacheBytes_(maxCacheBytes) {}

    const DngMetadata& metadata() const override {
        return source_.metadata();
    }

    std::size_t residentBytesUpperBound() const override {
        const auto sourceBytes = source_.residentBytesUpperBound();
        if (sourceBytes >
            std::numeric_limits<std::size_t>::max() - maxCacheBytes_) {
            return std::numeric_limits<std::size_t>::max();
        }
        return sourceBytes + maxCacheBytes_;
    }

    streaming_v0_1::StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        const auto& md = source_.metadata();
        if (!rawOut ||
            rect.hx0 < 0 ||
            rect.hy0 < 0 ||
            rect.hx1 > md.width ||
            rect.hy1 > md.height ||
            rect.hx1 <= rect.hx0 ||
            rect.hy1 <= rect.hy0) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument,
                "invalid row-band tile rectangle");
        }

        const auto requestWidth =
            static_cast<std::size_t>(rect.hx1 - rect.hx0);
        const auto requestHeight =
            static_cast<std::size_t>(rect.hy1 - rect.hy0);
        if (requestHeight != 0u &&
            requestWidth >
                std::numeric_limits<std::size_t>::max() / requestHeight) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument,
                "row-band request size overflow");
        }
        const auto requestCount = requestWidth * requestHeight;
        if (rawCount < requestCount) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument,
                "row-band raw output too small");
        }
        if (md.hasGainField) {
            if (!gainOut || gainCount < requestCount) {
                return streaming_v0_1::StreamStatus::error(
                    streaming_v0_1::StreamStatusCode::InvalidArgument,
                    "row-band gain output too small");
            }
        } else if (gainOut != nullptr && gainCount < requestCount) {
            return streaming_v0_1::StreamStatus::error(
                streaming_v0_1::StreamStatusCode::InvalidArgument,
                "row-band gain output count inconsistent");
        }

        const auto bandWidth =
            static_cast<std::size_t>(md.width);
        const auto bandHeight = requestHeight;
        if (bandHeight != 0u &&
            bandWidth >
                std::numeric_limits<std::size_t>::max() / bandHeight) {
            return fallback(
                rect,
                rawOut,
                rawCount,
                gainOut,
                gainCount);
        }
        const auto bandCount = bandWidth * bandHeight;
        const auto bytesPerSample =
            sizeof(std::uint16_t) +
            (md.hasGainField ? sizeof(float) : 0u);
        if (bytesPerSample != 0u &&
            bandCount >
                std::numeric_limits<std::size_t>::max() /
                    bytesPerSample) {
            return fallback(
                rect,
                rawOut,
                rawCount,
                gainOut,
                gainCount);
        }
        const auto bandBytes = bandCount * bytesPerSample;
        if (bandBytes == 0u || bandBytes > maxCacheBytes_) {
            return fallback(
                rect,
                rawOut,
                rawCount,
                gainOut,
                gainCount);
        }

        if (!cacheValid_ ||
            cachedHy0_ != rect.hy0 ||
            cachedHy1_ != rect.hy1) {
            rawBand_.resize(bandCount);
            if (md.hasGainField) {
                gainBand_.resize(bandCount);
            } else {
                gainBand_.clear();
            }

            TileRect band{};
            band.x0 = 0;
            band.y0 = rect.y0;
            band.x1 = md.width;
            band.y1 = rect.y1;
            band.hx0 = 0;
            band.hy0 = rect.hy0;
            band.hx1 = md.width;
            band.hy1 = rect.hy1;

            const auto status =
                source_.readRawTile(
                    band,
                    rawBand_.data(),
                    rawBand_.size(),
                    md.hasGainField ? gainBand_.data() : nullptr,
                    md.hasGainField ? gainBand_.size() : 0u);
            if (!status) {
                invalidate();
                return status;
            }
            cacheValid_ = true;
            cachedHy0_ = rect.hy0;
            cachedHy1_ = rect.hy1;
            ++bandFillCount_;
            peakCacheBytes_ =
                std::max(peakCacheBytes_, bandBytes);
        } else {
            ++bandCacheHitRequestCount_;
        }

        for (std::size_t yy = 0u; yy < requestHeight; ++yy) {
            const auto sourceOffset =
                yy * bandWidth +
                static_cast<std::size_t>(rect.hx0);
            const auto destOffset = yy * requestWidth;
            std::copy_n(
                rawBand_.data() + sourceOffset,
                requestWidth,
                rawOut + destOffset);
            if (md.hasGainField) {
                std::copy_n(
                    gainBand_.data() + sourceOffset,
                    requestWidth,
                    gainOut + destOffset);
            }
        }
        ++bandServedRequestCount_;
        return streaming_v0_1::StreamStatus::ok();
    }

    streaming_v0_1::StreamStatus readRowBias(
        int y0,
        int y1,
        float* out,
        std::size_t count) override {
        return source_.readRowBias(y0, y1, out, count);
    }

    streaming_v0_1::StreamStatus readColBias(
        int x0,
        int x1,
        float* out,
        std::size_t count) override {
        return source_.readColBias(x0, x1, out, count);
    }

    std::uint64_t bandFillCount() const noexcept {
        return bandFillCount_;
    }

    std::uint64_t bandServedRequestCount() const noexcept {
        return bandServedRequestCount_;
    }

    std::uint64_t bandCacheHitRequestCount() const noexcept {
        return bandCacheHitRequestCount_;
    }

    std::uint64_t fallbackRequestCount() const noexcept {
        return fallbackRequestCount_;
    }

    std::size_t peakCacheBytes() const noexcept {
        return peakCacheBytes_;
    }

    bool scientificValuesModified() const noexcept {
        return false;
    }

    bool createsNewEvidence() const noexcept {
        return false;
    }

    bool scientificWritebackAllowed() const noexcept {
        return false;
    }

private:
    streaming_v0_1::StreamStatus fallback(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) {
        invalidate();
        ++fallbackRequestCount_;
        return source_.readRawTile(
            rect,
            rawOut,
            rawCount,
            gainOut,
            gainCount);
    }

    void invalidate() noexcept {
        cacheValid_ = false;
        cachedHy0_ = -1;
        cachedHy1_ = -1;
    }

    streaming_v0_1::IRawTileSource& source_;
    std::size_t maxCacheBytes_ = 0u;
    std::vector<std::uint16_t> rawBand_{};
    std::vector<float> gainBand_{};
    bool cacheValid_ = false;
    int cachedHy0_ = -1;
    int cachedHy1_ = -1;
    std::uint64_t bandFillCount_ = 0u;
    std::uint64_t bandServedRequestCount_ = 0u;
    std::uint64_t bandCacheHitRequestCount_ = 0u;
    std::uint64_t fallbackRequestCount_ = 0u;
    std::size_t peakCacheBytes_ = 0u;
};

} // namespace truthraw::truthnegative_n2_row_band_reuse::v0_1

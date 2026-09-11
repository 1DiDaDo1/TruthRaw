#include "bounded_record_file_sink_v0_1.h"

#include <bit>
#include <cerrno>
#include <cstdint>
#include <limits>
#include <unistd.h>

namespace truthraw::room_abi::v0_2 {
namespace {

using streaming_v0_1::StreamStatus;
using streaming_v0_1::StreamStatusCode;

constexpr std::uint8_t kMagic[8] = {'T','R','S','I','N','K','0','1'};

void put_u32_le(std::uint8_t* dst, std::uint32_t v) noexcept {
    dst[0] = static_cast<std::uint8_t>(v);
    dst[1] = static_cast<std::uint8_t>(v >> 8U);
    dst[2] = static_cast<std::uint8_t>(v >> 16U);
    dst[3] = static_cast<std::uint8_t>(v >> 24U);
}

void put_u64_le(std::uint8_t* dst, std::uint64_t v) noexcept {
    for (int i = 0; i < 8; ++i) dst[i] = static_cast<std::uint8_t>(v >> (8U * i));
}

void put_f32_le(std::uint8_t* dst, float v) noexcept {
    put_u32_le(dst, std::bit_cast<std::uint32_t>(v));
}

bool checked_float_payload_bytes(std::size_t count, std::size_t& out) noexcept {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(float)) return false;
    out = count * sizeof(float);
    return true;
}

} // namespace

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeBytes(const void* data, std::size_t count) {
    if (fd_ < 0 || (!data && count != 0U)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid sink fd/data");
    }
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::size_t done = 0U;
    while (done < count) {
        const ssize_t n = ::write(fd_, p + done, count - done);
        if (n < 0) {
            if (errno == EINTR) continue;
            return StreamStatus::error(StreamStatusCode::SinkFailed, "sink write failed");
        }
        if (n == 0) return StreamStatus::error(StreamStatusCode::SinkFailed, "sink short write");
        done += static_cast<std::size_t>(n);
        bytesWritten_ += static_cast<std::uint64_t>(n);
    }
    return StreamStatus::ok();
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeRecordHeader(
    RecordType type,
    std::int32_t x0,
    std::int32_t y0,
    std::int32_t x1,
    std::int32_t y1,
    std::uint64_t floatCount) {
    std::uint8_t h[32]{};
    put_u32_le(h + 0, static_cast<std::uint32_t>(type));
    put_u32_le(h + 4, static_cast<std::uint32_t>(x0));
    put_u32_le(h + 8, static_cast<std::uint32_t>(y0));
    put_u32_le(h + 12, static_cast<std::uint32_t>(x1));
    put_u32_le(h + 16, static_cast<std::uint32_t>(y1));
    put_u32_le(h + 20, 0U);
    put_u64_le(h + 24, floatCount);
    return writeBytes(h, sizeof(h));
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeFloatPayload(
    const float* values,
    std::size_t count) {
    if (!values && count != 0U) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "null float payload");
    }
    std::uint8_t word[4]{};
    for (std::size_t i = 0; i < count; ++i) {
        put_f32_le(word, values[i]);
        auto s = writeBytes(word, sizeof(word));
        if (!s) return s;
    }
    return StreamStatus::ok();
}

bool BoundedRecordFileSink::validTile(const TileRect& r) const noexcept {
    return r.x0 >= 0 && r.y0 >= 0 && r.x1 > r.x0 && r.y1 > r.y0 &&
           r.x1 <= width_ && r.y1 <= height_;
}

bool BoundedRecordFileSink::validHalfRect(const streaming_v0_1::HalfStateRect& r) const noexcept {
    return r.x0 >= 0 && r.y0 >= 0 && r.x1 > r.x0 && r.y1 > r.y0 &&
           r.x1 <= halfWidth_ && r.y1 <= halfHeight_;
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::beginFrame(
    int width,
    int height,
    Orientation orientation,
    const ExposurePlan& exposure,
    bool hdrEnabled,
    bool diagnosticsEnabled) {
    if (fd_ < 0 || begun_ || finished_ || width <= 0 || height <= 0) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid beginFrame state");
    }

    width_ = width;
    height_ = height;
    halfWidth_ = (width + 1) / 2;
    halfHeight_ = (height + 1) / 2;
    diagnosticsEnabled_ = diagnosticsEnabled;

    std::uint8_t h[96]{};
    for (std::size_t i = 0; i < sizeof(kMagic); ++i) h[i] = kMagic[i];
    put_u32_le(h + 8, kFormatVersion);
    put_u32_le(h + 12, static_cast<std::uint32_t>(width));
    put_u32_le(h + 16, static_cast<std::uint32_t>(height));
    put_u32_le(h + 20, static_cast<std::uint32_t>(orientation));
    std::uint32_t flags = 0U;
    if (hdrEnabled) flags |= 1U;
    if (diagnosticsEnabled) flags |= 2U;
    put_u32_le(h + 24, flags);
    put_f32_le(h + 28, exposure.sceneToDisplayScalar);
    put_f32_le(h + 32, exposure.noiseSigmaAt2Pct);
    put_f32_le(h + 36, exposure.clipFraction);
    for (int i = 0; i < 5; ++i) put_f32_le(h + 40 + 4 * i, exposure.anchorsX[i]);
    for (int i = 0; i < 5; ++i) put_f32_le(h + 60 + 4 * i, exposure.anchorsY[i]);
    put_u64_le(h + 80, exposure.stage2Over1Count);
    put_u64_le(h + 88, 0U);

    auto s = writeBytes(h, sizeof(h));
    if (!s) return s;
    begun_ = true;
    return StreamStatus::ok();
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeSdrTile(
    const TileRect& r,
    const float* rgb,
    std::size_t count) {
    if (!begun_ || finished_ || !validTile(r)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid SDR tile state/rect");
    }
    const std::size_t pixels = static_cast<std::size_t>(r.x1 - r.x0) *
                               static_cast<std::size_t>(r.y1 - r.y0);
    if (pixels > std::numeric_limits<std::size_t>::max() / 3U || count != 3U * pixels) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid SDR tile count");
    }
    std::size_t payloadBytes = 0U;
    if (!checked_float_payload_bytes(count, payloadBytes)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "SDR payload overflow");
    }
    (void)payloadBytes;
    auto s = writeRecordHeader(RecordType::SdrTile, r.x0, r.y0, r.x1, r.y1, count);
    if (!s) return s;
    s = writeFloatPayload(rgb, count);
    if (!s) return s;
    ++sdrRecordCount_;
    return StreamStatus::ok();
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeHalfLogGainBlock(
    const streaming_v0_1::HalfStateRect& r,
    const float* gain,
    std::size_t count) {
    if (!begun_ || finished_ || !validHalfRect(r)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid half-gain state/rect");
    }
    const std::size_t expected = static_cast<std::size_t>(r.x1 - r.x0) *
                                 static_cast<std::size_t>(r.y1 - r.y0);
    if (count != expected) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid half-gain count");
    }
    auto s = writeRecordHeader(RecordType::HalfLogGainBlock, r.x0, r.y0, r.x1, r.y1, count);
    if (!s) return s;
    s = writeFloatPayload(gain, count);
    if (!s) return s;
    ++gainRecordCount_;
    return StreamStatus::ok();
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::writeStage2DiagnosticTile(
    const TileRect& r,
    const float* stage2,
    std::size_t count) {
    if (!diagnosticsEnabled_ || !begun_ || finished_ || !validTile(r)) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid diagnostic state/rect");
    }
    const std::size_t expected = static_cast<std::size_t>(r.x1 - r.x0) *
                                 static_cast<std::size_t>(r.y1 - r.y0);
    if (count != expected) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid diagnostic count");
    }
    auto s = writeRecordHeader(RecordType::Stage2DiagnosticTile, r.x0, r.y0, r.x1, r.y1, count);
    if (!s) return s;
    s = writeFloatPayload(stage2, count);
    if (!s) return s;
    ++diagnosticRecordCount_;
    return StreamStatus::ok();
}

streaming_v0_1::StreamStatus BoundedRecordFileSink::finishFrame() {
    if (!begun_ || finished_ || sdrRecordCount_ == 0U || gainRecordCount_ == 0U) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "invalid finishFrame state");
    }
    if (diagnosticsEnabled_ && diagnosticRecordCount_ == 0U) {
        return StreamStatus::error(StreamStatusCode::SinkFailed, "diagnostics enabled but absent");
    }
    auto s = writeRecordHeader(RecordType::EndFrame, 0, 0, 0, 0, 0U);
    if (!s) return s;
    finished_ = true;
    return StreamStatus::ok();
}

} // namespace truthraw::room_abi::v0_2

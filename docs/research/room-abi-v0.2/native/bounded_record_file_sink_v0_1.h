#pragma once

#include "full_frame_streaming_v0_1.h"

#include <cstddef>
#include <cstdint>

namespace truthraw::room_abi::v0_2 {

// File-backed streaming sink for the Room ABI v0.2 integration candidate.
//
// The file descriptor is borrowed and remains caller-owned. The sink writes a
// simple append-only research transport containing a frame header followed by
// typed tile/block records. It never materializes a full SDR, half-gain, or
// diagnostic frame in memory. This is an integration/output transport, not a
// DNG/JPEG/HEIF encoder and not a scientific-master container.
class BoundedRecordFileSink final : public streaming_v0_1::IStreamingSink {
public:
    static constexpr std::size_t kResidentAllowanceBytes = 64U * 1024U;
    static constexpr std::uint32_t kFormatVersion = 1U;

    explicit BoundedRecordFileSink(int fd) noexcept : fd_(fd) {}

    std::size_t residentBytesUpperBound() const override {
        return kResidentAllowanceBytes;
    }

    streaming_v0_1::StreamStatus beginFrame(
        int width,
        int height,
        Orientation orientation,
        const ExposurePlan& exposure,
        bool hdrEnabled,
        bool diagnosticsEnabled) override;

    streaming_v0_1::StreamStatus writeSdrTile(
        const TileRect& coreRect,
        const float* rgb,
        std::size_t floatCount) override;

    streaming_v0_1::StreamStatus writeHalfLogGainBlock(
        const streaming_v0_1::HalfStateRect& rect,
        const float* halfLogGain,
        std::size_t count) override;

    streaming_v0_1::StreamStatus writeStage2DiagnosticTile(
        const TileRect& coreRect,
        const float* stage2,
        std::size_t count) override;

    streaming_v0_1::StreamStatus finishFrame() override;

    std::uint64_t bytesWritten() const noexcept { return bytesWritten_; }
    std::uint64_t sdrRecordCount() const noexcept { return sdrRecordCount_; }
    std::uint64_t gainRecordCount() const noexcept { return gainRecordCount_; }
    std::uint64_t diagnosticRecordCount() const noexcept { return diagnosticRecordCount_; }
    bool begun() const noexcept { return begun_; }
    bool finished() const noexcept { return finished_; }

private:
    enum class RecordType : std::uint32_t {
        FrameHeader = 1U,
        SdrTile = 2U,
        HalfLogGainBlock = 3U,
        Stage2DiagnosticTile = 4U,
        EndFrame = 5U,
    };

    streaming_v0_1::StreamStatus writeRecordHeader(
        RecordType type,
        std::int32_t x0,
        std::int32_t y0,
        std::int32_t x1,
        std::int32_t y1,
        std::uint64_t floatCount);
    streaming_v0_1::StreamStatus writeFloatPayload(const float* values, std::size_t count);
    streaming_v0_1::StreamStatus writeBytes(const void* data, std::size_t count);

    bool validTile(const TileRect& rect) const noexcept;
    bool validHalfRect(const streaming_v0_1::HalfStateRect& rect) const noexcept;

    int fd_ = -1;
    int width_ = 0;
    int height_ = 0;
    int halfWidth_ = 0;
    int halfHeight_ = 0;
    bool diagnosticsEnabled_ = false;
    bool begun_ = false;
    bool finished_ = false;
    std::uint64_t bytesWritten_ = 0U;
    std::uint64_t sdrRecordCount_ = 0U;
    std::uint64_t gainRecordCount_ = 0U;
    std::uint64_t diagnosticRecordCount_ = 0U;
};

} // namespace truthraw::room_abi::v0_2

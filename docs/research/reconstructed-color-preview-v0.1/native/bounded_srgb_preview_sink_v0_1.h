#pragma once

#include "full_frame_streaming_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace truthraw::preview_surface_v0_1 {

std::uint8_t linear_to_srgb_u8(float linear);

class BoundedSrgbPreviewSink final : public streaming_v0_1::IStreamingSink {
public:
    explicit BoundedSrgbPreviewSink(int maxEdge);

    std::size_t residentBytesUpperBound() const override;

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

    int width() const { return previewWidth_; }
    int height() const { return previewHeight_; }
    Orientation orientation() const { return orientation_; }
    bool finished() const { return finished_; }
    std::size_t writtenPixelCount() const { return writtenPixels_; }
    std::size_t halfGainSamplesObserved() const { return halfGainSamplesObserved_; }
    const std::vector<std::uint32_t>& argb8888() const { return argb_; }

private:
    int maxEdge_ = 0;
    int sourceWidth_ = 0;
    int sourceHeight_ = 0;
    int displayWidth_ = 0;
    int displayHeight_ = 0;
    int previewWidth_ = 0;
    int previewHeight_ = 0;
    Orientation orientation_ = Orientation::Normal;
    bool begun_ = false;
    bool finished_ = false;
    bool hdrEnabled_ = false;
    bool diagnosticsEnabled_ = false;
    std::size_t writtenPixels_ = 0;
    std::size_t halfGainSamplesObserved_ = 0;
    std::vector<std::uint32_t> argb_;
    std::vector<std::uint8_t> writeOwners_;
};

} // namespace truthraw::preview_surface_v0_1

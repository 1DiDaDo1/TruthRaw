#pragma once

#include "scientific_master_linear_dng_projection_v0_1.h"
#include "full_frame_streaming_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace truthraw::scientific_master_linear_dng_projection::v0_1 {

// Replays the exact Stage-2 + camera-native reconstruction semantics used by
// Scientific Master Streaming Binding on the canonical 64x64 grid. It is a
// bounded read adapter only; it does not own, alter, or reclassify the evidence
// source and it does not create a second Scientific Master authority.
class StreamingScientificMasterTileSource final : public IScientificMasterTileSource {
public:
    StreamingScientificMasterTileSource(
        streaming_v0_1::IRawTileSource& source,
        IReconstructionBackend& reconstruction) noexcept;
    ~StreamingScientificMasterTileSource() override;

    StreamingScientificMasterTileSource(const StreamingScientificMasterTileSource&) = delete;
    StreamingScientificMasterTileSource& operator=(const StreamingScientificMasterTileSource&) = delete;

    std::size_t residentBytesUpperBound() const noexcept override;

    Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override;

private:
    struct Impl;
    streaming_v0_1::IRawTileSource& source_;
    IReconstructionBackend& reconstruction_;
    std::unique_ptr<Impl> impl_;
};

}  // namespace truthraw::scientific_master_linear_dng_projection::v0_1

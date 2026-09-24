#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "truthraw/core.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace truthraw::unified_output_preview_sources::v0_1 {

namespace float_dng =
    truthraw::scientific_master_linear_dng_projection::v0_1;

class RandomAccessScientificMasterSource final
    : public float_dng::IScientificMasterTileSource {
public:
    RandomAccessScientificMasterSource(
        streaming_v0_1::IRawTileSource& source,
        IReconstructionBackend& reconstruction) noexcept;
    ~RandomAccessScientificMasterSource() override;

    std::size_t residentBytesUpperBound() const noexcept override;

    float_dng::Status readCameraNativeTile(
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

class BoundedU16PrimarySource final
    : public float_dng::IScientificMasterTileSource {
public:
    explicit BoundedU16PrimarySource(
        float_dng::IScientificMasterTileSource& source) noexcept
        : source_(source) {}

    std::size_t residentBytesUpperBound() const noexcept override {
        return source_.residentBytesUpperBound();
    }

    float_dng::Status readCameraNativeTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* rgb,
        std::size_t floatCount) noexcept override;

private:
    float_dng::IScientificMasterTileSource& source_;
};

const char* schema_name() noexcept;

} // namespace truthraw::unified_output_preview_sources::v0_1

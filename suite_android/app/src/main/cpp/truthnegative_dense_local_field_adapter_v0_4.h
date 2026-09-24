#pragma once

#include "full_frame_streaming_v0_1.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "truthnegative_dense_projection_v0_3.h"
#include "truthnegative_local_authority_projection_v0_4.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace truthraw::truthnegative_dense_local_field_adapter::v0_4 {

namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
namespace dense = truthraw::truthnegative_dense_projection::v0_3;
namespace float_dng = truthraw::scientific_master_linear_dng_projection::v0_1;

class SourceFieldAdapter final : public local::IFieldTileSource {
public:
    SourceFieldAdapter(
        truthraw::streaming_v0_1::IRawTileSource& rawSource,
        float_dng::IScientificMasterTileSource& masterSource) noexcept;

    local::Geometry geometry() const noexcept override;

    bool readSourceTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        field::ChannelRecord* out,
        std::size_t recordCount) noexcept override;

private:
    truthraw::streaming_v0_1::IRawTileSource& raw_;
    float_dng::IScientificMasterTileSource& master_;
    local::Geometry geometry_{};
    std::vector<std::uint16_t> rawScratch_;
    std::vector<float> gainScratch_;
    std::vector<float> rgbScratch_;
    std::vector<field::ChannelRecord> fieldScratch_;
};

class ProjectedRgbAdapter final : public local::IProjectedRgbTileSource {
public:
    explicit ProjectedRgbAdapter(dense::DenseProjectionTileSource& denseSource) noexcept
        : dense_(denseSource) {}

    bool readTargetRgbTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        float* out,
        std::size_t floatCount) noexcept override;

private:
    dense::DenseProjectionTileSource& dense_;
};

bool compute_dense_local_authority_summary(
    truthraw::streaming_v0_1::IRawTileSource& rawSource,
    float_dng::IScientificMasterTileSource& masterSource,
    dense::DenseProjectionTileSource& denseSource,
    std::uint32_t tileEdge,
    local::ProjectionSummary& out) noexcept;

const char* schema_name() noexcept;

} // namespace truthraw::truthnegative_dense_local_field_adapter::v0_4

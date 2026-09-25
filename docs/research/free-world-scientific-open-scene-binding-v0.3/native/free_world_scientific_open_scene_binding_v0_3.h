#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "open_scene_field_v0_85.h"
#include "scientific_master_linear_dng_projection_v0_1.h"
#include "truthnegative_local_authority_projection_v0_4.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::free_world_scientific_open_scene_binding::v0_3 {

namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
namespace master = truthraw::scientific_master_linear_dng_projection::v0_1;

inline constexpr const char* kBindingId =
    "SCIENTIFIC_MASTER_OPEN_SCENE_FIELD_BINDING_V0_3";

struct BindingReport final {
    std::uint64_t tileLoads = 0u;
    std::uint64_t channelRecordsValidated = 0u;
    std::uint64_t masterFieldValueBitMatches = 0u;
    std::uint64_t masterFieldValueBitMismatches = 0u;
    bool fieldSchemaValidated = false;
    bool valueIdentityVerifiedForLoadedRecords = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

class BoundScenePlane final : public free_world::IScenePlaneSource {
public:
    BoundScenePlane(
        master::IScientificMasterTileSource& scientificMaster,
        local::IFieldTileSource& openSceneField,
        std::uint32_t width,
        std::uint32_t height) noexcept;

    std::uint32_t width() const noexcept override;
    std::uint32_t height() const noexcept override;

    bool readPixel(
        std::uint32_t x,
        std::uint32_t y,
        free_world::SourcePixel& out) const noexcept override;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    BindingReport report() const noexcept;

private:
    bool loadCanonicalTile(
        std::uint32_t tileX,
        std::uint32_t tileY) const noexcept;

    master::IScientificMasterTileSource& master_;
    local::IFieldTileSource& field_;
    std::uint32_t width_ = 0u;
    std::uint32_t height_ = 0u;
    bool valid_ = false;
    mutable std::string error_;

    mutable bool cacheValid_ = false;
    mutable std::uint32_t cacheX_ = 0u;
    mutable std::uint32_t cacheY_ = 0u;
    mutable std::uint32_t cacheWidth_ = 0u;
    mutable std::uint32_t cacheHeight_ = 0u;
    mutable std::vector<float> masterRgb_;
    mutable std::vector<field::ChannelRecord> fieldRecords_;
    mutable std::vector<free_world::SourcePixel> mappedPixels_;
    mutable BindingReport report_{};
};

const char* schema_name() noexcept;

}  // namespace truthraw::free_world_scientific_open_scene_binding::v0_3

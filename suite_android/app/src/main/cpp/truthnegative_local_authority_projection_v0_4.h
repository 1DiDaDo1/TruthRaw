#pragma once

#include "open_scene_field_v0_85.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace truthraw::truthnegative_local_authority_projection::v0_4 {

namespace field = truthraw::open_scene_field::v0_85;

inline constexpr std::uint32_t kScale = 4u;

struct Geometry final {
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
};

struct ProjectionSummary final {
    field::Digest contentSha256{};
    std::array<std::uint64_t,4> authorityCounts{};
    std::array<std::uint64_t,16> contributionMaskCounts{};
    std::uint64_t recordCount = 0u;
    std::uint64_t sourceCensoredSupportRecords = 0u;
    std::uint64_t sourceUnknownSupportRecords = 0u;
    std::uint64_t sceneLinearBoundRecords = 0u;
    bool perTargetChannelFieldAvailable = false;
    bool targetMeasuredClaimsCreated = false;
    bool uncertaintyPromotedByResampling = false;
    bool createsNewEvidence = false;
};

class IFieldTileSource {
public:
    virtual ~IFieldTileSource() = default;
    virtual Geometry geometry() const noexcept = 0;
    virtual bool readSourceTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        field::ChannelRecord* out,
        std::size_t recordCount) noexcept = 0;
};

bool project_target_tile(
    IFieldTileSource& source,
    std::uint32_t targetX,
    std::uint32_t targetY,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight,
    std::span<const float> targetRgb,
    std::vector<field::ChannelRecord>& out) noexcept;

bool summarize_full_projection(
    IFieldTileSource& source,
    const field::Digest& projectedRasterSha256,
    std::uint32_t tileEdge,
    ProjectionSummary& out) noexcept;

const char* schema_name() noexcept;
const char* policy_name() noexcept;

} // namespace truthraw::truthnegative_local_authority_projection::v0_4

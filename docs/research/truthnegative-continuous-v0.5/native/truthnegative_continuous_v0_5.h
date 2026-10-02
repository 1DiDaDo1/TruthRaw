#pragma once

#include "free_world_pixel_resolve_2d_v0_2.h"
#include "truthnegative_local_authority_projection_v0_4.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace truthraw::truthnegative_continuous::v0_5 {

namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeContinuousScientificNegative/0.5";
inline constexpr const char* kAuthorityFieldDigestMethod =
    "TRUTHNEGATIVE_CANONICAL_LOCAL_AUTHORITY_FIELD_SHA256_V0_5";
inline constexpr const char* kStateMethod =
    "TRUTHNEGATIVE_CONTINUOUS_CAMERA_PLANE_STATE_V0_5";

inline constexpr std::size_t kAuthorityDirectHashBatchRecordCount = 96u;
inline constexpr std::size_t kAuthorityDirectHashBatchBytes =
    field::kCanonicalAuthorityRecordBytes *
    kAuthorityDirectHashBatchRecordCount;

struct AuthorityFieldSummary final {
    Digest contentSha256{};
    std::array<std::uint64_t, 5u> creationRoleCounts{};
    std::array<std::uint64_t, 4u> authorityCounts{};
    std::uint64_t recordCount = 0u;
    std::uint64_t p95KnownCount = 0u;
    std::uint64_t supportKnownCount = 0u;
    std::uint64_t boundKnownCount = 0u;
    std::uint64_t tileCount = 0u;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

class AuthorityFieldAccumulator final {
public:
    AuthorityFieldAccumulator(
        std::uint32_t sourceWidth,
        std::uint32_t sourceHeight) noexcept;

    bool valid() const noexcept;

    bool appendRecords(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        std::span<const field::ChannelRecord> records) noexcept;

    bool appendSourceTile(
        CfaPattern cfa,
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        std::span<const std::uint16_t> raw,
        float whiteLevel,
        std::span<const float> cameraNativeRgb) noexcept;

    std::size_t residentBytesUpperBound() const noexcept;
    std::uint64_t directByteRecordCount() const noexcept;
    std::uint64_t genericFallbackRecordCount() const noexcept;
    std::uint64_t shaDirectInputBlockTransformCount() const noexcept;
    std::uint64_t shaBufferedInputBlockTransformCount() const noexcept;

    bool finalize(AuthorityFieldSummary& out) noexcept;

private:
    bool beginTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t recordCount) noexcept;

    bool appendRecord(
        const field::ChannelRecord& record) noexcept;

    bool accountCanonicalSourceRecord(
        const field::CanonicalSourceChannelRecord& record) noexcept;

    bool finishTile(
        std::uint32_t x,
        std::uint32_t y) noexcept;

    std::uint32_t sourceWidth_ = 0u;
    std::uint32_t sourceHeight_ = 0u;
    std::uint32_t expectedTileX_ = 0u;
    std::uint32_t expectedTileY_ = 0u;
    truthraw::sha256_v0_69::Hasher hasher_{};
    AuthorityFieldSummary partial_{};
    std::uint64_t directByteRecordCount_ = 0u;
    std::uint64_t genericFallbackRecordCount_ = 0u;
    bool valid_ = false;
    bool finalized_ = false;
};

struct StateInput final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::string reconstructionBackendId;
    std::string colourBindingId;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct State final {
    StateInput input{};
    Digest stateSha256{};
    bool finalized = false;
    bool isRasterIndependent = true;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kStateMethod;
};

struct QueryResult final {
    free_world::ResolvedPixel pixel{};
    Digest stateSha256{};
    Digest querySha256{};
    std::uint32_t targetWidth = 0u;
    std::uint32_t targetHeight = 0u;
    std::uint32_t targetX = 0u;
    std::uint32_t targetY = 0u;
    bool stateIdentityChangedByTargetRaster = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool summarizeAuthorityField(
    local::IFieldTileSource& source,
    AuthorityFieldSummary& out) noexcept;

bool finalizeState(
    const StateInput& input,
    State& out) noexcept;

bool resolvePixel(
    const free_world::IScenePlaneSource& scene,
    const State& state,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight,
    std::uint32_t targetX,
    std::uint32_t targetY,
    QueryResult& out) noexcept;

class RasterResolver final {
public:
    RasterResolver(
        const free_world::IScenePlaneSource& scene,
        const State& state,
        std::uint32_t targetWidth,
        std::uint32_t targetHeight) noexcept;

    bool valid() const noexcept;
    const std::string& error() const noexcept;
    std::uint32_t targetWidth() const noexcept;
    std::uint32_t targetHeight() const noexcept;

    bool resolvePixel(
        std::uint32_t targetX,
        std::uint32_t targetY,
        QueryResult& out) const noexcept;

private:
    const free_world::IScenePlaneSource& scene_;
    const State& state_;
    std::uint32_t targetWidth_ = 0u;
    std::uint32_t targetHeight_ = 0u;
    std::vector<std::vector<free_world::AxisContribution>> xWeights_;
    std::vector<std::vector<free_world::AxisContribution>> yWeights_;
    bool valid_ = false;
    std::string error_;
};

const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_continuous::v0_5

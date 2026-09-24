#pragma once

#include "open_scene_channel_authority_v0_78.h"
#include "truthraw/core.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace truthraw::open_scene_field::v0_85 {

using Digest = truthraw::sha256_v0_69::Digest;
using Authority = truthraw::open_scene_channel_authority::v0_78::Authority;
using UncertaintyKnowledge =
    truthraw::open_scene_channel_authority::v0_78::UncertaintyKnowledge;
using BoundDomain =
    truthraw::open_scene_channel_authority::v0_78::BoundDomain;

inline constexpr std::uint32_t kCanonicalTileEdge = 64u;

enum class CreationRole : std::uint8_t {
    Unknown = 0,
    SourceMeasuredCfa = 1,
    ScientificReconstruction = 2,
    DenseProjection = 3,
    RestorationDerivative = 4,
};

enum ContributionMask : std::uint8_t {
    ContributionNone = 0u,
    ContributionMeasured = 1u << 0u,
    ContributionReconstructed = 1u << 1u,
    ContributionCensored = 1u << 2u,
    ContributionUnknown = 1u << 3u,
};

enum class EncodingMode : std::uint8_t {
    Uniform = 1,
    Palette2 = 2,
    Palette4 = 3,
    Dense32 = 4,
};

struct ChannelRecord final {
    float value = 0.0f;
    CreationRole role = CreationRole::Unknown;
    Authority authority = Authority::Unknown;
    UncertaintyKnowledge uncertainty = UncertaintyKnowledge::Unresolved;
    BoundDomain boundDomain = BoundDomain::None;

    bool valuePresent = false;

    bool p95Known = false;
    float p95 = 0.0f;

    // Scientific/evidence support, not interpolation weight or appearance confidence.
    // Exact directly measured CFA support=1 is encoded implicitly.
    bool supportKnown = false;
    float support = 0.0f;

    bool boundKnown = false;
    float bound = 0.0f;

    std::uint8_t contributionMask = ContributionNone;
};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest zeroLineSha256{};
    Digest sceneScaleSha256{};

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;

    std::string reconstructionBackendId;
    std::string colourBindingId;
};

struct EncodedTile final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    EncodingMode mode = EncodingMode::Dense32;
    std::uint8_t paletteCount = 0u;
    std::uint32_t recordCount = 0u;
    std::uint32_t p95ScalarCount = 0u;
    std::uint32_t supportScalarCount = 0u;
    std::uint32_t boundScalarCount = 0u;
    std::vector<std::uint8_t> bytes;
};

struct Summary final {
    Digest contentSha256{};
    Digest encodingSha256{};
    Digest policySha256{};
    Digest artifactSha256{};

    std::array<std::uint64_t, 5> creationRoleCounts{};
    std::array<std::uint64_t, 4> authorityCounts{};
    std::array<std::uint64_t, 4> uncertaintyCounts{};

    std::uint64_t recordCount = 0u;
    std::uint64_t tileCount = 0u;
    std::uint64_t p95KnownCount = 0u;
    std::uint64_t supportKnownCount = 0u;
    std::uint64_t boundKnownCount = 0u;
    std::uint64_t encodedBytes = 0u;

    bool valueFieldBound = true;
    bool perPixelPerChannelAuthority = true;
    bool perPixelPerChannelUncertainty = true;
    bool perPixelPerChannelBounds = true;
    bool encodingChangesScientificIdentity = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool validate_record(const ChannelRecord& record) noexcept;

std::uint32_t classification_word(const ChannelRecord& record) noexcept;

bool build_source_tile_records(
    CfaPattern cfa,
    std::uint32_t globalX,
    std::uint32_t globalY,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint16_t> raw,
    float whiteLevel,
    std::span<const float> cameraNativeRgb,
    std::vector<ChannelRecord>& out) noexcept;

bool encode_tile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const ChannelRecord> records,
    EncodedTile& out) noexcept;

bool decode_tile(
    std::span<const std::uint8_t> encoded,
    EncodedTile& metadata,
    std::vector<ChannelRecord>& records) noexcept;

class Builder final {
public:
    explicit Builder(Binding binding);

    bool valid() const noexcept { return valid_; }

    bool appendTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        std::span<const ChannelRecord> records,
        std::span<const std::uint8_t> encodedBytes) noexcept;

    bool finalize(
        const Digest& parentOpenSceneArtifactSha256,
        Summary& out) noexcept;

private:
    Binding binding_{};
    truthraw::sha256_v0_69::Hasher contentHasher_{};
    truthraw::sha256_v0_69::Hasher encodingHasher_{};

    std::array<std::uint64_t, 5> roleCounts_{};
    std::array<std::uint64_t, 4> authorityCounts_{};
    std::array<std::uint64_t, 4> uncertaintyCounts_{};

    std::uint64_t records_ = 0u;
    std::uint64_t tiles_ = 0u;
    std::uint64_t p95Known_ = 0u;
    std::uint64_t supportKnown_ = 0u;
    std::uint64_t boundKnown_ = 0u;
    std::uint64_t encodedBytes_ = 0u;

    std::uint32_t expectedTileX_ = 0u;
    std::uint32_t expectedTileY_ = 0u;

    bool valid_ = false;
    bool finalized_ = false;
};

const char* schema_name() noexcept;
const char* creation_role_name(CreationRole role) noexcept;
const char* encoding_mode_name(EncodingMode mode) noexcept;

} // namespace truthraw::open_scene_field::v0_85

#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace truthraw::open_scene_channel_authority::v0_78 {

using Digest = truthraw::sha256_v0_69::Digest;

enum class Authority : std::uint8_t {
    CalibratedEstimate = 1,
    Reconstructed = 2,
    Censored = 3,
    Unknown = 4,
};

enum class UncertaintyKnowledge : std::uint8_t {
    Unresolved = 0,
    SourceNoiseProfileP95 = 1,
    BackendBoundP95 = 2,
    CertifiedVariance = 3,
};

enum class BoundDomain : std::uint8_t {
    None = 0,
    SourceRawCode = 1,
    SceneLinear = 2,
};

struct ChannelRecord final {
    Authority authority = Authority::Unknown;
    UncertaintyKnowledge uncertainty = UncertaintyKnowledge::Unresolved;
    BoundDomain boundDomain = BoundDomain::None;
    bool p95Known = false;
    float p95 = 0.0f;
    bool supportKnown = false;
    float support = 0.0f;
    bool boundKnown = false;
    float bound = 0.0f;
};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest zeroLineSha256{};
    Digest sceneScaleSha256{};
    Digest parentOpenSceneV070Sha256{};
    Digest uncertaintyBindingSha256{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    std::string reconstructionBackendId;
    bool reconstructedAuthorityAllowed = false;
};

struct Summary final {
    Digest contentSha256{};
    Digest policySha256{};
    Digest artifactSha256{};
    std::array<std::uint64_t, 4> authorityCounts{};
    std::array<std::uint64_t, 4> uncertaintyCounts{};
    std::uint64_t recordCount = 0u;
    std::uint64_t p95KnownCount = 0u;
    std::uint64_t censorBoundCount = 0u;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    bool counterfactualAuthorityPresent = false;
    bool chunkingChangesScientificIdentity = false;
};

class Builder final {
public:
    explicit Builder(Binding binding);
    bool valid() const noexcept { return valid_; }
    bool append(std::span<const ChannelRecord> records) noexcept;
    bool finalize(Summary& out) noexcept;
private:
    Binding binding_{};
    truthraw::sha256_v0_69::Hasher contentHasher_{};
    std::array<std::uint64_t,4> authorityCounts_{};
    std::array<std::uint64_t,4> uncertaintyCounts_{};
    std::uint64_t records_ = 0u;
    std::uint64_t p95Known_ = 0u;
    std::uint64_t boundKnown_ = 0u;
    bool valid_ = false;
    bool finalized_ = false;
};

bool build_generic_fail_closed_from_source(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const Binding& binding,
    Summary& out) noexcept;

const char* schema_name() noexcept;
const char* authority_name(Authority a) noexcept;
const char* uncertainty_name(UncertaintyKnowledge u) noexcept;

}  // namespace truthraw::open_scene_channel_authority::v0_78

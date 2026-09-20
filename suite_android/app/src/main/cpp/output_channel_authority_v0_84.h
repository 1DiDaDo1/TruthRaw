#pragma once

#include "full_frame_streaming_v0_1.h"
#include "open_scene_channel_authority_v0_78.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::output_channel_authority::v0_84 {

using Digest = truthraw::sha256_v0_69::Digest;
using Authority = truthraw::open_scene_channel_authority::v0_78::Authority;

enum class MappingMode : std::uint8_t {
    FullResolutionConservative = 1,
    ResampledFailClosedUnknown = 2,
};

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest canonicalOpenSceneSha256{};
    Digest sourceChannelAuthoritySha256{};
    Digest uncertaintyDecisionSha256{};
    std::uint32_t sourceWidth = 0u;
    std::uint32_t sourceHeight = 0u;
    std::uint32_t outputWidth = 0u;
    std::uint32_t outputHeight = 0u;
    std::uint32_t reconstructionSupportRadius = 0u;
    bool reconstructedUncertaintyAdmitted = false;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
    std::string reconstructionBackendId;
};

struct Summary final {
    Digest contentSha256{};
    Digest policySha256{};
    Digest artifactSha256{};
    std::array<std::uint64_t,4> authorityCounts{};
    std::uint64_t recordCount = 0u;
    std::uint64_t outputPixelCount = 0u;
    std::uint64_t censoredSupportPixels = 0u;
    MappingMode mappingMode = MappingMode::ResampledFailClosedUnknown;
    bool perOutputChannelAuthorityAvailable = false;
    bool reconstructedUncertaintyAdmitted = false;
    bool orientationTransformChangesAuthority = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool build_conservative(
    truthraw::streaming_v0_1::IRawTileSource& source,
    const Binding& binding,
    Summary& out) noexcept;

const char* schema_name() noexcept;
const char* mapping_mode_name(MappingMode mode) noexcept;

} // namespace truthraw::output_channel_authority::v0_84

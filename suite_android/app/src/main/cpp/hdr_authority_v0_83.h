#pragma once

#include "truthraw_sha256_v0_69.h"

#include <cstdint>

namespace truthraw::hdr_authority::v0_83 {

using Digest = truthraw::sha256_v0_69::Digest;

enum class ScientificHdrAuthority : std::uint8_t {
    Blocked = 0,
    DirectEvidenceBound = 1,
    ReconstructionUncertaintyBound = 2,
};

enum class PresentationHdrAuthority : std::uint8_t {
    Disabled = 0,
    AppearanceOnly = 1,
};

enum class BlockedReason : std::uint8_t {
    None = 0,
    NoPerOutputChannelAuthority = 1,
    UnknownChannelAuthorityPresent = 2,
    ReconstructedUncertaintyNotAdmitted = 3,
    CensoredGainNotSuppressed = 4,
    InvalidInput = 5,
};

struct Input final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest canonicalOpenSceneSha256{};
    Digest channelAuthoritySha256{};
    Digest uncertaintyAdmissionSha256{};
    Digest illuminationStateSha256{};

    std::uint64_t calibratedEstimateChannelRecords = 0u;
    std::uint64_t reconstructedChannelRecords = 0u;
    std::uint64_t censoredChannelRecords = 0u;
    std::uint64_t unknownChannelRecords = 0u;

    std::uint64_t outputPixelCount = 0u;
    std::uint64_t presentationHdrGainPixels = 0u;

    bool presentationHdrEnabled = false;
    bool perOutputChannelAuthorityAvailable = false;
    bool reconstructedUncertaintyAdmitted = false;
    bool censoredGainSuppressed = false;

    // Context only. These fields may change the state identity but may never
    // independently promote HDR scientific authority.
    bool illuminationWhitePointKnown = false;
    bool illuminationSpectrumKnown = false;

    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;
};

struct State final {
    ScientificHdrAuthority scientificAuthority = ScientificHdrAuthority::Blocked;
    PresentationHdrAuthority presentationAuthority =
        PresentationHdrAuthority::Disabled;
    BlockedReason blockedReason = BlockedReason::InvalidInput;

    bool scientificGainAllowed = false;
    bool presentationGainAllowed = false;
    bool censoredExactRecoveryAllowed = false;
    bool unknownHeadroomAllowed = false;
    bool illuminationCreatesHeadroom = false;
    bool requiresPerOutputChannelAuthority = true;
    bool requiresAdmittedUncertaintyForReconstructed = true;

    std::uint64_t presentationHdrGainPixels = 0u;
    std::uint64_t outputPixelCount = 0u;
    std::uint32_t physicalFrameCount = 1u;
    std::uint32_t independentEvidenceCount = 1u;

    Digest stateSha256{};
};

bool build(const Input& input, State& out) noexcept;

const char* schema_name() noexcept;
const char* scientific_authority_name(ScientificHdrAuthority authority) noexcept;
const char* presentation_authority_name(PresentationHdrAuthority authority) noexcept;
const char* blocked_reason_name(BlockedReason reason) noexcept;

}  // namespace truthraw::hdr_authority::v0_83

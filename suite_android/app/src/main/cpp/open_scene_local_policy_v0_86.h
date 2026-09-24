#pragma once

#include "open_scene_field_v0_85.h"

#include <array>
#include <cstdint>
#include <span>

namespace truthraw::open_scene_local_policy::v0_86 {

namespace field = truthraw::open_scene_field::v0_85;

enum class RestorationDisposition : std::uint8_t {
    Preserve = 1,
    PreserveReconstruction = 2,
    CensorBoundOnly = 3,
    AppearanceOnlyNoWriteback = 4,
};

enum class HdrDisposition : std::uint8_t {
    DirectEvidenceEligible = 1,
    ReconstructionBoundEligible = 2,
    CensoredExactGainForbidden = 3,
    UnknownHeadroomForbidden = 4,
};

enum class DetailSupportDisposition : std::uint8_t {
    MeasuredSupport = 1,
    ReconstructedSupport = 2,
    CensoredSupportBlocked = 3,
    UnknownSupportBlocked = 4,
};

struct Decision final {
    RestorationDisposition restoration =
        RestorationDisposition::AppearanceOnlyNoWriteback;
    HdrDisposition hdr = HdrDisposition::UnknownHeadroomForbidden;
    DetailSupportDisposition detail =
        DetailSupportDisposition::UnknownSupportBlocked;

    bool scientificWritebackAllowed = false;
    bool createsNewEvidence = false;
    bool exactCensoredRecoveryAllowed = false;
    bool unknownMayBecomeMeasured = false;
};

struct Summary final {
    std::array<std::uint64_t,4> restorationCounts{};
    std::array<std::uint64_t,4> hdrCounts{};
    std::array<std::uint64_t,4> detailCounts{};
    std::uint64_t recordCount = 0u;
    bool allScientificHdrEligible = false;
    bool anyCensored = false;
    bool anyUnknown = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool evaluate(const field::ChannelRecord& record, Decision& out) noexcept;

bool summarize(
    std::span<const field::ChannelRecord> records,
    Summary& out) noexcept;

const char* schema_name() noexcept;

} // namespace truthraw::open_scene_local_policy::v0_86

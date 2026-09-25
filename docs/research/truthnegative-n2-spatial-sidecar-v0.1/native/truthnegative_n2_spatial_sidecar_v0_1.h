#pragma once

#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>

namespace truthraw::truthnegative_n2_spatial_sidecar::v0_1 {

namespace cfa = truthraw::truthnegative_n2_cfa_audit::v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/N2SpatialAudit/0.1";

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    std::uint64_t tileCount = 0u;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    bool candidateApplied = false;
};

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const cfa::Result& audit,
    Report& out) noexcept;

} // namespace truthraw::truthnegative_n2_spatial_sidecar::v0_1

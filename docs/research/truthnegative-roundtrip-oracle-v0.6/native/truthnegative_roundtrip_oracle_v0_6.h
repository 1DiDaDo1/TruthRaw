#pragma once

#include "truthnegative_continuous_v0_5.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_roundtrip_oracle::v0_6 {

namespace tn = truthraw::truthnegative_continuous::v0_5;
namespace free_world = truthraw::free_world_pixel_resolve_2d::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeRoundTripExplainabilityOracle/0.6";
inline constexpr const char* kMethodId =
    "AREA_CONSERVATION_AUTHORITY_EXPLAINABILITY_V0_6";

struct RasterSpec final {
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
};

struct RasterReport final {
    RasterSpec raster{};
    std::uint64_t pixelCount = 0u;
    std::array<double, 3u> meanSceneLinear{};
    std::array<double, 3u> absoluteMeanError{};
    std::array<std::uint64_t, 3u> resolvedAuthorityCounts{};
    std::uint64_t footprintLinkCount = 0u;
    std::uint64_t measuredTargetClaimCount = 0u;
    Digest queryChainSha256{};
    bool stateIdentityPreserved = false;
    bool footprintsNormalized = false;
    bool noEvidencePromotion = false;
};

struct Report final {
    Digest truthNegativeStateSha256{};
    Digest oracleSha256{};
    std::array<double, 3u> canonicalGlobalMean{};
    std::vector<RasterReport> rasters;
    double tolerance = 0.0;
    double maximumAbsoluteMeanError = 0.0;

    bool globalAreaConserved = false;
    bool stateIdentityPreserved = false;
    bool footprintsNormalized = false;
    bool noEvidencePromotion = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kMethodId;
};

bool run(
    const free_world::IScenePlaneSource& scene,
    const tn::State& state,
    const std::vector<RasterSpec>& rasters,
    double tolerance,
    Report& out) noexcept;

const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_roundtrip_oracle::v0_6

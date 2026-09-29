#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::anchor_constrained_local_reconstruction::v0_1 {

namespace stream = truthraw::streaming_v0_1;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/AnchorConstrainedLocalReconstructionAudit/0.1";
inline constexpr std::uint32_t kLatticeFractionBits = 20u;
inline constexpr std::uint64_t kLatticeUnitsPerSourcePixel =
    1ull << kLatticeFractionBits;
inline constexpr std::uint32_t kHoldoutPeriod = 8u;
inline constexpr int kSupportRadius = 8;
inline constexpr std::uint32_t kMinPlaneSamples = 12u;

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
};

struct QueryRegion final {
    std::uint32_t id = 0u;
    std::uint32_t frontsideX = 0u;
    std::uint32_t frontsideY = 0u;
    std::uint32_t frontsideWidth = 0u;
    std::uint32_t frontsideHeight = 0u;
    std::uint32_t left = 0u;
    std::uint32_t top = 0u;
    std::uint32_t right = 0u;   // exclusive
    std::uint32_t bottom = 0u;  // exclusive
};

struct Metrics final {
    std::uint64_t holdouts = 0u;
    std::uint64_t targetCensoredSkipped = 0u;
    std::uint64_t solverValid = 0u;
    std::uint64_t solverInvalid = 0u;
    std::uint64_t baselineValid = 0u;
    std::uint64_t baselineInvalid = 0u;
    std::uint64_t bothValid = 0u;
    std::uint64_t solverLowerAbsError = 0u;
    std::uint64_t baselineLowerAbsError = 0u;
    std::uint64_t equalAbsError = 0u;
    std::uint64_t solverWithin1Sigma = 0u;
    std::uint64_t solverWithin2Sigma = 0u;
    std::uint64_t solverWithin3Sigma = 0u;
    std::array<std::uint64_t,4u> holdoutCfaPhase{};
    std::array<std::uint64_t,4u> solverValidCfaPhase{};
    long double solverAbsErrorSum = 0.0L;
    long double solverSquaredErrorSum = 0.0L;
    long double solverSignedErrorSum = 0.0L;
    long double baselineAbsErrorSum = 0.0L;
    long double baselineSquaredErrorSum = 0.0L;
    long double baselineSignedErrorSum = 0.0L;
    long double solverPredictionVarianceSum = 0.0L;
    long double solverVarianceInflationSum = 0.0L;
    double solverMaxAbsError = 0.0;
    double baselineMaxAbsError = 0.0;
    double solverMaxCombinedZ = 0.0;
    double solverMaxDirectionalDisagreementSigma = 0.0;
    double solverMaxCrossScaleDisagreementSigma = 0.0;
};

struct HoldoutRecord final {
    std::uint32_t queryId = 0u;
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t cfaPhase = 0u;
    std::uint32_t channel = 0u;
    std::uint32_t planeSamples = 0u;
    double actual = 0.0;
    double targetVariance = 0.0;
    double solverEstimate = 0.0;
    double solverPredictionVariance = 0.0;
    double solverVarianceInflation = 0.0;
    double solverAbsError = 0.0;
    double solverCombinedZ = 0.0;
    double baselineEstimate = 0.0;
    double baselinePredictionVariance = 0.0;
    double baselineAbsError = 0.0;
    bool solverValid = false;
    bool baselineValid = false;
};

struct QueryMetrics final {
    QueryRegion query{};
    Metrics metrics{};
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    Digest holdoutStreamSha256{};
    Metrics global{};
    std::vector<QueryMetrics> queries{};
    std::vector<HoldoutRecord> holdouts{};
    bool targetValueUsedBySolver = false;
    bool measuredAnchorsModified = false;
    bool unanchoredValuesPromotedToMeasured = false;
    bool reconstructedAuthorityOnly = true;
    bool uncertaintyDiagnosticOnly = true;
    bool noiseIndependenceAdmitted = false;
    bool solverAppliedToScientificMaster = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const std::vector<QueryRegion>& queries,
    Report& out) noexcept;

} // namespace truthraw::anchor_constrained_local_reconstruction::v0_1

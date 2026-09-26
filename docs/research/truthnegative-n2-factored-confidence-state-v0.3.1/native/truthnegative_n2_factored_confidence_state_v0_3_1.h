#pragma once

#include "truthnegative_n2_confidence_field_v0_3.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_n2_factored_confidence_state::v0_3_1 {

namespace cf = truthraw::truthnegative_n2_confidence_field::v0_3;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/N2FactoredConfidenceState/0.3.1";

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
    Digest v01CandidateSha256{};
    Digest v01AuditSha256{};
    Digest v01SpatialSha256{};
    Digest centerExcludedAuditSha256{};
    Digest confidenceFieldSha256{};
};

struct TileFacts final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;

    std::uint64_t sampled = 0u;
    std::uint64_t v01CandidateCenters = 0u;
    std::uint64_t predictorValid = 0u;
    std::uint64_t predictorInvalid = 0u;
    std::uint64_t pairsConsidered = 0u;
    std::uint64_t pairsAccepted = 0u;
    std::uint64_t pairsRejected = 0u;
    std::uint64_t scalesConsidered = 0u;
    std::uint64_t scalesAccepted = 0u;
    std::uint64_t scalesRejected = 0u;
    std::uint64_t centerZGt2 = 0u;
    std::uint64_t structureProtected = 0u;
    std::uint64_t censoredProtected = 0u;
    std::uint64_t censorBoundaryProtected = 0u;

    double candidateFraction = 0.0;
    double predictorCoverage = 0.0;
    double pairAcceptance = 0.0;
    double scaleAcceptance = 0.0;
    double centerZGt2Fraction = 0.0;
    double structureProtectionFraction = 0.0;
    double censorProtectionFraction = 0.0;
    double censorBoundaryProtectionFraction = 0.0;
    double meanEstimateToCenterVarianceRatio = 0.0;
    double maxEstimateToCenterVarianceRatio = 0.0;

    std::array<std::uint64_t,4u> v01CandidateCfaPhase{};
    std::array<std::uint64_t,4u> predictorValidCfaPhase{};

    bool hasCandidates = false;
    bool predictorAvailable = false;
    bool allCandidatesPredictable = false;
    bool centerOutlierFree = false;
    bool predictableAndCenterOutlierFree = false;
    bool pairRejectionFree = false;
    bool scaleRejectionFree = false;
    bool structureProtectionPresent = false;
    bool censorProtectionPresent = false;
    bool censorBoundaryProtectionPresent = false;
    bool maxPredictorVarianceLeCenterVariance = false;

    cf::SupportClass legacySupportClass = cf::SupportClass::NoCandidate;
    bool promotionEligible = false;
};

struct Result final {
    std::vector<TileFacts> tiles{};
    Digest stateSha256{};

    std::uint64_t hasCandidateTiles = 0u;
    std::uint64_t allCandidatesPredictableTiles = 0u;
    std::uint64_t centerOutlierFreeTiles = 0u;
    std::uint64_t predictableAndCenterOutlierFreeTiles = 0u;
    std::uint64_t pairRejectionFreeTiles = 0u;
    std::uint64_t scaleRejectionFreeTiles = 0u;
    std::uint64_t structureProtectionPresentTiles = 0u;
    std::uint64_t censorProtectionPresentTiles = 0u;
    std::uint64_t censorBoundaryProtectionPresentTiles = 0u;
    std::uint64_t maxPredictorVarianceLeCenterVarianceTiles = 0u;

    std::uint32_t tileEdge = 0u;
    std::uint32_t samplingPeriod = 0u;

    bool exactConfidenceFieldBindingVerified = false;
    bool vectorValuedNoScalarProbability = true;
    bool legacyClassNonAuthoritative = true;
    bool cfaPhaseDiagnosticOnly = true;
    bool supportDistanceAdmitted = false;
    bool promotionEligible = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    std::uint64_t tileCount = 0u;
    bool promotionEligible = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool derive(
    const Binding& binding,
    const cf::Result& confidenceField,
    Result& out) noexcept;

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const Result& state,
    Report& out) noexcept;

} // namespace truthraw::truthnegative_n2_factored_confidence_state::v0_3_1

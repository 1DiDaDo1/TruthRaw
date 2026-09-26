#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_center_excluded_neighborhood_v0_2.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"
#include "truthraw_sha256_v0_69.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1 {

namespace stream = truthraw::streaming_v0_1;
namespace v01 = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace ce = truthraw::truthnegative_center_excluded_neighborhood::v0_2;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "D.RAW/TruthNegative/N2CenterExcludedSpatialAudit/0.2.1";

struct Binding final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
    Digest v01CandidateSha256{};
    Digest v01AuditSha256{};
    Digest v01SpatialSha256{};
};

struct Metrics final {
    std::uint64_t sampled = 0u;
    std::uint64_t v01CandidateCenters = 0u;
    std::uint64_t predictorValid = 0u;
    std::uint64_t predictorInvalid = 0u;

    std::uint64_t symmetricPairsConsidered = 0u;
    std::uint64_t symmetricPairsAccepted = 0u;
    std::uint64_t symmetricPairsRejected = 0u;

    std::uint64_t scalesConsidered = 0u;
    std::uint64_t scalesAccepted = 0u;
    std::uint64_t scalesRejected = 0u;

    std::uint64_t centerResidualWithin1Sigma = 0u;
    std::uint64_t centerResidualBetween1And2Sigma = 0u;
    std::uint64_t centerResidualAbove2Sigma = 0u;

    std::uint64_t combinedResidualWithin1Sigma = 0u;
    std::uint64_t combinedResidualBetween1And2Sigma = 0u;
    std::uint64_t combinedResidualAbove2Sigma = 0u;

    double absResidualSum = 0.0;
    double maxAbsResidual = 0.0;
    double centerVarianceSum = 0.0;
    double estimateVarianceSum = 0.0;
    double estimateToCenterVarianceRatioSum = 0.0;
    double maxEstimateToCenterVarianceRatio = 0.0;
    double maxDirectionalDisagreementSigma = 0.0;
    double maxCrossScaleDisagreementSigma = 0.0;

    std::array<std::uint64_t,4u> v01CandidateCfaPhase{};
    std::array<std::uint64_t,4u> predictorValidCfaPhase{};
};

struct TileMetrics final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    Metrics metrics{};
};

struct Result final {
    Metrics metrics{};
    std::vector<TileMetrics> tiles{};
    Digest auditSha256{};
    std::uint32_t tileEdge = 0u;
    std::uint32_t samplingPeriod = 0u;
    bool v01TileParityVerified = false;
    bool centerOnlySigmaPrimary = true;
    bool combinedSigmaDiagnosticOnly = true;
    bool noiseIndependenceAdmitted = false;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

struct Report final {
    std::string json{};
    Digest jsonSha256{};
    std::uint64_t tileCount = 0u;
    bool candidateApplied = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const v01::Result& referenceV01,
    Result& out) noexcept;

bool encode(
    const Binding& binding,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    const Result& audit,
    Report& out) noexcept;

} // namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1

#pragma once

#include "full_frame_streaming_v0_1.h"
#include "truthnegative_center_excluded_spatial_audit_v0_2_1.h"
#include "truthnegative_n2_cfa_audit_v0_1.h"

namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2 {

namespace stream = truthraw::streaming_v0_1;
namespace v01 = truthraw::truthnegative_n2_cfa_audit::v0_1;
namespace v021 =
    truthraw::truthnegative_center_excluded_spatial_audit::v0_2_1;

struct Diagnostics final {
    double totalMs = 0.0;
    double fillStage2Ms = 0.0;
    double candidateLoopMs = 0.0;
    double predictorEstimateMs = 0.0;
    double profiledCenterSetupMs = 0.0;
    double profiledNeighborAcquireMs = 0.0;
    double profiledPathCensorMs = 0.0;
    double profiledNeighborVarianceMs = 0.0;
    double profiledPredictorEstimateMs = 0.0;
    double profiledResidualMetricsMs = 0.0;
    double predictorAdmissibilitySlotBuildMs = 0.0;
    double predictorPairBuildGateMs = 0.0;
    double predictorScaleConsistencyMs = 0.0;
    double predictorCrossScaleCombineMs = 0.0;
    double predictorFinalCombineMs = 0.0;
    double fixedTopologyParityOracleMs = 0.0;
    double finalHashMs = 0.0;
    std::uint64_t profileSampleStride = 64u;
    std::uint64_t paritySampleStride = 64u;
    std::uint64_t profiledCandidateCount = 0u;
    std::uint64_t profiledNeighborSampleCount = 0u;
    std::uint64_t profiledPathCheckCount = 0u;
    std::uint64_t profiledPathStepCount = 0u;
    std::uint64_t profiledNeighborVarianceCount = 0u;
    std::uint64_t predictorProfiledEstimateCount = 0u;
    std::uint64_t predictorSlotDuplicateCount = 0u;
    std::uint64_t predictorPairLookupCount = 0u;
    std::uint64_t predictorPairZDistanceCount = 0u;
    std::uint64_t predictorDirectionalZDistanceCount = 0u;
    std::uint64_t predictorCrossScaleZDistanceCount = 0u;
    std::uint64_t predictorInverseVariancePairCombineCount = 0u;
    std::uint64_t predictorInverseVarianceScaleCombineCount = 0u;
    std::uint64_t fixedTopologyFastPathCount = 0u;
    std::uint64_t fixedTopologyNotApplicableFallbackCount = 0u;
    std::uint64_t fixedTopologyFailureFallbackCount = 0u;
    std::uint64_t fixedTopologyParityOracleCount = 0u;
    std::uint64_t fixedTopologyParityMismatchCount = 0u;
    std::uint64_t tileCount = 0u;
    std::uint64_t candidateTileCount = 0u;
    std::uint64_t candidateCenterCount = 0u;
    bool fixedTopologyFastPathActive = false;
    bool fixedTopologyParityVerified = false;
    bool timingIsScientificEvidence = false;
    bool timingMayChangeScientificAuthority = false;
};

/**
 * Performance-only execution path for the v0.2.1 scientific contract.
 *
 * It consumes sparse corrected-sample coordinates recorded during the already
 * completed v0.1 pass instead of rerunning v0.1 per reporting tile.
 *
 * IMPORTANT: output is the exact v0.2.1 Result type. The implementation must
 * preserve all v0.2.1 metrics and audit identity byte-for-byte.
 */
bool runSparseReference(
    stream::IRawTileSource& source,
    const v021::Binding& binding,
    const v01::Result& referenceV01,
    v021::Result& out,
    Diagnostics* diagnostics = nullptr) noexcept;

} // namespace truthraw::truthnegative_center_excluded_spatial_audit::v0_2_2

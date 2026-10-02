#pragma once

#include <cstdint>
#include <vector>

namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2 {

enum class SampleAuthority : std::uint8_t {
    Measured = 1,
    CalibratedEstimate = 2,
    Reconstructed = 3,
    Unknown = 4,
    Censored = 5,
};

enum class Direction : std::uint8_t {
    Horizontal = 1,
    Vertical = 2,
    DiagonalDown = 3,
    DiagonalUp = 4,
};

struct Sample final {
    double value = 0.0;
    double variance = 0.0;
    int dx = 0;
    int dy = 0;
    SampleAuthority authority = SampleAuthority::Unknown;
    bool varianceKnown = false;
    bool sameChannel = true;
    bool sameObject = true;
    bool objectIdentityKnown = false;
    bool censorBoundary = false;
};

struct Input final {
    std::vector<Sample> neighbors{};
};

/**
 * Diagnostic-only sampled timing/counter sink for estimate paths.
 *
 * These fields never participate in Result, hashing, candidate authority,
 * evidence, reconstruction, calibration or writeback. Callers may pass null
 * for the canonical low-overhead route.
 */
struct Diagnostics final {
    double admissibilitySlotBuildMs = 0.0;
    double pairBuildGateMs = 0.0;
    double scaleConsistencyMs = 0.0;
    double crossScaleCombineMs = 0.0;
    double finalCombineMs = 0.0;
    std::uint64_t profiledEstimateCount = 0u;
    std::uint64_t slotDuplicateCount = 0u;
    std::uint64_t pairLookupCount = 0u;
    std::uint64_t pairZDistanceCount = 0u;
    std::uint64_t directionalZDistanceCount = 0u;
    std::uint64_t crossScaleZDistanceCount = 0u;
    std::uint64_t inverseVariancePairCombineCount = 0u;
    std::uint64_t inverseVarianceScaleCombineCount = 0u;
    bool timingIsScientificEvidence = false;
    bool timingMayChangeScientificAuthority = false;
};

struct Result final {
    double estimate = 0.0;
    double estimateVariance = 0.0;
    double effectiveWeight = 0.0;
    double maxDirectionalDisagreementSigma = 0.0;
    double maxCrossScaleDisagreementSigma = 0.0;
    std::uint32_t admissibleSamples = 0u;
    std::uint32_t symmetricPairsConsidered = 0u;
    std::uint32_t symmetricPairsAccepted = 0u;
    std::uint32_t symmetricPairsRejected = 0u;
    std::uint32_t scalesConsidered = 0u;
    std::uint32_t scalesAccepted = 0u;
    std::uint32_t scalesRejected = 0u;
    std::uint32_t finestAcceptedRadius = 0u;
    std::uint32_t coarsestAcceptedRadius = 0u;
    bool valid = false;
    bool centerExcluded = true;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

enum class FixedTopologyOutcome : std::uint8_t {
    NotApplicable = 0,
    Success = 1,
    Failure = 2,
};

/** Canonical generic predictor. */
bool estimate(
    const Input& input,
    Result& out,
    Diagnostics* diagnostics = nullptr) noexcept;

/**
 * Specialized predictor for the established 3 radii x 4 directions x 2 sides
 * Center-Excluded topology (radii 2,4,8).
 *
 * The function is only a transport/data-structure fast path. It preserves the
 * same admissibility, thresholds, floating-point operation order per accepted
 * pair/scale, Result semantics and scientific authority as estimate().
 * Unsupported admissible geometry returns NotApplicable so callers can fall
 * back to estimate() without broadening the scientific contract.
 */
FixedTopologyOutcome estimateFixedTopology(
    const Input& input,
    Result& out,
    Diagnostics* diagnostics = nullptr) noexcept;

}  // namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2

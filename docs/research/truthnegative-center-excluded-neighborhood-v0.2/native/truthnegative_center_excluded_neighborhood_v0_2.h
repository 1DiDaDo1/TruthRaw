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

bool estimate(const Input& input, Result& out) noexcept;

}  // namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2

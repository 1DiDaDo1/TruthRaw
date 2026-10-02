#pragma once

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
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

/** Diagnostic-only timing/counter sink. Never scientific authority. */
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
 * Specialized data-structure path for the established radii {2,4,8}, four
 * directions and two symmetric sides. Unsupported admissible geometry returns
 * NotApplicable so the caller can use estimate() as the canonical fallback.
 */
inline FixedTopologyOutcome estimateFixedTopology(
    const Input& input,
    Result& out,
    Diagnostics* diagnostics = nullptr) noexcept {
    out = Result{};
    try {
        using Clock = std::chrono::steady_clock;
        const auto elapsedMs = [](Clock::time_point a, Clock::time_point b) {
            return std::chrono::duration<double,std::milli>(b-a).count();
        };
        constexpr std::array<std::uint32_t,3u> radii{2u,4u,8u};
        constexpr std::array<Direction,4u> directions{
            Direction::Horizontal,
            Direction::Vertical,
            Direction::DiagonalDown,
            Direction::DiagonalUp};
        constexpr double pairAgreementSigma = 2.0;
        constexpr double directionalAgreementSigma = 2.0;
        constexpr double crossScaleAgreementSigma = 2.0;
        constexpr std::uint32_t minDirectionsPerScale = 2u;

        struct Geometry final {
            Direction direction = Direction::Horizontal;
            std::uint32_t radius = 0u;
            int side = 0;
        };
        struct Slot final {
            const Sample* sample = nullptr;
            bool seen = false;
        };
        struct Pair final {
            Direction direction = Direction::Horizontal;
            std::uint32_t radius = 0u;
            double estimate = 0.0;
            double variance = 0.0;
        };
        struct Scale final {
            std::uint32_t radius = 0u;
            double estimate = 0.0;
            double variance = 0.0;
            std::uint32_t directions = 0u;
        };

        const auto finitePositive = [](double v) noexcept {
            return std::isfinite(v) && v > 0.0;
        };
        const auto authorityAllowed = [](SampleAuthority a) noexcept {
            return a == SampleAuthority::Measured ||
                   a == SampleAuthority::CalibratedEstimate;
        };
        const auto decode = [](int dx, int dy, Geometry& g) noexcept {
            g = Geometry{};
            if (dx == 0 && dy == 0) return false;
            if (dy == 0 && dx != 0) {
                g.direction = Direction::Horizontal;
                g.radius = static_cast<std::uint32_t>(std::abs(dx));
                g.side = dx < 0 ? -1 : 1;
                return g.radius > 0u;
            }
            if (dx == 0 && dy != 0) {
                g.direction = Direction::Vertical;
                g.radius = static_cast<std::uint32_t>(std::abs(dy));
                g.side = dy < 0 ? -1 : 1;
                return g.radius > 0u;
            }
            if (std::abs(dx) != std::abs(dy)) return false;
            g.radius = static_cast<std::uint32_t>(std::abs(dx));
            if (g.radius == 0u) return false;
            if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0)) {
                g.direction = Direction::DiagonalDown;
                g.side = dx < 0 ? -1 : 1;
            } else {
                g.direction = Direction::DiagonalUp;
                g.side = dx < 0 ? -1 : 1;
            }
            return true;
        };
        const auto radiusIndex = [](std::uint32_t r) noexcept -> int {
            if (r == 2u) return 0;
            if (r == 4u) return 1;
            if (r == 8u) return 2;
            return -1;
        };
        const auto directionIndex = [](Direction d) noexcept -> int {
            switch (d) {
                case Direction::Horizontal: return 0;
                case Direction::Vertical: return 1;
                case Direction::DiagonalDown: return 2;
                case Direction::DiagonalUp: return 3;
            }
            return -1;
        };
        const auto zDistance = [&](double a, double va, double b, double vb) {
            const double v = va + vb;
            if (!finitePositive(v)) {
                return std::numeric_limits<double>::infinity();
            }
            return std::abs(a-b) / std::sqrt(v);
        };
        const auto combinePairs = [&](const Pair* values, std::size_t count,
                                      double& estimate, double& variance,
                                      double& weight) noexcept {
            long double sw = 0.0L;
            long double swx = 0.0L;
            for (std::size_t i = 0u; i < count; ++i) {
                const auto& v = values[i];
                if (!std::isfinite(v.estimate) || !finitePositive(v.variance)) {
                    return false;
                }
                const long double w =
                    1.0L / static_cast<long double>(v.variance);
                sw += w;
                swx += w * static_cast<long double>(v.estimate);
            }
            if (!(sw > 0.0L)) return false;
            estimate = static_cast<double>(swx/sw);
            variance = static_cast<double>(1.0L/sw);
            weight = static_cast<double>(sw);
            return std::isfinite(estimate) && finitePositive(variance) &&
                   finitePositive(weight);
        };
        const auto combineScales = [&](const Scale* values, std::size_t count,
                                       double& estimate, double& variance,
                                       double& weight) noexcept {
            long double sw = 0.0L;
            long double swx = 0.0L;
            for (std::size_t i = 0u; i < count; ++i) {
                const auto& v = values[i];
                if (!std::isfinite(v.estimate) || !finitePositive(v.variance)) {
                    return false;
                }
                const long double w =
                    1.0L / static_cast<long double>(v.variance);
                sw += w;
                swx += w * static_cast<long double>(v.estimate);
            }
            if (!(sw > 0.0L)) return false;
            estimate = static_cast<double>(swx/sw);
            variance = static_cast<double>(1.0L/sw);
            weight = static_cast<double>(sw);
            return std::isfinite(estimate) && finitePositive(variance) &&
                   finitePositive(weight);
        };

        if (diagnostics) ++diagnostics->profiledEstimateCount;
        std::array<std::array<std::array<Slot,2u>,4u>,3u> slots{};
        auto sectionStarted = Clock::now();
        for (const auto& s : input.neighbors) {
            if (!std::isfinite(s.value) || !s.varianceKnown ||
                !finitePositive(s.variance) || !s.sameChannel ||
                s.censorBoundary || !authorityAllowed(s.authority) ||
                (s.objectIdentityKnown && !s.sameObject)) {
                continue;
            }
            Geometry g{};
            if (!decode(s.dx,s.dy,g)) continue;
            const int ri = radiusIndex(g.radius);
            const int di = directionIndex(g.direction);
            if (ri < 0 || di < 0) {
                out = Result{};
                return FixedTopologyOutcome::NotApplicable;
            }
            const int si = g.side < 0 ? 0 : 1;
            ++out.admissibleSamples;
            auto& slot = slots[static_cast<std::size_t>(ri)]
                              [static_cast<std::size_t>(di)]
                              [static_cast<std::size_t>(si)];
            if (slot.seen) {
                slot.sample = nullptr;
                if (diagnostics) ++diagnostics->slotDuplicateCount;
            } else {
                slot.seen = true;
                slot.sample = &s;
            }
        }
        if (diagnostics) {
            diagnostics->admissibilitySlotBuildMs +=
                elapsedMs(sectionStarted,Clock::now());
            sectionStarted = Clock::now();
        }

        std::array<std::array<Pair,4u>,3u> pairsByScale{};
        std::array<std::size_t,3u> pairCounts{};
        for (std::size_t di = 0u; di < directions.size(); ++di) {
            for (std::size_t ri = 0u; ri < radii.size(); ++ri) {
                if (diagnostics) diagnostics->pairLookupCount += 2u;
                const Sample* a = slots[ri][di][0u].sample;
                const Sample* b = slots[ri][di][1u].sample;
                if (a == nullptr || b == nullptr) continue;
                ++out.symmetricPairsConsidered;
                if (diagnostics) ++diagnostics->pairZDistanceCount;
                const double pairZ =
                    zDistance(a->value,a->variance,b->value,b->variance);
                if (!std::isfinite(pairZ)) {
                    return FixedTopologyOutcome::Failure;
                }
                out.maxDirectionalDisagreementSigma = std::max(
                    out.maxDirectionalDisagreementSigma,pairZ);
                if (pairZ > pairAgreementSigma) {
                    ++out.symmetricPairsRejected;
                    continue;
                }
                const double wa = 1.0/a->variance;
                const double wb = 1.0/b->variance;
                const double sw = wa+wb;
                if (!finitePositive(sw)) return FixedTopologyOutcome::Failure;
                Pair pair{};
                pair.direction = directions[di];
                pair.radius = radii[ri];
                pair.estimate = (wa*a->value + wb*b->value)/sw;
                pair.variance = 1.0/sw;
                if (!std::isfinite(pair.estimate) ||
                    !finitePositive(pair.variance)) {
                    return FixedTopologyOutcome::Failure;
                }
                pairsByScale[ri][pairCounts[ri]++] = pair;
                ++out.symmetricPairsAccepted;
            }
        }
        if (diagnostics) {
            diagnostics->pairBuildGateMs += elapsedMs(sectionStarted,Clock::now());
            sectionStarted = Clock::now();
        }

        std::array<Scale,3u> candidateScales{};
        std::size_t candidateScaleCount = 0u;
        for (std::size_t ri = 0u; ri < radii.size(); ++ri) {
            const std::size_t count = pairCounts[ri];
            if (count == 0u) continue;
            ++out.scalesConsidered;
            if (count < minDirectionsPerScale) {
                ++out.scalesRejected;
                continue;
            }
            bool directionalConflict = false;
            for (std::size_t i = 0u; i < count; ++i) {
                for (std::size_t j = i+1u; j < count; ++j) {
                    if (diagnostics) ++diagnostics->directionalZDistanceCount;
                    const double z = zDistance(
                        pairsByScale[ri][i].estimate,
                        pairsByScale[ri][i].variance,
                        pairsByScale[ri][j].estimate,
                        pairsByScale[ri][j].variance);
                    if (!std::isfinite(z)) return FixedTopologyOutcome::Failure;
                    out.maxDirectionalDisagreementSigma = std::max(
                        out.maxDirectionalDisagreementSigma,z);
                    if (z > directionalAgreementSigma) directionalConflict = true;
                }
            }
            if (directionalConflict) {
                ++out.scalesRejected;
                continue;
            }
            Scale scale{};
            scale.radius = radii[ri];
            scale.directions = static_cast<std::uint32_t>(count);
            double unusedWeight = 0.0;
            if (diagnostics) ++diagnostics->inverseVariancePairCombineCount;
            if (!combinePairs(
                    pairsByScale[ri].data(),count,
                    scale.estimate,scale.variance,unusedWeight)) {
                return FixedTopologyOutcome::Failure;
            }
            candidateScales[candidateScaleCount++] = scale;
        }
        if (diagnostics) {
            diagnostics->scaleConsistencyMs +=
                elapsedMs(sectionStarted,Clock::now());
        }
        if (candidateScaleCount == 0u) return FixedTopologyOutcome::Success;

        std::array<Scale,3u> acceptedScales{};
        std::size_t acceptedScaleCount = 1u;
        acceptedScales[0] = candidateScales[0];
        out.finestAcceptedRadius = candidateScales[0].radius;
        out.coarsestAcceptedRadius = candidateScales[0].radius;
        ++out.scalesAccepted;
        double runningEstimate = candidateScales[0].estimate;
        double runningVariance = candidateScales[0].variance;
        double runningWeight = 1.0/runningVariance;

        if (diagnostics) sectionStarted = Clock::now();
        for (std::size_t i = 1u; i < candidateScaleCount; ++i) {
            const auto& scale = candidateScales[i];
            if (diagnostics) ++diagnostics->crossScaleZDistanceCount;
            const double z = zDistance(
                scale.estimate,scale.variance,
                runningEstimate,runningVariance);
            if (!std::isfinite(z)) return FixedTopologyOutcome::Failure;
            out.maxCrossScaleDisagreementSigma = std::max(
                out.maxCrossScaleDisagreementSigma,z);
            if (z > crossScaleAgreementSigma) {
                ++out.scalesRejected;
                continue;
            }
            acceptedScales[acceptedScaleCount++] = scale;
            ++out.scalesAccepted;
            out.coarsestAcceptedRadius = scale.radius;
            if (diagnostics) ++diagnostics->inverseVarianceScaleCombineCount;
            if (!combineScales(
                    acceptedScales.data(),acceptedScaleCount,
                    runningEstimate,runningVariance,runningWeight)) {
                return FixedTopologyOutcome::Failure;
            }
        }
        if (diagnostics) {
            diagnostics->crossScaleCombineMs +=
                elapsedMs(sectionStarted,Clock::now());
        }

        if (diagnostics) sectionStarted = Clock::now();
        if (diagnostics) ++diagnostics->inverseVarianceScaleCombineCount;
        if (!combineScales(
                acceptedScales.data(),acceptedScaleCount,
                out.estimate,out.estimateVariance,out.effectiveWeight)) {
            return FixedTopologyOutcome::Failure;
        }
        if (diagnostics) {
            diagnostics->finalCombineMs += elapsedMs(sectionStarted,Clock::now());
        }
        out.valid = true;
        if (!out.centerExcluded || out.createsNewEvidence ||
            out.scientificWritebackAllowed) {
            return FixedTopologyOutcome::Failure;
        }
        return FixedTopologyOutcome::Success;
    } catch (...) {
        out = Result{};
        return FixedTopologyOutcome::Failure;
    }
}

}  // namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2

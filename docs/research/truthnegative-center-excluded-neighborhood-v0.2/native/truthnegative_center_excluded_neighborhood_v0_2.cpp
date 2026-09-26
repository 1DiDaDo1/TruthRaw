#include "truthnegative_center_excluded_neighborhood_v0_2.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2 {
namespace {

constexpr double kPairAgreementSigma = 2.0;
constexpr double kDirectionalAgreementSigma = 2.0;
constexpr double kCrossScaleAgreementSigma = 2.0;
constexpr std::uint32_t kMinDirectionsPerScale = 2u;

struct Geometry final {
    Direction direction = Direction::Horizontal;
    std::uint32_t radius = 0u;
    int side = 0;
};

struct PairEstimate final {
    Direction direction = Direction::Horizontal;
    std::uint32_t radius = 0u;
    double estimate = 0.0;
    double variance = 0.0;
};

struct ScaleEstimate final {
    std::uint32_t radius = 0u;
    double estimate = 0.0;
    double variance = 0.0;
    std::uint32_t directions = 0u;
};

bool finite_positive(double v) noexcept {
    return std::isfinite(v) && v > 0.0;
}

bool allowed_authority(SampleAuthority authority) noexcept {
    return authority == SampleAuthority::Measured ||
           authority == SampleAuthority::CalibratedEstimate;
}

bool decode_geometry(int dx, int dy, Geometry& out) noexcept {
    out = Geometry{};
    if (dx == 0 && dy == 0) return false;

    if (dy == 0 && dx != 0) {
        out.direction = Direction::Horizontal;
        out.radius = static_cast<std::uint32_t>(std::abs(dx));
        out.side = dx < 0 ? -1 : 1;
        return out.radius > 0u;
    }
    if (dx == 0 && dy != 0) {
        out.direction = Direction::Vertical;
        out.radius = static_cast<std::uint32_t>(std::abs(dy));
        out.side = dy < 0 ? -1 : 1;
        return out.radius > 0u;
    }
    if (std::abs(dx) != std::abs(dy)) return false;

    out.radius = static_cast<std::uint32_t>(std::abs(dx));
    if (out.radius == 0u) return false;

    if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0)) {
        out.direction = Direction::DiagonalDown;
        out.side = dx < 0 ? -1 : 1;
    } else {
        out.direction = Direction::DiagonalUp;
        out.side = dx < 0 ? -1 : 1;
    }
    return true;
}

bool admissible(const Sample& s, Geometry& geometry) noexcept {
    if (!std::isfinite(s.value) || !s.varianceKnown ||
        !finite_positive(s.variance) || !s.sameChannel ||
        s.censorBoundary || !allowed_authority(s.authority) ||
        (s.objectIdentityKnown && !s.sameObject)) {
        return false;
    }
    return decode_geometry(s.dx, s.dy, geometry);
}

double z_distance(
    double a,
    double varianceA,
    double b,
    double varianceB) noexcept {
    const double v = varianceA + varianceB;
    if (!finite_positive(v)) return std::numeric_limits<double>::infinity();
    return std::abs(a - b) / std::sqrt(v);
}

bool inverse_variance_combine(
    const std::vector<PairEstimate>& values,
    double& estimate,
    double& variance,
    double& weight) noexcept {
    long double sw = 0.0L;
    long double swx = 0.0L;
    for (const auto& v : values) {
        if (!std::isfinite(v.estimate) || !finite_positive(v.variance)) {
            return false;
        }
        const long double w = 1.0L / static_cast<long double>(v.variance);
        sw += w;
        swx += w * static_cast<long double>(v.estimate);
    }
    if (!(sw > 0.0L)) return false;
    estimate = static_cast<double>(swx / sw);
    variance = static_cast<double>(1.0L / sw);
    weight = static_cast<double>(sw);
    return std::isfinite(estimate) && finite_positive(variance) &&
           finite_positive(weight);
}

bool inverse_variance_combine_scales(
    const std::vector<ScaleEstimate>& values,
    double& estimate,
    double& variance,
    double& weight) noexcept {
    long double sw = 0.0L;
    long double swx = 0.0L;
    for (const auto& v : values) {
        if (!std::isfinite(v.estimate) || !finite_positive(v.variance)) {
            return false;
        }
        const long double w = 1.0L / static_cast<long double>(v.variance);
        sw += w;
        swx += w * static_cast<long double>(v.estimate);
    }
    if (!(sw > 0.0L)) return false;
    estimate = static_cast<double>(swx / sw);
    variance = static_cast<double>(1.0L / sw);
    weight = static_cast<double>(sw);
    return std::isfinite(estimate) && finite_positive(variance) &&
           finite_positive(weight);
}

}  // namespace

bool estimate(const Input& input, Result& out) noexcept {
    out = Result{};
    try {
        using Key = std::tuple<std::uint32_t, std::uint8_t, int>;
        std::map<Key, const Sample*> slots;

        for (const auto& sample : input.neighbors) {
            Geometry geometry{};
            if (!admissible(sample, geometry)) continue;
            ++out.admissibleSamples;
            const Key key{
                geometry.radius,
                static_cast<std::uint8_t>(geometry.direction),
                geometry.side};
            if (slots.contains(key)) {
                slots[key] = nullptr;
            } else {
                slots.emplace(key, &sample);
            }
        }

        std::map<std::uint32_t, std::vector<PairEstimate>> pairsByScale;
        for (const auto direction : {
                 Direction::Horizontal,
                 Direction::Vertical,
                 Direction::DiagonalDown,
                 Direction::DiagonalUp}) {
            std::vector<std::uint32_t> radii;
            for (const auto& [key, sample] : slots) {
                if (sample == nullptr) continue;
                const auto [radius, dirRaw, side] = key;
                (void)side;
                if (dirRaw == static_cast<std::uint8_t>(direction)) {
                    radii.push_back(radius);
                }
            }
            std::sort(radii.begin(), radii.end());
            radii.erase(std::unique(radii.begin(), radii.end()), radii.end());

            for (const auto radius : radii) {
                const Key negative{
                    radius, static_cast<std::uint8_t>(direction), -1};
                const Key positive{
                    radius, static_cast<std::uint8_t>(direction), 1};
                const auto leftIt = slots.find(negative);
                const auto rightIt = slots.find(positive);
                if (leftIt == slots.end() || rightIt == slots.end() ||
                    leftIt->second == nullptr || rightIt->second == nullptr) {
                    continue;
                }

                ++out.symmetricPairsConsidered;
                const Sample& a = *leftIt->second;
                const Sample& b = *rightIt->second;
                const double pairZ = z_distance(
                    a.value, a.variance, b.value, b.variance);
                if (!std::isfinite(pairZ)) return false;
                out.maxDirectionalDisagreementSigma = std::max(
                    out.maxDirectionalDisagreementSigma, pairZ);
                if (pairZ > kPairAgreementSigma) {
                    ++out.symmetricPairsRejected;
                    continue;
                }

                const double wa = 1.0 / a.variance;
                const double wb = 1.0 / b.variance;
                const double sw = wa + wb;
                if (!finite_positive(sw)) return false;

                PairEstimate pair{};
                pair.direction = direction;
                pair.radius = radius;
                pair.estimate = (wa * a.value + wb * b.value) / sw;
                pair.variance = 1.0 / sw;
                if (!std::isfinite(pair.estimate) ||
                    !finite_positive(pair.variance)) {
                    return false;
                }
                pairsByScale[radius].push_back(pair);
                ++out.symmetricPairsAccepted;
            }
        }

        std::vector<ScaleEstimate> candidateScales;
        for (auto& [radius, pairs] : pairsByScale) {
            ++out.scalesConsidered;
            if (pairs.size() < kMinDirectionsPerScale) {
                ++out.scalesRejected;
                continue;
            }

            bool directionalConflict = false;
            for (std::size_t i = 0u; i < pairs.size(); ++i) {
                for (std::size_t j = i + 1u; j < pairs.size(); ++j) {
                    const double z = z_distance(
                        pairs[i].estimate, pairs[i].variance,
                        pairs[j].estimate, pairs[j].variance);
                    if (!std::isfinite(z)) return false;
                    out.maxDirectionalDisagreementSigma = std::max(
                        out.maxDirectionalDisagreementSigma, z);
                    if (z > kDirectionalAgreementSigma) {
                        directionalConflict = true;
                    }
                }
            }
            if (directionalConflict) {
                ++out.scalesRejected;
                continue;
            }

            ScaleEstimate scale{};
            scale.radius = radius;
            scale.directions = static_cast<std::uint32_t>(pairs.size());
            double unusedWeight = 0.0;
            if (!inverse_variance_combine(
                    pairs, scale.estimate, scale.variance, unusedWeight)) {
                return false;
            }
            candidateScales.push_back(scale);
        }

        if (candidateScales.empty()) return true;
        std::sort(
            candidateScales.begin(), candidateScales.end(),
            [](const ScaleEstimate& a, const ScaleEstimate& b) {
                return a.radius < b.radius;
            });

        std::vector<ScaleEstimate> acceptedScales;
        acceptedScales.push_back(candidateScales.front());
        out.finestAcceptedRadius = candidateScales.front().radius;
        out.coarsestAcceptedRadius = candidateScales.front().radius;
        ++out.scalesAccepted;

        double runningEstimate = candidateScales.front().estimate;
        double runningVariance = candidateScales.front().variance;
        double runningWeight = 1.0 / runningVariance;

        for (std::size_t i = 1u; i < candidateScales.size(); ++i) {
            const auto& scale = candidateScales[i];
            const double z = z_distance(
                scale.estimate, scale.variance,
                runningEstimate, runningVariance);
            if (!std::isfinite(z)) return false;
            out.maxCrossScaleDisagreementSigma = std::max(
                out.maxCrossScaleDisagreementSigma, z);
            if (z > kCrossScaleAgreementSigma) {
                ++out.scalesRejected;
                continue;
            }

            acceptedScales.push_back(scale);
            ++out.scalesAccepted;
            out.coarsestAcceptedRadius = scale.radius;
            if (!inverse_variance_combine_scales(
                    acceptedScales,
                    runningEstimate,
                    runningVariance,
                    runningWeight)) {
                return false;
            }
        }

        if (acceptedScales.empty()) return true;
        if (!inverse_variance_combine_scales(
                acceptedScales,
                out.estimate,
                out.estimateVariance,
                out.effectiveWeight)) {
            return false;
        }

        out.valid = true;
        return out.centerExcluded && !out.createsNewEvidence &&
               !out.scientificWritebackAllowed;
    } catch (...) {
        out = Result{};
        return false;
    }
}

}  // namespace truthraw::truthnegative_center_excluded_neighborhood::v0_2

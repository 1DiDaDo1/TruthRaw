#include "free_world_pixel_resolve_2d_v0_2.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <utility>

namespace truthraw::free_world_pixel_resolve_2d::v0_2 {
namespace {

inline bool finiteNonNegative(double v) noexcept {
    return std::isfinite(v) && v >= 0.0;
}

inline void addWeight(
    std::vector<double>& weights,
    std::uint32_t index,
    double value) {
    if (value > 0.0) weights[index] += value;
}

ResolvedAuthority resolveAuthority(const ChannelSupportSummary& s) noexcept {
    constexpr double kEps = 1e-15;
    if (s.unknownWeight > kEps) return ResolvedAuthority::Unknown;
    if (s.censoredWeight > kEps) return ResolvedAuthority::Censored;
    return ResolvedAuthority::Reconstructed;
}

void accumulateAuthority(
    ChannelSupportSummary& summary,
    SourceAuthority authority,
    double weight) noexcept {
    switch (authority) {
        case SourceAuthority::CalibratedEstimate:
            summary.calibratedEstimateWeight += weight;
            break;
        case SourceAuthority::Reconstructed:
            summary.reconstructedWeight += weight;
            break;
        case SourceAuthority::Censored:
            summary.censoredWeight += weight;
            break;
        case SourceAuthority::Unknown:
            summary.unknownWeight += weight;
            break;
    }
}

}  // namespace

std::vector<AxisContribution> axisAreaWeights(
    std::uint32_t sourceCount,
    std::uint32_t targetIndex,
    std::uint32_t targetCount) {
    if (sourceCount == 0u || targetCount == 0u || targetIndex >= targetCount) {
        return {};
    }

    const double scale =
        static_cast<double>(sourceCount) / static_cast<double>(targetCount);
    const double a = static_cast<double>(targetIndex) * scale - 0.5;
    const double b = static_cast<double>(targetIndex + 1u) * scale - 0.5;
    const double length = b - a;
    if (!(length > 0.0) || !std::isfinite(length)) return {};

    std::vector<double> weights(sourceCount, 0.0);
    if (sourceCount == 1u) {
        weights[0] = length;
    } else {
        if (a < 0.0) {
            addWeight(weights, 0u, std::max(0.0, std::min(b, 0.0) - a));
        }

        const double last = static_cast<double>(sourceCount - 1u);
        const double interiorA = std::max(a, 0.0);
        const double interiorB = std::min(b, last);

        if (interiorB > interiorA) {
            std::uint32_t segment =
                static_cast<std::uint32_t>(std::floor(interiorA));
            if (segment >= sourceCount - 1u) segment = sourceCount - 2u;

            double left = interiorA;
            while (left < interiorB) {
                const double segmentEnd =
                    std::min(interiorB, static_cast<double>(segment + 1u));
                const double u0 = left - static_cast<double>(segment);
                const double u1 = segmentEnd - static_cast<double>(segment);
                const double delta = segmentEnd - left;
                const double rightIntegral =
                    0.5 * (u1 * u1 - u0 * u0);
                const double leftIntegral = delta - rightIntegral;
                addWeight(weights, segment, leftIntegral);
                addWeight(weights, segment + 1u, rightIntegral);
                left = segmentEnd;
                if (left >= interiorB) break;
                ++segment;
                if (segment >= sourceCount - 1u) break;
            }
        }

        if (b > last) {
            addWeight(
                weights,
                sourceCount - 1u,
                b - std::max(a, last));
        }
    }

    std::vector<AxisContribution> out;
    out.reserve(sourceCount);
    double sum = 0.0;
    for (std::uint32_t i = 0u; i < sourceCount; ++i) {
        if (weights[i] <= 0.0) continue;
        const double normalized = weights[i] / length;
        if (!finiteNonNegative(normalized)) return {};
        out.push_back({i, normalized});
        sum += normalized;
    }
    if (out.empty() || !std::isfinite(sum) || std::abs(sum - 1.0) > 1e-12) {
        return {};
    }

    if (std::abs(sum - 1.0) > 0.0) {
        for (auto& item : out) item.weight /= sum;
    }
    return out;
}

Resolver::Resolver(
    const IScenePlaneSource& source,
    std::uint32_t targetWidth,
    std::uint32_t targetHeight) noexcept
    : source_(source) {
    geometry_.sourceWidth = source.width();
    geometry_.sourceHeight = source.height();
    geometry_.targetWidth = targetWidth;
    geometry_.targetHeight = targetHeight;

    if (geometry_.sourceWidth == 0u || geometry_.sourceHeight == 0u ||
        targetWidth == 0u || targetHeight == 0u) {
        error_ = "free-world resolve geometry must be non-zero";
        return;
    }
    valid_ = true;
}

bool Resolver::valid() const noexcept {
    return valid_;
}

const std::string& Resolver::error() const noexcept {
    return error_;
}

Geometry Resolver::geometry() const noexcept {
    return geometry_;
}

bool Resolver::resolvePixel(
    std::uint32_t targetX,
    std::uint32_t targetY,
    ResolvedPixel& out) const noexcept {
    try {
        out = ResolvedPixel{};
        if (!valid_ || targetX >= geometry_.targetWidth ||
            targetY >= geometry_.targetHeight) {
            return false;
        }

        const auto xWeights = axisAreaWeights(
            geometry_.sourceWidth, targetX, geometry_.targetWidth);
        const auto yWeights = axisAreaWeights(
            geometry_.sourceHeight, targetY, geometry_.targetHeight);
        if (xWeights.empty() || yWeights.empty()) return false;

        out.footprint.reserve(xWeights.size() * yWeights.size());
        std::array<bool, 3u> uncertaintyKnown = {true, true, true};
        double footprintSum = 0.0;

        for (const auto& yw : yWeights) {
            for (const auto& xw : xWeights) {
                const double w = xw.weight * yw.weight;
                if (!(w > 0.0) || !std::isfinite(w)) return false;

                SourcePixel sourcePixel{};
                if (!source_.readPixel(xw.index, yw.index, sourcePixel)) {
                    return false;
                }

                out.footprint.push_back({xw.index, yw.index, w});
                footprintSum += w;

                for (std::size_t c = 0u; c < 3u; ++c) {
                    const auto& src = sourcePixel.channel[c];
                    if (!std::isfinite(src.value)) return false;
                    out.sceneLinear[c] += src.value * w;
                    accumulateAuthority(out.support[c], src.authority, w);

                    if (!src.uncertaintyKnown) {
                        uncertaintyKnown[c] = false;
                    } else {
                        if (!finiteNonNegative(src.p95Uncertainty)) return false;
                        out.support[c].p95Uncertainty +=
                            src.p95Uncertainty * w;
                    }
                }
            }
        }

        if (std::abs(footprintSum - 1.0) > 1e-12) return false;

        for (std::size_t c = 0u; c < 3u; ++c) {
            auto& support = out.support[c];
            const double authorityWeight =
                support.calibratedEstimateWeight +
                support.reconstructedWeight +
                support.censoredWeight +
                support.unknownWeight;
            if (std::abs(authorityWeight - 1.0) > 1e-12) return false;
            support.uncertaintyKnown = uncertaintyKnown[c];
            if (!uncertaintyKnown[c]) support.p95Uncertainty = 0.0;
            support.authority = resolveAuthority(support);
        }

        return true;
    } catch (...) {
        out = ResolvedPixel{};
        return false;
    }
}

bool Resolver::resolveRaster(
    std::vector<ResolvedPixel>& out,
    bool includeFootprints) const noexcept {
    try {
        if (!valid_) return false;
        const std::uint64_t count =
            static_cast<std::uint64_t>(geometry_.targetWidth) *
            static_cast<std::uint64_t>(geometry_.targetHeight);
        if (count > static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max())) {
            return false;
        }
        out.clear();
        out.resize(static_cast<std::size_t>(count));

        for (std::uint32_t y = 0u; y < geometry_.targetHeight; ++y) {
            for (std::uint32_t x = 0u; x < geometry_.targetWidth; ++x) {
                auto& pixel = out[
                    static_cast<std::size_t>(y) * geometry_.targetWidth + x];
                if (!resolvePixel(x, y, pixel)) {
                    out.clear();
                    return false;
                }
                if (!includeFootprints) {
                    std::vector<FootprintContribution>().swap(pixel.footprint);
                }
            }
        }
        return true;
    } catch (...) {
        out.clear();
        return false;
    }
}

OutputIntentBinding bindOutputIntent(
    const std::string& sceneStateId,
    const ViewDisplayPolicy& policy) {
    OutputIntentBinding out{};
    out.sceneStateId = sceneStateId;
    out.appearanceModelId = policy.appearanceModelId;
    out.displayTargetId = policy.displayTargetId;
    out.scientificSceneMutationAllowed = false;
    return out;
}

const char* toString(SourceAuthority authority) noexcept {
    switch (authority) {
        case SourceAuthority::CalibratedEstimate: return "CALIBRATED_ESTIMATE";
        case SourceAuthority::Reconstructed: return "RECONSTRUCTED";
        case SourceAuthority::Censored: return "CENSORED";
        case SourceAuthority::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

const char* toString(ResolvedAuthority authority) noexcept {
    switch (authority) {
        case ResolvedAuthority::Reconstructed: return "RECONSTRUCTED";
        case ResolvedAuthority::Censored: return "CENSORED";
        case ResolvedAuthority::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

}  // namespace truthraw::free_world_pixel_resolve_2d::v0_2

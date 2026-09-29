#include "anchor_constrained_local_reconstruction_v0_1.h"

#include "full_frame_streaming_v0_1_internal.h"
#include "truthnegative_center_excluded_neighborhood_v0_2.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace truthraw::anchor_constrained_local_reconstruction::v0_1 {
namespace {

namespace detail = truthraw::streaming_v0_1::detail;
namespace ce = truthraw::truthnegative_center_excluded_neighborhood::v0_2;

constexpr std::array<int,3u> kBaselineRadii{{2,4,8}};
constexpr std::array<std::array<int,2u>,4u> kBaselineDirections{{
    {{1,0}},{{0,1}},{{1,1}},{{1,-1}}
}};

bool nonzero(const Digest& d) noexcept {
    return std::any_of(
        d.begin(), d.end(), [](std::uint8_t v){ return v != 0u; });
}

void hash_u32(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint32_t v) noexcept {
    const std::array<std::uint8_t,4u> b{
        static_cast<std::uint8_t>(v),
        static_cast<std::uint8_t>(v >> 8u),
        static_cast<std::uint8_t>(v >> 16u),
        static_cast<std::uint8_t>(v >> 24u)};
    h.update(b);
}

void hash_u64(
    truthraw::sha256_v0_69::Hasher& h,
    std::uint64_t v) noexcept {
    std::array<std::uint8_t,8u> b{};
    for (std::size_t i = 0u; i < 8u; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8u * i));
    }
    h.update(b);
}

void hash_f64(
    truthraw::sha256_v0_69::Hasher& h,
    double v) noexcept {
    hash_u64(h, std::bit_cast<std::uint64_t>(v));
}

std::string hex(const Digest& d) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(d.size() * 2u, '0');
    for (std::size_t i = 0u; i < d.size(); ++i) {
        out[2u * i] = kHex[d[i] >> 4u];
        out[2u * i + 1u] = kHex[d[i] & 0x0fu];
    }
    return out;
}

bool valid_noise_profile(const DngMetadata& md) noexcept {
    if (!md.hasNoiseProfile) return false;
    for (float v : md.noiseProfile) {
        if (!std::isfinite(v) || v < 0.0f) return false;
    }
    return true;
}

int measured_channel(CfaPattern cfa, int x, int y) noexcept {
    const int phase = (y & 1) * 2 + (x & 1);
    static constexpr int bggr[4] = {2,1,1,0};
    static constexpr int rggb[4] = {0,1,1,2};
    static constexpr int grbg[4] = {1,0,2,1};
    static constexpr int gbrg[4] = {1,2,0,1};
    const int* map = bggr;
    switch (cfa) {
        case CfaPattern::RGGB: map = rggb; break;
        case CfaPattern::GRBG: map = grbg; break;
        case CfaPattern::GBRG: map = gbrg; break;
        case CfaPattern::BGGR: map = bggr; break;
    }
    return map[phase];
}

bool variance_for(
    const DngMetadata& md,
    const detail::Workspace& w,
    std::size_t i,
    int channel,
    double mu,
    double& variance) noexcept {
    variance = 0.0;
    if (!md.hasNoiseProfile || channel < 0 || channel > 2 ||
        i >= w.stage2.size()) {
        return false;
    }
    const double g = md.hasGainField
        ? (i < w.gain.size()
               ? static_cast<double>(w.gain[i])
               : std::numeric_limits<double>::quiet_NaN())
        : 1.0;
    if (!std::isfinite(g) || !(g > 0.0) || !std::isfinite(mu)) return false;
    const double s = md.noiseProfile[2 * channel];
    const double o = md.noiseProfile[2 * channel + 1];
    variance = g * s * std::max(mu, 0.0) + g * g * o;
    return std::isfinite(variance) && variance > 0.0;
}

bool valid_queries(
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<QueryRegion>& queries) noexcept {
    if (queries.empty() || queries.size() > 512u) return false;
    for (const auto& q : queries) {
        if (q.right <= q.left || q.bottom <= q.top ||
            q.left >= width || q.top >= height ||
            q.right > width || q.bottom > height ||
            q.frontsideWidth == 0u || q.frontsideHeight == 0u) {
            return false;
        }
    }
    return true;
}

bool invert3x3(
    const double input[3][3],
    double inverse[3][3]) noexcept {
    double a[3][6]{};
    double scale = 0.0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            a[r][c] = input[r][c];
            scale = std::max(scale, std::abs(input[r][c]));
        }
        a[r][r + 3] = 1.0;
    }
    if (!(scale > 0.0) || !std::isfinite(scale)) return false;

    const double pivotFloor = scale * 1.0e-12;
    for (int col = 0; col < 3; ++col) {
        int pivot = col;
        for (int r = col + 1; r < 3; ++r) {
            if (std::abs(a[r][col]) > std::abs(a[pivot][col])) pivot = r;
        }
        if (!std::isfinite(a[pivot][col]) ||
            std::abs(a[pivot][col]) <= pivotFloor) {
            return false;
        }
        if (pivot != col) {
            for (int c = 0; c < 6; ++c) {
                std::swap(a[pivot][c], a[col][c]);
            }
        }
        const double d = a[col][col];
        for (int c = 0; c < 6; ++c) a[col][c] /= d;
        for (int r = 0; r < 3; ++r) {
            if (r == col) continue;
            const double f = a[r][col];
            for (int c = 0; c < 6; ++c) a[r][c] -= f * a[col][c];
        }
    }

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            inverse[r][c] = a[r][c + 3];
            if (!std::isfinite(inverse[r][c])) return false;
        }
    }
    return true;
}

struct PlaneSample final {
    double x = 0.0;
    double y = 0.0;
    double value = 0.0;
    double variance = 0.0;
};

struct PlaneResult final {
    double estimate = 0.0;
    double predictionVariance = 0.0;
    double varianceInflation = 0.0;
    std::uint32_t samples = 0u;
    bool valid = false;
};

bool solve_plane(
    const std::vector<PlaneSample>& samples,
    PlaneResult& out) noexcept {
    out = {};
    if (samples.size() < kMinPlaneSamples) return true;

    double normal[3][3]{};
    double rhs[3]{};
    for (const auto& s : samples) {
        if (!std::isfinite(s.x) || !std::isfinite(s.y) ||
            !std::isfinite(s.value) ||
            !std::isfinite(s.variance) || !(s.variance > 0.0)) {
            return false;
        }
        const double w = 1.0 / s.variance;
        const double v[3]{1.0, s.x, s.y};
        for (int r = 0; r < 3; ++r) {
            rhs[r] += w * v[r] * s.value;
            for (int c = 0; c < 3; ++c) {
                normal[r][c] += w * v[r] * v[c];
            }
        }
    }

    double inv[3][3]{};
    if (!invert3x3(normal, inv)) return true;

    double beta[3]{};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) beta[r] += inv[r][c] * rhs[c];
        if (!std::isfinite(beta[r])) return false;
    }

    long double weightedSse = 0.0L;
    for (const auto& s : samples) {
        const double predicted = beta[0] + beta[1] * s.x + beta[2] * s.y;
        const double residual = s.value - predicted;
        if (!std::isfinite(predicted) || !std::isfinite(residual)) return false;
        weightedSse +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual) /
            static_cast<long double>(s.variance);
    }

    const std::uint32_t dof =
        static_cast<std::uint32_t>(samples.size() - 3u);
    if (dof == 0u) return true;
    const double normalizedResidual =
        static_cast<double>(weightedSse / static_cast<long double>(dof));
    if (!std::isfinite(normalizedResidual) || normalizedResidual < 0.0) {
        return false;
    }

    const double inflation = std::max(1.0, normalizedResidual);
    const double predictionVariance = inv[0][0] * inflation;
    if (!std::isfinite(predictionVariance) ||
        !(predictionVariance > 0.0) ||
        !std::isfinite(inflation)) {
        return true;
    }

    out.estimate = beta[0];
    out.predictionVariance = predictionVariance;
    out.varianceInflation = inflation;
    out.samples = static_cast<std::uint32_t>(samples.size());
    out.valid = true;
    return true;
}

void add_error(
    Metrics& m,
    double actual,
    double estimate,
    bool solver,
    double predictionVariance,
    double targetVariance,
    double inflation) noexcept {
    const double signedError = estimate - actual;
    const double absError = std::abs(signedError);
    if (solver) {
        ++m.solverValid;
        m.solverAbsErrorSum += absError;
        m.solverSquaredErrorSum +=
            static_cast<long double>(signedError) * signedError;
        m.solverSignedErrorSum += signedError;
        m.solverMaxAbsError = std::max(m.solverMaxAbsError, absError);
        m.solverPredictionVarianceSum += predictionVariance;
        m.solverVarianceInflationSum += inflation;
        const double combined = targetVariance + predictionVariance;
        if (combined > 0.0 && std::isfinite(combined)) {
            const double z = absError / std::sqrt(combined);
            if (std::isfinite(z)) {
                if (z <= 1.0) ++m.solverWithin1Sigma;
                if (z <= 2.0) ++m.solverWithin2Sigma;
                if (z <= 3.0) ++m.solverWithin3Sigma;
                m.solverMaxCombinedZ = std::max(m.solverMaxCombinedZ, z);
            }
        }
    } else {
        ++m.baselineValid;
        m.baselineAbsErrorSum += absError;
        m.baselineSquaredErrorSum +=
            static_cast<long double>(signedError) * signedError;
        m.baselineSignedErrorSum += signedError;
        m.baselineMaxAbsError = std::max(m.baselineMaxAbsError, absError);
    }
}

void accumulate(Metrics& a, const Metrics& b) noexcept {
    a.holdouts += b.holdouts;
    a.targetCensoredSkipped += b.targetCensoredSkipped;
    a.solverValid += b.solverValid;
    a.solverInvalid += b.solverInvalid;
    a.baselineValid += b.baselineValid;
    a.baselineInvalid += b.baselineInvalid;
    a.bothValid += b.bothValid;
    a.solverLowerAbsError += b.solverLowerAbsError;
    a.baselineLowerAbsError += b.baselineLowerAbsError;
    a.equalAbsError += b.equalAbsError;
    a.solverWithin1Sigma += b.solverWithin1Sigma;
    a.solverWithin2Sigma += b.solverWithin2Sigma;
    a.solverWithin3Sigma += b.solverWithin3Sigma;
    for (std::size_t i = 0u; i < 4u; ++i) {
        a.holdoutCfaPhase[i] += b.holdoutCfaPhase[i];
        a.solverValidCfaPhase[i] += b.solverValidCfaPhase[i];
    }
    a.solverAbsErrorSum += b.solverAbsErrorSum;
    a.solverSquaredErrorSum += b.solverSquaredErrorSum;
    a.solverSignedErrorSum += b.solverSignedErrorSum;
    a.baselineAbsErrorSum += b.baselineAbsErrorSum;
    a.baselineSquaredErrorSum += b.baselineSquaredErrorSum;
    a.baselineSignedErrorSum += b.baselineSignedErrorSum;
    a.solverPredictionVarianceSum += b.solverPredictionVarianceSum;
    a.solverVarianceInflationSum += b.solverVarianceInflationSum;
    a.solverMaxAbsError = std::max(a.solverMaxAbsError, b.solverMaxAbsError);
    a.baselineMaxAbsError =
        std::max(a.baselineMaxAbsError, b.baselineMaxAbsError);
    a.solverMaxCombinedZ =
        std::max(a.solverMaxCombinedZ, b.solverMaxCombinedZ);
    a.solverMaxDirectionalDisagreementSigma = std::max(
        a.solverMaxDirectionalDisagreementSigma,
        b.solverMaxDirectionalDisagreementSigma);
    a.solverMaxCrossScaleDisagreementSigma = std::max(
        a.solverMaxCrossScaleDisagreementSigma,
        b.solverMaxCrossScaleDisagreementSigma);
}

double mean(long double sum, std::uint64_t n) noexcept {
    return n == 0u ? 0.0
                   : static_cast<double>(sum / static_cast<long double>(n));
}

double rmse(long double sumSq, std::uint64_t n) noexcept {
    if (n == 0u) return 0.0;
    const long double v = sumSq / static_cast<long double>(n);
    return v <= 0.0L ? 0.0 : std::sqrt(static_cast<double>(v));
}

void write_u64_array(
    std::ostringstream& o,
    const std::array<std::uint64_t,4u>& values) {
    o << "[";
    for (std::size_t i = 0u; i < values.size(); ++i) {
        if (i) o << ",";
        o << values[i];
    }
    o << "]";
}

void write_metrics(std::ostringstream& o, const Metrics& m) {
    o << "\"holdouts\":" << m.holdouts;
    o << ",\"target_censored_skipped\":" << m.targetCensoredSkipped;
    o << ",\"solver_valid\":" << m.solverValid;
    o << ",\"solver_invalid\":" << m.solverInvalid;
    o << ",\"baseline_valid\":" << m.baselineValid;
    o << ",\"baseline_invalid\":" << m.baselineInvalid;
    o << ",\"both_valid\":" << m.bothValid;
    o << ",\"solver_lower_abs_error\":" << m.solverLowerAbsError;
    o << ",\"baseline_lower_abs_error\":" << m.baselineLowerAbsError;
    o << ",\"equal_abs_error\":" << m.equalAbsError;
    o << ",\"solver_mae\":" << mean(m.solverAbsErrorSum, m.solverValid);
    o << ",\"solver_rmse\":" << rmse(m.solverSquaredErrorSum, m.solverValid);
    o << ",\"solver_bias\":" << mean(m.solverSignedErrorSum, m.solverValid);
    o << ",\"solver_max_abs_error\":" << m.solverMaxAbsError;
    o << ",\"baseline_mae\":" << mean(m.baselineAbsErrorSum, m.baselineValid);
    o << ",\"baseline_rmse\":" << rmse(m.baselineSquaredErrorSum, m.baselineValid);
    o << ",\"baseline_bias\":" << mean(m.baselineSignedErrorSum, m.baselineValid);
    o << ",\"baseline_max_abs_error\":" << m.baselineMaxAbsError;
    o << ",\"solver_within_1sigma\":" << m.solverWithin1Sigma;
    o << ",\"solver_within_2sigma\":" << m.solverWithin2Sigma;
    o << ",\"solver_within_3sigma\":" << m.solverWithin3Sigma;
    o << ",\"solver_coverage_1sigma\":"
      << (m.solverValid == 0u ? 0.0
                              : static_cast<double>(m.solverWithin1Sigma) /
                                    static_cast<double>(m.solverValid));
    o << ",\"solver_coverage_2sigma\":"
      << (m.solverValid == 0u ? 0.0
                              : static_cast<double>(m.solverWithin2Sigma) /
                                    static_cast<double>(m.solverValid));
    o << ",\"solver_coverage_3sigma\":"
      << (m.solverValid == 0u ? 0.0
                              : static_cast<double>(m.solverWithin3Sigma) /
                                    static_cast<double>(m.solverValid));
    o << ",\"solver_mean_prediction_variance\":"
      << mean(m.solverPredictionVarianceSum, m.solverValid);
    o << ",\"solver_mean_variance_inflation\":"
      << mean(m.solverVarianceInflationSum, m.solverValid);
    o << ",\"solver_max_combined_z\":" << m.solverMaxCombinedZ;
    o << ",\"baseline_max_directional_disagreement_sigma\":"
      << m.solverMaxDirectionalDisagreementSigma;
    o << ",\"baseline_max_cross_scale_disagreement_sigma\":"
      << m.solverMaxCrossScaleDisagreementSigma;
    o << ",\"holdout_cfa_phase\":";
    write_u64_array(o, m.holdoutCfaPhase);
    o << ",\"solver_valid_cfa_phase\":";
    write_u64_array(o, m.solverValidCfaPhase);
}

} // namespace

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    const std::vector<QueryRegion>& queries,
    Report& out) noexcept {
    out = {};
    try {
        const auto& md = source.metadata();
        const auto width = static_cast<std::uint32_t>(md.width);
        const auto height = static_cast<std::uint32_t>(md.height);
        if (!nonzero(binding.sourceEvidenceSha256) ||
            !nonzero(binding.scientificMasterSha256) ||
            !nonzero(binding.authorityFieldSha256) ||
            !nonzero(binding.truthNegativeStateSha256) ||
            md.width <= 2 * kSupportRadius ||
            md.height <= 2 * kSupportRadius ||
            !valid_noise_profile(md) ||
            !valid_queries(width, height, queries)) {
            return false;
        }

        truthraw::sha256_v0_69::Hasher holdoutHasher;
        constexpr char domain[] =
            "D_RAW_ANCHOR_CONSTRAINED_LOCAL_RECONSTRUCTION_HOLDOUT_V0_1";
        holdoutHasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        holdoutHasher.update(binding.sourceEvidenceSha256);
        holdoutHasher.update(binding.truthNegativeStateSha256);
        hash_u32(holdoutHasher, kHoldoutPeriod);
        hash_u32(holdoutHasher, static_cast<std::uint32_t>(kSupportRadius));
        hash_u64(holdoutHasher, kLatticeUnitsPerSourcePixel);

        detail::Workspace workspace{};
        out.queries.reserve(queries.size());

        for (const auto& query : queries) {
            QueryMetrics qm{};
            qm.query = query;

            TileRect support{};
            support.x0 = static_cast<int>(query.left);
            support.y0 = static_cast<int>(query.top);
            support.x1 = static_cast<int>(query.right);
            support.y1 = static_cast<int>(query.bottom);
            support.hx0 = std::max(0, support.x0 - kSupportRadius);
            support.hy0 = std::max(0, support.y0 - kSupportRadius);
            support.hx1 = std::min(md.width, support.x1 + kSupportRadius);
            support.hy1 = std::min(md.height, support.y1 + kSupportRadius);

            const auto filled = detail::fill_stage2(source, support, workspace);
            if (!filled) return false;

            const int tw = support.hx1 - support.hx0;
            const int th = support.hy1 - support.hy0;
            const std::size_t expected =
                static_cast<std::size_t>(tw) *
                static_cast<std::size_t>(th);
            if (tw <= 0 || th <= 0 ||
                workspace.stage2.size() != expected ||
                workspace.raw.size() != expected ||
                (md.hasGainField && workspace.gain.size() != expected)) {
                return false;
            }

            const auto indexOf =
                [&](int gx, int gy, std::size_t& index) noexcept {
                    if (gx < support.hx0 || gy < support.hy0 ||
                        gx >= support.hx1 || gy >= support.hy1) {
                        return false;
                    }
                    index =
                        static_cast<std::size_t>(gy - support.hy0) *
                            static_cast<std::size_t>(tw) +
                        static_cast<std::size_t>(gx - support.hx0);
                    return index < expected;
                };

            const auto pathTouchesCensor =
                [&](int gx, int gy, int dx, int dy) noexcept {
                    const int adx = std::abs(dx);
                    const int ady = std::abs(dy);
                    const int steps = std::gcd(adx, ady);
                    if (steps <= 0) return true;
                    const int stepX = dx / steps;
                    const int stepY = dy / steps;
                    for (int s = 1; s <= steps; ++s) {
                        std::size_t i = 0u;
                        if (!indexOf(gx + s * stepX, gy + s * stepY, i)) {
                            return true;
                        }
                        if (static_cast<float>(workspace.raw[i]) >=
                            md.whiteLevel) {
                            return true;
                        }
                    }
                    return false;
                };

            for (int gy = support.y0; gy < support.y1; ++gy) {
                if (static_cast<std::uint32_t>(gy) % kHoldoutPeriod !=
                    static_cast<std::uint32_t>(gy & 1)) {
                    continue;
                }
                for (int gx = support.x0; gx < support.x1; ++gx) {
                    if (static_cast<std::uint32_t>(gx) % kHoldoutPeriod !=
                        static_cast<std::uint32_t>(gx & 1)) {
                        continue;
                    }

                    std::size_t centerIndex = 0u;
                    if (!indexOf(gx, gy, centerIndex)) return false;
                    if (static_cast<float>(workspace.raw[centerIndex]) >=
                        md.whiteLevel) {
                        ++qm.metrics.targetCensoredSkipped;
                        continue;
                    }

                    const int channel = measured_channel(md.cfa, gx, gy);
                    if (channel < 0 || channel > 2) return false;
                    const auto phase =
                        static_cast<std::uint32_t>((gy & 1) * 2 + (gx & 1));
                    const double actual = workspace.stage2[centerIndex];
                    double targetVariance = 0.0;
                    if (!std::isfinite(actual) ||
                        !variance_for(
                            md,
                            workspace,
                            centerIndex,
                            channel,
                            actual,
                            targetVariance)) {
                        return false;
                    }

                    ++qm.metrics.holdouts;
                    ++qm.metrics.holdoutCfaPhase[phase];

                    // New lattice solver: weighted affine local model using
                    // only other measured anchors of the exact same CFA phase.
                    // The held-out target value is never inserted here.
                    std::vector<PlaneSample> planeSamples;
                    planeSamples.reserve(80u);
                    for (int dy = -kSupportRadius;
                         dy <= kSupportRadius; dy += 2) {
                        for (int dx = -kSupportRadius;
                             dx <= kSupportRadius; dx += 2) {
                            if (dx == 0 && dy == 0) continue;
                            const int nx = gx + dx;
                            const int ny = gy + dy;
                            std::size_t ni = 0u;
                            if (!indexOf(nx, ny, ni)) continue;
                            if ((nx & 1) != (gx & 1) ||
                                (ny & 1) != (gy & 1) ||
                                measured_channel(md.cfa, nx, ny) != channel) {
                                return false;
                            }
                            const bool censored =
                                static_cast<float>(workspace.raw[ni]) >=
                                md.whiteLevel;
                            if (censored || pathTouchesCensor(gx, gy, dx, dy)) {
                                continue;
                            }
                            const double value = workspace.stage2[ni];
                            double variance = 0.0;
                            if (!std::isfinite(value) ||
                                !variance_for(
                                    md,
                                    workspace,
                                    ni,
                                    channel,
                                    value,
                                    variance)) {
                                continue;
                            }
                            PlaneSample s{};
                            s.x = static_cast<double>(dx) /
                                static_cast<double>(kSupportRadius);
                            s.y = static_cast<double>(dy) /
                                static_cast<double>(kSupportRadius);
                            s.value = value;
                            s.variance = variance;
                            planeSamples.push_back(s);
                        }
                    }

                    PlaneResult solver{};
                    if (!solve_plane(planeSamples, solver)) return false;

                    // Existing center-excluded v0.2 predictor is the
                    // raster-bound reference. It also excludes the target.
                    ce::Input baselineInput{};
                    baselineInput.neighbors.reserve(
                        kBaselineRadii.size() *
                        kBaselineDirections.size() * 2u);
                    for (const int radius : kBaselineRadii) {
                        for (const auto& direction : kBaselineDirections) {
                            for (const int side : {-1, 1}) {
                                const int dx =
                                    side * radius * direction[0];
                                const int dy =
                                    side * radius * direction[1];
                                const int nx = gx + dx;
                                const int ny = gy + dy;
                                std::size_t ni = 0u;
                                if (!indexOf(nx, ny, ni)) continue;
                                if ((nx & 1) != (gx & 1) ||
                                    (ny & 1) != (gy & 1) ||
                                    measured_channel(md.cfa, nx, ny) != channel) {
                                    return false;
                                }
                                const double value = workspace.stage2[ni];
                                const bool censored =
                                    static_cast<float>(workspace.raw[ni]) >=
                                    md.whiteLevel;
                                double variance = 0.0;
                                const bool varianceKnown =
                                    !censored &&
                                    std::isfinite(value) &&
                                    variance_for(
                                        md,
                                        workspace,
                                        ni,
                                        channel,
                                        value,
                                        variance);

                                ce::Sample sample{};
                                sample.value = value;
                                sample.variance = variance;
                                sample.dx = dx;
                                sample.dy = dy;
                                sample.authority = censored
                                    ? ce::SampleAuthority::Censored
                                    : ce::SampleAuthority::Measured;
                                sample.varianceKnown = varianceKnown;
                                sample.sameChannel = true;
                                sample.sameObject = true;
                                sample.objectIdentityKnown = false;
                                sample.censorBoundary =
                                    pathTouchesCensor(gx, gy, dx, dy);
                                baselineInput.neighbors.push_back(sample);
                            }
                        }
                    }

                    ce::Result baseline{};
                    if (!ce::estimate(baselineInput, baseline) ||
                        !baseline.centerExcluded ||
                        baseline.createsNewEvidence ||
                        baseline.scientificWritebackAllowed) {
                        return false;
                    }

                    HoldoutRecord record{};
                    record.queryId = query.id;
                    record.x = static_cast<std::uint32_t>(gx);
                    record.y = static_cast<std::uint32_t>(gy);
                    record.cfaPhase = phase;
                    record.channel = static_cast<std::uint32_t>(channel);
                    record.actual = actual;
                    record.targetVariance = targetVariance;

                    if (solver.valid) {
                        record.solverValid = true;
                        record.planeSamples = solver.samples;
                        record.solverEstimate = solver.estimate;
                        record.solverPredictionVariance =
                            solver.predictionVariance;
                        record.solverVarianceInflation =
                            solver.varianceInflation;
                        record.solverAbsError =
                            std::abs(solver.estimate - actual);
                        const double combined =
                            targetVariance + solver.predictionVariance;
                        if (!(combined > 0.0) || !std::isfinite(combined)) {
                            return false;
                        }
                        record.solverCombinedZ =
                            record.solverAbsError / std::sqrt(combined);
                        if (!std::isfinite(record.solverCombinedZ)) return false;
                        ++qm.metrics.solverValidCfaPhase[phase];
                        add_error(
                            qm.metrics,
                            actual,
                            solver.estimate,
                            true,
                            solver.predictionVariance,
                            targetVariance,
                            solver.varianceInflation);
                    } else {
                        ++qm.metrics.solverInvalid;
                    }

                    if (baseline.valid) {
                        record.baselineValid = true;
                        record.baselineEstimate = baseline.estimate;
                        record.baselinePredictionVariance =
                            baseline.estimateVariance;
                        record.baselineAbsError =
                            std::abs(baseline.estimate - actual);
                        add_error(
                            qm.metrics,
                            actual,
                            baseline.estimate,
                            false,
                            baseline.estimateVariance,
                            targetVariance,
                            1.0);
                        qm.metrics.solverMaxDirectionalDisagreementSigma =
                            std::max(
                                qm.metrics.solverMaxDirectionalDisagreementSigma,
                                baseline.maxDirectionalDisagreementSigma);
                        qm.metrics.solverMaxCrossScaleDisagreementSigma =
                            std::max(
                                qm.metrics.solverMaxCrossScaleDisagreementSigma,
                                baseline.maxCrossScaleDisagreementSigma);
                    } else {
                        ++qm.metrics.baselineInvalid;
                    }

                    if (solver.valid && baseline.valid) {
                        ++qm.metrics.bothValid;
                        const double se = record.solverAbsError;
                        const double be = record.baselineAbsError;
                        const double eps =
                            std::numeric_limits<double>::epsilon() *
                            std::max({1.0, se, be}) * 8.0;
                        if (se + eps < be) {
                            ++qm.metrics.solverLowerAbsError;
                        } else if (be + eps < se) {
                            ++qm.metrics.baselineLowerAbsError;
                        } else {
                            ++qm.metrics.equalAbsError;
                        }
                    }

                    hash_u32(holdoutHasher, query.id);
                    hash_u32(
                        holdoutHasher,
                        static_cast<std::uint32_t>(gx));
                    hash_u32(
                        holdoutHasher,
                        static_cast<std::uint32_t>(gy));
                    hash_u32(holdoutHasher, phase);
                    hash_u32(
                        holdoutHasher,
                        static_cast<std::uint32_t>(channel));
                    hash_f64(holdoutHasher, actual);
                    hash_f64(holdoutHasher, targetVariance);
                    hash_u32(
                        holdoutHasher,
                        solver.valid ? 1u : 0u);
                    if (solver.valid) {
                        hash_u32(holdoutHasher, solver.samples);
                        hash_f64(holdoutHasher, solver.estimate);
                        hash_f64(
                            holdoutHasher,
                            solver.predictionVariance);
                        hash_f64(
                            holdoutHasher,
                            solver.varianceInflation);
                    }
                    hash_u32(
                        holdoutHasher,
                        baseline.valid ? 1u : 0u);
                    if (baseline.valid) {
                        hash_f64(holdoutHasher, baseline.estimate);
                        hash_f64(
                            holdoutHasher,
                            baseline.estimateVariance);
                    }

                    out.holdouts.push_back(record);
                }
            }

            if (qm.metrics.holdouts == 0u ||
                qm.metrics.solverValid + qm.metrics.solverInvalid !=
                    qm.metrics.holdouts ||
                qm.metrics.baselineValid + qm.metrics.baselineInvalid !=
                    qm.metrics.holdouts) {
                return false;
            }

            accumulate(out.global, qm.metrics);
            out.queries.push_back(qm);
        }

        if (out.queries.size() != queries.size() ||
            out.global.holdouts == 0u ||
            out.holdouts.size() != out.global.holdouts) {
            return false;
        }

        out.holdoutStreamSha256 = holdoutHasher.finalize();
        if (!nonzero(out.holdoutStreamSha256)) return false;

        std::ostringstream o;
        o.setf(std::ios::fixed);
        o << std::setprecision(12);
        o << "{\n";
        o << "  \"schema\":\"" << kSchemaName << "\",\n";
        o << "  \"source_sha256\":\""
          << hex(binding.sourceEvidenceSha256) << "\",\n";
        o << "  \"scientific_master_sha256\":\""
          << hex(binding.scientificMasterSha256) << "\",\n";
        o << "  \"authority_field_sha256\":\""
          << hex(binding.authorityFieldSha256) << "\",\n";
        o << "  \"truthnegative_state_sha256\":\""
          << hex(binding.truthNegativeStateSha256) << "\",\n";
        o << "  \"lattice_fraction_bits\":" << kLatticeFractionBits << ",\n";
        o << "  \"lattice_units_per_source_pixel\":"
          << kLatticeUnitsPerSourcePixel << ",\n";
        o << "  \"holdout_period\":" << kHoldoutPeriod << ",\n";
        o << "  \"solver_support_radius_source_px\":"
          << kSupportRadius << ",\n";
        o << "  \"solver_name\":\"INVERSE_VARIANCE_LOCAL_AFFINE_PLANE_V0_1\",\n";
        o << "  \"baseline_name\":\"CENTER_EXCLUDED_MULTISCALE_V0_2\",\n";
        o << "  \"held_out_target_authority\":\"MEASURED_SOURCE_BOUND_STAGE2\",\n";
        o << "  \"solver_output_authority\":\"RECONSTRUCTED_PRIVATE_AUDIT_ONLY\",\n";
        o << "  \"target_value_used_by_solver\":false,\n";
        o << "  \"measured_anchors_modified\":false,\n";
        o << "  \"unanchored_values_promoted_to_measured\":false,\n";
        o << "  \"reconstructed_authority_only\":true,\n";
        o << "  \"uncertainty_diagnostic_only\":true,\n";
        o << "  \"noise_independence_admitted\":false,\n";
        o << "  \"solver_applied_to_scientific_master\":false,\n";
        o << "  \"candidate_applied\":false,\n";
        o << "  \"creates_new_evidence\":false,\n";
        o << "  \"scientific_writeback_allowed\":false,\n";
        o << "  \"holdout_stream_sha256\":\""
          << hex(out.holdoutStreamSha256) << "\",\n";
        o << "  \"global\":{";
        write_metrics(o, out.global);
        o << "},\n";
        o << "  \"queries\":[\n";
        for (std::size_t i = 0u; i < out.queries.size(); ++i) {
            const auto& q = out.queries[i];
            o << "    {\"id\":" << q.query.id
              << ",\"frontside_x\":" << q.query.frontsideX
              << ",\"frontside_y\":" << q.query.frontsideY
              << ",\"frontside_width\":" << q.query.frontsideWidth
              << ",\"frontside_height\":" << q.query.frontsideHeight
              << ",\"source_rect\":["
              << q.query.left << "," << q.query.top << ","
              << q.query.right << "," << q.query.bottom << "],";
            write_metrics(o, q.metrics);
            o << "}";
            if (i + 1u < out.queries.size()) o << ",";
            o << "\n";
        }
        o << "  ],\n";
        o << "  \"holdout_records\":[\n";
        for (std::size_t i = 0u; i < out.holdouts.size(); ++i) {
            const auto& h = out.holdouts[i];
            o << "    {\"query_id\":" << h.queryId
              << ",\"x\":" << h.x
              << ",\"y\":" << h.y
              << ",\"lattice_u\":"
              << static_cast<std::uint64_t>(h.x) *
                    kLatticeUnitsPerSourcePixel
              << ",\"lattice_v\":"
              << static_cast<std::uint64_t>(h.y) *
                    kLatticeUnitsPerSourcePixel
              << ",\"cfa_phase\":" << h.cfaPhase
              << ",\"channel\":" << h.channel
              << ",\"actual\":" << h.actual
              << ",\"target_variance\":" << h.targetVariance
              << ",\"solver_valid\":"
              << (h.solverValid ? "true" : "false")
              << ",\"solver_plane_samples\":" << h.planeSamples;
            if (h.solverValid) {
                o << ",\"solver_estimate\":" << h.solverEstimate
                  << ",\"solver_prediction_variance\":"
                  << h.solverPredictionVariance
                  << ",\"solver_variance_inflation\":"
                  << h.solverVarianceInflation
                  << ",\"solver_abs_error\":" << h.solverAbsError
                  << ",\"solver_combined_z\":" << h.solverCombinedZ;
            }
            o << ",\"baseline_valid\":"
              << (h.baselineValid ? "true" : "false");
            if (h.baselineValid) {
                o << ",\"baseline_estimate\":" << h.baselineEstimate
                  << ",\"baseline_prediction_variance\":"
                  << h.baselinePredictionVariance
                  << ",\"baseline_abs_error\":" << h.baselineAbsError;
            }
            o << "}";
            if (i + 1u < out.holdouts.size()) o << ",";
            o << "\n";
        }
        o << "  ]\n";
        o << "}\n";

        out.json = o.str();
        truthraw::sha256_v0_69::Hasher jsonHasher;
        jsonHasher.update(
            reinterpret_cast<const std::uint8_t*>(out.json.data()),
            out.json.size());
        out.jsonSha256 = jsonHasher.finalize();

        out.targetValueUsedBySolver = false;
        out.measuredAnchorsModified = false;
        out.unanchoredValuesPromotedToMeasured = false;
        out.reconstructedAuthorityOnly = true;
        out.uncertaintyDiagnosticOnly = true;
        out.noiseIndependenceAdmitted = false;
        out.solverAppliedToScientificMaster = false;
        out.candidateApplied = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;

        return !out.json.empty() &&
               nonzero(out.jsonSha256) &&
               !out.targetValueUsedBySolver &&
               !out.measuredAnchorsModified &&
               !out.unanchoredValuesPromotedToMeasured &&
               out.reconstructedAuthorityOnly &&
               out.uncertaintyDiagnosticOnly &&
               !out.noiseIndependenceAdmitted &&
               !out.solverAppliedToScientificMaster &&
               !out.candidateApplied &&
               !out.createsNewEvidence &&
               !out.scientificWritebackAllowed;
    } catch (...) {
        out = {};
        return false;
    }
}

} // namespace truthraw::anchor_constrained_local_reconstruction::v0_1

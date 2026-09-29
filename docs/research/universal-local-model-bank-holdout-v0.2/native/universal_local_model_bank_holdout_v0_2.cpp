#include "universal_local_model_bank_holdout_v0_2.h"

#include "full_frame_streaming_v0_2_internal.h"
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

namespace truthraw::universal_local_model_bank_holdout::v0_2 {
namespace {

namespace detail = truthraw::streaming_v0_2::detail;
namespace ce = truthraw::truthnegative_center_excluded_neighborhood::v0_2;

constexpr std::array<int,3u> kBaselineRadii{{2,4,8}};
constexpr std::array<std::array<int,2u>,4u> kDirections{{
    {{1,0}},{{0,1}},{{1,1}},{{1,-1}}
}};
constexpr std::uint32_t kMinConstantSamples = 8u;
constexpr std::uint32_t kMinDirectionalSamples = 8u;
constexpr std::uint32_t kMinAffineSamples = 12u;
constexpr std::uint32_t kMinQuadraticSamples = 24u;
constexpr std::uint32_t kMinCrossfitConstantTrain = 12u;
constexpr std::uint32_t kMinCrossfitDirectionalTrain = 10u;
constexpr std::uint32_t kMinCrossfitAffineTrain = 16u;
constexpr std::uint32_t kMinCrossfitQuadraticTrain = 32u;
constexpr std::uint32_t kMinCrossfitValidation = 8u;
constexpr std::uint32_t kMinCrossfitDirectionalValidation = 4u;
constexpr double kDirectionalStripHalfWidthSourcePx = 2.0;
constexpr double kScoreEpsilon = 1.0e-18;

struct Sample final {
    int dx = 0;
    int dy = 0;
    double x = 0.0;
    double y = 0.0;
    double value = 0.0;
    bool validation = false;
};

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

const char* model_name(ModelId id) noexcept {
    switch (id) {
        case ModelId::RobustMedianConstant:
            return "ROBUST_MEDIAN_CONSTANT";
        case ModelId::DirectionalStripLine:
            return "DIRECTIONAL_STRIP_LINE";
        case ModelId::AffinePlane:
            return "AFFINE_PLANE";
        case ModelId::QuadraticSurface:
            return "QUADRATIC_SURFACE";
        case ModelId::None:
        default:
            return "NO_RECONSTRUCTION";
    }
}

std::size_t model_index(ModelId id) noexcept {
    const auto i = static_cast<std::size_t>(id);
    return i < 5u ? i : 0u;
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

bool valid_noise_profile(const DngMetadata& md) noexcept {
    if (!md.hasNoiseProfile) return false;
    for (float v : md.noiseProfile) {
        if (!std::isfinite(v) || v < 0.0f) return false;
    }
    return true;
}

bool variance_for(
    const DngMetadata& md,
    const detail::Workspace& w,
    std::size_t i,
    int channel,
    double mu,
    double& variance) noexcept {
    variance = 0.0;
    if (!valid_noise_profile(md) ||
        channel < 0 || channel > 2 ||
        i >= w.stage2.size()) {
        return false;
    }
    const double g = md.hasGainField
        ? (i < w.gain.size()
               ? static_cast<double>(w.gain[i])
               : std::numeric_limits<double>::quiet_NaN())
        : 1.0;
    if (!std::isfinite(g) || !(g > 0.0) || !std::isfinite(mu)) {
        return false;
    }
    const double s = md.noiseProfile[2 * channel];
    const double offset = md.noiseProfile[2 * channel + 1];
    variance =
        g * s * std::max(mu, 0.0) +
        g * g * offset;
    return std::isfinite(variance) && variance > 0.0;
}

double score_from_sse(
    double sse,
    std::size_t n,
    std::size_t parameters) noexcept {
    if (!std::isfinite(sse) || n <= parameters || parameters == 0u) {
        return std::numeric_limits<double>::infinity();
    }
    const double mse = std::max(
        sse / static_cast<double>(n),
        kScoreEpsilon);
    const double score =
        static_cast<double>(n) * std::log(mse) +
        static_cast<double>(parameters) *
            std::log(static_cast<double>(n));
    return std::isfinite(score)
        ? score
        : std::numeric_limits<double>::infinity();
}

bool invert_normal(
    const std::array<std::array<double,6u>,6u>& input,
    std::size_t p,
    std::array<std::array<double,6u>,6u>& inverse) noexcept {
    inverse = {};
    if (p == 0u || p > 6u) return false;

    std::array<std::array<double,12u>,6u> a{};
    double scale = 0.0;
    for (std::size_t r = 0u; r < p; ++r) {
        for (std::size_t c = 0u; c < p; ++c) {
            a[r][c] = input[r][c];
            scale = std::max(scale, std::abs(input[r][c]));
        }
        a[r][p + r] = 1.0;
    }
    if (!(scale > 0.0) || !std::isfinite(scale)) return false;

    const double pivotFloor = scale * 1.0e-12;
    for (std::size_t col = 0u; col < p; ++col) {
        std::size_t pivot = col;
        for (std::size_t r = col + 1u; r < p; ++r) {
            if (std::abs(a[r][col]) > std::abs(a[pivot][col])) {
                pivot = r;
            }
        }
        if (!std::isfinite(a[pivot][col]) ||
            std::abs(a[pivot][col]) <= pivotFloor) {
            return false;
        }
        if (pivot != col) std::swap(a[pivot], a[col]);

        const double d = a[col][col];
        for (std::size_t c = 0u; c < 2u * p; ++c) {
            a[col][c] /= d;
        }
        for (std::size_t r = 0u; r < p; ++r) {
            if (r == col) continue;
            const double f = a[r][col];
            for (std::size_t c = 0u; c < 2u * p; ++c) {
                a[r][c] -= f * a[col][c];
            }
        }
    }

    for (std::size_t r = 0u; r < p; ++r) {
        for (std::size_t c = 0u; c < p; ++c) {
            inverse[r][c] = a[r][p + c];
            if (!std::isfinite(inverse[r][c])) return false;
        }
    }
    return true;
}

void basis_for(
    ModelId model,
    const Sample& s,
    int directionDx,
    int directionDy,
    std::array<double,6u>& b,
    std::size_t& p) noexcept {
    b = {};
    switch (model) {
        case ModelId::DirectionalStripLine: {
            p = 2u;
            const double norm =
                std::sqrt(
                    static_cast<double>(
                        directionDx * directionDx +
                        directionDy * directionDy));
            const double t =
                norm > 0.0
                    ? (s.x * static_cast<double>(directionDx) +
                       s.y * static_cast<double>(directionDy)) / norm
                    : 0.0;
            b[0] = 1.0;
            b[1] = t;
            break;
        }
        case ModelId::AffinePlane:
            p = 3u;
            b[0] = 1.0;
            b[1] = s.x;
            b[2] = s.y;
            break;
        case ModelId::QuadraticSurface:
            p = 6u;
            b[0] = 1.0;
            b[1] = s.x;
            b[2] = s.y;
            b[3] = s.x * s.x;
            b[4] = s.x * s.y;
            b[5] = s.y * s.y;
            break;
        default:
            p = 0u;
            break;
    }
}

bool fit_least_squares(
    const std::vector<Sample>& samples,
    ModelId model,
    int directionDx,
    int directionDy,
    std::uint32_t minSamples,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = model;
    if (samples.size() < minSamples) return true;

    std::array<std::array<double,6u>,6u> normal{};
    std::array<double,6u> rhs{};
    std::size_t p = 0u;

    for (const auto& s : samples) {
        if (!std::isfinite(s.value) ||
            !std::isfinite(s.x) ||
            !std::isfinite(s.y)) {
            return false;
        }
        std::array<double,6u> b{};
        std::size_t currentP = 0u;
        basis_for(model, s, directionDx, directionDy, b, currentP);
        if (p == 0u) p = currentP;
        if (currentP == 0u || currentP != p) return false;
        for (std::size_t r = 0u; r < p; ++r) {
            rhs[r] += b[r] * s.value;
            for (std::size_t c = 0u; c < p; ++c) {
                normal[r][c] += b[r] * b[c];
            }
        }
    }
    if (p == 0u || samples.size() <= p) return true;

    std::array<std::array<double,6u>,6u> inverse{};
    if (!invert_normal(normal, p, inverse)) return true;

    std::array<double,6u> beta{};
    for (std::size_t r = 0u; r < p; ++r) {
        for (std::size_t c = 0u; c < p; ++c) {
            beta[r] += inverse[r][c] * rhs[c];
        }
        if (!std::isfinite(beta[r])) return false;
    }

    long double sseLd = 0.0L;
    for (const auto& s : samples) {
        std::array<double,6u> b{};
        std::size_t currentP = 0u;
        basis_for(model, s, directionDx, directionDy, b, currentP);
        if (currentP != p) return false;
        double predicted = 0.0;
        for (std::size_t c = 0u; c < p; ++c) {
            predicted += beta[c] * b[c];
        }
        const double residual = s.value - predicted;
        if (!std::isfinite(predicted) || !std::isfinite(residual)) {
            return false;
        }
        sseLd +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual);
    }

    const double sse = static_cast<double>(sseLd);
    const std::size_t n = samples.size();
    const std::size_t dof = n - p;
    if (!std::isfinite(sse) || dof == 0u) return true;

    const double residualVariance =
        std::max(sse / static_cast<double>(dof), kScoreEpsilon);
    const double predictionVariance =
        inverse[0][0] * residualVariance;
    const double score = score_from_sse(sse, n, p);
    if (!std::isfinite(residualVariance) ||
        !std::isfinite(predictionVariance) ||
        !(predictionVariance > 0.0) ||
        !std::isfinite(score)) {
        return true;
    }

    out.valid = true;
    out.estimate = beta[0];
    out.residualRms =
        std::sqrt(std::max(0.0, sse / static_cast<double>(n)));
    out.selectionScore = score;
    out.predictionVariance = predictionVariance;
    out.supportSamples = static_cast<std::uint32_t>(n);
    out.directionDx = directionDx;
    out.directionDy = directionDy;
    return std::isfinite(out.estimate) && std::isfinite(out.residualRms);
}

bool fit_robust_constant(
    const std::vector<Sample>& samples,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = ModelId::RobustMedianConstant;
    if (samples.size() < kMinConstantSamples) return true;

    std::vector<double> values;
    values.reserve(samples.size());
    for (const auto& s : samples) {
        if (!std::isfinite(s.value)) return false;
        values.push_back(s.value);
    }
    std::sort(values.begin(), values.end());

    const std::size_t n = values.size();
    const double median =
        (n & 1u) != 0u
            ? values[n / 2u]
            : 0.5 * (values[n / 2u - 1u] + values[n / 2u]);

    long double sseLd = 0.0L;
    for (const double v : values) {
        const double r = v - median;
        sseLd +=
            static_cast<long double>(r) *
            static_cast<long double>(r);
    }
    const double sse = static_cast<double>(sseLd);
    if (!std::isfinite(sse) || n <= 1u) return true;

    const double residualVariance =
        std::max(sse / static_cast<double>(n - 1u), kScoreEpsilon);
    const double predictionVariance =
        residualVariance / static_cast<double>(n);
    const double score = score_from_sse(sse, n, 1u);
    if (!std::isfinite(score) ||
        !std::isfinite(predictionVariance) ||
        !(predictionVariance > 0.0)) {
        return true;
    }

    out.valid = true;
    out.estimate = median;
    out.residualRms =
        std::sqrt(std::max(0.0, sse / static_cast<double>(n)));
    out.selectionScore = score;
    out.predictionVariance = predictionVariance;
    out.supportSamples = static_cast<std::uint32_t>(n);
    return true;
}

bool fit_directional(
    const std::vector<Sample>& samples,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = ModelId::DirectionalStripLine;
    bool have = false;
    for (const auto& d : kDirections) {
        ModelPrediction candidate{};
        if (!fit_least_squares(
                samples,
                ModelId::DirectionalStripLine,
                d[0],
                d[1],
                kMinDirectionalSamples,
                candidate)) {
            return false;
        }
        if (!candidate.valid) continue;
        if (!have ||
            candidate.selectionScore + 1.0e-12 < out.selectionScore) {
            out = candidate;
            have = true;
        }
    }
    return true;
}


struct LinearFit final {
    bool valid = false;
    std::array<double,6u> beta{};
    std::size_t parameters = 0u;
    double residualRms = 0.0;
    double predictionVariance = 0.0;
};

bool validation_offset(int dx, int dy) noexcept {
    const int qx = (dx + kSupportRadius) / 2;
    const int qy = (dy + kSupportRadius) / 2;
    return ((qx + qy) % 3) == 0;
}

bool within_direction_strip(
    const Sample& s,
    int directionDx,
    int directionDy) noexcept {
    const double norm = std::sqrt(
        static_cast<double>(
            directionDx * directionDx +
            directionDy * directionDy));
    if (!(norm > 0.0) || !std::isfinite(norm)) return false;
    const double perpendicular =
        std::abs(
            -static_cast<double>(directionDy) *
                static_cast<double>(s.dx) +
            static_cast<double>(directionDx) *
                static_cast<double>(s.dy));
    return perpendicular <=
        kDirectionalStripHalfWidthSourcePx * norm + 1.0e-12;
}

bool solve_linear_fit(
    const std::vector<Sample>& samples,
    ModelId model,
    int directionDx,
    int directionDy,
    std::uint32_t minSamples,
    LinearFit& out) noexcept {
    out = {};
    if (samples.size() < minSamples) return true;

    std::array<std::array<double,6u>,6u> normal{};
    std::array<double,6u> rhs{};
    std::size_t p = 0u;

    for (const auto& s : samples) {
        if (!std::isfinite(s.value) ||
            !std::isfinite(s.x) ||
            !std::isfinite(s.y)) {
            return false;
        }
        std::array<double,6u> b{};
        std::size_t currentP = 0u;
        basis_for(model, s, directionDx, directionDy, b, currentP);
        if (p == 0u) p = currentP;
        if (currentP == 0u || currentP != p) return false;
        for (std::size_t r = 0u; r < p; ++r) {
            rhs[r] += b[r] * s.value;
            for (std::size_t col = 0u; col < p; ++col) {
                normal[r][col] += b[r] * b[col];
            }
        }
    }
    if (p == 0u || samples.size() <= p) return true;

    std::array<std::array<double,6u>,6u> inverse{};
    if (!invert_normal(normal, p, inverse)) return true;

    for (std::size_t r = 0u; r < p; ++r) {
        for (std::size_t col = 0u; col < p; ++col) {
            out.beta[r] += inverse[r][col] * rhs[col];
        }
        if (!std::isfinite(out.beta[r])) return false;
    }

    long double sseLd = 0.0L;
    for (const auto& s : samples) {
        std::array<double,6u> b{};
        std::size_t currentP = 0u;
        basis_for(model, s, directionDx, directionDy, b, currentP);
        if (currentP != p) return false;
        double predicted = 0.0;
        for (std::size_t k = 0u; k < p; ++k) {
            predicted += out.beta[k] * b[k];
        }
        const double residual = s.value - predicted;
        if (!std::isfinite(predicted) || !std::isfinite(residual)) {
            return false;
        }
        sseLd +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual);
    }

    const double sse = static_cast<double>(sseLd);
    const std::size_t dof = samples.size() - p;
    if (!std::isfinite(sse) || dof == 0u) return true;

    const double residualVariance =
        std::max(sse / static_cast<double>(dof), kScoreEpsilon);
    const double predictionVariance =
        inverse[0][0] * residualVariance;
    if (!std::isfinite(residualVariance) ||
        !std::isfinite(predictionVariance) ||
        !(predictionVariance > 0.0)) {
        return true;
    }

    out.valid = true;
    out.parameters = p;
    out.residualRms =
        std::sqrt(
            std::max(
                0.0,
                sse / static_cast<double>(samples.size())));
    out.predictionVariance = predictionVariance;
    return true;
}

bool predict_linear(
    const LinearFit& fit,
    ModelId model,
    int directionDx,
    int directionDy,
    const Sample& s,
    double& predicted) noexcept {
    predicted = 0.0;
    if (!fit.valid || fit.parameters == 0u) return false;
    std::array<double,6u> b{};
    std::size_t p = 0u;
    basis_for(model, s, directionDx, directionDy, b, p);
    if (p != fit.parameters) return false;
    for (std::size_t k = 0u; k < p; ++k) {
        predicted += fit.beta[k] * b[k];
    }
    return std::isfinite(predicted);
}

double crossfit_predictive_score(
    double validationSse,
    std::size_t validationSamples,
    std::size_t trainSamples,
    std::size_t parameters) noexcept {
    if (!std::isfinite(validationSse) ||
        validationSamples == 0u ||
        trainSamples <= parameters ||
        parameters == 0u) {
        return std::numeric_limits<double>::infinity();
    }
    const double mse = std::max(
        validationSse /
            static_cast<double>(validationSamples),
        kScoreEpsilon);
    const double penalty =
        static_cast<double>(parameters) *
        std::log(static_cast<double>(trainSamples)) /
        static_cast<double>(validationSamples);
    const double score = std::log(mse) + penalty;
    return std::isfinite(score)
        ? score
        : std::numeric_limits<double>::infinity();
}

bool fit_least_squares_crossfit(
    const std::vector<Sample>& samples,
    ModelId model,
    int directionDx,
    int directionDy,
    std::uint32_t minTrain,
    std::uint32_t minValidation,
    bool directionConditioned,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = model;

    std::vector<Sample> train;
    std::vector<Sample> validation;
    std::vector<Sample> admitted;
    train.reserve(samples.size());
    validation.reserve(samples.size());
    admitted.reserve(samples.size());

    for (const auto& s : samples) {
        if (directionConditioned &&
            !within_direction_strip(
                s,
                directionDx,
                directionDy)) {
            continue;
        }
        admitted.push_back(s);
        if (s.validation) validation.push_back(s);
        else train.push_back(s);
    }

    if (train.size() < minTrain ||
        validation.size() < minValidation) {
        return true;
    }

    LinearFit trainFit{};
    if (!solve_linear_fit(
            train,
            model,
            directionDx,
            directionDy,
            minTrain,
            trainFit)) {
        return false;
    }
    if (!trainFit.valid) return true;

    long double validationSseLd = 0.0L;
    for (const auto& s : validation) {
        double predicted = 0.0;
        if (!predict_linear(
                trainFit,
                model,
                directionDx,
                directionDy,
                s,
                predicted)) {
            return false;
        }
        const double residual = s.value - predicted;
        validationSseLd +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual);
    }
    const double validationSse =
        static_cast<double>(validationSseLd);
    const double score =
        crossfit_predictive_score(
            validationSse,
            validation.size(),
            train.size(),
            trainFit.parameters);
    if (!std::isfinite(score)) return true;

    // After the selection score is frozen from train->validation prediction,
    // refit the same candidate on all surrounding admitted anchors. This still
    // happens before the held-out target is read and does not create evidence.
    LinearFit finalFit{};
    if (!solve_linear_fit(
            admitted,
            model,
            directionDx,
            directionDy,
            minTrain,
            finalFit)) {
        return false;
    }
    if (!finalFit.valid) return true;

    out.valid = true;
    out.estimate = finalFit.beta[0];
    out.residualRms = finalFit.residualRms;
    out.selectionScore = score;
    out.predictionVariance = finalFit.predictionVariance;
    out.supportSamples =
        static_cast<std::uint32_t>(admitted.size());
    out.trainSamples =
        static_cast<std::uint32_t>(train.size());
    out.validationSamples =
        static_cast<std::uint32_t>(validation.size());
    out.validationRms =
        std::sqrt(
            std::max(
                0.0,
                validationSse /
                    static_cast<double>(validation.size())));
    out.directionDx = directionDx;
    out.directionDy = directionDy;
    return std::isfinite(out.estimate) &&
           std::isfinite(out.validationRms);
}

bool fit_robust_constant_crossfit(
    const std::vector<Sample>& samples,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = ModelId::RobustMedianConstant;

    std::vector<double> train;
    std::vector<double> validation;
    std::vector<double> all;
    train.reserve(samples.size());
    validation.reserve(samples.size());
    all.reserve(samples.size());

    for (const auto& s : samples) {
        if (!std::isfinite(s.value)) return false;
        all.push_back(s.value);
        if (s.validation) validation.push_back(s.value);
        else train.push_back(s.value);
    }

    if (train.size() < kMinCrossfitConstantTrain ||
        validation.size() < kMinCrossfitValidation) {
        return true;
    }

    auto median_of = [](std::vector<double> values) noexcept -> double {
        if (values.empty()) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        std::sort(values.begin(), values.end());
        const std::size_t n = values.size();
        return (n & 1u) != 0u
            ? values[n / 2u]
            : 0.5 * (values[n / 2u - 1u] +
                     values[n / 2u]);
    };

    const double trainMedian = median_of(train);
    if (!std::isfinite(trainMedian)) return false;

    long double validationSseLd = 0.0L;
    for (const double value : validation) {
        const double residual = value - trainMedian;
        validationSseLd +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual);
    }
    const double validationSse =
        static_cast<double>(validationSseLd);
    const double score =
        crossfit_predictive_score(
            validationSse,
            validation.size(),
            train.size(),
            1u);
    if (!std::isfinite(score)) return true;

    const double finalMedian = median_of(all);
    if (!std::isfinite(finalMedian) || all.size() <= 1u) {
        return true;
    }

    long double finalSseLd = 0.0L;
    for (const double value : all) {
        const double residual = value - finalMedian;
        finalSseLd +=
            static_cast<long double>(residual) *
            static_cast<long double>(residual);
    }
    const double finalSse = static_cast<double>(finalSseLd);
    if (!std::isfinite(finalSse)) return false;

    const double residualVariance =
        std::max(
            finalSse /
                static_cast<double>(all.size() - 1u),
            kScoreEpsilon);
    const double predictionVariance =
        residualVariance /
        static_cast<double>(all.size());

    out.valid = true;
    out.estimate = finalMedian;
    out.residualRms =
        std::sqrt(
            std::max(
                0.0,
                finalSse /
                    static_cast<double>(all.size())));
    out.selectionScore = score;
    out.predictionVariance = predictionVariance;
    out.supportSamples =
        static_cast<std::uint32_t>(all.size());
    out.trainSamples =
        static_cast<std::uint32_t>(train.size());
    out.validationSamples =
        static_cast<std::uint32_t>(validation.size());
    out.validationRms =
        std::sqrt(
            std::max(
                0.0,
                validationSse /
                    static_cast<double>(validation.size())));
    return true;
}

bool fit_directional_crossfit(
    const std::vector<Sample>& samples,
    ModelPrediction& out) noexcept {
    out = {};
    out.model = ModelId::DirectionalStripLine;
    bool have = false;

    for (const auto& d : kDirections) {
        ModelPrediction candidate{};
        if (!fit_least_squares_crossfit(
                samples,
                ModelId::DirectionalStripLine,
                d[0],
                d[1],
                kMinCrossfitDirectionalTrain,
                kMinCrossfitDirectionalValidation,
                true,
                candidate)) {
            return false;
        }
        if (!candidate.valid) continue;
        if (!have ||
            candidate.selectionScore + 1.0e-12 <
                out.selectionScore ||
            (std::abs(
                 candidate.selectionScore -
                 out.selectionScore) <= 1.0e-12 &&
             std::pair<int,int>(
                 candidate.directionDx,
                 candidate.directionDy) <
             std::pair<int,int>(
                 out.directionDx,
                 out.directionDy))) {
            out = candidate;
            have = true;
        }
    }
    return true;
}

ModelPrediction choose_target_blind(
    const ModelPrediction& constant,
    const ModelPrediction& directional,
    const ModelPrediction& affine,
    const ModelPrediction& quadratic) noexcept {
    const std::array<const ModelPrediction*,4u> candidates{{
        &constant, &directional, &affine, &quadratic
    }};
    ModelPrediction best{};
    best.model = ModelId::None;
    bool have = false;
    for (const auto* c : candidates) {
        if (c == nullptr || !c->valid ||
            !std::isfinite(c->selectionScore)) {
            continue;
        }
        if (!have ||
            c->selectionScore + 1.0e-12 < best.selectionScore ||
            (std::abs(c->selectionScore - best.selectionScore) <= 1.0e-12 &&
             static_cast<std::uint32_t>(c->model) <
                 static_cast<std::uint32_t>(best.model))) {
            best = *c;
            have = true;
        }
    }
    return best;
}

void add_model_error(
    Metrics& metrics,
    const ModelPrediction& prediction,
    double actual,
    bool selected,
    bool baselineValid,
    double baselineAbsError) noexcept {
    if (!prediction.valid) return;
    auto& m = metrics.model[model_index(prediction.model)];
    ++m.valid;
    const double signedError = prediction.estimate - actual;
    const double absError = std::abs(signedError);
    m.absErrorSum += absError;
    m.squaredErrorSum +=
        static_cast<long double>(signedError) *
        static_cast<long double>(signedError);
    m.signedErrorSum += signedError;
    m.residualRmsSum += prediction.residualRms;
    if (selected) ++m.selected;
    if (baselineValid && absError < baselineAbsError) {
        ++m.lowerAbsErrorThanBaseline;
    }
}

double mean(long double sum, std::uint64_t n) noexcept {
    return n == 0u
        ? 0.0
        : static_cast<double>(
            sum / static_cast<long double>(n));
}

double rmse(long double sumSq, std::uint64_t n) noexcept {
    if (n == 0u) return 0.0;
    const long double v =
        sumSq / static_cast<long double>(n);
    return v <= 0.0L
        ? 0.0
        : std::sqrt(static_cast<double>(v));
}

void write_u64_array(
    std::ostringstream& o,
    const std::array<std::uint64_t,4u>& values) {
    o << "[";
    for (std::size_t i = 0u; i < values.size(); ++i) {
        if (i != 0u) o << ",";
        o << values[i];
    }
    o << "]";
}

void write_prediction(
    std::ostringstream& o,
    const char* prefix,
    const ModelPrediction& p) {
    o << ",\"" << prefix << "_valid\":"
      << (p.valid ? "true" : "false");
    if (!p.valid) return;
    o << ",\"" << prefix << "_model\":\""
      << model_name(p.model) << "\"";
    o << ",\"" << prefix << "_estimate\":" << p.estimate;
    o << ",\"" << prefix << "_residual_rms\":" << p.residualRms;
    o << ",\"" << prefix << "_selection_score\":" << p.selectionScore;
    o << ",\"" << prefix << "_prediction_variance\":"
      << p.predictionVariance;
    o << ",\"" << prefix << "_support_samples\":"
      << p.supportSamples;
    o << ",\"" << prefix << "_train_samples\":"
      << p.trainSamples;
    o << ",\"" << prefix << "_validation_samples\":"
      << p.validationSamples;
    o << ",\"" << prefix << "_validation_rms\":"
      << p.validationRms;
    if (p.model == ModelId::DirectionalStripLine) {
        o << ",\"" << prefix << "_direction\":["
          << p.directionDx << "," << p.directionDy << "]";
    }
}

void write_model_aggregate(
    std::ostringstream& o,
    const ModelAggregate& m) {
    o << "{\"valid\":" << m.valid;
    o << ",\"selected\":" << m.selected;
    o << ",\"lower_abs_error_than_baseline\":"
      << m.lowerAbsErrorThanBaseline;
    o << ",\"mae\":" << mean(m.absErrorSum, m.valid);
    o << ",\"rmse\":" << rmse(m.squaredErrorSum, m.valid);
    o << ",\"bias\":" << mean(m.signedErrorSum, m.valid);
    o << ",\"mean_internal_residual_rms\":"
      << mean(m.residualRmsSum, m.valid);
    o << "}";
}

} // namespace

bool run(
    stream::IRawTileSource& source,
    const Binding& binding,
    Report& out) noexcept {
    out = {};
    try {
        const auto& md = source.metadata();
        if (!nonzero(binding.sourceEvidenceSha256) ||
            !nonzero(binding.scientificMasterSha256) ||
            !nonzero(binding.authorityFieldSha256) ||
            !nonzero(binding.truthNegativeStateSha256) ||
            md.width <= 2 * kSupportRadius ||
            md.height <= 2 * kSupportRadius) {
            return false;
        }

        const int width = md.width;
        const int height = md.height;

        truthraw::sha256_v0_69::Hasher holdoutHasher;
        constexpr char domain[] =
            "D_RAW_UNIVERSAL_LOCAL_MODEL_BANK_HOLDOUT_V0_2";
        holdoutHasher.update(
            reinterpret_cast<const std::uint8_t*>(domain),
            sizeof(domain) - 1u);
        holdoutHasher.update(binding.sourceEvidenceSha256);
        holdoutHasher.update(binding.truthNegativeStateSha256);
        hash_u32(holdoutHasher, kHoldoutPeriod);
        hash_u32(
            holdoutHasher,
            static_cast<std::uint32_t>(kSupportRadius));
        hash_u64(holdoutHasher, kLatticeUnitsPerSourcePixel);

        detail::Workspace workspace{};

        for (int coreY = 0; coreY < height; coreY += kCoreTileExtent) {
            for (int coreX = 0; coreX < width; coreX += kCoreTileExtent) {
                TileRect tile{};
                tile.x0 = coreX;
                tile.y0 = coreY;
                tile.x1 = std::min(width, coreX + kCoreTileExtent);
                tile.y1 = std::min(height, coreY + kCoreTileExtent);
                tile.hx0 = std::max(0, tile.x0 - kSupportRadius);
                tile.hy0 = std::max(0, tile.y0 - kSupportRadius);
                tile.hx1 = std::min(width, tile.x1 + kSupportRadius);
                tile.hy1 = std::min(height, tile.y1 + kSupportRadius);

                const auto filled =
                    detail::fill_stage2(source, tile, workspace);
                if (!filled) return false;

                const int tw = tile.hx1 - tile.hx0;
                const int th = tile.hy1 - tile.hy0;
                const std::size_t expected =
                    static_cast<std::size_t>(tw) *
                    static_cast<std::size_t>(th);
                if (tw <= 0 || th <= 0 ||
                    workspace.stage2.size() != expected ||
                    workspace.raw.size() != expected) {
                    return false;
                }

                const auto indexOf =
                    [&](int gx, int gy, std::size_t& index) noexcept {
                        if (gx < tile.hx0 || gy < tile.hy0 ||
                            gx >= tile.hx1 || gy >= tile.hy1) {
                            return false;
                        }
                        index =
                            static_cast<std::size_t>(gy - tile.hy0) *
                                static_cast<std::size_t>(tw) +
                            static_cast<std::size_t>(gx - tile.hx0);
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
                            if (!indexOf(
                                    gx + s * stepX,
                                    gy + s * stepY,
                                    i)) {
                                return true;
                            }
                            if (static_cast<float>(workspace.raw[i]) >=
                                md.whiteLevel) {
                                return true;
                            }
                        }
                        return false;
                    };

                for (int gy = tile.y0; gy < tile.y1; ++gy) {
                    if (gy < kSupportRadius ||
                        gy >= height - kSupportRadius ||
                        static_cast<std::uint32_t>(gy) % kHoldoutPeriod !=
                            static_cast<std::uint32_t>(gy & 1)) {
                        continue;
                    }
                    for (int gx = tile.x0; gx < tile.x1; ++gx) {
                        if (gx < kSupportRadius ||
                            gx >= width - kSupportRadius ||
                            static_cast<std::uint32_t>(gx) % kHoldoutPeriod !=
                                static_cast<std::uint32_t>(gx & 1)) {
                            continue;
                        }

                        std::size_t centerIndex = 0u;
                        if (!indexOf(gx, gy, centerIndex)) return false;
                        if (static_cast<float>(workspace.raw[centerIndex]) >=
                            md.whiteLevel) {
                            ++out.global.targetCensoredSkipped;
                            continue;
                        }

                        const int channel =
                            measured_channel(md.cfa, gx, gy);
                        if (channel < 0 || channel > 2) return false;
                        const auto phase =
                            static_cast<std::uint32_t>(
                                (gy & 1) * 2 + (gx & 1));
                        // Do not even read the held-out Stage-2 value yet.
                        // Model fitting and selection below see only other
                        // same-phase measured anchors.
                        std::vector<Sample> samples;
                        samples.reserve(80u);
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
                                    measured_channel(md.cfa, nx, ny) !=
                                        channel) {
                                    return false;
                                }
                                if (static_cast<float>(workspace.raw[ni]) >=
                                        md.whiteLevel ||
                                    pathTouchesCensor(
                                        gx, gy, dx, dy)) {
                                    continue;
                                }
                                const double value =
                                    workspace.stage2[ni];
                                if (!std::isfinite(value)) continue;
                                Sample s{};
                                s.dx = dx;
                                s.dy = dy;
                                s.x =
                                    static_cast<double>(dx) /
                                    static_cast<double>(kSupportRadius);
                                s.y =
                                    static_cast<double>(dy) /
                                    static_cast<double>(kSupportRadius);
                                s.value = value;
                                s.validation =
                                    validation_offset(dx, dy);
                                samples.push_back(s);
                            }
                        }

                        HoldoutRecord record{};
                        record.x =
                            static_cast<std::uint32_t>(gx);
                        record.y =
                            static_cast<std::uint32_t>(gy);
                        record.cfaPhase = phase;
                        record.channel =
                            static_cast<std::uint32_t>(channel);
                        record.supportSamples =
                            static_cast<std::uint32_t>(samples.size());

                        if (!fit_robust_constant_crossfit(
                                samples,
                                record.robustConstant) ||
                            !fit_directional_crossfit(
                                samples,
                                record.directional) ||
                            !fit_least_squares_crossfit(
                                samples,
                                ModelId::AffinePlane,
                                0,
                                0,
                                kMinCrossfitAffineTrain,
                                kMinCrossfitValidation,
                                false,
                                record.affine) ||
                            !fit_least_squares_crossfit(
                                samples,
                                ModelId::QuadraticSurface,
                                0,
                                0,
                                kMinCrossfitQuadraticTrain,
                                kMinCrossfitValidation,
                                false,
                                record.quadratic)) {
                            return false;
                        }

                        // IMPORTANT: target-blind selection happens here
                        // while the held-out Stage-2 value has not even been
                        // read from the workspace.
                        record.selected =
                            choose_target_blind(
                                record.robustConstant,
                                record.directional,
                                record.affine,
                                record.quadratic);

                        ce::Input baselineInput{};
                        baselineInput.neighbors.reserve(
                            kBaselineRadii.size() *
                            kDirections.size() * 2u);
                        for (const int radius : kBaselineRadii) {
                            for (const auto& direction : kDirections) {
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
                                        measured_channel(
                                            md.cfa, nx, ny) != channel) {
                                        return false;
                                    }
                                    const double value =
                                        workspace.stage2[ni];
                                    const bool censored =
                                        static_cast<float>(
                                            workspace.raw[ni]) >=
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
                                        pathTouchesCensor(
                                            gx, gy, dx, dy);
                                    baselineInput.neighbors.push_back(
                                        sample);
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
                        if (baseline.valid) {
                            record.baselineValid = true;
                            record.baselineEstimate =
                                baseline.estimate;
                        }

                        // Target reveal occurs only after both the research
                        // selector and the independent reference predictor
                        // have been fully frozen.
                        const double actual =
                            workspace.stage2[centerIndex];
                        if (!std::isfinite(actual)) return false;
                        record.actual = actual;
                        if (record.baselineValid) {
                            record.baselineAbsError =
                                std::abs(
                                    record.baselineEstimate - actual);
                        }

                        ++out.global.holdouts;
                        ++out.global.holdoutCfaPhase[phase];

                        const std::array<ModelPrediction*,4u> models{{
                            &record.robustConstant,
                            &record.directional,
                            &record.affine,
                            &record.quadratic
                        }};

                        ModelId oracleModel = ModelId::None;
                        double oracleError =
                            std::numeric_limits<double>::infinity();
                        for (auto* model : models) {
                            if (model == nullptr || !model->valid) continue;
                            const double error =
                                std::abs(model->estimate - actual);
                            const bool selected =
                                record.selected.valid &&
                                record.selected.model == model->model;
                            add_model_error(
                                out.global,
                                *model,
                                actual,
                                selected,
                                record.baselineValid,
                                record.baselineAbsError);
                            if (error < oracleError) {
                                oracleError = error;
                                oracleModel = model->model;
                            }
                        }
                        record.oracleBestModel = oracleModel;
                        if (std::isfinite(oracleError)) {
                            record.oracleBestAbsError = oracleError;
                        }

                        if (record.baselineValid) {
                            ++out.global.baselineValid;
                            const double signedError =
                                record.baselineEstimate - actual;
                            out.global.baselineAbsErrorSum +=
                                std::abs(signedError);
                            out.global.baselineSquaredErrorSum +=
                                static_cast<long double>(signedError) *
                                static_cast<long double>(signedError);
                            out.global.baselineSignedErrorSum +=
                                signedError;
                        }

                        if (record.selected.valid) {
                            ++out.global.selectedValid;
                            ++out.global.selectedCfaPhase[phase];
                            const double signedError =
                                record.selected.estimate - actual;
                            record.selectedAbsError =
                                std::abs(signedError);
                            out.global.selectedAbsErrorSum +=
                                record.selectedAbsError;
                            out.global.selectedSquaredErrorSum +=
                                static_cast<long double>(signedError) *
                                static_cast<long double>(signedError);
                            out.global.selectedSignedErrorSum +=
                                signedError;

                            if (std::isfinite(oracleError)) {
                                record.selectorRegretAbsError =
                                    std::max(
                                        0.0,
                                        record.selectedAbsError -
                                            oracleError);
                                out.global.selectorRegretAbsErrorSum +=
                                    record.selectorRegretAbsError;
                                out.global.maxSelectorRegretAbsError =
                                    std::max(
                                        out.global.maxSelectorRegretAbsError,
                                        record.selectorRegretAbsError);
                            }

                            if (record.baselineValid) {
                                const double se =
                                    record.selectedAbsError;
                                const double be =
                                    record.baselineAbsError;
                                const double eps =
                                    std::numeric_limits<double>::epsilon() *
                                    std::max({1.0, se, be}) * 8.0;
                                if (se + eps < be) {
                                    ++out.global
                                        .selectedLowerAbsErrorThanBaseline;
                                } else if (be + eps < se) {
                                    ++out.global
                                        .baselineLowerAbsErrorThanSelected;
                                } else {
                                    ++out.global
                                        .equalSelectedBaselineAbsError;
                                }
                            }
                        } else {
                            ++out.global.noModelSelected;
                        }

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
                        hash_u32(
                            holdoutHasher,
                            static_cast<std::uint32_t>(
                                record.selected.model));
                        if (record.selected.valid) {
                            hash_f64(
                                holdoutHasher,
                                record.selected.estimate);
                            hash_f64(
                                holdoutHasher,
                                record.selected.selectionScore);
                        }
                        // The target enters the audit stream only after the
                        // target-blind model selection above has been frozen.
                        hash_f64(holdoutHasher, actual);

                        out.holdouts.push_back(record);
                    }
                }
            }
        }

        if (out.global.holdouts == 0u ||
            out.holdouts.size() != out.global.holdouts ||
            out.global.selectedValid +
                    out.global.noModelSelected !=
                out.global.holdouts) {
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
        o << "  \"source_width\":" << width << ",\n";
        o << "  \"source_height\":" << height << ",\n";
        o << "  \"source_raster_role\":"
             "\"EXACT_MEASURED_ANCHOR_GEOMETRY_AND_FULL_RESOLUTION_SOURCE_SUPPORT\",\n";
        o << "  \"source_raster_used_only_for_noise\":false,\n";
        o << "  \"source_raster_is_world_resolution_authority\":false,\n";
        o << "  \"scientific_solution_domain\":"
             "\"RASTER_INDEPENDENT_SPARSE_FIXED_POINT_LATTICE\",\n";
        o << "  \"lattice_fraction_bits\":"
          << kLatticeFractionBits << ",\n";
        o << "  \"lattice_units_per_source_pixel\":"
          << kLatticeUnitsPerSourcePixel << ",\n";
        o << "  \"holdout_period\":" << kHoldoutPeriod << ",\n";
        o << "  \"support_radius_source_px\":"
          << kSupportRadius << ",\n";
        o << "  \"holdout_scope\":"
             "\"STRATIFIED_FULL_RESOLUTION_MEASURED_CFA_ANCHORS\",\n";
        o << "  \"selection_score\":"
             "\"TARGET_BLIND_SUPPORT_CROSSFIT_PREDICTIVE_SCORE_V0_2\",\n";
        o << "  \"support_crossfit_partition\":"
             "\"VALIDATION_WHEN_OFFSET_LATTICE_INDEX_SUM_MOD_3_EQUALS_0\",\n";
        o << "  \"selector_uses_support_crossfit\":true,\n";
        o << "  \"crossfit_partition_uses_target\":false,\n";
        o << "  \"final_candidate_refit_uses_all_admitted_neighbor_support\":true,\n";
        o << "  \"final_candidate_refit_occurs_before_target_reveal\":true,\n";
        o << "  \"directional_support_conditioned\":true,\n";
        o << "  \"directional_strip_half_width_source_px\":"
          << kDirectionalStripHalfWidthSourcePx << ",\n";
        o << "  \"model_bank\":["
             "\"ROBUST_MEDIAN_CONSTANT\","
             "\"DIRECTIONAL_STRIP_LINE\","
             "\"AFFINE_PLANE\","
             "\"QUADRATIC_SURFACE\","
             "\"NO_RECONSTRUCTION\"],\n";
        o << "  \"baseline_name\":"
             "\"CENTER_EXCLUDED_MULTISCALE_V0_2\",\n";
        o << "  \"target_censor_state_used_for_holdout_admission\":true,\n";
        o << "  \"target_numeric_stage2_value_read_before_selection\":false,\n";
        o << "  \"target_value_used_by_models\":false,\n";
        o << "  \"target_value_used_by_selector\":false,\n";
        o << "  \"holdout_error_used_by_selector\":false,\n";
        o << "  \"post_reveal_oracle_used_by_selector\":false,\n";
        o << "  \"post_reveal_oracle_is_diagnostic_only\":true,\n";
        o << "  \"lens_calibration_used\":false,\n";
        o << "  \"camera_model_used\":false,\n";
        o << "  \"vendor_mapping_used\":false,\n";
        o << "  \"noise_profile_required_for_selection\":false,\n";
        o << "  \"measured_anchors_modified\":false,\n";
        o << "  \"unanchored_values_promoted_to_measured\":false,\n";
        o << "  \"model_bank_applied_to_scientific_master\":false,\n";
        o << "  \"candidate_applied\":false,\n";
        o << "  \"creates_new_evidence\":false,\n";
        o << "  \"scientific_writeback_allowed\":false,\n";
        o << "  \"holdout_stream_sha256\":\""
          << hex(out.holdoutStreamSha256) << "\",\n";
        o << "  \"global\":{";
        o << "\"holdouts\":" << out.global.holdouts;
        o << ",\"target_censored_skipped\":"
          << out.global.targetCensoredSkipped;
        o << ",\"baseline_valid\":"
          << out.global.baselineValid;
        o << ",\"selected_valid\":"
          << out.global.selectedValid;
        o << ",\"no_model_selected\":"
          << out.global.noModelSelected;
        o << ",\"selected_lower_abs_error_than_baseline\":"
          << out.global.selectedLowerAbsErrorThanBaseline;
        o << ",\"baseline_lower_abs_error_than_selected\":"
          << out.global.baselineLowerAbsErrorThanSelected;
        o << ",\"equal_selected_baseline_abs_error\":"
          << out.global.equalSelectedBaselineAbsError;
        o << ",\"selected_mae\":"
          << mean(
                 out.global.selectedAbsErrorSum,
                 out.global.selectedValid);
        o << ",\"selected_rmse\":"
          << rmse(
                 out.global.selectedSquaredErrorSum,
                 out.global.selectedValid);
        o << ",\"selected_bias\":"
          << mean(
                 out.global.selectedSignedErrorSum,
                 out.global.selectedValid);
        o << ",\"baseline_mae\":"
          << mean(
                 out.global.baselineAbsErrorSum,
                 out.global.baselineValid);
        o << ",\"baseline_rmse\":"
          << rmse(
                 out.global.baselineSquaredErrorSum,
                 out.global.baselineValid);
        o << ",\"baseline_bias\":"
          << mean(
                 out.global.baselineSignedErrorSum,
                 out.global.baselineValid);
        o << ",\"mean_selector_regret_abs_error\":"
          << mean(
                 out.global.selectorRegretAbsErrorSum,
                 out.global.selectedValid);
        o << ",\"max_selector_regret_abs_error\":"
          << out.global.maxSelectorRegretAbsError;
        o << ",\"holdout_cfa_phase\":";
        write_u64_array(o, out.global.holdoutCfaPhase);
        o << ",\"selected_cfa_phase\":";
        write_u64_array(o, out.global.selectedCfaPhase);
        o << ",\"models\":{";
        o << "\"ROBUST_MEDIAN_CONSTANT\":";
        write_model_aggregate(
            o,
            out.global.model[
                model_index(ModelId::RobustMedianConstant)]);
        o << ",\"DIRECTIONAL_STRIP_LINE\":";
        write_model_aggregate(
            o,
            out.global.model[
                model_index(ModelId::DirectionalStripLine)]);
        o << ",\"AFFINE_PLANE\":";
        write_model_aggregate(
            o,
            out.global.model[
                model_index(ModelId::AffinePlane)]);
        o << ",\"QUADRATIC_SURFACE\":";
        write_model_aggregate(
            o,
            out.global.model[
                model_index(ModelId::QuadraticSurface)]);
        o << "}},\n";
        o << "  \"holdout_records\":[\n";

        for (std::size_t i = 0u; i < out.holdouts.size(); ++i) {
            const auto& h = out.holdouts[i];
            o << "    {\"x\":" << h.x
              << ",\"y\":" << h.y
              << ",\"lattice_u\":"
              << static_cast<std::uint64_t>(h.x) *
                    kLatticeUnitsPerSourcePixel
              << ",\"lattice_v\":"
              << static_cast<std::uint64_t>(h.y) *
                    kLatticeUnitsPerSourcePixel
              << ",\"cfa_phase\":" << h.cfaPhase
              << ",\"channel\":" << h.channel
              << ",\"support_samples\":" << h.supportSamples;

            write_prediction(
                o, "constant", h.robustConstant);
            write_prediction(
                o, "directional", h.directional);
            write_prediction(
                o, "affine", h.affine);
            write_prediction(
                o, "quadratic", h.quadratic);

            o << ",\"selected_model\":\""
              << model_name(h.selected.model) << "\"";
            o << ",\"selected_valid\":"
              << (h.selected.valid ? "true" : "false");
            if (h.selected.valid) {
                o << ",\"selected_estimate\":"
                  << h.selected.estimate;
                o << ",\"selected_selection_score\":"
                  << h.selected.selectionScore;
            }

            o << ",\"actual_post_reveal\":" << h.actual;
            o << ",\"baseline_valid\":"
              << (h.baselineValid ? "true" : "false");
            if (h.baselineValid) {
                o << ",\"baseline_estimate\":"
                  << h.baselineEstimate
                  << ",\"baseline_abs_error\":"
                  << h.baselineAbsError;
            }
            if (h.selected.valid) {
                o << ",\"selected_abs_error\":"
                  << h.selectedAbsError;
            }
            o << ",\"oracle_best_model_post_reveal\":\""
              << model_name(h.oracleBestModel) << "\"";
            if (h.oracleBestModel != ModelId::None) {
                o << ",\"oracle_best_abs_error_post_reveal\":"
                  << h.oracleBestAbsError;
                o << ",\"selector_regret_abs_error\":"
                  << h.selectorRegretAbsError;
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
            reinterpret_cast<const std::uint8_t*>(
                out.json.data()),
            out.json.size());
        out.jsonSha256 = jsonHasher.finalize();

        out.targetValueUsedByModels = false;
        out.selectorUsesSupportCrossfit = true;
        out.directionalSupportConditioned = true;
        out.targetValueUsedBySelector = false;
        out.holdoutErrorUsedBySelector = false;
        out.postRevealOracleUsedBySelector = false;
        out.lensCalibrationUsed = false;
        out.cameraModelUsed = false;
        out.vendorMappingUsed = false;
        out.measuredAnchorsModified = false;
        out.unanchoredValuesPromotedToMeasured = false;
        out.modelBankAppliedToScientificMaster = false;
        out.candidateApplied = false;
        out.createsNewEvidence = false;
        out.scientificWritebackAllowed = false;

        return !out.json.empty() &&
               nonzero(out.jsonSha256) &&
               !out.targetValueUsedByModels &&
               out.selectorUsesSupportCrossfit &&
               out.directionalSupportConditioned &&
               !out.targetValueUsedBySelector &&
               !out.holdoutErrorUsedBySelector &&
               !out.postRevealOracleUsedBySelector &&
               !out.lensCalibrationUsed &&
               !out.cameraModelUsed &&
               !out.vendorMappingUsed &&
               !out.measuredAnchorsModified &&
               !out.unanchoredValuesPromotedToMeasured &&
               !out.modelBankAppliedToScientificMaster &&
               !out.candidateApplied &&
               !out.createsNewEvidence &&
               !out.scientificWritebackAllowed;
    } catch (...) {
        out = {};
        return false;
    }
}

} // namespace truthraw::universal_local_model_bank_holdout::v0_2

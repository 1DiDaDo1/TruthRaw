#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>

namespace truthraw_precision_v09 {

constexpr const char* kV5gP1UncertaintyBindingSha256V09 =
    "61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";
constexpr const char* kV5gP1FeatureSchemaSha256V09 =
    "8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3";
constexpr std::size_t kV5gP1FeatureCountV09 = 18u;

enum class V5gP1RoleV09 : std::uint8_t {
    R = 0,
    G1 = 1,
    G2 = 2,
    B = 3,
};

struct V5gP1LocalQuantilesV09 {
    bool bindingAccepted = false;
    bool valid = false;
    bool censored = false;
    V5gP1RoleV09 role = V5gP1RoleV09::R;
    int snrBin = -1;
    double predictedSnr = std::numeric_limits<double>::quiet_NaN();
    double mu = std::numeric_limits<double>::quiet_NaN();
    double p50 = std::numeric_limits<double>::quiet_NaN();
    double p95 = std::numeric_limits<double>::quiet_NaN();
};

bool v5g_p1_runtime_binding_exact_v0_9(
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256);

// Exact native evaluator for the canonical v5.0g-p1 model specification.
// The 18 features must already be expressed in the canonical feature schema.
// predictedSnr is supplied separately only for selecting the calibration bin;
// it does not replace feature[2], which remains log1p(predicted_snr) as defined
// by the canonical model.
//
// Output semantics are empirical absolute-error quantiles in Stage-2 normalized
// scene-linear units. They are NOT sigma, variance, covariance, probability, or
// a joint RGB confidence region.
V5gP1LocalQuantilesV09 evaluate_v5g_p1_local_quantiles_v0_9(
    const std::array<double, kV5gP1FeatureCountV09>& features,
    double predictedSnr,
    V5gP1RoleV09 role,
    bool censored,
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256);

} // namespace truthraw_precision_v09

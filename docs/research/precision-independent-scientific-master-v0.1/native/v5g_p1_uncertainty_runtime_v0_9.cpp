#include "v5g_p1_uncertainty_runtime_v0_9.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v09 {
namespace {

constexpr double kIntercept = -9.859522042356929;
constexpr double kLogEpsilon = 2e-05;
constexpr std::array<double, kV5gP1FeatureCountV09> kCoefficients = {
    0.39560182865755256,
    1.0161682914538537,
    -0.1427753298770904,
    0.7359219854369202,
    -0.23091424097593158,
    0.021758142187742753,
    -0.10094162838787113,
    -0.0034295231121772435,
    0.0824666634235143,
    0.026693573250413768,
    0.48590059304532635,
    0.019262741845627946,
    0.2818476268259488,
    0.5190842824097119,
    0.042559419347716135,
    -0.04993336769428747,
    -0.03652103344000874,
    0.04389498180821168
};

constexpr double kQ50[4][5] = {
    {1.231865682669854, 1.2908136692538486, 1.2504420015126712, 1.2305798219573738, 1.2590962743751166},
    {1.640727841085955, 1.4059890073821433, 1.3522913191345247, 1.3226677467986496, 1.2809214266235962},
    {1.6422864247994864, 1.3943906655134495, 1.3529949563092272, 1.3241946628400298, 1.2385172282582537},
    {1.291396354412495, 1.2781368351623865, 1.251897127263868, 1.2392970084648944, 1.2094740477517203}
};

constexpr double kQ95[4][5] = {
    {3.546796840273797, 3.793965306343965, 3.62211432587943, 3.6443451690918227, 3.6221063333240333},
    {4.763977410108175, 4.276152316934799, 3.965925252841995, 3.8354128722599263, 3.837643438378962},
    {4.738723164710131, 4.221709375100408, 3.92232166293173, 3.8142002053215953, 3.763539378516662},
    {3.719499471273777, 3.824158517583519, 3.6299484745794905, 3.56435722257653, 3.567995560138259}
};

bool finite_all(const std::array<double, kV5gP1FeatureCountV09>& features) {
    for (double v : features) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}

int snr_bin(double snr) {
    if (!std::isfinite(snr) || snr < 0.0) return -1;
    if (snr < 2.0) return 0;
    if (snr < 4.0) return 1;
    if (snr < 8.0) return 2;
    if (snr < 16.0) return 3;
    return 4;
}

int role_index(V5gP1RoleV09 role) {
    const int i = static_cast<int>(role);
    return (i >= 0 && i < 4) ? i : -1;
}

} // namespace

bool v5g_p1_runtime_binding_exact_v0_9(
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256) {
    return uncertaintyBindingSha256 == kV5gP1UncertaintyBindingSha256V09 &&
           featureSchemaSha256 == kV5gP1FeatureSchemaSha256V09;
}

V5gP1LocalQuantilesV09 evaluate_v5g_p1_local_quantiles_v0_9(
    const std::array<double, kV5gP1FeatureCountV09>& features,
    double predictedSnr,
    V5gP1RoleV09 role,
    bool censored,
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256) {
    V5gP1LocalQuantilesV09 out;
    out.role = role;
    out.predictedSnr = predictedSnr;
    out.censored = censored;
    out.bindingAccepted = v5g_p1_runtime_binding_exact_v0_9(
        uncertaintyBindingSha256, featureSchemaSha256);

    if (!out.bindingAccepted || censored || !finite_all(features)) return out;

    const int r = role_index(role);
    const int b = snr_bin(predictedSnr);
    if (r < 0 || b < 0) return out;

    double logPrediction = kIntercept;
    for (std::size_t i = 0; i < kV5gP1FeatureCountV09; ++i) {
        logPrediction += kCoefficients[i] * features[i];
    }
    if (!std::isfinite(logPrediction)) return out;

    const double e = std::exp(logPrediction);
    if (!std::isfinite(e)) return out;

    const double mu = std::max(e - kLogEpsilon, 1e-6);
    const double p50 = mu * kQ50[r][b];
    const double p95 = std::max(mu * kQ95[r][b], p50);
    if (!std::isfinite(mu) || !std::isfinite(p50) || !std::isfinite(p95) ||
        mu < 0.0 || p50 < 0.0 || p95 < p50) return out;

    out.snrBin = b;
    out.mu = mu;
    out.p50 = p50;
    out.p95 = p95;
    out.valid = true;
    return out;
}

} // namespace truthraw_precision_v09

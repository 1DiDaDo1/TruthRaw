#include "v5g_p1_uncertainty_runtime_v0_9.h"

#include <array>
#include <cassert>
#include <cmath>
#include <limits>

using namespace truthraw_precision_v09;

namespace {

bool near(double a, double b, double rel = 1e-12, double abs = 1e-15) {
    const double d = std::abs(a - b);
    return d <= abs + rel * std::max(std::abs(a), std::abs(b));
}

} // namespace

int main() {
    std::array<double, kV5gP1FeatureCountV09> zero{};

    assert(v5g_p1_runtime_binding_exact_v0_9(
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09));
    assert(!v5g_p1_runtime_binding_exact_v0_9("wrong", kV5gP1FeatureSchemaSha256V09));

    const auto baseline = evaluate_v5g_p1_local_quantiles_v0_9(
        zero, 1.0, V5gP1RoleV09::R, false,
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09);
    assert(baseline.bindingAccepted);
    assert(baseline.valid);
    assert(!baseline.censored);
    assert(baseline.snrBin == 0);
    assert(near(baseline.mu, 3.224731571279024e-05));
    assert(near(baseline.p50, 3.972436158480665e-05));
    assert(near(baseline.p95, 1.1437467747743598e-04));
    assert(baseline.p95 >= baseline.p50);

    const double probes[5] = {0.0, 2.0, 4.0, 8.0, 16.0};
    for (int role = 0; role < 4; ++role) {
        for (int bin = 0; bin < 5; ++bin) {
            const auto q = evaluate_v5g_p1_local_quantiles_v0_9(
                zero, probes[bin], static_cast<V5gP1RoleV09>(role), false,
                kV5gP1UncertaintyBindingSha256V09,
                kV5gP1FeatureSchemaSha256V09);
            assert(q.valid);
            assert(q.snrBin == bin);
            assert(std::isfinite(q.mu));
            assert(std::isfinite(q.p50));
            assert(std::isfinite(q.p95));
            assert(q.mu >= 1e-6);
            assert(q.p95 >= q.p50);
        }
    }

    const auto censored = evaluate_v5g_p1_local_quantiles_v0_9(
        zero, 4.0, V5gP1RoleV09::G1, true,
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09);
    assert(censored.bindingAccepted);
    assert(censored.censored);
    assert(!censored.valid);

    const auto wrongBinding = evaluate_v5g_p1_local_quantiles_v0_9(
        zero, 4.0, V5gP1RoleV09::G1, false,
        "wrong",
        kV5gP1FeatureSchemaSha256V09);
    assert(!wrongBinding.bindingAccepted);
    assert(!wrongBinding.valid);

    auto nonFinite = zero;
    nonFinite[3] = std::numeric_limits<double>::quiet_NaN();
    const auto badFeature = evaluate_v5g_p1_local_quantiles_v0_9(
        nonFinite, 4.0, V5gP1RoleV09::B, false,
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09);
    assert(!badFeature.valid);

    const auto badSnr = evaluate_v5g_p1_local_quantiles_v0_9(
        zero, -1.0, V5gP1RoleV09::B, false,
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09);
    assert(!badSnr.valid);

    auto overflow = zero;
    overflow[1] = 1e308;
    const auto overflowResult = evaluate_v5g_p1_local_quantiles_v0_9(
        overflow, 4.0, V5gP1RoleV09::R, false,
        kV5gP1UncertaintyBindingSha256V09,
        kV5gP1FeatureSchemaSha256V09);
    assert(!overflowResult.valid);

    return 0;
}

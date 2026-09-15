#include "local_uncertainty_binding_v0_9.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v09 {

LocalQuantileStorageStatsV09 assess_local_v5g_p1_quantile_storage_v0_9(
    const double* f64Reference,
    const float* f32Stored,
    std::size_t count,
    const LocalQuantileFieldV09& field,
    double maxErrorOverP50,
    double maxErrorOverP95) {

    LocalQuantileStorageStatsV09 out;
    out.samples = static_cast<std::uint64_t>(count);
    out.sameScientificMasterQuantity = field.sameScientificMasterQuantity;
    out.bindingIdentityAccepted = truthraw_precision_v07::v5g_p1_binding_exact_v0_7(
        field.uncertaintyBindingSha256,
        field.featureSchemaSha256);

    if (!f64Reference || !f32Stored || !field.p50 || !field.p95 ||
        field.count != count || !field.sameScientificMasterQuantity ||
        !out.bindingIdentityAccepted ||
        !std::isfinite(maxErrorOverP50) || maxErrorOverP50 < 0.0 ||
        !std::isfinite(maxErrorOverP95) || maxErrorOverP95 < 0.0) {
        out.unresolved = out.samples;
        return out;
    }

    for (std::size_t i = 0; i < count; ++i) {
        const bool censored = field.censored && field.censored[i] != 0u;
        if (censored) {
            ++out.censored;
            ++out.unresolved;
            continue;
        }

        const auto binding = truthraw_precision_v07::bind_v5g_p1_quantiles_v0_7(
            field.p50[i],
            field.p95[i],
            false,
            field.uncertaintyBindingSha256,
            field.featureSchemaSha256);

        if (!binding.bindingAccepted || !binding.p50.known || !binding.p95.known ||
            !std::isfinite(f64Reference[i]) || !std::isfinite(f32Stored[i])) {
            ++out.unresolved;
            continue;
        }

        // The audit stream owns the actual stored scalar. Require exact agreement
        // with the one-step Float64->Float32 storage candidate used by v0.7.
        if (f32Stored[i] != static_cast<float>(f64Reference[i])) {
            ++out.unresolved;
            continue;
        }

        const double absError = std::abs(static_cast<double>(f32Stored[i]) - f64Reference[i]);
        out.maxAbsStorageError = std::max(out.maxAbsStorageError, absError);

        const auto p50Assessment = truthraw_precision_v07::assess_f64_to_f32_storage_v0_7(
            f64Reference[i], binding.p50, maxErrorOverP50);
        const auto p95Assessment = truthraw_precision_v07::assess_f64_to_f32_storage_v0_7(
            f64Reference[i], binding.p95, maxErrorOverP95);

        if (!p50Assessment.comparable || !p95Assessment.comparable ||
            p50Assessment.gaussianEquivalentComparison || p95Assessment.gaussianEquivalentComparison) {
            ++out.unresolved;
            continue;
        }

        ++out.resolved;
        out.maxErrorOverP50 = std::max(out.maxErrorOverP50, p50Assessment.errorOverAnchor);
        out.maxErrorOverP95 = std::max(out.maxErrorOverP95, p95Assessment.errorOverAnchor);

        if (p50Assessment.withinRequestedRatio) ++out.p50WithinRatio;
        else ++out.p50OutsideRatio;
        if (p95Assessment.withinRequestedRatio) ++out.p95WithinRatio;
        else ++out.p95OutsideRatio;
    }

    out.scientificAuthorityChanged = false;
    return out;
}

} // namespace truthraw_precision_v09

#include "uncertainty_relative_storage_v0_5.h"

#include <algorithm>
#include <cmath>

namespace truthraw_precision_v05 {

bool uncertainty_binding_exact_v0_5(const std::string& uncertaintyBindingSha256,
                                    const std::string& featureSchemaSha256) {
    return uncertaintyBindingSha256 == kExpectedUncertaintyBindingSha256V05 &&
           featureSchemaSha256 == kExpectedFeatureSchemaSha256V05;
}

UncertaintyRelativeStorageStatsV05 evaluate_f64_to_f32_storage_against_bound_uncertainty_v0_5(
    const StorageUncertaintySampleV05* samples,
    std::size_t count,
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256,
    std::vector<float>* storedF32) {

    UncertaintyRelativeStorageStatsV05 out{};
    out.total = static_cast<std::uint64_t>(count);
    out.bindingAccepted = uncertainty_binding_exact_v0_5(
        uncertaintyBindingSha256, featureSchemaSha256);

    if (storedF32) {
        storedF32->clear();
        storedF32->reserve(count);
    }
    if (!samples || count == 0) return out;

    long double sumSqError = 0.0L;
    long double sumSqP50Ratio = 0.0L;
    long double sumSqP95Ratio = 0.0L;

    for (std::size_t i = 0; i < count; ++i) {
        const auto& s = samples[i];
        const float q = static_cast<float>(s.f64Value);
        if (storedF32) storedF32->push_back(q);

        if (s.authority == ChannelAuthorityV05::MeasuredContribution) {
            ++out.measuredContributionSamples;
        } else {
            ++out.reconstructedSamples;
        }

        if (!std::isfinite(s.f64Value)) {
            ++out.nonFiniteValues;
            ++out.invalid;
            continue;
        }
        if (!std::isfinite(q)) {
            ++out.storageNonFinite;
            ++out.invalid;
            continue;
        }

        if (!out.bindingAccepted) {
            // Fail closed. Do not reinterpret the supplied uncertainty values
            // under a different or unknown model identity.
            ++out.invalid;
            continue;
        }

        switch (s.uncertainty.state) {
            case UncertaintyStateV05::Unknown:
                ++out.unknown;
                continue;
            case UncertaintyStateV05::Censored:
                ++out.censored;
                continue;
            case UncertaintyStateV05::Invalid:
                ++out.invalid;
                continue;
            case UncertaintyStateV05::Known:
                break;
        }

        const double p50 = s.uncertainty.p50;
        const double p95 = s.uncertainty.p95;
        if (!std::isfinite(p50) || !std::isfinite(p95) ||
            !(p50 > 0.0) || !(p95 > 0.0) || p95 < p50) {
            ++out.invalid;
            continue;
        }

        const double err = std::abs(s.f64Value - static_cast<double>(q));
        const double r50 = err / p50;
        const double r95 = err / p95;

        ++out.admitted;
        out.maxAbsStorageError = std::max(out.maxAbsStorageError, err);
        out.maxErrorOverP50 = std::max(out.maxErrorOverP50, r50);
        out.maxErrorOverP95 = std::max(out.maxErrorOverP95, r95);

        sumSqError += static_cast<long double>(err) * static_cast<long double>(err);
        sumSqP50Ratio += static_cast<long double>(r50) * static_cast<long double>(r50);
        sumSqP95Ratio += static_cast<long double>(r95) * static_cast<long double>(r95);
    }

    if (out.admitted > 0) {
        const long double n = static_cast<long double>(out.admitted);
        out.rmsStorageError = std::sqrt(static_cast<double>(sumSqError / n));
        out.rmsErrorOverP50 = std::sqrt(static_cast<double>(sumSqP50Ratio / n));
        out.rmsErrorOverP95 = std::sqrt(static_cast<double>(sumSqP95Ratio / n));
    }
    return out;
}

} // namespace truthraw_precision_v05

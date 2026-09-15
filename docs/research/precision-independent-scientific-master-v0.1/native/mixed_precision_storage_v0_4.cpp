#include "mixed_precision_storage_v0_4.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw_precision_v04 {

StorageQuantizationStatsV04 quantize_f64_to_f32_storage_v0_4(
    const double* input,
    std::size_t count,
    std::vector<float>& stored) {

    StorageQuantizationStatsV04 s{};
    stored.clear();
    if (!input || count == 0) return s;
    stored.resize(count);

    long double sumSq = 0.0L;
    for (std::size_t i = 0; i < count; ++i) {
        const double x = input[i];
        const float q = static_cast<float>(x);
        stored[i] = q;
        if (!std::isfinite(x)) {
            ++s.nonFiniteInputs;
            continue;
        }
        if (!std::isfinite(q)) {
            ++s.finiteRoundTripFailures;
            continue;
        }
        const double back = static_cast<double>(q);
        const double e = std::abs(back - x);
        s.maxAbsError = std::max(s.maxAbsError, e);
        sumSq += static_cast<long double>(e) * static_cast<long double>(e);
        const double denom = std::max(std::abs(x), std::numeric_limits<double>::min());
        s.maxRelativeError = std::max(s.maxRelativeError, e / denom);
    }
    s.samples = static_cast<std::uint64_t>(count);
    const std::uint64_t finiteCount = s.samples - s.nonFiniteInputs - s.finiteRoundTripFailures;
    if (finiteCount > 0) {
        s.rmsError = std::sqrt(static_cast<double>(sumSq / static_cast<long double>(finiteCount)));
    }
    return s;
}

bool mixed_precision_policy_authority_invariant_v0_4(const MixedPrecisionPolicyV04& p) {
    return p.preserveExactIntegerEvidence &&
           p.calibrationUsesFloat64 &&
           p.covarianceUsesFloat64 &&
           !p.mayChangeEvidenceAuthority;
}

} // namespace truthraw_precision_v04

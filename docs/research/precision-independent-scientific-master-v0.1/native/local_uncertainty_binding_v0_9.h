#pragma once

#include "uncertainty_relative_storage_v0_7.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw_precision_v09 {

struct LocalQuantileFieldV09 {
    const double* p50 = nullptr;
    const double* p95 = nullptr;
    const std::uint8_t* censored = nullptr;
    std::size_t count = 0;
    std::string uncertaintyBindingSha256;
    std::string featureSchemaSha256;
    bool sameScientificMasterQuantity = false;
};

struct LocalQuantileStorageStatsV09 {
    std::uint64_t samples = 0;
    std::uint64_t resolved = 0;
    std::uint64_t unresolved = 0;
    std::uint64_t censored = 0;
    std::uint64_t p50WithinRatio = 0;
    std::uint64_t p95WithinRatio = 0;
    std::uint64_t p50OutsideRatio = 0;
    std::uint64_t p95OutsideRatio = 0;
    double maxAbsStorageError = 0.0;
    double maxErrorOverP50 = 0.0;
    double maxErrorOverP95 = 0.0;
    bool bindingIdentityAccepted = false;
    bool sameScientificMasterQuantity = false;
    bool scientificAuthorityChanged = false;
};

// Evaluates already-produced local p50/p95 uncertainty fields against F64->F32
// storage quantization. This function does not generate uncertainty. The field
// must describe the exact same output scalar represented by f64Reference[i].
// Missing/mismatched identity, coordinate binding, censoring, invalid quantiles,
// or non-finite storage remains unresolved. Quantiles are never converted to
// Gaussian sigma or covariance.
LocalQuantileStorageStatsV09 assess_local_v5g_p1_quantile_storage_v0_9(
    const double* f64Reference,
    const float* f32Stored,
    std::size_t count,
    const LocalQuantileFieldV09& field,
    double maxErrorOverP50,
    double maxErrorOverP95);

} // namespace truthraw_precision_v09

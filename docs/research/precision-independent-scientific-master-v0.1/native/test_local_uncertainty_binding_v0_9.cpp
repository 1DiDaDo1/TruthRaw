#include "local_uncertainty_binding_v0_9.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    const std::vector<double> f64 = {0.125000003, 0.500000031, 0.875000049, 1.12500007};
    std::vector<float> f32(f64.size());
    for (std::size_t i = 0; i < f64.size(); ++i) f32[i] = static_cast<float>(f64[i]);

    const std::vector<double> p50 = {0.008, 0.009, 0.010, 0.011};
    const std::vector<double> p95 = {0.030, 0.031, 0.032, 0.033};
    const std::vector<std::uint8_t> censored = {0, 0, 1, 0};

    truthraw_precision_v09::LocalQuantileFieldV09 field;
    field.p50 = p50.data();
    field.p95 = p95.data();
    field.censored = censored.data();
    field.count = f64.size();
    field.uncertaintyBindingSha256 = truthraw_precision_v07::kV5gP1UncertaintyBindingSha256V07;
    field.featureSchemaSha256 = truthraw_precision_v07::kV5gP1FeatureSchemaSha256V07;
    field.sameScientificMasterQuantity = true;

    const auto ok = truthraw_precision_v09::assess_local_v5g_p1_quantile_storage_v0_9(
        f64.data(), f32.data(), f64.size(), field, 1e-3, 1e-3);
    assert(ok.samples == 4u);
    assert(ok.resolved == 3u);
    assert(ok.unresolved == 1u);
    assert(ok.censored == 1u);
    assert(ok.bindingIdentityAccepted);
    assert(ok.sameScientificMasterQuantity);
    assert(!ok.scientificAuthorityChanged);
    assert(ok.p50OutsideRatio == 0u);
    assert(ok.p95OutsideRatio == 0u);
    assert(ok.p50WithinRatio == 3u);
    assert(ok.p95WithinRatio == 3u);
    assert(ok.maxAbsStorageError > 0.0);

    auto badIdentity = field;
    badIdentity.uncertaintyBindingSha256 = "mismatch";
    const auto unresolvedIdentity = truthraw_precision_v09::assess_local_v5g_p1_quantile_storage_v0_9(
        f64.data(), f32.data(), f64.size(), badIdentity, 1e-3, 1e-3);
    assert(unresolvedIdentity.resolved == 0u);
    assert(unresolvedIdentity.unresolved == 4u);
    assert(!unresolvedIdentity.bindingIdentityAccepted);

    auto wrongQuantity = field;
    wrongQuantity.sameScientificMasterQuantity = false;
    const auto unresolvedQuantity = truthraw_precision_v09::assess_local_v5g_p1_quantile_storage_v0_9(
        f64.data(), f32.data(), f64.size(), wrongQuantity, 1e-3, 1e-3);
    assert(unresolvedQuantity.resolved == 0u);
    assert(unresolvedQuantity.unresolved == 4u);

    auto mismatchedStorage = f32;
    mismatchedStorage[1] = mismatchedStorage[1] + 0.01f;
    const auto storageMismatch = truthraw_precision_v09::assess_local_v5g_p1_quantile_storage_v0_9(
        f64.data(), mismatchedStorage.data(), f64.size(), field, 1e-3, 1e-3);
    assert(storageMismatch.resolved == 2u);
    assert(storageMismatch.unresolved == 2u);

    std::cout << "test_local_uncertainty_binding_v0_9 PASS"
              << " resolved=" << ok.resolved
              << " unresolved=" << ok.unresolved
              << " max_abs_storage=" << ok.maxAbsStorageError
              << "\n";
    return 0;
}

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw_precision_v05 {

// Exact binding declared by canonical/uncertainty/v5.0g-p1/UNCERTAINTY_MODEL_v5_0g.json.
// This research bridge does not recompute that model. It only consumes uncertainty
// bands produced under this binding and compares downstream storage quantization
// against them.
constexpr const char* kExpectedUncertaintyBindingSha256V05 =
    "61c99b0e29316730ca1323fd3e91fd069ebbc50cba73127cd81b9803b10178a0";
constexpr const char* kExpectedFeatureSchemaSha256V05 =
    "8c8e56b762b83a5a846c5201ab3ba43c9a2549d896873cc0ceb66574e74a83e3";

enum class UncertaintyStateV05 : std::uint8_t {
    Known = 0,
    Unknown = 1,
    Censored = 2,
    Invalid = 3,
};

enum class ChannelAuthorityV05 : std::uint8_t {
    MeasuredContribution = 0,
    Reconstructed = 1,
};

struct BoundUncertaintyBandsV05 {
    // Stage-2 normalized scene-linear absolute-error bands from the bound model.
    double p50 = 0.0;
    double p95 = 0.0;
    UncertaintyStateV05 state = UncertaintyStateV05::Unknown;
};

struct StorageUncertaintySampleV05 {
    // Result after all branch-sensitive scientific computation in Float64.
    double f64Value = 0.0;
    BoundUncertaintyBandsV05 uncertainty{};
    ChannelAuthorityV05 authority = ChannelAuthorityV05::Reconstructed;
};

struct UncertaintyRelativeStorageStatsV05 {
    std::uint64_t total = 0;
    std::uint64_t admitted = 0;
    std::uint64_t unknown = 0;
    std::uint64_t censored = 0;
    std::uint64_t invalid = 0;
    std::uint64_t nonFiniteValues = 0;
    std::uint64_t measuredContributionSamples = 0;
    std::uint64_t reconstructedSamples = 0;

    double maxAbsStorageError = 0.0;
    double rmsStorageError = 0.0;
    double maxErrorOverP50 = 0.0;
    double maxErrorOverP95 = 0.0;
    double rmsErrorOverP50 = 0.0;
    double rmsErrorOverP95 = 0.0;

    // This flag only means the requested uncertainty binding was exact. It does
    // not promote storage or photographic authority by itself.
    bool bindingAccepted = false;
};

bool uncertainty_binding_exact_v0_5(const std::string& uncertaintyBindingSha256,
                                    const std::string& featureSchemaSha256);

// Compare Float64 scientific results with their Float32 storage representation.
// A sample is admitted only when:
//  - binding + feature schema are exact;
//  - value is finite;
//  - uncertainty state is Known;
//  - p50 and p95 are finite, positive, and p95 >= p50.
// Unknown/censored/invalid samples stay out of the safety claim. No missing band
// is converted to zero uncertainty.
UncertaintyRelativeStorageStatsV05 evaluate_f64_to_f32_storage_against_bound_uncertainty_v0_5(
    const StorageUncertaintySampleV05* samples,
    std::size_t count,
    const std::string& uncertaintyBindingSha256,
    const std::string& featureSchemaSha256,
    std::vector<float>* storedF32 = nullptr);

} // namespace truthraw_precision_v05

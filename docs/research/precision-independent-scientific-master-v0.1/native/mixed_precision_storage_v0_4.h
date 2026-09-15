#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace truthraw_precision_v04 {

enum class ComputePrecisionV04 : std::uint8_t {
    Float32 = 0,
    Float64 = 1,
};

enum class StoragePrecisionV04 : std::uint8_t {
    Float32 = 0,
    Float64 = 1,
};

struct MixedPrecisionPolicyV04 {
    ComputePrecisionV04 stage2Compute = ComputePrecisionV04::Float64;
    ComputePrecisionV04 reconstructionCompute = ComputePrecisionV04::Float64;
    StoragePrecisionV04 scientificMasterStorage = StoragePrecisionV04::Float64;
    bool preserveExactIntegerEvidence = true;
    bool calibrationUsesFloat64 = true;
    bool covarianceUsesFloat64 = true;
    bool mayChangeEvidenceAuthority = false;
};

struct StorageQuantizationStatsV04 {
    std::uint64_t samples = 0;
    double maxAbsError = 0.0;
    double rmsError = 0.0;
    double maxRelativeError = 0.0;
    std::uint64_t nonFiniteInputs = 0;
    std::uint64_t finiteRoundTripFailures = 0;
};

// Quantizes an already-computed Float64 scientific result to Float32 storage.
// This is intentionally downstream of all branch-sensitive reconstruction.
// It does not re-run reconstruction and cannot alter which branch was selected.
StorageQuantizationStatsV04 quantize_f64_to_f32_storage_v0_4(
    const double* input,
    std::size_t count,
    std::vector<float>& stored);

// True only for policies that keep source evidence exact and do not let a
// precision/storage decision raise scientific authority.
bool mixed_precision_policy_authority_invariant_v0_4(const MixedPrecisionPolicyV04& p);

} // namespace truthraw_precision_v04

#pragma once

#include "truthnegative_continuous_v0_5.h"
#include "truthraw_sha256_v0_69.h"

#include <cstdint>
#include <string>

namespace truthraw::drawnegative::v0_1 {

namespace legacy_tn = truthraw::truthnegative_continuous::v0_5;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName = "D.RAW/D.RAWnegative/0.1";
inline constexpr const char* kStateMethod =
    "D_RAW_NEGATIVE_OBSERVATION_BOUND_STATE_V0_1";

enum class GaugeRelation : std::uint8_t {
    SourceLocalOnly = 0u,
    SharedRelative = 1u,
    SharedAbsolute = 2u,
};

enum class CanonicalStorage : std::uint8_t {
    Float32Validated = 0u,
    Float64 = 1u,
};

struct Input final {
    legacy_tn::State truthNegativeState{};
    std::string observationId{};
    std::string scaleGaugeId{};
    std::string sharedFreeWorldGaugeId{};
    GaugeRelation gaugeRelation = GaugeRelation::SourceLocalOnly;
    CanonicalStorage canonicalStorage = CanonicalStorage::Float32Validated;
    bool truthRangeCoordinateFamilyDeclared = true;
    bool perSampleTruthRangeMaterialized = false;
};

struct State final {
    Digest stateSha256{};
    Digest parentTruthNegativeStateSha256{};
    std::string observationId{};
    std::string scaleGaugeId{};
    std::string sharedFreeWorldGaugeId{};
    GaugeRelation gaugeRelation = GaugeRelation::SourceLocalOnly;
    CanonicalStorage canonicalStorage = CanonicalStorage::Float32Validated;
    bool finalized = false;
    bool isRasterIndependent = true;
    bool isPerObservationLineage = true;
    bool commonGaugeAdmitted = false;
    bool crossObservationRadiometricEqualityAllowed = false;
    bool crossObservationRadiometricFusionAllowed = false;
    bool truthRangeCoordinateFamilyDeclared = true;
    bool perSampleTruthRangeMaterialized = false;
    bool branchSensitiveComputeFloat64 = true;
    bool precisionUpgradesAuthority = false;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
    std::string methodId = kStateMethod;
};

bool finalize(const Input& input, State& out) noexcept;

const char* schema_name() noexcept;

} // namespace truthraw::drawnegative::v0_1

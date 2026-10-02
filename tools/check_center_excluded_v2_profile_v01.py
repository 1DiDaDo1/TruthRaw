#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"
CE_SPATIAL = ROOT / "docs/research/truthnegative-center-excluded-spatial-audit-v0.2.2/native"
CE_NEIGHBOR_ROOT = ROOT / "docs/research/truthnegative-center-excluded-neighborhood-v0.2"
CE_NEIGHBOR = CE_NEIGHBOR_ROOT / "native"

neighbor_h = (CE_NEIGHBOR / "truthnegative_center_excluded_neighborhood_v0_2.h").read_text()
neighbor_cpp = (CE_NEIGHBOR / "truthnegative_center_excluded_neighborhood_v0_2.cpp").read_text()
neighbor_test = (CE_NEIGHBOR_ROOT / "tests/test_truthnegative_center_excluded_neighborhood_v0_2.cpp").read_text()
spatial_h = (CE_SPATIAL / "truthnegative_center_excluded_spatial_audit_v0_2_2.h").read_text()
spatial_cpp = (CE_SPATIAL / "truthnegative_center_excluded_spatial_audit_v0_2_2.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
stage_cache = (JAVA / "ResearchProfileStageCacheV01.kt").read_text()
readme = (ROOT / "docs/research/center-excluded-v2-profile-v0.1/README.md").read_text()
version_code = int((ROOT / "suite_android/VERSION_CODE").read_text().strip())
lineage = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())

for token in [
    "struct Diagnostics final",
    "admissibilitySlotBuildMs",
    "pairBuildGateMs",
    "scaleConsistencyMs",
    "crossScaleCombineMs",
    "finalCombineMs",
    "profiledEstimateCount",
    "Diagnostics* diagnostics = nullptr",
    "enum class FixedTopologyOutcome",
    "NotApplicable",
    "estimateFixedTopology",
    "std::array<std::array<std::array<Slot,2u>,4u>,3u>",
]:
    assert token in neighbor_h, f"predictor diagnostic/fast-path contract missing {token}"

for token in [
    "diagnostics->admissibilitySlotBuildMs",
    "diagnostics->pairBuildGateMs",
    "diagnostics->scaleConsistencyMs",
    "diagnostics->crossScaleCombineMs",
    "diagnostics->finalCombineMs",
    "diagnostics->pairZDistanceCount",
    "diagnostics->directionalZDistanceCount",
    "diagnostics->crossScaleZDistanceCount",
    "if (diagnostics == nullptr)",
    "estimateFixedTopology(input,fixed,nullptr)",
    "FixedTopologyOutcome::Success",
]:
    assert token in neighbor_cpp, f"predictor implementation missing {token}"

for token in [
    "exact_result_equal",
    "require_fixed_parity",
    "FixedTopologyOutcome::NotApplicable",
    "n::estimate(in,generic,&forceGeneric)",
]:
    assert token in neighbor_test, f"exact parity oracle missing {token}"

for token in [
    "profileSampleStride = 64u",
    "profiledCandidateCount",
    "profiledNeighborAcquireMs",
    "profiledPathCensorMs",
    "profiledNeighborVarianceMs",
    "profiledPredictorEstimateMs",
    "predictorAdmissibilitySlotBuildMs",
    "predictorPairBuildGateMs",
    "predictorScaleConsistencyMs",
]:
    assert token in spatial_h, f"spatial profile contract missing {token}"

for token in [
    "currentProfileOrdinal=profileOrdinal++",
    "profileCandidate",
    "profiledPathStepCount",
    "ce::Diagnostics predictorProfile",
    "? &predictorProfile",
    ": nullptr",
    "profiledResidualMetricsMs",
]:
    assert token in spatial_cpp, f"spatial sampled profiling missing {token}"

# Diagnostic profile values must stay outside the established scientific hash.
hash_region = spatial_cpp[spatial_cpp.index("truthraw::sha256_v0_69::Hasher hasher"):]
for forbidden in [
    "profiledCandidateCount",
    "profiledNeighborAcquireMs",
    "profiledPathCensorMs",
    "predictorAdmissibilitySlotBuildMs",
    "predictorPairBuildGateMs",
    "profileSampleStride",
    "fixedTopology",
]:
    assert forbidden not in hash_region, f"diagnostic/fast-path state leaked into audit hash: {forbidden}"

for token in [
    "centerExcludedProfileSampleStride",
    "centerExcludedProfiledCandidateCount",
    "centerExcludedProfiledNeighborAcquireMs",
    "centerExcludedProfiledPathCensorMs",
    "centerExcludedPredictorAdmissibilitySlotBuildMs",
    "centerExcludedPredictorPairBuildGateMs",
]:
    assert token in bridge, f"JNI profile telemetry missing {token}"

for token in [
    "center_excluded_profile_sample_stride",
    "center_excluded_profiled_candidate_count",
    "center_excluded_profiled_neighbor_acquire_ms",
    "center_excluded_profiled_path_censor_ms",
    "center_excluded_predictor_admissibility_slot_build_ms",
    "center_excluded_predictor_pair_build_gate_ms",
]:
    assert token in binding, f"binding profile telemetry missing {token}"
    assert token in diag, f"Foundation profile telemetry missing {token}"

# Keep the R12 public stage identity but force one recompute for the new
# implementation epoch. The epoch exists only in the private derived cache
# wrapper and therefore cannot become scientific evidence.
for token in [
    "N2_LOCAL_SPATIAL_V01_R12_CENTER_EXCLUDED_PROFILE",
    "CENTER_EXCLUDED_FIXED_TOPOLOGY_V01",
    'wrapper.optString("implementation_epoch")',
    'wrapper.put("implementation_epoch", it)',
]:
    assert token in stage_cache, f"fixed-topology cache epoch missing {token}"

assert "N2_LOCAL_SPATIAL_V01_R12_CENTER_EXCLUDED_PROFILE" in profiler
assert "N2_LOCAL_SPATIAL_V01_R11_AUTHORITY_TEMPLATE_REUSE" in profiler
assert version_code >= 26100120
assert lineage["current_version_code"] == version_code
assert lineage["current_version_code"] > lineage["previous_version_code"]
assert lineage["scientific_authority_affected"] is False

for token in [
    "1-in-64",
    "DIAGNOSTIC_RUNTIME_ONLY",
    "must not be silently multiplied",
    "do not enter the v0.2.1 audit hash",
    "No SIMD/NEON",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

for text in [neighbor_h, neighbor_cpp, spatial_h, spatial_cpp, bridge, binding, diag, readme, stage_cache]:
    for forbidden in [
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
    ]:
        assert forbidden not in text, f"profiling gained scientific authority: {forbidden}"

print("center_excluded_v2_profile_v0_1_static_integrity=PASS")
print("center_excluded_fixed_topology_v0_1_static_integrity=PASS")

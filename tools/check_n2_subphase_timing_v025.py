#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"
CE = ROOT / "docs/research/truthnegative-center-excluded-spatial-audit-v0.2.2/native"

pipeline_h = (CPP / "truthnegative_pipeline_bridge_common.h").read_text()
pipeline_cpp = (CPP / "truthnegative_pipeline_bridge_common.cpp").read_text()
bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
ce_h = (CE / "truthnegative_center_excluded_spatial_audit_v0_2_2.h").read_text()
ce_cpp = (CE / "truthnegative_center_excluded_spatial_audit_v0_2_2.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()

for token in [
    "struct PreparationTiming",
    "sealSourceMs",
    "preOpenReverifyMs",
    "bindScientificMasterMs",
    "summarizeAuthorityFieldMs",
    "struct SharedAcquireTiming",
    "probeSealMs",
    "prepareTotalMs",
]:
    assert token in pipeline_h, f"shared preparation timing contract missing {token}"

for token in [
    "elapsed_ms(",
    "duplicateAndByteSourceMs",
    "sealSourceMs",
    "colorBindingMs",
    "prepareColorSourceMs",
    "preOpenReverifyMs",
    "openDngAdapterMs",
    "bindScientificMasterMs",
    "finalizePhase2Ms",
    "summarizeAuthorityFieldMs",
    "finalizeTruthNegativeMs",
    "finalizeDrawNegativeMs",
    "probeSealMs",
    "cacheLookupMs",
]:
    assert token in pipeline_cpp, f"shared preparation timing implementation missing {token}"

for token in [
    "struct Diagnostics",
    "fillStage2Ms",
    "candidateLoopMs",
    "predictorEstimateMs",
    "finalHashMs",
    "candidateTileCount",
    "candidateCenterCount",
    "timingIsScientificEvidence = false",
    "timingMayChangeScientificAuthority = false",
]:
    assert token in ce_h, f"center-excluded diagnostics contract missing {token}"

for token in [
    "Diagnostics* diagnostics",
    "diagnostics->fillStage2Ms",
    "diagnostics->candidateLoopMs",
    "diagnostics->predictorEstimateMs",
    "diagnostics->finalHashMs",
    "diagnostics->candidateCenterCount",
]:
    assert token in ce_cpp, f"center-excluded diagnostic timing missing {token}"

# Timing must stay outside the established scientific hash payload.
hash_region = ce_cpp[ce_cpp.index("truthraw::sha256_v0_69::Hasher hasher"):]
for forbidden in [
    "hash_f64(hasher,diagnostics",
    "hasher.update(diagnostics",
    "hash_u64(hasher,diagnostics",
]:
    assert forbidden not in hash_region, f"diagnostic timing leaked into scientific hash: {forbidden}"

for token in [
    "SharedAcquireTiming sharedAcquireTiming",
    "ce_sparse::Diagnostics centerExcludedDiagnostics",
    "sharedAcquireProbeSealMs",
    "prepareBindScientificMasterMs",
    "prepareSummarizeAuthorityFieldMs",
    "centerExcludedFillStage2Ms",
    "centerExcludedCandidateLoopMs",
    "centerExcludedPredictorEstimateMs",
    "centerExcludedCandidateCenterCount",
]:
    assert token in bridge, f"JNI subphase telemetry missing {token}"

for token in [
    "shared_acquire_subphase_timing_available",
    "shared_acquire_probe_seal_ms",
    "prepare_bind_scientific_master_ms",
    "prepare_summarize_authority_field_ms",
    "center_excluded_subphase_timing_available",
    "center_excluded_fill_stage2_ms",
    "center_excluded_candidate_loop_ms",
    "center_excluded_predictor_estimate_ms",
    "center_excluded_candidate_center_count",
]:
    assert token in binding, f"binding subphase telemetry missing {token}"

for token in [
    '"subphase_timing"',
    "shared_acquire_probe_seal_ms",
    "prepare_bind_scientific_master_ms",
    "center_excluded_candidate_loop_ms",
    "center_excluded_candidate_center_count",
    '"timing_is_scientific_evidence", false',
    '"timing_may_change_scientific_authority",',
]:
    assert token in diag, f"research subphase telemetry missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.9-sha-direct-block-v1",
    "N2_LOCAL_SPATIAL_V01_R9_SHA_DIRECT_BLOCK",
]:
    assert token in profiler, f"fresh subphase profiler identity missing {token}"

# Diagnostic timing cannot promote calibration, correction, evidence, or writeback.
for text in [binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"subphase telemetry gained authority: {forbidden}"

print("n2_subphase_timing_v0_2_5_static_integrity=PASS")

#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

trace = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
aggregate = (JAVA / "FreeWorldPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
profile_store = (JAVA / "ResearchUniversalProfileStoreV01.kt").read_text()
foundation = (JAVA / "FreeWorldObservationGeometryFoundationV01.kt").read_text()

for token in [
    "D.RAW/ResearchPerformanceDiagnostics/0.1",
    "SystemClock.elapsedRealtimeNanos()",
    "profile_elapsed_ms",
    "profile_cache_generation",
    "derived_stage_cache_hit",
    "shared_pipeline_prepare_cache_hit",
    "cache_hit_count",
    "cache_miss_count",
    "max_process_local_contexts",
    "single_source_context_policy",
    "cross_observation_context_reuse_allowed",
    "cache_release_after_profile_attempted",
    "cache_release_after_profile_succeeded",
    "source_values_modified",
    "candidate_applied",
    "creates_new_evidence",
    "scientific_writeback_allowed",
    "row_band_telemetry_reported",
    "row_band_reuse_active",
    "row_band_peak_cache_bytes",
    "row_band_scientific_values_modified",
    "native_phase_timing_available",
    "phase_shared_acquire_ms",
    "phase_v01_cfa_audit_ms",
    "phase_center_excluded_ms",
    "phase_confidence_derive_ms",
    "phase_factored_derive_ms",
    "phase_factored_encode_ms",
    "phase_write_readback_reverify_ms",
    "phase_total_bridge_ms",
    "phase_timing_is_scientific_evidence",
    "phase_timing_may_change_scientific_authority",
    "profile_elapsed_scope",
    "stage_elapsed_scope",
    "CURRENT_RUN_SEPARATE_FROM_CACHED_ORIGIN_COMPUTE",
    "current_run_stage_elapsed_ms",
    "current_run_derived_stage_cache_hit",
    "native_telemetry_origin",
    "native_phase_timing_origin",
    "native_phase_timing_represents_current_run",
    "native_phase_timing_is_cached_origin_compute",
    "current_run_derived_stage_cache_hit_count",
    "current_run_derived_stage_cache_miss_count",
    "current_run_native_audit_compute_count",
    "cached_origin_native_audit_telemetry_count",
]:
    assert token in trace, f"trace missing {token}"

for token in [
    "D.RAW/FreeWorldPerformanceDiagnostics/0.1",
    "PERFORMANCE_DIAGNOSTICS_AVAILABLE",
    "aggregate_profile_elapsed_ms",
    "expected_profile_cache_generation",
    "profile_cache_generation",
    "shared_scientific_preparation_summary",
    "profiles_cache_release_succeeded",
    "all_reported_profiles_single_source_context_policy",
    "all_reported_profiles_cross_observation_reuse_disallowed",
    "DIAGNOSTIC_RUNTIME_ONLY",
    "timing_is_scientific_evidence",
    "performance_changes_measurement_authority",
    "creates_new_evidence",
    "scientific_writeback_allowed",
    "aggregate_profile_elapsed_scope",
    "timing_provenance_policy",
    "profiles_with_cached_origin_compute_telemetry",
    "current_run_derived_stage_cache_hit_count",
    "current_run_derived_stage_cache_miss_count",
    "current_run_native_audit_compute_count",
    "cached_origin_native_audit_telemetry_count",
]:
    assert token in aggregate, f"aggregate missing {token}"

for token in [
    "ResearchPerformanceDiagnosticsV01.ProfileTrace()",
    "trace.onProgress(event)",
    "clearSharedPipelineCache()",
    "trace.attach(",
    "D.RAW/UniversalSourceProfileCache/0.2.10-authority-pixel-triplet-v1",
    '"profile_cache_generation"',
    "N2_LOCAL_SPATIAL_V01_R10_AUTHORITY_PIXEL_TRIPLET",
]:
    assert token in profiler, f"profiler missing {token}"

for token in [
    "isCurrentProfile",
    '"profile_cache_generation"',
    "UniversalSourceProfiler.CACHE_GENERATION",
    "Refusing to persist a UniversalSourceProfile from a stale profiler cache generation.",
]:
    assert token in profile_store, f"profile store missing {token}"

for token in [
    '"performance_diagnostics_v0_1"',
    "FreeWorldPerformanceDiagnosticsV01.build(profiles)",
]:
    assert token in foundation, f"foundation missing {token}"

# Diagnostics may observe performance only. They must not gain scientific
# authority or enable correction/writeback.
for forbidden in [
    '.put("calibration_promoted", true)',
    '.put("correction_authorized", true)',
    '.put("scientific_writeback_allowed", true)',
    '.put("creates_new_evidence", true)',
    '.put("candidate_applied", true)',
]:
    assert forbidden not in trace
    assert forbidden not in aggregate
    assert forbidden not in profile_store

print("research_performance_diagnostics_v0_1_integrity=PASS")

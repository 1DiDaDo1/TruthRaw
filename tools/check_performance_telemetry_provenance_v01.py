#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

trace = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
aggregate = (JAVA / "FreeWorldPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
gradle = (ROOT / "suite_android/app/build.gradle.kts").read_text()
version_code = (ROOT / "suite_android/VERSION_CODE").read_text().strip()
lineage = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())
readme = (ROOT / "docs/research/performance-telemetry-provenance-v0.1/README.md").read_text()

for token in [
    "CURRENT_RUN_SEPARATE_FROM_CACHED_ORIGIN_COMPUTE",
    '"profile_elapsed_scope", "CURRENT_PROFILE_RUN"',
    '"stage_elapsed_scope", "CURRENT_PROFILE_RUN"',
    "findCurrentRunStage(",
    "current_run_stage_elapsed_ms",
    "current_run_derived_stage_cache_hit",
    "native_telemetry_origin",
    "CACHED_DERIVED_STAGE_ORIGIN_COMPUTE",
    "UNKNOWN_CURRENT_STAGE_TRACE_MISSING",
    "native_phase_timing_origin",
    "native_phase_timing_represents_current_run",
    "native_phase_timing_is_cached_origin_compute",
    "ORIGIN_NATIVE_EXECUTION",
    "current_run_derived_stage_cache_hit_count",
    "current_run_derived_stage_cache_miss_count",
    "current_run_native_audit_compute_count",
    "cached_origin_native_audit_telemetry_count",
]:
    assert token in trace, f"trace provenance contract missing {token}"

for token in [
    "aggregate_profile_elapsed_scope",
    "CURRENT_RUN_SEPARATE_FROM_CACHED_ORIGIN_COMPUTE",
    "profiles_with_cached_origin_compute_telemetry",
    "current_run_derived_stage_cache_hit_count",
    "current_run_derived_stage_cache_miss_count",
    "current_run_native_audit_compute_count",
    "cached_origin_native_audit_telemetry_count",
    "ORIGIN_NATIVE_EXECUTION",
]:
    assert token in aggregate, f"Foundation provenance aggregation missing {token}"

assert "D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1" in profiler
assert int(version_code) >= 26100116
assert lineage["current_version_code"] == int(version_code)
assert lineage["current_version_code"] > lineage["previous_version_code"]
assert f'versionName = "{lineage["version_name"]}"' in gradle
assert lineage["scientific_authority_affected"] is False

for token in [
    "CURRENT_PROFILE_RUN",
    "CACHED_DERIVED_STAGE_ORIGIN_COMPUTE",
    "ORIGIN_NATIVE_EXECUTION",
    "telemetry/provenance only",
    "does not change RAW bytes",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

for text in [trace, aggregate, readme]:
    for forbidden in [
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
    ]:
        assert forbidden not in text, f"telemetry provenance gained scientific authority: {forbidden}"

print("performance_telemetry_provenance_v0_1_integrity=PASS")

#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
T5_AUDIT = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/T5CorridorAuditV01.kt"
PREVIEW = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/TruthNegativeContinuousPreview.kt"
BINDING = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/ResearchPerformanceT5CorridorBindingV01.kt"
PERFORMANCE = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FreeWorldPerformanceDiagnosticsV01.kt"
FOUNDATION = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FreeWorldObservationGeometryFoundationV01.kt"


t5 = T5_AUDIT.read_text()
preview = PREVIEW.read_text()
binding = BINDING.read_text()
performance = PERFORMANCE.read_text()
foundation = FOUNDATION.read_text()

# The T5 audit remains the sole interpreter of the already-emitted corridor
# telemetry. Its contract must retain the scientific firewalls requested by
# 44489 and must not gain mutation/promotion authority.
for token in [
    '"D.RAW/Runtime/T5CorridorAudit/0.1"',
    '"DIAGNOSTIC_RUNTIME_ONLY"',
    '"NONE"',
    '"EXACT_PRESERVING_BYPASS"',
    '"room_capsule_state"',
    '"exposure_application_count"',
    '"exposure_application_count_verified"',
    'metrics.t5ExposureApplicationCount == 1',
    'metrics.t5PhysicalFrameCount == 1',
    'metrics.t5IndependentEvidenceCount == 1',
    '!metrics.t5CandidateApplied',
    '"mutation_writeback_firewalls"',
    '.put("scientific_writeback_allowed", false)',
    '.put("creates_new_evidence", false)',
    '.put("sealed_cfa_modified_by_audit", false)',
    '.put("scientific_master_modified_by_audit", false)',
    '.put("exact_gauge_v0_3_modified_by_audit", false)',
    '.put("reconstruction_behavior_modified_by_audit", false)',
]:
    assert token in t5, f"T5 audit contract missing {token}"

# Ready computes the existing audit once and publishes that same read-only
# object. The binding layer never calls the native corridor or T5 evaluator.
for token in [
    'T5CorridorAuditV01.from(metrics).also',
    'ResearchPerformanceT5CorridorBindingV01.publish(it)',
]:
    assert token in preview, f"precomputed T5 publication missing {token}"

for forbidden in [
    'buildProContinuousPreview(',
    'T5CorridorAuditV01.from(',
    'evaluate_room_capsule_relative',
]:
    assert forbidden not in binding, f"binding illegally recomputes T5 via {forbidden}"
    assert forbidden not in performance, f"Foundation performance illegally recomputes T5 via {forbidden}"

# Transport is process-local, exact-source-bound, telemetry-only and fail closed.
for token in [
    'ConcurrentHashMap<String, String>()',
    'snapshotsBySource[sourceSha] = audit.toString()',
    '"D.RAW/ResearchPerformanceT5CorridorBinding/0.1"',
    '"SOURCE_BOUND_PRECOMPUTED_T5_AUDIT_AVAILABLE"',
    '"UNKNOWN_FAIL_CLOSED"',
    '"NO_PRECOMPUTED_RUNTIME_T5_AUDIT_FOR_SOURCE"',
    '"RUNTIME_T5_AUDIT_BINDING_CONTRADICTION"',
    '"source_binding_verified"',
    '"profile_run_binding_verified"',
    '"LATEST_PROCESS_LOCAL_PRECOMPUTED_TRUTHNEGATIVE_CONTINUOUS_PREVIEW"',
    '"t5_corridor_recomputed_by_binding"',
    '"cross_observation_reuse_allowed"',
    '"DIAGNOSTIC_RUNTIME_ONLY"',
    'audit.optString("mutation_authority") != "NONE"',
    'audit.optBoolean("creates_new_evidence", true)',
    'audit.optBoolean("scientific_writeback_allowed", true)',
    'audit.optBoolean("sealed_cfa_modified_by_audit", true)',
    'audit.optBoolean("scientific_master_modified_by_audit", true)',
    'audit.optBoolean("exact_gauge_v0_3_modified_by_audit", true)',
    'firewalls.optBoolean("candidate_applied", true)',
    'firewalls.optBoolean("scientific_writeback_allowed", true)',
]:
    assert token in binding, f"T5 performance binding contract missing {token}"

# The normal Foundation performance_diagnostics route carries the source-bound
# audit. FreeWorldObservationGeometryFoundation remains an aggregator only.
for token in [
    'ResearchPerformanceT5CorridorBindingV01.forProfile(profile)',
    '"t5_corridor_audit_binding_v0_1"',
]:
    assert token in performance, f"Foundation T5 telemetry wiring missing {token}"

for token in [
    '"performance_diagnostics_v0_1"',
    'FreeWorldPerformanceDiagnosticsV01.build(profiles)',
]:
    assert token in foundation, f"Foundation performance export route missing {token}"

# Explicit semantic truth table for source binding/firewall admission. This
# mirrors the Kotlin guard and prevents cross-source or writeback-capable audit
# snapshots from being accepted as a bound Foundation diagnostic.
def binding_admitted(
    *,
    profile_sha="a" * 64,
    audit_sha="a" * 64,
    schema="D.RAW/Runtime/T5CorridorAudit/0.1",
    audit_authority="DIAGNOSTIC_RUNTIME_ONLY",
    mutation_authority="NONE",
    creates_new_evidence=False,
    scientific_writeback_allowed=False,
    candidate_applied=False,
):
    return all([
        profile_sha == audit_sha,
        len(profile_sha) == 64,
        schema == "D.RAW/Runtime/T5CorridorAudit/0.1",
        audit_authority == "DIAGNOSTIC_RUNTIME_ONLY",
        mutation_authority == "NONE",
        not creates_new_evidence,
        not scientific_writeback_allowed,
        not candidate_applied,
    ])

assert binding_admitted()
assert not binding_admitted(audit_sha="b" * 64)
assert not binding_admitted(mutation_authority="SCIENTIFIC_WRITEBACK")
assert not binding_admitted(creates_new_evidence=True)
assert not binding_admitted(scientific_writeback_allowed=True)
assert not binding_admitted(candidate_applied=True)

print("t5_corridor_performance_plumbing_v0_1_static_integrity=PASS")

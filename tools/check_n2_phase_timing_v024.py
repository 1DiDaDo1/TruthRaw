#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"
CPP = ROOT / "suite_android/app/src/main/cpp"

bridge = (CPP / "truthnegative_n2_factored_confidence_bridge.cpp").read_text()
binding = (JAVA / "N2LocalSpatialBindingAudit.kt").read_text()
diag = (JAVA / "ResearchPerformanceDiagnosticsV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()

for token in [
    "#include <chrono>",
    "using SteadyClock = std::chrono::steady_clock",
    "elapsed_ms(",
    "phaseSharedAcquireMs",
    "phaseSharedContextLockWaitMs",
    "phaseV01CfaAuditMs",
    "phaseCenterExcludedMs",
    "phaseConfidenceDeriveMs",
    "phaseFactoredDeriveMs",
    "phaseFactoredEncodeMs",
    "phaseWriteReadbackReverifyMs",
    "phaseTotalBridgeMs",
    "phaseTimingIsScientificEvidence",
    "phaseTimingMayChangeScientificAuthority",
]:
    assert token in bridge, f"native phase timing missing {token}"

# Timings are emitted only in the JNI status side-channel. They must not enter
# the encoded factored scientific report or any established scientific hash.
status_region = bridge[bridge.index("std::ostringstream o;"):]
for token in [
    "phaseSharedAcquireMs",
    "phaseSharedContextLockWaitMs",
    "phaseV01CfaAuditMs",
    "phaseCenterExcludedMs",
    "phaseConfidenceDeriveMs",
    "phaseFactoredDeriveMs",
    "phaseFactoredEncodeMs",
    "phaseWriteReadbackReverifyMs",
    "phaseTotalBridgeMs",
]:
    assert token in status_region, f"phase timing not bound to diagnostic status: {token}"

for token in [
    "native_phase_timing_available",
    "phase_shared_acquire_ms",
    "phase_shared_context_lock_wait_ms",
    "phase_v01_cfa_audit_ms",
    "phase_center_excluded_ms",
    "phase_confidence_derive_ms",
    "phase_factored_derive_ms",
    "phase_factored_encode_ms",
    "phase_write_readback_reverify_ms",
    "phase_total_bridge_ms",
    "phase_timing_is_scientific_evidence",
    "phase_timing_may_change_scientific_authority",
]:
    assert token in binding, f"N2 binding missing {token}"
    assert token in diag, f"performance diagnostics missing {token}"

for token in [
    "D.RAW/UniversalSourceProfileCache/0.2.6-authority-fused-v1",
    "N2_LOCAL_SPATIAL_V01_R6_AUTHORITY_FUSED",
]:
    assert token in profiler, f"fresh phase-timing profile identity missing {token}"

# Runtime timing may diagnose performance only. It cannot create evidence,
# authorize a candidate/correction, or change Scientific Master authority.
for text in [binding, diag]:
    for forbidden in [
        '.put("calibration_promoted", true)',
        '.put("correction_authorized", true)',
        '.put("candidate_applied", true)',
        '.put("creates_new_evidence", true)',
        '.put("scientific_writeback_allowed", true)',
    ]:
        assert forbidden not in text, f"phase telemetry gained authority: {forbidden}"

assert 'o<<",\\\"phaseTimingIsScientificEvidence\\\":false"' in bridge
assert 'o<<",\\\"phaseTimingMayChangeScientificAuthority\\\":false"' in bridge

print("n2_phase_timing_v0_2_4_static_integrity=PASS")

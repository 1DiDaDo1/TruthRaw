#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ROOT / "suite_android/app/src/main/cpp/scientific_master_pass_artifact_dispatch_v0_1.cpp"
JNI = ROOT / "suite_android/app/src/main/cpp/truthnegative_n2_factored_confidence_bridge.cpp"
N2_AUDIT = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/N2LocalSpatialBindingAudit.kt"
ATTRIBUTION = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/ScientificMasterPassArtifactPerformanceAttributionV01.kt"
PERFORMANCE = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/ResearchPerformanceDiagnosticsV01.kt"

native = NATIVE.read_text()
jni = JNI.read_text()
n2_audit = N2_AUDIT.read_text()
attribution = ATTRIBUTION.read_text()
performance = PERFORMANCE.read_text()

# Native telemetry must be published only after a successful, firewall-clean
# Scientific-Master bind and must be retrieved only by exact Scientific-Master
# Hash256 equality. Telemetry failure is not allowed to trigger scientific replay.
for token in [
    "publish_bound_diagnostics(out.scientificMasterHash, diagnostics);",
    "scientificMasterHash != gBoundScientificMasterHash",
    '\"binding\":\"SCIENTIFIC_MASTER_SHA256\"',
    '\"available\":false',
    '\"binding_verified\":false',
    '\"available\":true',
    '\"binding_verified\":true',
    '\"authority\":\"DIAGNOSTIC_RUNTIME_ONLY\"',
    "if (diagnostics.sourceValuesModified)",
    "if (diagnostics.createsNewEvidence)",
    "if (diagnostics.scientificWritebackAllowed)",
    "if (diagnostics.candidateApplied)",
    "if (!status)",
]:
    assert token in native, f"native PassArtifact telemetry contract missing {token}"

status_check = native.index("if (!status)")
publish = native.index("publish_bound_diagnostics(out.scientificMasterHash, diagnostics);")
assert status_check < publish, "diagnostics published before successful scientific bind"
for firewall_token in [
    "if (diagnostics.sourceValuesModified)",
    "if (diagnostics.createsNewEvidence)",
    "if (diagnostics.scientificWritebackAllowed)",
    "if (diagnostics.candidateApplied)",
]:
    assert native.index(firewall_token) < publish, (
        f"diagnostics published before firewall check {firewall_token}"
    )

assert "bound_diagnostics_json(" in jni
assert "ctx->scientific.scientificMasterHash" in jni
assert "last_thread_diagnostics()" not in jni, (
    "JNI attribution must not use thread-local diagnostics as cross-stage authority"
)

# N2 binding audit may carry unavailable telemetry forward, but any available
# snapshot that violates the diagnostic schema/firewall must fail closed.
for token in [
    '"scientificMasterPassArtifactDiagnostics"',
    '"D.RAW/ScientificMasterPassArtifactDiagnostics/0.1"',
    '"PASS_ARTIFACT_DIAGNOSTICS_CONTRADICTION"',
    '"EXACT_GAUGE_FLOAT32_BITS"',
    '"0.3"',
    '"candidate_applied"',
    '"source_values_modified"',
    '"creates_new_evidence"',
    '"scientific_writeback_allowed"',
    '"scientific_master_pass_artifact_v0_1"',
]:
    assert token in n2_audit, f"N2 PassArtifact audit contract missing {token}"

# Foundation route attribution must derive authority only from explicit native
# diagnostics, never from source-read counts or timing heuristics.
for token in [
    '"D.RAW/ScientificMasterPassArtifactAttribution/0.1"',
    '"D.RAW/ScientificMasterPassArtifactDiagnostics/0.1"',
    '"SCIENTIFIC_MASTER_SHA256"',
    '"EXACT_GAUGE_FLOAT32_BITS"',
    '"0.3"',
    '"retained_exact_float32_bits_v0.3"',
    '"canonical_v0.2_fallback"',
    '"UNKNOWN_FAIL_CLOSED"',
    '"EXACT_GAUGE_RETAINED_V0_3"',
    '"CANONICAL_V0_2_FALLBACK"',
    '"SCIENTIFIC_MASTER_HASH_BINDING_UNVERIFIED"',
    '"SCIENTIFIC_MASTER_HASH_BINDING_CONTRACT_MISMATCH"',
    '"DIAGNOSTICS_SCHEMA_MISMATCH"',
    '"ARTIFACT_TYPE_MISMATCH"',
    '"ARTIFACT_VERSION_MISMATCH"',
    '"CALLER_BUDGET_MISMATCH"',
    '"CANDIDATE_APPLIED_FORBIDDEN"',
    '"SOURCE_VALUES_MODIFIED_FORBIDDEN"',
    '"CREATES_NEW_EVIDENCE_FORBIDDEN"',
    '"SCIENTIFIC_WRITEBACK_FORBIDDEN"',
    '"RETAINED_ROUTE_WITHOUT_OPTIMIZATION"',
    '"RETAINED_ROUTE_WITH_FALLBACK_REASON"',
    '"RETAINED_ROUTE_WITHOUT_AVOIDED_PASS2_READS"',
    '"CANONICAL_FALLBACK_WITH_OPTIMIZATION"',
    '"CANONICAL_FALLBACK_WITHOUT_REASON"',
    '"CANONICAL_FALLBACK_WITH_AVOIDED_PASS2_READS"',
    '"UNKNOWN_ROUTE"',
    '.put("attribution_uses_source_read_count", false)',
    '.put("timing_is_scientific_evidence", false)',
    '.put("timing_may_change_scientific_authority", false)',
    '.put("creates_new_evidence", false)',
    '.put("scientific_writeback_allowed", false)',
]:
    assert token in attribution, f"Foundation PassArtifact attribution missing {token}"

for forbidden in [
    'source_read_raw_call_count',
    'reconstruction_call_count',
    '.put("attribution_uses_source_read_count", true)',
    '.put("timing_is_scientific_evidence", true)',
    '.put("timing_may_change_scientific_authority", true)',
]:
    assert forbidden not in attribution, (
        f"route attribution illegally depends on independent performance evidence: {forbidden}"
    )

# The performance report may show read counts next to route attribution, but it
# must label the count as non-authoritative and name the diagnostics-only policy.
for token in [
    '"scientific_master_pass_artifact_attribution_v0_1"',
    "ScientificMasterPassArtifactPerformanceAttributionV01.from(",
    '"scientific_master_pass_artifact_v0_1"',
    '"source_read_raw_call_count"',
    '"source_read_raw_count_is_route_evidence"',
    'false',
    '"EXPLICIT_PASS_ARTIFACT_DIAGNOSTICS_ONLY"',
    '"DIAGNOSTIC_RUNTIME_ONLY"',
]:
    assert token in performance, f"performance attribution wiring missing {token}"

# Small explicit truth table for the contract the Kotlin source is required to
# encode. This does not replace runtime testing; it prevents the static gate
# itself from silently accepting ambiguous route semantics.
def classify(
    *,
    available=True,
    schema="D.RAW/ScientificMasterPassArtifactDiagnostics/0.1",
    binding_verified=True,
    binding="SCIENTIFIC_MASTER_SHA256",
    artifact_type="EXACT_GAUGE_FLOAT32_BITS",
    artifact_version="0.3",
    budget=64 * 1024 * 1024,
    expected_budget=64 * 1024 * 1024,
    candidate_applied=False,
    source_values_modified=False,
    creates_new_evidence=False,
    scientific_writeback_allowed=False,
    route="retained_exact_float32_bits_v0.3",
    fallback_reason="none",
    optimization_applied=True,
    pass2_reads_avoided=1,
):
    if not available:
        return "UNKNOWN_FAIL_CLOSED"
    contradiction = any([
        schema != "D.RAW/ScientificMasterPassArtifactDiagnostics/0.1",
        not binding_verified,
        binding != "SCIENTIFIC_MASTER_SHA256",
        artifact_type != "EXACT_GAUGE_FLOAT32_BITS",
        artifact_version != "0.3",
        budget != expected_budget,
        candidate_applied,
        source_values_modified,
        creates_new_evidence,
        scientific_writeback_allowed,
    ])
    if route == "retained_exact_float32_bits_v0.3":
        contradiction = contradiction or (
            not optimization_applied
            or fallback_reason != "none"
            or pass2_reads_avoided <= 0
        )
        resolved = "EXACT_GAUGE_RETAINED_V0_3"
    elif route == "canonical_v0.2_fallback":
        contradiction = contradiction or (
            optimization_applied
            or fallback_reason in {"none", "unknown"}
            or pass2_reads_avoided != 0
        )
        resolved = "CANONICAL_V0_2_FALLBACK"
    else:
        contradiction = True
        resolved = "UNKNOWN_FAIL_CLOSED"
    return "UNKNOWN_FAIL_CLOSED" if contradiction else resolved

assert classify() == "EXACT_GAUGE_RETAINED_V0_3"
assert classify(
    route="canonical_v0.2_fallback",
    fallback_reason="caller_budget_exceeded",
    optimization_applied=False,
    pass2_reads_avoided=0,
) == "CANONICAL_V0_2_FALLBACK"
assert classify(available=False) == "UNKNOWN_FAIL_CLOSED"
assert classify(binding_verified=False) == "UNKNOWN_FAIL_CLOSED"
assert classify(binding="THREAD_LOCAL_ONLY") == "UNKNOWN_FAIL_CLOSED"
assert classify(source_values_modified=True) == "UNKNOWN_FAIL_CLOSED"
assert classify(route="unknown") == "UNKNOWN_FAIL_CLOSED"
assert classify(pass2_reads_avoided=0) == "UNKNOWN_FAIL_CLOSED"
assert classify(
    route="canonical_v0.2_fallback",
    fallback_reason="caller_budget_exceeded",
    optimization_applied=True,
    pass2_reads_avoided=0,
) == "UNKNOWN_FAIL_CLOSED"

print("scientific_master_pass_artifact_attribution_v0_1_static_integrity=PASS")

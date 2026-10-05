#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BINDER = ROOT / "docs/research/scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.cpp"
FOUNDATION = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FreeWorldPerformanceDiagnosticsV01.kt"
README = ROOT / "docs/research/scientific-master-tile-read-attribution-v0.1/README.md"

binder = BINDER.read_text()
foundation = FOUNDATION.read_text()
readme = README.read_text()

# Canonical v0.2 remains the proven deterministic two-pass fallback schedule.
for token in [
    "Status for_each_canonical_tile(",
    "const auto fill = fill_stage2(source, tile, workspace);",
    "auto firstPass = for_each_canonical_tile(",
    "auto secondPass = for_each_canonical_tile(",
]:
    assert token in binder, f"v0.2 binder schedule proof missing {token}"

first_pass = binder.index("auto firstPass = for_each_canonical_tile(")
second_pass = binder.index("auto secondPass = for_each_canonical_tile(")
assert first_pass < second_pass
assert "reconstruction.reconstructTile(" in binder[first_pass:second_pass], (
    "reconstruction must be present in canonical pass 1"
)
assert "reconstruction.reconstructTile(" not in binder[second_pass:], (
    "canonical v0.2 pass 2 must remain non-reconstructing"
)

# Foundation attribution must select route semantics only from the existing
# hash-bound PassArtifact attribution, never from independent read counts.
for token in [
    '"D.RAW/ScientificMasterTileReadAttribution/0.1"',
    '"D.RAW/ScientificMasterPassArtifactAttribution/0.1"',
    '"DIAGNOSTIC_ROUTE_ATTRIBUTION"',
    '"DIAGNOSTIC_RUNTIME_ONLY"',
    '"EXACT_GAUGE_RETAINED_V0_3"',
    '"CANONICAL_V0_2_FALLBACK"',
    '"EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED"',
    '"CANONICAL_V0_2_TWO_PASS_RECONCILED"',
    '"UNKNOWN_FAIL_CLOSED"',
    '"EXPLICIT_PASS_ARTIFACT_DIAGNOSTICS_EXACT_GAUGE_V0_3"',
    '"EXPLICIT_PASS_ARTIFACT_DIAGNOSTICS_CANONICAL_V0_2_FALLBACK"',
    '"expected_two_pass_raw_call_count"',
    '"expected_active_route_raw_call_count"',
    '"pass_1_raw_call_count"',
    '"pass_2_raw_call_count"',
    '"pass_2_raw_calls_avoided"',
    '"unattributed_raw_call_count"',
    '"raw_call_count_reconciles"',
    '"route_attribution_verified"',
    '"per_pass_source_read_timing_available", false',
    '"per_pass_timing_inferred", false',
    '"source_values_modified", false',
    '"candidate_applied", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
    '"scientific_master_pass_artifact_attribution_v0_1"',
]:
    assert token in foundation, f"route-aware tile-read attribution missing {token}"

# Route choice occurs before count reconciliation; counts can only confirm the
# already-proven route footprint.
exact_route_index = foundation.index(
    'routeAttribution == "EXACT_GAUGE_RETAINED_V0_3"'
)
canonical_route_index = foundation.index(
    'routeAttribution == "CANONICAL_V0_2_FALLBACK"'
)
raw_reconcile_index = foundation.index(
    'rawCalls == expectedActiveRouteRawCalls'
)
assert exact_route_index < raw_reconcile_index
assert canonical_route_index < raw_reconcile_index
assert 'rawCalls == reconstructionCalls' not in foundation
assert 'rawCalls == canonicalTwoPassRawCalls' not in foundation

# Exact Gauge v0.3 one-pass reconciliation must require explicit, firewall-clean
# PassArtifact telemetry and exact avoided-pass count agreement.
for token in [
    'passArtifactAttribution.optBoolean("telemetry_available", false)',
    'passArtifactAttribution.optBoolean("binding_verified", false)',
    '"route_attribution_contradiction"',
    'passArtifactAttribution.optBoolean("creates_new_evidence", true)',
    '"scientific_writeback_allowed"',
    'passArtifactAttribution!!.optBoolean("optimization_applied", false)',
    '"stage2_gauge_scan_passes_actually_used"',
    '"pass2_stage2_tile_reads_avoided"',
    '== reconstructionCalls',
    'exactGaugeRoute -> reconstructionCalls',
    'exactGaugeRoute -> 0L',
]:
    assert token in foundation, f"Exact Gauge reconciliation missing {token}"

# Canonical fallback remains two-pass and explicitly non-optimized.
for token in [
    '!passArtifactAttribution!!.optBoolean("optimization_applied", true)',
    'canonicalFallbackRoute && safeTwoPassCount -> canonicalTwoPassRawCalls',
    'canonicalFallbackRoute -> reconstructionCalls',
    'canonicalFallbackRoute -> 0L',
]:
    assert token in foundation, f"canonical fallback reconciliation missing {token}"

for forbidden in [
    '"per_pass_source_read_timing_available", true',
    '"per_pass_timing_inferred", true',
    '.put("source_values_modified", true)',
    '.put("candidate_applied", true)',
    '.put("creates_new_evidence", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert forbidden not in foundation, f"diagnostic attribution exceeded authority: {forbidden}"

# Explicit truth table for the two admitted route shapes and fail-closed cases.
def reconcile(route, raw_calls, reconstruction_calls, *, usable=True,
              optimization=True, pass2_avoided=None, stage2_passes=1):
    if reconstruction_calls <= 0:
        return "UNKNOWN_FAIL_CLOSED"
    if pass2_avoided is None:
        pass2_avoided = (
            reconstruction_calls
            if route == "EXACT_GAUGE_RETAINED_V0_3"
            else 0
        )
    exact = (
        usable
        and route == "EXACT_GAUGE_RETAINED_V0_3"
        and optimization
        and stage2_passes == 1
        and pass2_avoided == reconstruction_calls
    )
    canonical = (
        usable
        and route == "CANONICAL_V0_2_FALLBACK"
        and not optimization
        and pass2_avoided == 0
    )
    expected = (
        reconstruction_calls
        if exact
        else reconstruction_calls * 2
        if canonical
        else None
    )
    if expected is None or raw_calls != expected:
        return "UNKNOWN_FAIL_CLOSED"
    return (
        "EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED"
        if exact
        else "CANONICAL_V0_2_TWO_PASS_RECONCILED"
    )

assert reconcile("EXACT_GAUGE_RETAINED_V0_3", 3072, 3072) == \
    "EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED"
assert reconcile("EXACT_GAUGE_RETAINED_V0_3", 6144, 3072) == \
    "UNKNOWN_FAIL_CLOSED"
assert reconcile(
    "EXACT_GAUGE_RETAINED_V0_3",
    3072,
    3072,
    pass2_avoided=0,
) == "UNKNOWN_FAIL_CLOSED"
assert reconcile(
    "CANONICAL_V0_2_FALLBACK",
    6144,
    3072,
    optimization=False,
) == "CANONICAL_V0_2_TWO_PASS_RECONCILED"
assert reconcile(
    "CANONICAL_V0_2_FALLBACK",
    3072,
    3072,
    optimization=False,
) == "UNKNOWN_FAIL_CLOSED"
assert reconcile("UNKNOWN_FAIL_CLOSED", 3072, 3072) == "UNKNOWN_FAIL_CLOSED"
assert reconcile(
    "EXACT_GAUGE_RETAINED_V0_3",
    3072,
    3072,
    usable=False,
) == "UNKNOWN_FAIL_CLOSED"

for token in [
    "canonical v0.2",
    "exact gauge v0.3",
    "route attribution",
    "one-pass",
    "two-pass",
    "unknown_fail_closed",
    "read counts do not choose the route",
    "no 50/50 timing assumption",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

print("scientific_master_tile_read_attribution_v0_1_static_integrity=PASS")

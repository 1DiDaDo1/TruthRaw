#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
BINDER = ROOT / "docs/research/scientific-master-streaming-binding-v0.2/native/scientific_master_streaming_binding_v0_2.cpp"
FOUNDATION = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/FreeWorldPerformanceDiagnosticsV01.kt"
README = ROOT / "docs/research/scientific-master-tile-read-attribution-v0.1/README.md"
GRADLE = ROOT / "suite_android/app/build.gradle.kts"
VERSION = ROOT / "suite_android/VERSION_CODE"
LINEAGE = ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json"

binder = BINDER.read_text()
foundation = FOUNDATION.read_text()
readme = README.read_text()
gradle = GRADLE.read_text()
version_code = int(VERSION.read_text().strip())
lineage = json.loads(LINEAGE.read_text())

# Prove the exact binder schedule on which the count attribution is based.
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
    "v0.1 attribution assumes canonical pass 2 does not reconstruct"
)

# Foundation attribution must fail closed and must not invent a per-pass time.
for token in [
    '"D.RAW/ScientificMasterTileReadAttribution/0.1"',
    '"TWO_PASS_BINDER_SCHEDULE_RECONCILED"',
    '"UNKNOWN_FAIL_CLOSED"',
    '"SCIENTIFIC_MASTER_STREAMING_BINDING_V0_2_TWO_CANONICAL_PASSES"',
    '"pass_1_raw_call_count"',
    '"pass_2_raw_call_count"',
    '"unattributed_raw_call_count"',
    '"raw_call_count_reconciles"',
    '"per_pass_source_read_timing_available", false',
    '"per_pass_timing_inferred", false',
    '"optimization_applied", false',
    '"source_values_modified", false',
    '"candidate_applied", false',
    '"creates_new_evidence", false',
    '"scientific_writeback_allowed", false',
    '"DIAGNOSTIC_RUNTIME_ONLY"',
]:
    assert token in foundation, f"tile-read attribution contract missing {token}"

assert "rawCalls == expectedRawCalls" in foundation
assert "reconstructionCalls * 2L" in foundation
assert '"tile_read_attribution_v0_1"' in foundation

for forbidden in [
    '"per_pass_source_read_timing_available", true',
    '"per_pass_timing_inferred", true',
    '"optimization_applied", true',
    '.put("source_values_modified", true)',
    '.put("candidate_applied", true)',
    '.put("creates_new_evidence", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert forbidden not in foundation, f"diagnostic attribution exceeded authority: {forbidden}"

for token in [
    "two-pass binder",
    "source_read_raw_call_count == 2 * reconstruction_call_count",
    "unknown_fail_closed",
    "per_pass_source_read_timing_available=false",
    "per_pass_timing_inferred=false",
    "no 50/50 timing assumption",
    "exact-parity proof",
    "fail-closed fallback",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

assert version_code >= 26100124
assert lineage["current_version_code"] == version_code
assert lineage["previous_version_code"] < lineage["current_version_code"]
assert lineage["version_name"] == "0.53-v0.84.2-scientific-master-tile-read-attribution-v01"
assert f'versionName = "{lineage["version_name"]}"' in gradle
assert lineage["scientific_authority_affected"] is False

print("scientific_master_tile_read_attribution_v0_1_static_integrity=PASS")

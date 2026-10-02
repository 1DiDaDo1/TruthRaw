#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

main = (JAVA / "MainActivity.kt").read_text()
journal = (JAVA / "ResearchBatchJournalV02.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
gradle = (ROOT / "suite_android/app/build.gradle.kts").read_text()
version_code = (ROOT / "suite_android/VERSION_CODE").read_text().strip()
lineage = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())
readme = (ROOT / "docs/research/research-live-status-v0.2/README.md").read_text()

start_marker = "The Research page can already be resumed before the user starts a"
assert start_marker in main
start_pos = main.index(start_marker)
start_end = main.index("render()\n    }", start_pos) + len("render()\n    }")
start_slice = main[start_pos:start_end]
for token in [
    "researchBatchLastHeartbeatWallMs = 0L",
    "researchStatusHandler.removeCallbacks(",
    "researchStatusHandler.post(",
    "researchStatusPoll",
]:
    assert token in start_slice, f"explicit-start polling repair missing {token}"

helper_start = main.index("private fun researchBatchOperationStatusView(")
helper = main[helper_start:helper_start+2800]
for token in [
    "isResearchBatchActive(key)",
    '"attempt_started_at_wall_ms"',
    '"attempt_finished_at_wall_ms"',
    "operationStatusVisual(",
]:
    assert token in helper, f"Research status timing helper missing {token}"

assert "researchBatchOperationStatusView(" in main
assert 'backgroundOperationStatusView(\n            fieldResponseRepeatabilityAnalysisOperationKey()' not in main

for token in [
    '"attempt_finished_at_wall_ms"',
    '"attempt_elapsed_ms"',
    '"attempt_started_at_wall_ms"',
]:
    assert token in journal, f"durable attempt timing missing {token}"

assert "D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1" in profiler
assert int(version_code) >= 26100115
assert lineage["current_version_code"] == int(version_code)
assert lineage["current_version_code"] > lineage["previous_version_code"]
assert f'versionName = "{lineage["version_name"]}"' in gradle
assert lineage["scientific_authority_affected"] is False

for token in [
    "orchestration/telemetry only",
    "No RAW bytes",
    "Activity resume",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

# Scientific-safety assertions are covered by the inherited authority/canonical
# integrity gates. This lifecycle-specific oracle verifies that the profiler
# generation and Android lineage stay on the same non-authoritative line.

print("research_live_status_v0_2_static_integrity=PASS")

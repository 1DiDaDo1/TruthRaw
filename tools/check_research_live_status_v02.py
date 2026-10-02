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
start_slice = main[main.index(start_marker):main.index(start_marker)+1600]
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
assert version_code == "26100115"
assert 'versionName = "0.53-v0.84.2-authority-template-reuse-research-live-status"' in gradle
assert lineage["previous_version_code"] == 26100114
assert lineage["current_version_code"] == 26100115
assert lineage["scientific_authority_affected"] is False

for token in [
    "orchestration/telemetry only",
    "No RAW bytes",
    "Activity resume",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

journal_finish_start = journal.index("fun finish(")
journal_finish = journal[journal_finish_start:journal_finish_start+1800]
changed_scope = start_slice + helper + journal_finish + readme
for forbidden in [
    'candidate_applied", true',
    'creates_new_evidence", true',
    'scientific_writeback_allowed", true',
    'calibration_promoted", true',
    'correction_authorized", true',
]:
    assert forbidden not in changed_scope, f"live-status repair gained scientific authority: {forbidden}"

print("research_live_status_v0_2_static_integrity=PASS")

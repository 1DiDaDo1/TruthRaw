#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

main = (JAVA / "MainActivity.kt").read_text()
service = (JAVA / "TruthRawMediaProcessingForegroundService.kt").read_text()
journal = (JAVA / "ResearchBatchJournalV02.kt").read_text()
store = (JAVA / "ResearchUniversalProfileStoreV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
gradle = (ROOT / "suite_android/app/build.gradle.kts").read_text()
version_code = (ROOT / "suite_android/VERSION_CODE").read_text().strip()
lineage = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())
readme = (ROOT / "docs/research/research-fresh-rerun-v0.1/README.md").read_text()

for token in [
    "An explicit tap on universal analysis means a fresh measurement",
    "ResearchUniversalProfileStoreV01.remove(",
    "universalProfiles.remove(job.id)",
    "freeWorldFoundationStatus = null",
    "TruthRawOperationStore.recoverInterruptedIfNeeded(",
    ".isResearchBatchActive(key)",
]:
    assert token in main, f"MainActivity fresh-rerun contract missing {token}"

for token in [
    "if (!redelivered)",
    "ResearchUniversalProfileStoreV01.remove(",
    "Fresh Research-run kon afgeleide profielcache niet volledig resetten.",
    "isResearchBatchActive",
    "RESEARCH_HEARTBEAT_INTERVAL_MS",
]:
    assert token in service, f"service fresh-rerun contract missing {token}"

for token in [
    '"run_mode"',
    '"FRESH_USER_RUN"',
    '"REDELIVERED_RESUME"',
    '.put("status", "PENDING")',
    'root.remove("current_stage")',
    'existing.remove("completed_at_wall_ms")',
]:
    assert token in journal, f"journal fresh-attempt reset missing {token}"

for token in [
    'wrapper.optString("profile_cache_generation") !=',
    "UniversalSourceProfiler.CACHE_GENERATION",
    "!isCurrentProfile(profile)",
    "fun remove(",
    "): Boolean",
]:
    assert token in store, f"profile store freshness guard missing {token}"

assert "D.RAW/UniversalSourceProfileCache/0.2.11-authority-template-reuse-v1" in profiler
assert version_code == "26100113"
assert 'versionName = "0.53-v0.84.2-authority-template-reuse-fresh-rerun"' in gradle
assert lineage["previous_version_code"] == 26100112
assert lineage["current_version_code"] == 26100113
assert lineage["scientific_authority_affected"] is False

for token in [
    "derived diagnostic caches only",
    "does not delete",
    "only Android service redelivery",
    "fresh measurement attempt",
]:
    assert token.lower() in readme.lower(), f"README missing {token}"

for text in [main, service, journal, store]:
    for forbidden in [
        'candidate_applied", true',
        'creates_new_evidence", true',
        'scientific_writeback_allowed", true',
        'calibration_promoted", true',
        'correction_authorized", true',
    ]:
        assert forbidden not in text, f"lifecycle fix gained scientific authority: {forbidden}"

print("research_fresh_rerun_v0_1_static_integrity=PASS")

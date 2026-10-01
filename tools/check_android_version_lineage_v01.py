#!/usr/bin/env python3
from pathlib import Path
import json
import re

ROOT = Path(__file__).resolve().parents[1]
state = json.loads((ROOT / "state/DRAW_ANDROID_VERSION_LINEAGE_V01.json").read_text())
version_path = ROOT / state["version_code_source"]
version_code = int(version_path.read_text().strip())
gradle = (ROOT / "suite_android/app/build.gradle.kts").read_text()

assert version_code == state["current_version_code"]
assert version_code > state["previous_version_code"]
assert 1 <= version_code <= 2_100_000_000
assert 'applicationId = "com.truthraw.adaptiveui"' in gradle
assert "versionCode = drawVersionCode" in gradle
assert 'rootProject.file("VERSION_CODE")' in gradle
assert f'versionName = "{state["version_name"]}"' in gradle
assert "DRAW_SIGNING_STORE_FILE" in gradle
assert "DRAW_SIGNING_STORE_PASSWORD" in gradle
assert "DRAW_SIGNING_KEY_ALIAS" in gradle
assert "DRAW_SIGNING_KEY_PASSWORD" in gradle

workflows = [
    ROOT / ".github/workflows/draw-suite-universal-intake-v01.yml",
    ROOT / ".github/workflows/draw-android-dngcreator-compat-v01.yml",
    ROOT / ".github/workflows/draw-universal-physical-capture-v01.yml",
]
cert = state["stable_signing_certificate_sha256"]
for path in workflows:
    text = path.read_text()
    for token in [
        "DRAW_DEBUG_KEYSTORE_B64",
        "DRAW_DEBUG_STORE_PASSWORD",
        "DRAW_DEBUG_KEY_ALIAS",
        "DRAW_DEBUG_KEY_PASSWORD",
        "DRAW_SIGNING_STORE_FILE",
        "DRAW_SIGNING_STORE_PASSWORD",
        "DRAW_SIGNING_KEY_ALIAS",
        "DRAW_SIGNING_KEY_PASSWORD",
        cert,
        "verify --print-certs",
    ]:
        assert token in text, f"{path.name}: missing stable update invariant {token}"

assert state["version_code_must_increase_for_next_distributable_baseline"] is True
assert state["same_package_required_for_update"] is True
assert state["same_signing_lineage_required_for_update"] is True
assert state["scientific_authority_affected"] is False

print(f"android_version_lineage_v0_1=PASS versionCode={version_code}")

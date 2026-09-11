#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "MANIFEST_SHA256.txt"
STATE = ROOT / "state/STATE_v0_1.json"
METRICS = ROOT / "evidence/HOST_TEST_METRICS_v0_1.txt"
ANDROID = ROOT / "evidence/ANDROID_ARM64_APK_BUILD_v0_1.txt"

def die(msg: str) -> None:
    print(f"ANDROID_ON_DEVICE_VALIDATION_V0_1_INTEGRITY_FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

if not MANIFEST.is_file():
    die("manifest missing")
for line in MANIFEST.read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    expected, rel = line.split(None, 1)
    path = ROOT / rel.strip()
    if not path.is_file():
        die(f"missing {rel.strip()}")
    got = sha256(path)
    if got != expected:
        die(f"sha mismatch {rel.strip()}: {got} != {expected}")

state = json.loads(STATE.read_text(encoding="utf-8"))
if state.get("schema") != "TruthRawAndroidOnDeviceValidationState/0.1": die("state schema")
if state.get("status") != "RESEARCH_HARNESS_HOST_CI_AND_ANDROID_ARM64_BUILD_PASS_DEVICE_EXECUTION_PENDING": die("state status")
if state.get("stacked_base_sha") != "497bbea144f2e0b30c203aa7c5b3f40c8f47a16e": die("stacked base changed")
if state.get("validated_host_implementation_sha") != "b32b7a784f01a4792550e4c47cff70c55c850cd5": die("validated host implementation changed")

validation = state.get("validation", {})
required = {
    "host_run_id": 34566632602,
    "building_runtime_v0_1_integrity": "PASS",
    "technical_backplane_v0_1_integrity": "PASS",
    "gcc_release": "PASS",
    "clang_release": "PASS",
    "asan_ubsan": "PASS",
    "android_arm64_build_run_id": 34571628677,
    "android_arm64_build_job_id": 103174785811,
    "android_arm64_build_source_commit": "74d10a84a02a8019d4a3c31e9e91050b5e1d0354",
    "android_arm64_build_result": "PASS",
    "apk_sha256": "9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6",
    "jni_bridge_size_bytes": 992968,
    "artifact_archive_sha256": "950ed07a9057270aa5c042973940e7f7c6d5eb16337037480303f384343ac952",
    "android_device_evidence": "PENDING",
}
if validation != required: die("validation record changed")

authority = state.get("authority_boundary", {})
if authority != {
    "building_runtime_remains_resource_policy_authority": True,
    "room_abi_v0_2_remains_admission_authority": True,
    "validator_may_reschedule": False,
    "validator_may_change_truth_authority": False,
    "validator_may_modify_scientific_state": False,
    "runtime_telemetry_may_become_scene_evidence": False,
}: die("authority boundary changed")

truth = state.get("truth_invariants", {})
if truth != {
    "physical_frame_count": 1,
    "independent_evidence_count": 1,
    "scene_iso_axis_present": False,
    "global_zero_line_identity_preserved": True,
    "complete_technical_backplane_identity_checked_before_after": True,
    "canonical_reconstruction_v4_7i_mutated": False,
}: die("truth invariants changed")

measurement = state.get("measurement_contract", {})
if measurement.get("process_rss_source") != "/proc/self/status:VmRSS": die("RSS source changed")
if not measurement.get("budget_gate_uses_session_rss_delta"): die("RSS delta gate disabled")
if measurement.get("absolute_process_rss_used_as_job_budget"): die("absolute RSS cannot become job budget")
if measurement.get("vmhwm_used_as_session_budget_gate"): die("VmHWM cannot become session budget gate")
if not measurement.get("missing_allocator_coverage_reported_explicitly"): die("allocator coverage must stay explicit")
if measurement.get("thermal_sdk_bridge_compiled_for_android") is not True: die("Android thermal bridge build proof missing")
if measurement.get("thermal_sdk_bridge_proven_on_physical_android") is not False: die("physical Android thermal proof promoted")
if measurement.get("telemetry_storage_unbounded"): die("telemetry cannot be unbounded")
if measurement.get("latency_histogram_buckets") != 20: die("latency histogram contract changed")

proof = state.get("proof_boundary", {})
for key in ("android_arm64_apk_build", "android_native_bridge_packaged", "android_thermal_sdk_bridge_compiled"):
    if proof.get(key) is not True: die(f"Android build proof missing: {key}")
for key in ("android_device_execution", "android_proc_self_status_tested", "android_rss_measured", "android_allocator_telemetry_bound", "android_thermal_source_bound_on_device", "android_real_throughput_measured", "android_sustained_thermal_tested", "android_vulkan_tested", "target_200mp_workload_tested", "production_media_encoder_performance_tested"):
    if proof.get(key) is not False: die(f"unproven device claim promoted: {key}")
if proof.get("host_proc_parser_tested") is not True or proof.get("host_live_proc_self_status_tested") is not True: die("host proc proof missing")

pairs = {}
metrics_text = METRICS.read_text(encoding="utf-8")
if "ANDROID_ON_DEVICE_VALIDATION_V0_1_HOST_PASS" not in metrics_text: die("host pass marker missing")
for line in metrics_text.splitlines():
    if "=" in line:
        k, v = line.split("=", 1); pairs[k] = v
required_metrics = {
    "validated_host_implementation_sha":"b32b7a784f01a4792550e4c47cff70c55c850cd5", "host_run_id":"34566632602", "building_runtime_v0_1_integrity":"PASS", "technical_backplane_v0_1_integrity":"PASS", "gcc_release":"PASS", "clang_release":"PASS", "asan_ubsan":"PASS", "low_budget_bytes":"33554432", "low_concurrency":"1", "low_tile":"128", "high_budget_bytes":"268435456", "high_concurrency":"4", "high_tile":"512", "normal_peak_rss_delta_bytes":"8388608", "normal_allocator_slack_bytes":"6291456", "normal_timing_records":"3", "normal_thermal_transitions":"1", "memory_budget_gate":"PASS", "thermal_replan_gate":"PASS", "background_replan_gate":"PASS", "scientific_identity_gate":"PASS", "physical_frame_count":"1", "independent_evidence_count":"1", "scene_iso_axis":"ABSENT", "android_device_evidence":"PENDING",
}
for key, expected in required_metrics.items():
    if pairs.get(key) != expected: die(f"metric {key}: {pairs.get(key)} != {expected}")

android_pairs = {}
for line in ANDROID.read_text(encoding="utf-8").splitlines():
    if "=" in line:
        k, v = line.split("=", 1); android_pairs[k] = v
required_android = {
    "status":"PASS", "source_commit":"74d10a84a02a8019d4a3c31e9e91050b5e1d0354", "run_id":"34571628677", "job_id":"103174785811", "compile_sdk":"35", "target_sdk":"35", "min_sdk":"31", "ndk":"27.2.12479018", "cmake":"3.22.1", "gradle":"8.9", "abi":"arm64-v8a", "jni_library":"lib/arm64-v8a/libtruthraw_validation_bridge.so", "jni_library_size_bytes":"992968", "apk_sha256":"9e35956667e3d54eda139253c7664ee0d6940155831738e69517602e3f42d6c6", "artifact_archive_sha256":"950ed07a9057270aa5c042973940e7f7c6d5eb16337037480303f384343ac952", "host_validation_run_id":"34566632602", "android_device_execution":"NOT_RUN", "scene_iso_axis":"ABSENT_BY_CONTRACT", "scientific_state_mutation":"FORBIDDEN",
}
for key, expected in required_android.items():
    if android_pairs.get(key) != expected: die(f"android evidence {key}: {android_pairs.get(key)} != {expected}")

repo_root = Path(subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "--show-toplevel"], text=True, stderr=subprocess.DEVNULL).strip()).resolve()
upstream = {
    "docs/research/building-runtime-v0.1/native/building_runtime_v0_1.h": "c15bf38df62afda21ea241f1d64aa2fab8870d1a",
    "docs/research/building-runtime-v0.1/ANDROID_RESOURCE_GOVERNOR_v0_1.md": "65069de5a655b2b85e1156ea5beea957ea81ae04",
    "docs/research/room-abi-v0.2/native/room_abi_v0_2.h": "91685d0080e49e5632df1847b5fd2bf0c048e2b2",
    "docs/research/technical-backplane-v0.1/native/technical_backplane_v0_1.h": "a2722aa3ddd12cf2a6b678fed7c086ba6ef70924",
}
for rel, expected in upstream.items():
    p = repo_root / rel
    if not p.is_file(): die(f"upstream dependency missing {rel}")
    got = subprocess.check_output(["git", "hash-object", str(p)], text=True).strip()
    if got != expected: die(f"upstream git blob mismatch {rel}: {got} != {expected}")
for rel in ("capture/android/on-device-validation-v01/app/src/main/java/com/truthraw/validation/MainActivity.kt", "capture/android/on-device-validation-v01/app/src/main/cpp/validation_bridge.cpp", "capture/android/on-device-validation-v01/app/src/main/cpp/CMakeLists.txt", ".github/workflows/android-on-device-validation-v0-1-apk-build.yml"):
    if not (repo_root / rel).is_file(): die(f"Android harness dependency missing {rel}")

native_text = (ROOT / "native/android_on_device_validation_v0_1.h").read_text(encoding="utf-8") + "\n" + (ROOT / "native/android_on_device_validation_v0_1.cpp").read_text(encoding="utf-8")
if "std::vector" in native_text: die("unbounded vector storage introduced")
for forbidden in ("nominalIso", "captureIso", "isoValue"):
    if forbidden in native_text: die(f"ISO authority field introduced: {forbidden}")
if "SerializedBackplane baselineBackplane" not in native_text: die("full Backplane identity binding missing")
if 'std::ifstream input("/proc/self/status"' not in native_text: die("self proc probe missing")

print("ANDROID_ON_DEVICE_VALIDATION_V0_1_INTEGRITY_PASS")
print("validated_host_implementation_sha=b32b7a784f01a4792550e4c47cff70c55c850cd5")
print("android_arm64_build=PASS")
print("android_device_evidence=PENDING")

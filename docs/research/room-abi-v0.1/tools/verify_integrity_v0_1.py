#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

root = Path(__file__).resolve().parents[1]
repo = root.parents[2]

expected_blobs = {
    "docs/research/building-runtime-v0.1/native/building_runtime_v0_1.h": "c15bf38df62afda21ea241f1d64aa2fab8870d1a",
    "docs/research/building-runtime-v0.1/native/building_runtime_v0_1.cpp": "1957456cb6e79c1a6ba56d22f66777e6513533f0",
    "canonical/reconstruction/v4.7i/native/include/truthraw/core.h": "cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a",
    "canonical/reconstruction/v4.7i/native/src/core.cpp": "f79b951ba54cff08db400023e528e4eb91909ba6",
    "docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.h": "da5def4b1f40300548782a123420e671349f5b6b",
    "docs/research/manifold-conditioning-v1/native/manifold_conditioning_v1.cpp": "e41e8c6adbfdcd10c4afac6c4148a2bbfcb66ba0",
    "docs/research/local-illumination-room-capsule-v0.1/native/room_capsule_v0_1.h": "534062f4b42161c842cadc29d20a5e6667adea01",
    "docs/research/local-illumination-room-capsule-v0.1/native/room_capsule_v0_1.cpp": "1e12d155cc5972d07a3296e91bb8c754dd257581",
}

def die(code: int, msg: str) -> None:
    print(msg)
    raise SystemExit(code)

def git_blob_sha(path: Path) -> str:
    data = path.read_bytes()
    hdr = f"blob {len(data)}\0".encode()
    return hashlib.sha1(hdr + data).hexdigest()

for rel, expected in expected_blobs.items():
    p = repo / rel
    if not p.exists():
        die(2, f"MISSING_UPSTREAM {rel}")
    got = git_blob_sha(p)
    if got != expected:
        die(3, f"UPSTREAM_DRIFT {rel} expected={expected} got={got}")

if (root / "local_syntax").exists():
    die(4, "FORBIDDEN_LOCAL_STUBS_PRESENT_IN_CANDIDATE")

manifest = root / "MANIFEST_SHA256.txt"
for line in manifest.read_text().splitlines():
    if not line.strip():
        continue
    expected, rel = line.split("  ", 1)
    p = root / rel
    if not p.is_file():
        die(5, f"MISSING_MANIFEST_FILE {rel}")
    got = hashlib.sha256(p.read_bytes()).hexdigest()
    if got != expected:
        die(6, f"MANIFEST_DRIFT {rel} expected={expected} got={got}")

state = json.loads((root / "state/STATE_v0_1.json").read_text())
assert state["base_main"] == "c5a51610fbb7750c1e8bf95e97c4b1349fbaec7e"
assert state["scientific_master_modified"] is False
assert state["physical_frame_count"] == 1
assert state["independent_evidence_count"] == 1
assert state["canonical_algorithm_files_modified"] is False
assert state["hardening"]["invalid_enum_fail_closed"] is True
assert state["hardening"]["room_capsule_counterfactual_floor_required"] is True
assert state["hardening"]["effective_budget_uses_total_and_per_room_minimum"] is True
assert state["hardening"]["actual_granted_capacity_budget_bound"] is True
assert state["hardening"]["zero_positive_lease_budget_fail_closed"] is True
assert state["hardening"]["planner_peak_reservation"] is True
assert state["hardening"]["metadata_headroom_materialized_as_fake_buffer"] is False
assert state["local_stub_harness_is_integration_evidence"] is False
assert state["tile_native_dng_bound_in_this_version"] is False
assert state["full_frame_streaming_bound_in_this_version"] is False
print("ROOM_ABI_V0_1_INTEGRITY_PASS")

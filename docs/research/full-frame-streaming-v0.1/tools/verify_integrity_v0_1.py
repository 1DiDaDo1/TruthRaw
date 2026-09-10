#!/usr/bin/env python3
from pathlib import Path
import hashlib, json, sys

HERE = Path(__file__).resolve().parents[1]
REPO = HERE.parents[2]
MANIFEST = HERE / "MANIFEST_SHA256.txt"

def gitblob(p: Path) -> str:
    b = p.read_bytes()
    return hashlib.sha1(b"blob " + str(len(b)).encode() + b"\0" + b).hexdigest()

errors = []

WORKFLOW = REPO / ".github/workflows/full-frame-streaming-v0-1-integrity.yml"
EXPECTED_WORKFLOW_SHA256 = "6453270462131f9ff684f6eb19deb6f88a0f4b68a65a652bd26ad94a5084ef6d"
if not WORKFLOW.is_file():
    errors.append("workflow_missing:.github/workflows/full-frame-streaming-v0-1-integrity.yml")
else:
    got = hashlib.sha256(WORKFLOW.read_bytes()).hexdigest()
    if got != EXPECTED_WORKFLOW_SHA256:
        errors.append(f"workflow_sha256:expected={EXPECTED_WORKFLOW_SHA256}:got={got}")
for line in MANIFEST.read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    sha, rel = line.split("  ", 1)
    p = HERE / rel
    if not p.is_file():
        errors.append(f"missing:{rel}")
        continue
    got = hashlib.sha256(p.read_bytes()).hexdigest()
    if got != sha:
        errors.append(f"sha256:{rel}:expected={sha}:got={got}")

upstream = {
    "canonical/reconstruction/v4.7i/native/include/truthraw/core.h": "cfb9fd42bc310ddb4fd16ee8f26c3ed554a0f92a",
    "canonical/reconstruction/v4.7i/native/src/core.cpp": "f79b951ba54cff08db400023e528e4eb91909ba6"
}
for rel, expected in upstream.items():
    p = REPO / rel
    if not p.is_file():
        errors.append(f"upstream_missing:{rel}")
    else:
        got = gitblob(p)
        if got != expected:
            errors.append(f"upstream_gitblob:{rel}:expected={expected}:got={got}")

state = json.loads((HERE / "state/STATE_v0_1.json").read_text(encoding="utf-8"))
if state.get("canonical_v4_7i_modified") is not False:
    errors.append("state:canonical_v4_7i_modified_must_be_false")
inv = state.get("scientific_invariants", {})
if inv.get("physicalFrameCount") != 1 or inv.get("independentEvidenceCount") != 1:
    errors.append("state:single_frame_evidence_invariant")
if inv.get("appearance_modifies_scientific_master") is not False:
    errors.append("state:appearance_master_boundary")
if state.get("execution", {}).get("half_resolution_scratch_store_required") is not False:
    errors.append("state:half_scratch_store_should_be_removed")
if (HERE / "local_stub").exists():
    errors.append("repository_candidate_must_not_contain_local_stub")

if errors:
    print("FULL_FRAME_STREAMING_V0_1_INTEGRITY_FAIL")
    for e in errors:
        print(e)
    sys.exit(1)
print("FULL_FRAME_STREAMING_V0_1_INTEGRITY_PASS")
print("canonical_v4_7i_bytes_bound=true")
print("adapter_full_frame_owned_buffers=0")
print("half_resolution_scratch_store=false")

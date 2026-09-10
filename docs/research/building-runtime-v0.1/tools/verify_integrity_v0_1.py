#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import hashlib, json, os, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "MANIFEST_SHA256.txt"
STATE = ROOT / "state/STATE_v0_1.json"
METRICS = ROOT / "evidence/TEST_METRICS_v0_1.txt"

def die(msg: str) -> None:
    print(f"BUILDING_RUNTIME_V0_1_INTEGRITY_FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024*1024), b''):
            h.update(chunk)
    return h.hexdigest()

def git_blob(path: str) -> str:
    try:
        return subprocess.check_output(["git", "hash-object", path], text=True).strip()
    except Exception as e:
        die(f"cannot hash upstream dependency {path}: {e}")

if not MANIFEST.is_file(): die("manifest missing")
for line in MANIFEST.read_text(encoding='utf-8').splitlines():
    if not line.strip(): continue
    expected, rel = line.split(None, 1)
    p = ROOT / rel.strip()
    if not p.is_file(): die(f"missing {rel}")
    got = sha256(p)
    if got != expected: die(f"sha mismatch {rel}: {got} != {expected}")

state = json.loads(STATE.read_text(encoding='utf-8'))
if state.get("schema") != "truthraw.building_runtime.state.v0_1": die("state schema")
if state.get("status") != "RESEARCH_CANDIDATE_LOCAL_PASS": die("candidate status")
inv = state["truth_invariants"]
if inv != {
    "appearanceMayFeedScientificState": False,
    "counterfactualMayFeedScientificState": False,
    "independentEvidenceCount": 1,
    "physicalFrameCount": 1,
    "runtimeMayPromoteScientificClaims": False,
    "scientificMasterModified": False,
}: die("truth invariants changed")
if state["graph"]["roomCount"] != 12: die("room count")
if not state["graph"]["corridorsForwardOnly"] or not state["graph"]["externalArtifactHandleOnly"]: die("corridor contract")

for rel, expected in state["candidate_sha256"].items():
    got = sha256(ROOT / rel)
    if got != expected: die(f"candidate binding {rel}")

repo_root_env = os.environ.get("TRUTHRAW_REPO_ROOT")
repo_root = Path(repo_root_env).resolve() if repo_root_env else None
if repo_root is None:
    try:
        repo_root = Path(subprocess.check_output(
            ["git", "-C", str(ROOT), "rev-parse", "--show-toplevel"], text=True, stderr=subprocess.DEVNULL
        ).strip()).resolve()
    except Exception:
        repo_root = None

if repo_root is None:
    if os.environ.get("TRUTHRAW_LOCAL_NO_GIT_SKIP_UPSTREAM") != "1":
        die("cannot resolve repository root for upstream dependency bindings")
    print("BUILDING_RUNTIME_V0_1_UPSTREAM_BINDINGS_DEFERRED_LOCAL_NO_GIT")
else:
    for rel, expected in state["upstream_git_blob_bindings"].items():
        p = repo_root / rel
        if not p.is_file(): die(f"upstream dependency missing {rel}")
        got = subprocess.check_output(["git", "hash-object", str(p)], text=True).strip()
        if got != expected: die(f"upstream git blob mismatch {rel}: {got} != {expected}")

pairs = {}
for line in METRICS.read_text(encoding='utf-8').splitlines():
    if '=' in line:
        k,v=line.split('=',1); pairs[k]=v
required = {
    "physicalFrameCount":"1",
    "independentEvidenceCount":"1",
    "scientificMasterModified":"0",
    "decisionCount":"12",
    "ledgerCount":"12",
    "sizeofRuntimeResult":"240",
    "lowBudgetBytes":"33554432",
    "lowConcurrency":"1",
    "lowTile":"128",
    "highBudgetBytes":"268435456",
    "highConcurrency":"4",
    "highTile":"512",
    "lowWaves":"11",
    "highWaves":"8",
}
if "BUILDING_RUNTIME_V0_1_TEST_PASS" not in METRICS.read_text(encoding='utf-8'): die("test pass marker")
for k,v in required.items():
    if pairs.get(k) != v: die(f"metric {k}: {pairs.get(k)} != {v}")
if int(pairs["highWaves"]) > int(pairs["lowWaves"]): die("high tier cannot require more waves in fixture")

print("BUILDING_RUNTIME_V0_1_INTEGRITY_PASS")

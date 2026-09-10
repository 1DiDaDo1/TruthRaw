#!/usr/bin/env python3
from pathlib import Path
import hashlib, json

root = Path(__file__).resolve().parents[1]
manifest = root / "MANIFEST_SHA256.txt"

def require(cond, msg):
    if not cond:
        raise SystemExit(msg)

for line in manifest.read_text().splitlines():
    line = line.strip()
    if not line:
        continue
    expected, rel = line.split(None, 1)
    p = root / rel.strip()
    require(p.is_file(), f"missing {rel}")
    actual = hashlib.sha256(p.read_bytes()).hexdigest()
    require(actual == expected, f"hash mismatch {rel}: {actual} != {expected}")

state = json.loads((root / "state/STATE_v0_1.json").read_text())
require(state.get("scientificMasterModified") is False, "scientific master modification forbidden")
require(state.get("zeroLineModified") is False, "zero-line modification forbidden")
require(state.get("counterfactualLightStatesAreEvidence") is False, "light states cannot become evidence")
require(state.get("physicalFrameCount") == 1, "physicalFrameCount must remain one")
require(state.get("independentEvidenceCount") == 1, "independentEvidenceCount must remain one")
require(state.get("cpuBaselineRequired") is True, "CPU baseline is mandatory")
require(state.get("gpuAccelerationOptional") is True, "GPU must remain optional")
require(state.get("fullScene3dRequired") is False, "full-scene 3D must not become mandatory")
require(str(state.get("physicalRelightClaim", "")).startswith("BLOCKED"), "physical relight claim must stay blocked")
require(state.get("cicmDependency") == "SATISFIED_ON_MAIN_CI_GREEN", "CICM dependency state mismatch")
require(state.get("cicmDependencyCommit") == "71dcd031b309658ef39b99b6c2b46031f09165d9", "CICM dependency commit mismatch")
require(state.get("cicmDependencyTree") == "5c645005b82195659360b2ade6cebb5e16c1143a", "CICM dependency tree mismatch")
print("PASS: Room Capsule v0.1 integrity")
print("SCIENTIFIC_STATUS=" + state["status"])

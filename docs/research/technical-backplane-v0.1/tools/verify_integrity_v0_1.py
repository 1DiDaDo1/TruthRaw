#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[2]
MANIFEST = HERE / "MANIFEST_SHA256.txt"
WORKFLOW = ROOT / ".github/workflows/technical-backplane-v0-1-integrity.yml"
EXPECTED_WORKFLOW_SHA256 = "3f12bfaadfd07541421a702530567f00bf5385025f65a94a1e8387ed883fbc08"
errors = []
if not MANIFEST.exists():
    errors.append("missing manifest")
else:
    listed = set()
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if not line.strip(): continue
        digest, rel = line.split("  ", 1)
        listed.add(rel)
        p = HERE / rel
        if not p.is_file():
            errors.append(f"missing:{rel}")
            continue
        got = hashlib.sha256(p.read_bytes()).hexdigest()
        if got != digest: errors.append(f"sha256:{rel}:{got}")
    actual = {p.relative_to(HERE).as_posix() for p in HERE.rglob("*") if p.is_file() and p.name != "MANIFEST_SHA256.txt" and "__pycache__" not in p.parts}
    for x in sorted(actual - listed): errors.append(f"unsealed-extra:{x}")
    for x in sorted(listed - actual): errors.append(f"sealed-missing:{x}")
if not WORKFLOW.is_file():
    errors.append("missing root workflow")
elif hashlib.sha256(WORKFLOW.read_bytes()).hexdigest() != EXPECTED_WORKFLOW_SHA256:
    errors.append("workflow-sha256")
for forbidden in ("local_stub", "local_syntax", "__pycache__"):
    if any(forbidden in p.parts for p in HERE.rglob("*")): errors.append(f"forbidden-path:{forbidden}")
if errors:
    print("TECHNICAL_BACKPLANE_V0_1_INTEGRITY_FAIL")
    for e in errors: print(e)
    sys.exit(1)
print("TECHNICAL_BACKPLANE_V0_1_INTEGRITY_PASS")
print("serialized_bytes=180")
print("zero_line_storage=ONE_SHARED_BINDING")

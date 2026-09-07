#!/usr/bin/env python3
"""Reassemble the byte-exact canonical TruthRaw v5.0e source from Git-stored chunks."""
from pathlib import Path
import hashlib
import sys

HERE = Path(__file__).resolve().parent
PARTS = HERE / "source-parts"
OUT = HERE / "unified_material_v5_0e.py"
EXPECTED_SHA256 = "7a60703c47a981da8aa4f9613545e1615a6a25cdbb3a8347ed1c1a4df076b1e0"
EXPECTED_BYTES = 26771

payload = b"".join((PARTS / f"unified_material_v5_0e.py.part{i:02d}").read_bytes() for i in range(1, 5))
sha = hashlib.sha256(payload).hexdigest()
if len(payload) != EXPECTED_BYTES or sha != EXPECTED_SHA256:
    raise SystemExit(f"FAIL: reconstructed source bytes={len(payload)} sha256={sha}")
OUT.write_bytes(payload)
print(f"PASS: {OUT.name} bytes={len(payload)} sha256={sha}")

#!/usr/bin/env python3
from pathlib import Path
import hashlib
import sys

base = Path(__file__).resolve().parents[1]
manifest = base / "MANIFEST_SHA256.txt"
ok = True
for line in manifest.read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    expected, rel = line.split("  ", 1)
    p = base / rel
    if not p.is_file():
        print(f"MISSING {rel}")
        ok = False
        continue
    actual = hashlib.sha256(p.read_bytes()).hexdigest()
    if actual != expected:
        print(f"MISMATCH {rel}: {actual} != {expected}")
        ok = False
if not ok:
    sys.exit(2)
print("PROFESSIONAL_RAW_DECODER_ADAPTER_V0_1_INTEGRITY_PASS")

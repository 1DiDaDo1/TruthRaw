#!/usr/bin/env python3
"""TruthRaw canonical integrity scope v2.

Purpose:
- retain the original byte-exact scientific verifier unchanged as historical logic;
- exclude living root bootstrap/governance files from its frozen blob set;
- verify their prior frozen blobs remain reachable in Git history;
- keep all canonical scientific/evidence checks from v1.
"""
from pathlib import Path
import hashlib, json, subprocess, sys

ROOT = Path(__file__).resolve().parents[1]
V1 = ROOT / "tools/verify_canonical_integrity.py"
LEDGER = ROOT / "evidence/HISTORICAL_BOOTSTRAP_BLOBS_2026-09-10.json"
EXPECTED_LEDGER_SHA256 = "35a9243aa48dd863e76d98df64508cc175d4b3d4344ec5782eac38757542f612"

if hashlib.sha256(LEDGER.read_bytes()).hexdigest() != EXPECTED_LEDGER_SHA256:
    raise SystemExit("FAIL historical bootstrap ledger bytes")

ledger = json.loads(LEDGER.read_text(encoding="utf-8"))
expected_history = {
    "README.md": "341cdf2903e626ffd7b53d53de5edbb4fced9fc2",
    "START_HERE_NEW_CHAT.md": "fab115fe4303554ecd1383dcb1aaf1dc10724812",
    ".github/workflows/canonical-integrity.yml": "3d1213b5fe9c0f07fabbdc1c5711bfeeeebb5297",
}
if ledger.get("historical_blobs") != expected_history:
    raise SystemExit("FAIL historical bootstrap ledger semantics")

for rel, blob in expected_history.items():
    probe = subprocess.run(
        ["git", "-C", str(ROOT), "cat-file", "-e", blob + "^{blob}"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if probe.returncode != 0:
        raise SystemExit(f"FAIL historical blob unreachable {rel} {blob}")
    print("PASS historical blob", rel, blob)

# Reuse the frozen v1 scientific verifier, but remove only the three files that
# are intentionally living governance/bootstrap surfaces. No scientific module
# or evidence entry is removed.
source = V1.read_text(encoding="utf-8")
removals = [
    " 'README.md':'341cdf2903e626ffd7b53d53de5edbb4fced9fc2',\n",
    " 'START_HERE_NEW_CHAT.md':'fab115fe4303554ecd1383dcb1aaf1dc10724812',\n",
    " '.github/workflows/canonical-integrity.yml':'3d1213b5fe9c0f07fabbdc1c5711bfeeeebb5297',\n",
]
for line in removals:
    if source.count(line) != 1:
        raise SystemExit("FAIL v1 verifier layout changed; manual scope review required")
    source = source.replace(line, "", 1)

scope = {
    "__file__": str(V1),
    "__name__": "__main__",
}
exec(compile(source, str(V1), "exec"), scope, scope)

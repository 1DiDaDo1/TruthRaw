#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "SHA256_MANIFEST_v0_7.json"

def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def main() -> int:
    m = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if m.get("schema") != "TruthRawXyzD50UncertaintySha256Manifest/0.7":
        raise SystemExit("wrong manifest schema")
    expected = {x["path"]: x for x in m["files"]}
    for rel, rec in expected.items():
        p = ROOT / rel
        if not p.is_file():
            raise SystemExit(f"missing: {rel}")
        if p.stat().st_size != rec["bytes"]:
            raise SystemExit(f"byte-size mismatch: {rel}")
        got = sha256(p)
        if got != rec["sha256"]:
            raise SystemExit(f"sha256 mismatch: {rel}: {got}")

    h = (ROOT / "native/xyz_d50_uncertainty_v0_7.h").read_text(encoding="utf-8")
    c = (ROOT / "native/xyz_d50_uncertainty_v0_7.cpp").read_text(encoding="utf-8")
    r = (ROOT / "README.md").read_text(encoding="utf-8")

    if "Unknown camera-RGB correlation is never replaced by independence." not in h:
        raise SystemExit("missing no-independence header guard")
    if "can_exactly_propagate_linear_covariance_v0_6" not in c:
        raise SystemExit("exact propagation is not gated by v0.6")
    if "v += m_at(matrix, r, i) * sin[i][j] * m_at(matrix, c, j);" not in c:
        raise SystemExit("missing full M Sigma M^T implementation")
    if "Cauchy-Schwarz" not in c:
        raise SystemExit("missing unresolved-covariance bound guard")
    if "out.covarianceKnownMask = 0u;" not in c:
        raise SystemExit("missing non-full XYZ off-diagonal unknown guard")
    if "p50/p95 error bands are not converted to variance" not in c:
        raise SystemExit("missing quantile anti-promotion guard")
    if "v0.7 does not infer, interpolate, calibrate, or choose a camera color matrix" not in r:
        raise SystemExit("missing matrix provenance boundary")
    if "Roadmap item 152 remains separate" not in r:
        raise SystemExit("missing topology boundary")

    print("TRUTHRAW_XYZ_D50_UNCERTAINTY_V0_7_INTEGRITY_PASS")
    print(f"files={len(expected)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())

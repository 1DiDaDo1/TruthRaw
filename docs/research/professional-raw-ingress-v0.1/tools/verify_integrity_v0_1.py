#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "MANIFEST_SHA256.txt"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> None:
    seen: set[str] = set()
    for line in MANIFEST.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        digest, rel = line.split("  ", 1)
        if rel in seen:
            raise SystemExit(f"duplicate manifest path: {rel}")
        seen.add(rel)
        path = ROOT / rel
        if not path.is_file():
            raise SystemExit(f"missing sealed file: {rel}")
        actual = sha256(path)
        if actual != digest:
            raise SystemExit(f"sha256 mismatch: {rel}: expected {digest}, got {actual}")
    if not seen:
        raise SystemExit("empty manifest")
    print(f"PROFESSIONAL_RAW_INGRESS_V0_1_INTEGRITY_PASS files={len(seen)}")


if __name__ == "__main__":
    main()

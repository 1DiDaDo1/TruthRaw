#!/usr/bin/env python3
"""Hash APK ZIP payload independently from the APK Signing Block.

This is a build/provenance diagnostic only. It does not replace Android signature
verification. The digest is SHA-256 over sorted ZIP entry names and uncompressed
entry bytes, with explicit length framing to avoid concatenation ambiguity.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path


def payload_sha256(apk: Path) -> str:
    h = hashlib.sha256()
    with zipfile.ZipFile(apk, "r") as zf:
        for name in sorted(zf.namelist()):
            data = zf.read(name)
            encoded = name.encode("utf-8")
            h.update(struct.pack(">I", len(encoded)))
            h.update(encoded)
            h.update(struct.pack(">Q", len(data)))
            h.update(data)
    return h.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("apk", type=Path)
    args = parser.parse_args()
    print(payload_sha256(args.apk))


if __name__ == "__main__":
    main()

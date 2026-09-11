#!/usr/bin/env python3
from pathlib import Path
import sys

base = Path(__file__).resolve().parents[1]
native = base / "native" / "libraw_compat_probe_v0_1.cpp"
header = base / "native" / "libraw_compat_probe_v0_1.h"
state = base / "state" / "STATE_v0_1.json"

required = [native, header, state]
for p in required:
    if not p.is_file():
        print(f"MISSING {p.relative_to(base)}")
        sys.exit(2)

source = native.read_text(encoding="utf-8")
native_all = source + "\n" + header.read_text(encoding="utf-8")
for forbidden in (
    ".unpack(",
    ".unpack_thumb(",
    ".unpack_thumb_ex(",
    ".raw2image(",
    ".dcraw_process(",
    "rawdata.raw_image",
    "imgdata.image",
    "iso_speed",
):
    if forbidden in native_all:
        print(f"FORBIDDEN production probe token: {forbidden}")
        sys.exit(2)

if "open_file(" not in source:
    print("MISSING metadata open_file path")
    sys.exit(2)
if "dng_version" not in source or "raw_count" not in source or "filters" not in source:
    print("MISSING required topology metadata")
    sys.exit(2)
if "pixelsUnpacked = false" not in source:
    print("MISSING explicit unpack-state assignment")
    sys.exit(2)

state_text = state.read_text(encoding="utf-8")
for required_state in (
    '"pixel_unpack_allowed": false',
    '"sample_scientific_admission": false',
    '"filename_extension_is_evidence": false',
):
    if required_state not in state_text:
        print(f"MISSING state invariant: {required_state}")
        sys.exit(2)

print("LIBRAW_COMPAT_PROBE_V0_1_CONTRACT_PASS")

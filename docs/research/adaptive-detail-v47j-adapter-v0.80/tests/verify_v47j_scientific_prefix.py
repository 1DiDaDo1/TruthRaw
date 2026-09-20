#!/usr/bin/env python3
from pathlib import Path
import sys

repo = Path(__file__).resolve().parents[4]
v47i = (repo / "canonical/reconstruction/v4.7i/native/src/core.cpp").read_text()
v47j = (repo / "canonical/detail/v4.7j/native/src/core.cpp").read_text()
marker = "Status NeutralReferenceAppearance::applyTile"
if marker not in v47i or marker not in v47j:
    print("V47J_SCIENTIFIC_PREFIX_FAIL marker_missing")
    sys.exit(2)
a = v47i.split(marker, 1)[0]
b = v47j.split(marker, 1)[0]
if a != b:
    print("V47J_SCIENTIFIC_PREFIX_FAIL reconstruction_prefix_differs")
    sys.exit(2)
print("V47J_SCIENTIFIC_PREFIX_PASS")
print("scientific_reconstruction_prefix_byte_exact=1")

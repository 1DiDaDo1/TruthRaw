#!/usr/bin/env python3
from __future__ import annotations

from hashlib import sha256
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/research/drawnegative-v0.1/SEAL_MANIFEST_v0_1.json"

EXPECTED = {
    "docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE_v0_2_DRAWNEGATIVE.md":
        "05486aa27fb8839271e39e58f698e2c41d3ef5ac82660418111c4dfd563892f3",
    "docs/research/lens-independent-free-world-observation-contract-v0.2/README.md":
        "4d620db58ba3e516d38fbaa5a7a65539953cc4337f1badb4e482ec02d97e8ab8",
    "docs/research/lens-independent-free-world-observation-contract-v0.2/DRAW_OBSERVATION_CONTRACT_v0_2.json":
        "1761d67aee46a11074685bb33eeb29e5842a95e3ef9ac71e5a62b7324229e8d4",
    "docs/research/drawnegative-v0.1/README.md":
        "4a262a67e7262e90dd8dc1c26d54fa1562f76bba91ef79c3d4bae0c21cb00dcb",
    "docs/research/drawnegative-v0.1/native/drawnegative_v0_1.h":
        "0cbfe5442750e575cbe52cba056645d1d41ef208fa1aefef20e70ae6a246fd24",
    "docs/research/drawnegative-v0.1/native/drawnegative_v0_1.cpp":
        "a337c710701e768adb3c22577b71136775e286922ceae2422a0c7167c638cfe5",
}

errors: list[str] = []

try:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
except Exception as exc:
    print(f"DRAWNEGATIVE_SEAL_FAIL manifest:{exc}")
    raise SystemExit(1)

if manifest.get("schema") != "D.RAW/D.RAWnegativeSealManifest/0.1":
    errors.append("manifest_schema")
if manifest.get("status") != "SEALED":
    errors.append("manifest_status")
if manifest.get("current_scientific_negative_name") != "D.RAWnegative":
    errors.append("current_name")
if manifest.get("legacy_parent_name") != "TruthNegative Continuous v0.5":
    errors.append("legacy_parent")
if manifest.get("immutable_successor_rule") != (
    "SEALED_DRAWNEGATIVE_V0_1_BYTES_MAY_NOT_CHANGE; CREATE_VERSIONED_SUCCESSOR"
):
    errors.append("successor_rule")

actual_manifest_files = {
    x.get("path"): x.get("sha256")
    for x in manifest.get("files", [])
    if isinstance(x, dict)
}
if actual_manifest_files != EXPECTED:
    errors.append("manifest_file_set")

for rel, expected in EXPECTED.items():
    p = ROOT / rel
    if not p.is_file():
        errors.append(f"missing:{rel}")
        continue
    actual = sha256(p.read_bytes()).hexdigest()
    if actual != expected:
        errors.append(f"byte_seal:{rel}:{actual}")

contract_path = ROOT / (
    "docs/research/lens-independent-free-world-observation-contract-v0.2/"
    "DRAW_OBSERVATION_CONTRACT_v0_2.json"
)
try:
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
except Exception as exc:
    errors.append(f"contract_json:{exc}")
    contract = {}

if contract.get("schema") != "D.RAW/LensIndependentFreeWorldObservationContract/0.2":
    errors.append("contract_schema")
if contract.get("current_scientific_negative_name") != "D.RAWnegative":
    errors.append("contract_current_name")
if contract.get("legacy_scientific_negative_name") != "TruthNegative":
    errors.append("contract_legacy_name")
if contract.get("parent_bytes_mutated") is not False:
    errors.append("parent_bytes_mutated")

dn = contract.get("drawnegative") or {}
if dn.get("schema") != "D.RAW/D.RAWnegative/0.1":
    errors.append("drawnegative_schema")
for key in (
    "one_observation_lineage",
    "raster_independent",
    "parent_truthnegative_state_required",
):
    if dn.get(key) is not True:
        errors.append(f"drawnegative_true:{key}")
for key in (
    "target_raster_part_of_identity",
    "creates_new_evidence",
    "scientific_writeback_allowed",
    "appearance_writeback_allowed",
    "per_sample_truthrange_materialized_by_v0_1",
):
    if dn.get(key) is not False:
        errors.append(f"drawnegative_false:{key}")

zero = contract.get("zero_line") or {}
if zero.get("formula") != "T=log2(L/L0)":
    errors.append("zero_formula")
if zero.get("scale_gauge_required") is not True:
    errors.append("scale_gauge_required")
if zero.get("source_local_gauge_allows_cross_observation_equality") is not False:
    errors.append("source_local_equality")
if zero.get("source_local_gauge_allows_cross_observation_fusion") is not False:
    errors.append("source_local_fusion")
if zero.get("shared_gauge_relation_must_be_admitted") is not True:
    errors.append("shared_gauge_gate")

precision = contract.get("precision") or {}
if precision.get("branch_sensitive_compute") != "FLOAT64":
    errors.append("float64_compute")
if precision.get("precision_upgrades_authority") is not False:
    errors.append("precision_authority")

compat = contract.get("compatibility") or {}
if compat.get("legacy_truthnegative_is_second_world") is not False:
    errors.append("legacy_second_world")
if compat.get("legacy_tnc_is_native_drawnegative_container") is not False:
    errors.append("legacy_tnc_boundary")

sem = manifest.get("semantics") or {}
if sem.get("truthnegative_parent_required") is not True:
    errors.append("manifest_parent_required")
if sem.get("parent_bytes_rewritten") is not False:
    errors.append("manifest_parent_rewrite")
if sem.get("per_observation_lineage") is not True:
    errors.append("manifest_per_observation")
if sem.get("raster_independent") is not True:
    errors.append("manifest_raster")
if sem.get("truthrange_formula") != "T=log2(L/L0)":
    errors.append("manifest_zero_formula")
if sem.get("source_local_gauge_allows_cross_observation_fusion") is not False:
    errors.append("manifest_local_fusion")
if sem.get("creates_new_evidence") is not False:
    errors.append("manifest_new_evidence")
if sem.get("scientific_writeback_allowed") is not False:
    errors.append("manifest_writeback")

if errors:
    print("DRAWNEGATIVE_V01_SEAL_FAIL")
    for e in errors:
        print(e)
    raise SystemExit(1)

print("DRAWNEGATIVE_V01_SEAL_PASS")
print("sealed_files=6")
print("current_scientific_negative=D.RAWnegative")
print("legacy_parent=TruthNegative Continuous v0.5")
print("source_local_cross_observation_fusion=false")
print("creates_new_evidence=false")
print("scientific_writeback_allowed=false")

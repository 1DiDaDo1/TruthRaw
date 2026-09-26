#!/usr/bin/env python3
"""Byte and semantic seal verifier for D.RAW lens-independent Free World v0.1."""

from __future__ import annotations

from hashlib import sha256
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs/research/lens-independent-free-world-observation-contract-v0.1/SEAL_MANIFEST_v0_1.json"

EXPECTED = {
    "docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md":
        "a6dff18fd8c43224163df9a438655be1625458596194b853698862bd4ac6da88",
    "docs/research/lens-independent-free-world-observation-contract-v0.1/README.md":
        "c943bea678e6b9c2045363ac725aebe57f8b86a290cf303c7138ffc7549a6c6e",
    "docs/research/lens-independent-free-world-observation-contract-v0.1/DRAW_OBSERVATION_CONTRACT_v0_1.json":
        "7edf5eb211ae66347586ce4ff375b4bc62f451fe578fa83e336b19818205b522",
    "state/LENS_INDEPENDENT_FREE_WORLD_V01_STATE_2026-09-26.json":
        "dbd5c6ce3f835657182ea302d9d49bd1d9b2dfa0f8fb6dd3fcf11d3dcf1ec1f3",
}

errors: list[str] = []

try:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
except Exception as exc:
    print(f"SEAL_FAIL manifest_invalid:{exc}")
    sys.exit(1)

if manifest.get("schema") != "D.RAW/LensIndependentFreeWorldSealManifest/0.1":
    errors.append("manifest_schema")
if manifest.get("status") != "SEALED":
    errors.append("manifest_status")
if manifest.get("immutable_successor_rule") != (
    "SEALED_V0_1_BYTES_MAY_NOT_CHANGE; CREATE_VERSIONED_SUCCESSOR"
):
    errors.append("successor_rule")
if manifest.get("permanent_rule") != (
    "Representation can exceed the source. Knowledge claims cannot exceed the evidence."
):
    errors.append("permanent_rule")
if manifest.get("short_law") != (
    "One Free World. Many sealed observations. One evidence law."
):
    errors.append("short_law")

manifest_files = {
    item.get("path"): item.get("sha256")
    for item in manifest.get("files", [])
    if isinstance(item, dict)
}
if manifest_files != EXPECTED:
    errors.append("manifest_file_set_or_hashes")

for rel, expected in EXPECTED.items():
    path = ROOT / rel
    if not path.is_file():
        errors.append(f"missing:{rel}")
        continue
    actual = sha256(path.read_bytes()).hexdigest()
    if actual != expected:
        errors.append(f"byte_seal:{rel}:{actual}")

contract_path = ROOT / (
    "docs/research/lens-independent-free-world-observation-contract-v0.1/"
    "DRAW_OBSERVATION_CONTRACT_v0_1.json"
)
state_path = ROOT / "state/LENS_INDEPENDENT_FREE_WORLD_V01_STATE_2026-09-26.json"

try:
    contract = json.loads(contract_path.read_text(encoding="utf-8"))
except Exception as exc:
    errors.append(f"contract_json:{exc}")
    contract = {}

try:
    state = json.loads(state_path.read_text(encoding="utf-8"))
except Exception as exc:
    errors.append(f"state_json:{exc}")
    state = {}

if contract.get("schema") != "D.RAW/LensIndependentFreeWorldObservationContract/0.1":
    errors.append("contract_schema")
if contract.get("status") != "SEALED_ARCHITECTURE_CONTRACT":
    errors.append("contract_status")
if contract.get("permanent_rule") != (
    "Representation can exceed the source. Knowledge claims cannot exceed the evidence."
):
    errors.append("contract_permanent_rule")
if contract.get("short_law") != (
    "One Free World. Many sealed observations. One evidence law."
):
    errors.append("contract_short_law")

zero = contract.get("zero_line") or {}
if zero.get("formula") != "T=log2(L/L0)":
    errors.append("zero_line_formula")
if zero.get("address_space") != "UNBOUNDED_COORDINATE_FAMILY":
    errors.append("zero_line_address_space")
if zero.get("common_numeric_coordinate_implies_common_gauge") is not False:
    errors.append("common_gauge_must_not_be_implicit")
if zero.get(
    "cross_observation_comparison_requires_admitted_gauge_relation"
) is not True:
    errors.append("cross_observation_gauge_gate")

precision = contract.get("precision") or {}
if precision.get("branch_sensitive_compute") != "FLOAT64":
    errors.append("float64_compute_contract")
if precision.get("canonical_storage") != "FLOAT32_WHERE_VALIDATED":
    errors.append("float32_storage_contract")
if precision.get("float64_increases_authority") is not False:
    errors.append("precision_authority_guard")
if precision.get("float32_to_float64_restores_prior_branch_loss") is not False:
    errors.append("float32_loss_guard")

tn = contract.get("truthnegative") or {}
if tn.get("role") != (
    "RASTER_INDEPENDENT_EVIDENCE_AWARE_SCIENTIFIC_NEGATIVE_PER_OBSERVATION"
):
    errors.append("truthnegative_role")
if tn.get("fuses_multiple_observations_by_definition") is not False:
    errors.append("truthnegative_must_not_implicitly_fuse")
if tn.get("target_raster_part_of_state_identity") is not False:
    errors.append("truthnegative_raster_identity_guard")
if tn.get("resampling_creates_measured_target_claims") is not False:
    errors.append("truthnegative_resampling_authority_guard")

world = contract.get("free_world") or {}
if world.get("lens_independent") is not True:
    errors.append("free_world_lens_independence")
if world.get("camera_independent") is not True:
    errors.append("free_world_camera_independence")
if world.get("may_contain_multiple_sealed_observations") is not True:
    errors.append("free_world_multi_observation")
if world.get("geometry_authority_separate_from_radiometric_authority") is not True:
    errors.append("authority_axis_separation")
if world.get("finite_pixel_is_projection_not_primitive") is not True:
    errors.append("output_pixel_projection_law")

inv = contract.get("invariants") or {}
required_false = (
    "scientific_master_appearance_writeback",
    "truthnegative_appearance_writeback",
    "virtual_view_creates_physical_evidence",
    "resampling_creates_physical_evidence",
    "lens_identity_upgrades_authority",
    "file_format_upgrades_authority",
    "geometry_authority_upgrades_radiometry",
    "radiometric_authority_upgrades_geometry",
    "cross_source_calibration_transfer_implicit",
)
for key in required_false:
    if inv.get(key) is not False:
        errors.append(f"invariant_false:{key}")
if inv.get("sealed_source_immutable") is not True:
    errors.append("sealed_source_must_be_immutable")

if state.get("schema") != "DRAW_LENS_INDEPENDENT_FREE_WORLD_STATE/2026-09-26/v0.1":
    errors.append("state_schema")
if state.get("status") != "SEALED_ARCHITECTURE_OVERLAY":
    errors.append("state_status")
if (state.get("code_effect") or {}).get("existing_pixel_route_changed") is not False:
    errors.append("existing_pixel_route_must_remain_unchanged")

seal_semantics = manifest.get("semantic_seal") or {}
for key in (
    "lens_is_observation_instrument_not_world_boundary",
    "free_world_lens_independent",
    "truthnegative_is_per_observation_lineage",
    "cross_observation_radiometric_equality_requires_admitted_gauge",
    "finite_output_pixel_is_projection_not_primitive",
):
    if seal_semantics.get(key) is not True:
        errors.append(f"semantic_seal_true:{key}")
for key in (
    "common_coordinate_implies_common_gauge",
    "float64_compute_increases_authority",
    "float32_storage_increases_authority",
    "appearance_scientific_writeback_allowed",
):
    if seal_semantics.get(key) is not False:
        errors.append(f"semantic_seal_false:{key}")

if errors:
    print("DRAW_LENS_INDEPENDENT_FREE_WORLD_V01_SEAL_FAIL")
    for error in errors:
        print(error)
    sys.exit(1)

print("DRAW_LENS_INDEPENDENT_FREE_WORLD_V01_SEAL_PASS")
print("sealed_files=4")
print("lens_independent=true")
print("truthnegative_per_observation=true")
print("zero_line=T=log2(L/L0)")
print("common_gauge_implicit=false")
print("float64_compute_authority_upgrade=false")
print("float32_storage_authority_upgrade=false")
print("finite_output_pixel=PROJECTION_NOT_PRIMITIVE")

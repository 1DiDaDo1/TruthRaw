#!/usr/bin/env python3
"""TruthRaw documentation governance verifier.

The verifier follows the living 2026-09-16 integration architecture while
preserving older dated snapshots as historical provenance. It deliberately does
not require an older file merely because its filename contains CURRENT.
"""
from pathlib import Path
import json
import re
import sys

repo = Path(__file__).resolve().parents[1]
errors: list[str] = []


def need(path: str) -> str:
    p = repo / path
    if not p.exists():
        errors.append(f"missing:{path}")
        return ""
    return p.read_text(encoding="utf-8")


root_readme = need("README.md")
bootstrap = need("START_HERE_NEW_CHAT.md")
architecture = need("docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md")
index = need("docs/DOCUMENT_STATUS_INDEX_2026-09-16.md")
current_index = need("docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md")
history = need("docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md")
handoff = need("docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md")
next_handoff = need("docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md")
state_text = need("state/CURRENT_PROJECT_STATE_2026-09-16.json")
current_state_text = need("state/CURRENT_PROJECT_STATE_2026-09-19.json")
need("state/README.md")

# New research foundations that the current integration line explicitly carries.
need("docs/research/scene-physics-calibration-structure-hdr-v0.1/README.md")
need("docs/research/conservation-restoration-authority-v0.1/README.md")

current_pointers = (
    "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md",
    "state/CURRENT_PROJECT_STATE_2026-09-19.json",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md",
    "docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md",
)
for required in current_pointers:
    if required not in root_readme and required != "docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md":
        errors.append(f"root_readme_missing_current_pointer:{required}")
    if required not in bootstrap and required != "docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md":
        errors.append(f"bootstrap_missing_current_pointer:{required}")
    if required != "docs/DOCUMENT_STATUS_INDEX_2026-09-19_MULTIVENDOR.md" and required not in current_index:
        errors.append(f"current_index_missing_current_pointer:{required}")

# Retain older consolidated pointers as provenance/background discoverability.
legacy_pointers = (
    "state/CURRENT_PROJECT_STATE_2026-09-16.json",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-16.md",
    "docs/PROJECT_HISTORY_AND_CHANGES_2026-09-16.md",
    "docs/handoff/TRUTHRAW_CONSOLIDATED_HANDOFF_2026-09-16.md",
)
for required in legacy_pointers:
    if required not in bootstrap and required not in current_index:
        errors.append(f"legacy_pointer_not_discoverable:{required}")

# Historical snapshots remain present and classified, but are not global-current.
historical = (
    "docs/CURRENT_HOUSE_ARCHITECTURE_2026-09-10.md",
    "docs/DOCUMENT_STATUS_INDEX_2026-09-10.md",
    "state/CURRENT_CANONICAL_STATE_2026-09-10.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-06.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-08.json",
    "state/CURRENT_CANONICAL_STATE_2026-09-09.json",
    "docs/PROJECT_STATE_AUDIT_2026-09-08.md",
)
for old in historical:
    if not (repo / old).exists():
        errors.append(f"historical_snapshot_missing:{old}")
    if old not in index:
        errors.append(f"historical_snapshot_not_classified:{old}")

# Current bootstrap must have a numbered reading-order section. Historical state
# files may be mentioned as provenance but may not lead that order.
m = re.search(r"## Mandatory current reading order\n([\s\S]*?)(?=\n## )", bootstrap)
mandatory = m.group(1) if m else ""
if not m:
    errors.append("mandatory_current_reading_order_section_missing")
if re.search(
    r"^\s*\d+\..*CURRENT_(?:CANONICAL_STATE|HOUSE_ARCHITECTURE)_2026-09-(?:06|08|09|10)",
    mandatory,
    re.MULTILINE,
):
    errors.append("historical_state_in_current_mandatory_reading_order")

try:
    state = json.loads(state_text)
except Exception as exc:
    errors.append(f"current_project_state_invalid_json:{exc}")
    state = {}

if state.get("schema") != "TruthRawCurrentProjectState/2026-09-16":
    errors.append("historical_2026_09_16_state_schema_mismatch")
if state.get("status") != "CURRENT_RESEARCH_INTEGRATION_STATE_NOT_MAIN_PROMOTION":
    errors.append("historical_2026_09_16_state_status_mismatch")

try:
    current_state = json.loads(current_state_text)
except Exception as exc:
    errors.append(f"current_2026_09_19_project_state_invalid_json:{exc}")
    current_state = {}

if current_state.get("schema") != "TruthRawCurrentProjectState/2026-09-19":
    errors.append("current_2026_09_19_project_state_schema_mismatch")
if current_state.get("status") != "CURRENT_RESEARCH_INTEGRATION_STATE_NOT_MAIN_PROMOTION":
    errors.append("current_2026_09_19_project_state_status_mismatch")
if current_state.get("activeBranch") != "integration/truthraw-suite-v0-63-crc-ui-branding":
    errors.append("current_active_branch_mismatch")
if current_state.get("nextChatHandoff") != "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-19.md":
    errors.append("current_next_chat_handoff_mismatch")

product = current_state.get("productArchitecture") or {}
if product.get("primaryFrontDoor") != "RAW_FILE_IMPORT":
    errors.append("current_primary_front_door_must_be_raw_file_import")
if product.get("secondaryFrontDoor") != "CAMERA_CAPTURE":
    errors.append("current_secondary_front_door_must_be_camera_capture")
if product.get("convergencePoint") != "SEALED_SOURCE_ADMISSION":
    errors.append("current_ingress_convergence_point_mismatch")

v059 = (((current_state.get("multiVendorRaw") or {}).get("v059")) or {})
on_binding = v059.get("onValidExactScopeBinding") or {}
if on_binding.get("scientificAdmissionReady") is not False:
    errors.append("nef_v059_scientific_admission_must_remain_blocked")
if v059.get("realNikonCalibrationPackAdmitted") is not False:
    errors.append("nef_v059_real_calibration_pack_must_remain_unadmitted")

v060 = (((current_state.get("multiVendorRaw") or {}).get("v060")) or {})
v060_output = v060.get("output") or {}
if v060.get("role") != "RESTORED_TRUTHRAW_PURE_FLOAT32_SCIENTIFIC_DNG_IN_CURRENT_MULTIVENDOR_APP":
    errors.append("v060_pure_role_mismatch")
if v060_output.get("bitsPerSample") != 32:
    errors.append("v060_pure_bits_per_sample_must_be_32")
if v060_output.get("sampleFormat") != "IEEE_FLOAT":
    errors.append("v060_pure_sample_format_must_be_ieee_float")
if v060_output.get("negativeValuesPreserved") is not True:
    errors.append("v060_pure_negative_values_must_be_preserved")
if v060_output.get("overOneValuesPreserved") is not True:
    errors.append("v060_pure_over_one_values_must_be_preserved")
if v060_output.get("bounded01Clipping") is not False:
    errors.append("v060_pure_must_not_clip_to_unit_interval")
compat = v060.get("compatibilitySeparation") or {}
if compat.get("uint16LinearDngRole") != "COMPATIBILITY_ONLY":
    errors.append("v060_uint16_linear_dng_must_remain_compatibility_only")

v061 = (((current_state.get("multiVendorRaw") or {}).get("v061")) or {})
v061_binding = v061.get("selfBinding") or {}
if v061.get("privateContract") != "TRUTHRAW_PURE_SELF_BINDING_V0_61":
    errors.append("v061_private_contract_mismatch")
if v061.get("inheritsV060PixelRouteWithoutPixelMathChange") is not True:
    errors.append("v061_must_not_change_v060_pixel_math")
if v061_binding.get("zeroLineL0ExactBinary64Bits") is not True:
    errors.append("v061_zero_line_l0_exact_bits_required")
if v061_binding.get("exactSerializedTechnicalBackplaneBytes") != 180:
    errors.append("v061_exact_backplane_bytes_required")
if (v061.get("historicalCertificateFinding") or {}).get("reactivatedAsCurrentCertificate") is not False:
    errors.append("legacy_trcert01_must_not_be_reactivated")

v062 = (((current_state.get("multiVendorRaw") or {}).get("v062")) or {})
v062_gate = v062.get("postWriteGate") or {}
if v062.get("writerPrivateContract") != "TRUTHRAW_PURE_SELF_BINDING_V0_61":
    errors.append("v062_must_inherit_v061_writer_contract")
if v062.get("pixelMathChanged") is not False:
    errors.append("v062_must_not_change_pure_pixel_math")
if v062_gate.get("reopensExactDestinationUri") is not True:
    errors.append("v062_postwrite_destination_reopen_required")
if v062_gate.get("requiresTechnicalBackplaneBytes") != 180:
    errors.append("v062_exact_backplane_postwrite_gate_required")
if v062_gate.get("rejectsLegacyCompatibilityRoleMarker") is not True:
    errors.append("v062_must_reject_legacy_compatibility_role_marker")

v063 = (((current_state.get("multiVendorRaw") or {}).get("v063")) or {})
v063_crc = v063.get("technicalBackplaneCrc") or {}
v063_ui = v063.get("ui") or {}
if v063.get("privateContract") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v063_private_contract_mismatch")
if v063.get("pixelMathChanged") is not False:
    errors.append("v063_must_not_change_pure_pixel_math")
if v063_crc.get("serializedBytes") != 180:
    errors.append("v063_backplane_size_must_remain_180")
if v063_crc.get("crcScope") != "PREFIX_176_BYTES":
    errors.append("v063_crc_scope_must_be_prefix_176")
if v063_crc.get("storedCrcOffset") != 176:
    errors.append("v063_stored_crc_offset_mismatch")
if v063_crc.get("postWriteRecomputesCrc") is not True:
    errors.append("v063_postwrite_crc_recompute_required")
if v063_crc.get("postWriteComparesInternalStoredCrc") is not True:
    errors.append("v063_internal_crc_compare_required")
if v063_crc.get("postWriteComparesDngDeclaredCrc") is not True:
    errors.append("v063_declared_crc_compare_required")
if v063_crc.get("oldFull180ResidueBehaviorRejected") is not True:
    errors.append("v063_old_full180_crc_residue_must_be_rejected")
if v063_ui.get("sameSealedSourceConvergence") is not True:
    errors.append("v063_ui_routes_must_share_sealed_source_convergence")
if v063_ui.get("diagnosticsMovedBehindSettings") is not True:
    errors.append("v063_diagnostics_must_move_behind_settings")

laws = state.get("scientific_laws") or {}
for key, expected in {
    "source_evidence_immutable": True,
    "representation_may_exceed_source": True,
    "knowledge_claims_may_not_exceed_evidence": True,
    "measured_may_not_be_relabelled_from_reconstruction": True,
    "counterfactual_may_not_be_relabelled_as_capture_evidence": True,
    "appearance_or_transport_may_upgrade_authority": False,
    "physical_frame_count": 1,
    "independent_evidence_count": 1,
}.items():
    if laws.get(key) != expected:
        errors.append(f"scientific_law_mismatch:{key}")

if (state.get("governance") or {}).get("current_navigation") != "docs/DOCUMENT_STATUS_INDEX_2026-09-16.md":
    errors.append("current_navigation_not_2026_09_16_index")

# Guard the current 200 MP boundary.
camera5 = state.get("camera5_maximum_resolution") or {}
if camera5.get("qualifying_raw_sensor_evidence_supplied") is not False:
    errors.append("camera5_200mp_runtime_evidence_must_remain_open_until_real_capture")
if camera5.get("forbidden_unproven_claim") != "UNTOUCHED_NATIVE_200MP_ADC":
    errors.append("camera5_200mp_forbidden_claim_guard_missing")

# The new research foundations must be discoverable from current governance.
for p in (
    "docs/research/scene-physics-calibration-structure-hdr-v0.1/README.md",
    "docs/research/conservation-restoration-authority-v0.1/README.md",
):
    if p not in index and p not in architecture and p not in history:
        errors.append(f"current_research_foundation_not_indexed:{p}")

# README-like files are version-local unless explicitly global-current.
for p in repo.rglob("*"):
    if not p.is_file():
        continue
    rel = p.relative_to(repo).as_posix()
    name = p.name
    if not (
        name.startswith("README")
        or name == "START_HERE_NEW_CHAT.md"
        or re.match(r"CURRENT_(?:CANONICAL|PROJECT)_STATE_\d{4}-\d{2}-\d{2}\.json$", name)
        or re.match(r"PROJECT_STATE_AUDIT_\d{4}-\d{2}-\d{2}\.md$", name)
    ):
        continue
    classified = (
        rel in {"README.md", "START_HERE_NEW_CHAT.md", "state/README.md"}
        or rel.startswith("canonical/")
        or rel.startswith("docs/research/")
        or rel.startswith("capture/")
        or rel.startswith("docs/calibration/")
        or rel.startswith("tests/")
        or rel.startswith("android/")
        or rel.startswith("state/CURRENT_CANONICAL_STATE_")
        or rel in {"state/CURRENT_PROJECT_STATE_2026-09-16.json", "state/CURRENT_PROJECT_STATE_2026-09-19.json"}
        or rel.startswith("docs/PROJECT_STATE_AUDIT_")
    )
    if not classified:
        errors.append(f"unclassified_readme_like_path:{rel}")

# PTC acronym guard remains permanent.
for text, label in (
    (root_readme, "root"),
    (bootstrap, "bootstrap"),
    (architecture, "architecture"),
    (index, "index"),
    (history, "history"),
    (handoff, "handoff"),
):
    if "canonical/ptc/v1.1" in text and "Pure Truth Certificate" not in text:
        errors.append(f"ptc_name_guard_missing:{label}")

if errors:
    print("DOCUMENTATION_GOVERNANCE_FAIL")
    for e in errors:
        print(e)
    sys.exit(1)

print("DOCUMENTATION_GOVERNANCE_PASS")
print("current_navigation=2026-09-19_multivendor")
print("historical_snapshots_preserved=7")
print("source_evidence=IMMUTABLE")
print("free_scientific_space=OPEN_WORLD_EVIDENCE_BOUNDED")
print("camera5_200mp_runtime_gate=OPEN_REAL_CAPTURE_REQUIRED")

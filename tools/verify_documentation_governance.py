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
v073_handoff = need("docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md")
v084_handoff = need("docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md")
state_text = need("state/CURRENT_PROJECT_STATE_2026-09-16.json")
current_state_text = need("state/CURRENT_PROJECT_STATE_2026-09-19.json")
v073_state_text = need("state/CURRENT_PROJECT_STATE_2026-09-20.json")
v084_state_text = need("state/CURRENT_PROJECT_STATE_2026-09-21.json")
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

# Current 2026-09-20 overlay validation.
# The detailed v0.72 baseline checks below remain intact; this block validates
# the moving current integration overlay without hard-coding one historical app version.
for required in (
    "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md",
    "state/CURRENT_PROJECT_STATE_2026-09-20.json",
):
    if required not in bootstrap:
        errors.append(f"bootstrap_missing_current_2026_09_20_pointer:{required}")

try:
    overlay_state = json.loads(v073_state_text)
except Exception as exc:
    errors.append(f"current_2026_09_20_project_state_invalid_json:{exc}")
    overlay_state = {}

if overlay_state.get("schema") != "TruthRawCurrentProjectState/2026-09-20":
    errors.append("current_2026_09_20_project_state_schema_mismatch")
if overlay_state.get("status") != "CURRENT_RESEARCH_INTEGRATION_STATE_NOT_MAIN_PROMOTION":
    errors.append("current_2026_09_20_project_state_status_mismatch")
active_overlay_branch = overlay_state.get("activeBranch")
if not isinstance(active_overlay_branch, str) or not active_overlay_branch.startswith("integration/truthraw-suite-v0-"):
    errors.append("current_2026_09_20_active_branch_invalid")
if overlay_state.get("nextChatHandoff") != "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-20.md":
    errors.append("current_2026_09_20_next_chat_handoff_mismatch")

laws = overlay_state.get("permanentLaws") or {}
if laws.get("sourceEvidenceImmutable") is not True:
    errors.append("current_overlay_source_evidence_must_remain_immutable")
if laws.get("knowledgeClaimsMayNotExceedEvidence") is not True:
    errors.append("current_overlay_knowledge_claims_may_not_exceed_evidence")
if laws.get("physicalFrameCount") != 1 or laws.get("independentEvidenceCount") != 1:
    errors.append("current_overlay_frame_evidence_must_remain_one_one")
if laws.get("appearanceNeverWritesBack") is not True:
    errors.append("current_overlay_appearance_writeback_forbidden")
if laws.get("counterfactualNeverEvidence") is not True:
    errors.append("current_overlay_counterfactual_never_evidence")

frozen_overlay = overlay_state.get("frozenScience") or {}
if frozen_overlay.get("pureContract") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("current_overlay_must_keep_v063_pure_contract")
if frozen_overlay.get("restorationAlgorithm") != "v0.67 unchanged":
    errors.append("current_overlay_must_keep_v067_restoration_frozen")
if frozen_overlay.get("canonicalOpenSceneParent") != "v0.70":
    errors.append("current_overlay_must_keep_v070_open_scene_parent")

open_validation = overlay_state.get("openValidation") or {}
if open_validation.get("native16320x12288ScientificAdmission") not in (
    "UNPROVEN_AND_NOT_ALLOWED", "OPEN", None
):
    errors.append("current_overlay_16320x12288_may_not_be_promoted_by_documentation")

# Current 2026-09-21 adaptive-compute/output-authority overlay validation.
for required in (
    "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md",
    "state/CURRENT_PROJECT_STATE_2026-09-21.json",
):
    if required not in bootstrap:
        errors.append(f"bootstrap_missing_current_2026_09_21_pointer:{required}")

try:
    current_2026_09_21 = json.loads(v084_state_text)
except Exception as exc:
    errors.append(f"current_2026_09_21_project_state_invalid_json:{exc}")
    current_2026_09_21 = {}

if current_2026_09_21.get("schema") != "TruthRawCurrentProjectState/2026-09-21":
    errors.append("current_2026_09_21_project_state_schema_mismatch")
if current_2026_09_21.get("status") != "CURRENT_RESEARCH_INTEGRATION_STATE_NOT_MAIN_PROMOTION":
    errors.append("current_2026_09_21_project_state_status_mismatch")
if current_2026_09_21.get("activeBranch") != "integration/truthraw-suite-v0-84-2-adaptive-compute-router":
    errors.append("current_2026_09_21_active_branch_mismatch")
if current_2026_09_21.get("nextChatHandoff") != "docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-21.md":
    errors.append("current_2026_09_21_next_chat_handoff_mismatch")

laws_21 = current_2026_09_21.get("permanentLaws") or {}
for key, expected in {
    "sourceEvidenceImmutable": True,
    "knowledgeClaimsMayNotExceedEvidence": True,
    "physicalFrameCount": 1,
    "independentEvidenceCount": 1,
    "appearanceNeverWritesBack": True,
    "counterfactualNeverEvidence": True,
    "computeResourcesNeverIncreaseAuthority": True,
    "sealedFullFrameStreamingV01ByteFrozen": True,
}.items():
    if laws_21.get(key) != expected:
        errors.append(f"current_2026_09_21_law_mismatch:{key}")

authority_21 = current_2026_09_21.get("v084OutputAuthority") or {}
if authority_21.get("perOutputChannelAuthorityAvailable") is not True:
    errors.append("current_2026_09_21_output_authority_must_be_available")
if authority_21.get("scientificHdrAuthority") != "BLOCKED":
    errors.append("current_2026_09_21_scientific_hdr_must_remain_blocked")
if authority_21.get("scientificHdrBlockedReason") != "UNKNOWN_CHANNEL_AUTHORITY_PRESENT":
    errors.append("current_2026_09_21_hdr_block_reason_mismatch")
if authority_21.get("scientificGainAllowed") is not False:
    errors.append("current_2026_09_21_scientific_gain_must_remain_forbidden")
if authority_21.get("reconstructedUncertaintyCurrentlyAdmitted") is not False:
    errors.append("current_2026_09_21_reconstructed_uncertainty_must_remain_unadmitted")

ci_21 = current_2026_09_21.get("ci") or {}
if ci_21.get("status") != "SUCCESS":
    errors.append("current_2026_09_21_ci_must_be_green")
if ci_21.get("run") != 35545744042:
    errors.append("current_2026_09_21_ci_run_mismatch")
if ci_21.get("headSha") != "86307391ca726c2389fc3468fd79dee8b7dc864b":
    errors.append("current_2026_09_21_code_head_mismatch")
if ci_21.get("apkSha256") != "d2f1eb0f76d6b3ca302a98d17d12ec1b2b382280b8d39341c964bb52f44cec3a":
    errors.append("current_2026_09_21_apk_sha_mismatch")

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
if current_state.get("activeBranch") != "integration/truthraw-suite-v0-72-projection-lifecycle-progress-fix":
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

v064 = (((current_state.get("multiVendorRaw") or {}).get("v064")) or {})
v064_modules = v064.get("modules") or {}
v064_invariants = v064.get("invariants") or {}
v064_output = v064.get("outputScope") or {}
if v064.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v064_must_keep_v063_pure_writer_contract")
if v064.get("purePixelMathChanged") is not False:
    errors.append("v064_must_not_change_pure_pixel_math")
if v064.get("scientificMasterWriteback") is not False:
    errors.append("v064_advanced_must_not_write_back_scientific_master")
if (v064_modules.get("naturalLightBalance") or {}).get("physicalRelightClaim") is not False:
    errors.append("v064_light_balance_must_not_claim_physical_relight")
if (v064_modules.get("naturalHdr") or {}).get("createsMeasuredDynamicRange") is not False:
    errors.append("v064_hdr_must_not_create_measured_dynamic_range")
if (v064_modules.get("restoration") or {}).get("underlyingAuthorityRemains") != "CENSORED":
    errors.append("v064_restoration_must_preserve_censored_authority")
if (v064_modules.get("restoration") or {}).get("measuredPixelsRelabelled") is not False:
    errors.append("v064_restoration_must_not_relabel_measured_pixels")
if v064_invariants.get("physicalFrameCount") != 1 or v064_invariants.get("independentEvidenceCount") != 1:
    errors.append("v064_frame_evidence_must_remain_one_one")
if v064_invariants.get("scientificMasterModifiedByAppearance") is not False:
    errors.append("v064_appearance_must_not_modify_scientific_master")
if v064_invariants.get("counterfactualObservationCreated") is not False:
    errors.append("v064_advanced_v01_must_not_create_counterfactual_observation")
if v064_output.get("advancedPreviewMaxEdge") != 384:
    errors.append("v064_advanced_preview_scope_mismatch")
if v064_output.get("fullResolutionAdvancedExportReady") is not False:
    errors.append("v064_full_resolution_advanced_export_must_remain_open")
if v064_output.get("pureFloat32DngStillFullResolution") is not True:
    errors.append("v064_pure_full_resolution_must_remain_available")

v065 = (((current_state.get("multiVendorRaw") or {}).get("v065")) or {})
v065_ui = v065.get("uiFixes") or {}
v065_icon = v065.get("icon") or {}
if v065.get("inheritsV064AdvancedDerivative") is not True:
    errors.append("v065_must_inherit_v064_advanced")
if v065.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v065_must_keep_v063_pure_writer_contract")
if v065.get("purePixelMathChanged") is not False:
    errors.append("v065_must_not_change_pure_pixel_math")
if v065.get("advancedMathChanged") is not False:
    errors.append("v065_must_not_change_advanced_math")
if v065_ui.get("scrollViewportOwnsSystemBarInsets") is not True:
    errors.append("v065_scroll_viewport_must_own_system_bar_insets")
if v065_ui.get("contentNoLongerScrollsUnderStatusBar") is not True:
    errors.append("v065_status_bar_overlap_fix_missing")
if v065_ui.get("bottomNavigationSafeArea") is not True:
    errors.append("v065_bottom_navigation_safe_area_missing")
if v065_ui.get("truthrawPureTitleForcedToTwoLines") is not True:
    errors.append("v065_pure_title_wrap_contract_missing")
if v065_ui.get("truthrawAdvancedTitleForcedToTwoLines") is not True:
    errors.append("v065_advanced_title_wrap_contract_missing")
if v065_icon.get("cleanTrMonogram") is not True:
    errors.append("v065_clean_tr_icon_missing")
if v065_icon.get("subtleLensReflectionInUpperR") is not True:
    errors.append("v065_lens_reflection_icon_contract_missing")
if v065_icon.get("assetSha256") != "398874fcf8052761fce9451e70088e20985210ef49b869e7c6f14cf7cb4fe607":
    errors.append("v065_icon_asset_sha256_mismatch")

v066 = (((current_state.get("multiVendorRaw") or {}).get("v066")) or {})
v066_tn = v066.get("truthNegative") or {}
v066_ow = v066.get("openWorld") or {}
v066_da = v066.get("dynamicAuthority") or {}
if v066.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v066_must_keep_v063_pure_writer_contract")
if v066_tn.get("active") is not True or v066_tn.get("exactMasterReplayRequired") is not True:
    errors.append("v066_truthnegative_master_replay_required")
if v066_tn.get("createsNewEvidence") is not False or v066_tn.get("createsSecondScientificWorld") is not False:
    errors.append("v066_truthnegative_may_not_inflate_evidence_or_worlds")
if v066_ow.get("genericPhysicalRelightClaim") is not False:
    errors.append("v066_open_world_may_not_claim_generic_physical_relight")
if v066_da.get("censoredSupportNotPromoted") is not True:
    errors.append("v066_dynamic_authority_censored_support_must_not_promote")
if v066_da.get("restorationScientificWriteback") is not False:
    errors.append("v066_restoration_scientific_writeback_forbidden")

v067 = (((current_state.get("multiVendorRaw") or {}).get("v067")) or {})
v067_rest = v067.get("fullResolutionRestoration") or {}
v067_output = v067.get("output") or {}
if v067.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v067_must_keep_v063_pure_writer_contract")
if v067_rest.get("active") is not True or v067_rest.get("sourceResolutionOneToOne") is not True:
    errors.append("v067_fullres_restoration_must_be_one_to_one")
if v067_rest.get("retreatable") is not True or v067_rest.get("provenanceBound") is not True:
    errors.append("v067_restoration_must_be_retreatable_and_provenance_bound")
if v067_rest.get("scientificMasterModified") is not False:
    errors.append("v067_restoration_must_not_modify_scientific_master")
if v067_rest.get("scientificWritebackAllowed") is not False:
    errors.append("v067_restoration_scientific_writeback_forbidden")
if v067_rest.get("createsNewEvidence") is not False or v067_rest.get("createsSecondScientificWorld") is not False:
    errors.append("v067_restoration_may_not_inflate_evidence_or_worlds")
if v067_rest.get("physicalFrameCount") != 1 or v067_rest.get("independentEvidenceCount") != 1:
    errors.append("v067_frame_evidence_must_remain_one_one")
if v067_output.get("extension") != ".trr":
    errors.append("v067_restoration_container_extension_mismatch")
if v067_output.get("conventionalDngExrTiffProjectionReady") is not False:
    errors.append("v067_conventional_restoration_projection_must_remain_open")

v068 = (((current_state.get("multiVendorRaw") or {}).get("v068")) or {})
v068_tx = v068.get("transaction") or {}
v068_inv = v068.get("scientificInvariants") or {}
v068_find = v068.get("realDeviceV067LifecycleFinding") or {}
if v068.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v068_must_keep_v063_pure_writer_contract")
if v068.get("inheritsV067ScientificRestorationUnchanged") is not True:
    errors.append("v068_must_not_change_v067_restoration_science")
if v068_find.get("incompleteArtifactObserved") is not True:
    errors.append("v068_must_preserve_v067_lifecycle_failure_finding")
if v068_find.get("finalHeaderPresent") is not False:
    errors.append("v068_v067_failure_header_finding_mismatch")
if v068_tx.get("foregroundServiceType") != "dataSync":
    errors.append("v068_foreground_service_type_mismatch")
if v068_tx.get("stagingLocation") != "APP_PRIVATE_FILES":
    errors.append("v068_staging_must_be_app_private")
if v068_tx.get("finalSafDestinationReceivesNativeLongRunDirectly") is not False:
    errors.append("v068_native_long_run_must_not_write_direct_to_saf")
if v068_tx.get("stagingVerifiedBeforeCommit") is not True:
    errors.append("v068_staging_verify_required")
if v068_tx.get("destinationHeaderPolicy") != "ZERO_HEADER_UNTIL_BODY_COMPLETE_THEN_VALID_HEADER_LAST":
    errors.append("v068_header_last_commit_required")
if v068_tx.get("exactDestinationReopened") is not True:
    errors.append("v068_exact_destination_reopen_required")
if v068_tx.get("wholeFileSha256ComparedToStaging") is not True:
    errors.append("v068_whole_file_sha_compare_required")
if v068_tx.get("staleProcessDeathCleanup") is not True:
    errors.append("v068_stale_cleanup_required")
if v068_inv.get("v067NativeAlgorithmChanged") is not False:
    errors.append("v068_may_not_change_v067_native_algorithm")
if v068_inv.get("scientificMasterModified") is not False:
    errors.append("v068_may_not_modify_scientific_master")
if v068_inv.get("scientificWritebackAllowed") is not False:
    errors.append("v068_scientific_writeback_forbidden")
if v068_inv.get("createsNewEvidence") is not False or v068_inv.get("createsSecondScientificWorld") is not False:
    errors.append("v068_may_not_inflate_evidence_or_worlds")
if v068_inv.get("physicalFrameCount") != 1 or v068_inv.get("independentEvidenceCount") != 1:
    errors.append("v068_frame_evidence_must_remain_one_one")

v069 = (((current_state.get("multiVendorRaw") or {}).get("v069")) or {})
v069_tn = v069.get("truthNegative") or {}
v069_proj = v069.get("restorationProjection") or {}
v069_cables = v069.get("directLooseCables") or {}
if v069.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v069_must_keep_v063_pure_writer_contract")
if v069_tn.get("version") != 3 or v069_tn.get("fullFrameOpenSceneState") is not True:
    errors.append("v069_truthnegative_tn3_required")
if v069_tn.get("scientificMasterWritebackAllowed") is not False:
    errors.append("v069_open_scene_scientific_writeback_forbidden")
if v069_tn.get("createsNewEvidence") is not False or v069_tn.get("createsSecondScientificWorld") is not False:
    errors.append("v069_tn3_may_not_inflate_evidence_or_worlds")
if v069_proj.get("fullResolution") is not True or v069_proj.get("requiresTrrDerivativeDigestMatch") is not True:
    errors.append("v069_projection_must_be_fullres_and_derivative_bound")
if v069_proj.get("stagingWholeFileSha256") is not True or v069_proj.get("destinationWholeFileSha256Match") is not True:
    errors.append("v069_projection_transaction_sha_required")
for cable in (
    "canonicalOpenSceneBinding",
    "restorationRoleMaskExportBinding",
    "externalDngTiffExrConformance",
    "effectfulCensoredRestorationDeviceValidation",
):
    if v069_cables.get(cable) != "OPEN":
        errors.append("v069_loose_cable_state_must_remain_explicit_" + cable)

v070 = (((current_state.get("multiVendorRaw") or {}).get("v070")) or {})
v070_scene = v070.get("canonicalOpenScene") or {}
v070_role = v070.get("restorationRoleMaskBinding") or {}
v070_cables = v070.get("directLooseCables") or {}
if v070.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v070_must_keep_v063_pure_writer_contract")
if v070_scene.get("schema") != "TruthRawOpenSceneCanonicalState/0.70":
    errors.append("v070_open_scene_schema_mismatch")
if v070_scene.get("semanticParentRegion") != "TruthRawOpenSceneRegion/0.7":
    errors.append("v070_region_parent_mismatch")
if v070_scene.get("semanticParentStream") != "TruthRawOpenSceneStateSummary/0.8":
    errors.append("v070_stream_parent_mismatch")
if v070_scene.get("chunkingChangesScientificIdentity") is not False:
    errors.append("v070_chunking_may_not_change_scene_identity")
if v070_scene.get("scientificMasterWritebackAllowed") is not False:
    errors.append("v070_open_scene_writeback_forbidden")
if v070_scene.get("createsNewEvidence") is not False:
    errors.append("v070_open_scene_may_not_create_evidence")
if v070_scene.get("physicalFrameCount") != 1 or v070_scene.get("independentEvidenceCount") != 1:
    errors.append("v070_frame_evidence_must_remain_one_one")
if v070_role.get("exactMaskSha256ComputedFromTrr") is not True:
    errors.append("v070_role_mask_exact_hash_required")
if v070_role.get("dngPrivateBinding") is not True or v070_role.get("tiffProvenanceBinding") is not True or v070_role.get("exrProvenanceBinding") is not True:
    errors.append("v070_role_mask_must_bind_all_normal_projections")
if v070_role.get("fullMaskBytesRemainInTrr") is not True:
    errors.append("v070_full_role_mask_source_must_remain_trr")
if v070_role.get("selfContainedMaskEmbeddingReady") is not False:
    errors.append("v070_self_contained_mask_embedding_must_remain_open")
if v070_cables.get("canonicalOpenSceneBinding") != "PARTIAL_V070_TN3_AND_PROJECTIONS_CONNECTED":
    errors.append("v070_canonical_open_scene_cable_state_mismatch")
if v070_cables.get("restorationRoleMaskExportBinding") != "PARTIAL_V070_EXACT_HASH_BOUND_ALL_NORMAL_PROJECTIONS":
    errors.append("v070_role_mask_cable_state_mismatch")
if v070_cables.get("externalDngTiffExrConformance") != "OPEN":
    errors.append("v070_external_conformance_must_remain_open")
if v070_cables.get("effectfulCensoredRestorationDeviceValidation") != "OPEN":
    errors.append("v070_effectful_restoration_validation_must_remain_open")

v071 = (((current_state.get("multiVendorRaw") or {}).get("v071")) or {})
v071_scene = v071.get("canonicalOpenScene") or {}
v071_role = v071.get("restorationRoleMask") or {}
v071_cables = v071.get("directLooseCables") or {}
if v071.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v071_must_keep_v063_pure_writer_contract")
if v071.get("restorationAlgorithmUnchangedFromV067") is not True:
    errors.append("v071_must_not_change_v067_restoration_algorithm")
if v071_scene.get("schema") != "TruthRawOpenSceneCanonicalState/0.70":
    errors.append("v071_open_scene_schema_mismatch")
if v071_scene.get("oneSharedSourceBuilder") is not True:
    errors.append("v071_shared_open_scene_builder_required")
for consumer in ("TN3", "ADVANCED", "TRR", "DNG_PROJECTION", "TIFF_PROJECTION", "EXR_PROJECTION"):
    if consumer not in (v071_scene.get("exactArtifactConsumers") or []):
        errors.append("v071_missing_open_scene_consumer:" + consumer)
if v071_scene.get("projectionRequiresTrrArtifactMatch") is not True:
    errors.append("v071_projection_must_match_trr_open_scene_artifact")
if v071_scene.get("scientificMasterWritebackAllowed") is not False:
    errors.append("v071_open_scene_writeback_forbidden")
if v071_scene.get("createsNewEvidence") is not False:
    errors.append("v071_open_scene_may_not_create_evidence")
if v071_role.get("canonicalEncoding") != "CANONICAL_64X64_CELL_SEQUENCE_UINT8":
    errors.append("v071_role_mask_encoding_mismatch")
if ((v071_role.get("dng") or {}).get("fullMaskEmbedded")) is not True:
    errors.append("v071_dng_full_role_mask_required")
if ((v071_role.get("tiff") or {}).get("fullMaskEmbedded")) is not True:
    errors.append("v071_tiff_full_role_mask_required")
if ((v071_role.get("exr") or {}).get("fullMaskEmbedded")) is not True:
    errors.append("v071_exr_full_role_mask_required")
if v071_cables.get("canonicalOpenSceneBinding") != "CLOSED_IMPLEMENTATION_V071":
    errors.append("v071_open_scene_cable_must_be_closed")
if v071_cables.get("restorationRoleMaskExportBinding") != "CLOSED_IMPLEMENTATION_V071_FULL_PAYLOAD_SELF_CONTAINED":
    errors.append("v071_role_mask_cable_must_be_closed")
if v071_cables.get("externalDngTiffExrConformance") != "OPEN":
    errors.append("v071_external_conformance_must_remain_open_until_tested")
if v071_cables.get("effectfulCensoredRestorationDeviceValidation") != "OPEN":
    errors.append("v071_effectful_restoration_gate_must_remain_open")

v072 = (((current_state.get("multiVendorRaw") or {}).get("v072")) or {})
v072_lifecycle = v072.get("lifecycleFixes") or {}
v072_artifacts = v072.get("uploadedRealDeviceArtifacts") or {}
v072_remaining = v072.get("remainingDirectValidation") or {}
if v072.get("pureWriterContractUnchanged") != "TRUTHRAW_PURE_SELF_BINDING_V0_63":
    errors.append("v072_must_keep_v063_pure_writer_contract")
if v072.get("restorationAlgorithmUnchangedFromV067") is not True:
    errors.append("v072_must_not_change_v067_restoration_algorithm")
if v072.get("scientificPixelMathChanged") is not False:
    errors.append("v072_must_be_lifecycle_only")
if v072_lifecycle.get("startPhase") != "STARTING":
    errors.append("v072_projection_start_phase_required")
if v072_lifecycle.get("startupGraceMs") != 30000:
    errors.append("v072_projection_startup_grace_mismatch")
if v072_lifecycle.get("foregroundRunningFlagSetBeforeStartForegroundService") is not True:
    errors.append("v072_foreground_race_fix_required")
if v072_lifecycle.get("oneProjectionAtATime") is not True:
    errors.append("v072_single_projection_required")
if v072_lifecycle.get("persistentPendingProjectionFormat") is not True:
    errors.append("v072_picker_persistence_required")
if v072_lifecycle.get("sourceSessionRestoredAfterPickerActivityRecreation") is not True:
    errors.append("v072_picker_recreation_source_restore_required")
if v072_lifecycle.get("visibleElapsedChronometer") is not True:
    errors.append("v072_visible_elapsed_progress_required")
trr = v072_artifacts.get("restorationTrr") or {}
dng = v072_artifacts.get("restorationDng") or {}
if trr.get("censoredSourcePixels") != 15855 or trr.get("restoredRole1Pixels") != 15855:
    errors.append("v072_effectful_uploaded_trr_counts_mismatch")
if trr.get("roleMaskSha256") != "8cf0f1bff562c20190f53e7cc6969d7d6d7cf7d855df999c3b479cb09f2211ae":
    errors.append("v072_uploaded_trr_role_hash_mismatch")
if dng.get("fullRoleMaskEmbedded") is not True or dng.get("embeddedRoleCountsMatchTrr") is not True:
    errors.append("v072_uploaded_dng_role_embedding_validation_required")
if v072_remaining.get("independentDngTiffExrConformance") != "OPEN":
    errors.append("v072_external_conformance_must_remain_open")
if v072_remaining.get("realDeviceV072TiffExrCompletionUi") != "OPEN":
    errors.append("v072_tiff_exr_ui_retest_must_remain_open")
if v072_remaining.get("originalSourceReplayForEffectfulRestoration") != "OPEN":
    errors.append("v072_effectful_source_replay_must_remain_open")

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
        or rel in {
            "state/CURRENT_PROJECT_STATE_2026-09-16.json",
            "state/CURRENT_PROJECT_STATE_2026-09-19.json",
            "state/CURRENT_PROJECT_STATE_2026-09-20.json",
            "state/CURRENT_PROJECT_STATE_2026-09-21.json",
        }
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

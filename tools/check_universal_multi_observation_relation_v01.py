#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

code = (JAVA / "UniversalMultiObservationRelationProtocolV01.kt").read_text()
readme = (ROOT / "docs/research/universal-multi-observation-relation-v0.1/README.md").read_text()
state = json.loads(
    (ROOT / "state/UNIVERSAL_MULTI_OBSERVATION_RELATION_PROTOCOL_STATE_2026-09-30.json").read_text()
)

for needle in [
    "D.RAW/UniversalMultiObservationRelationProtocol/0.1",
    "RELATE_OBSERVATIONS_WITH_EXPLICIT_EVIDENCE_NEVER_BY_CAMERA_OR_LENS_NAME",
    "SEALED_CAPTURE_SESSION_PROVENANCE",
    "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
    "OBSERVATION_REPEATABILITY_CANDIDATE",
    "USER_GROUPING_HINT_ONLY",
    "RELATION_UNPROVEN",
    "camera_model_name_may_prove_relation",
    "lens_model_name_may_prove_relation",
    "visual_similarity_may_prove_relation",
    "FIELD_RESPONSE_REPEATABILITY",
    "COLOUR_RELATION",
    "OPTICAL_SUPPORT_RELATION",
    "DARK_NOISE_OFFSET_RELATION",
    "TEMPORAL_CAPTURE_RELATION",
    "minimum_independent_observations",
    "minimum_characterized_illuminants",
    "held_out_validation_required",
    "evaluatePair",
    "same_physical_camera_proven",
    "same_physical_lens_proven",
    "calibration_promoted",
    "correction_authorized",
    "scientific_writeback_allowed",
]:
    assert needle in code, f"missing relation invariant: {needle}"

for forbidden in [
    '.put("camera_model_name_may_prove_relation", true)',
    '.put("lens_model_name_may_prove_relation", true)',
    '.put("vendor_name_may_prove_relation", true)',
    '.put("file_extension_may_prove_relation", true)',
    '.put("visual_similarity_may_prove_relation", true)',
    '.put("source_evidence_merged", true)',
    '.put("independent_evidence_count_upgraded", true)',
    '.put("calibration_promoted", true)',
    '.put("correction_authorized", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert forbidden not in code, f"forbidden relation promotion: {forbidden}"

for token in [
    "tensorflow",
    "pytorch",
    "onnxruntime",
    "neural network",
    "generative model",
]:
    assert token not in code.lower(), f"banned runtime/model token: {token}"

assert state["identity_independence"]["camera_model_name_may_prove_relation"] is False
assert state["identity_independence"]["lens_model_name_may_prove_relation"] is False
assert state["relation_graph"]["source_evidence_merged"] is False
assert state["relation_graph"]["independent_evidence_count_upgraded"] is False
assert state["promotion_boundary"]["relation_protocol_promotes_calibration"] is False
assert state["promotion_boundary"]["relation_protocol_authorizes_correction"] is False
assert state["promotion_boundary"]["relation_protocol_authorizes_deconvolution"] is False
assert state["promotion_boundary"]["relation_protocol_authorizes_scientific_writeback"] is False
assert state["invariants"]["scientific_writeback_allowed"] is False

for phrase in [
    "Relate observations with explicit evidence, never by camera or lens name.",
    "Two observations remain two immutable evidence roots.",
    "no calibration promotion from relation alone",
]:
    assert phrase.lower() in readme.lower(), f"missing documentation boundary: {phrase}"

print("Universal Multi-Observation Relation Protocol v0.1 integrity: PASS")

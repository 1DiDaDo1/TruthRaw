#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

code = (JAVA / "ObservationWorldFieldSeparationV01.kt").read_text()
main = (JAVA / "MainActivity.kt").read_text()
readme = (ROOT / "docs/research/observation-world-field-separation-v0.1/README.md").read_text()
state = json.loads(
    (ROOT / "state/OBSERVATION_WORLD_FIELD_SEPARATION_STATE_2026-09-30.json").read_text()
)

for needle in [
    "D.RAW/ObservationWorldFieldSeparation/0.1",
    "SEALED_MEASUREMENT_GEOMETRY_AND_SENSOR_RELATIVE_FIELD",
    "RELATIVE_WORLD_STRUCTURE_RELATION_BETWEEN_OBSERVATIONS",
    "DERIVED_VIEW_PROJECTION_AND_APPEARANCE",
    "UNREGISTERED_V0_1",
    "SEPARATE_WORLD_FIXED_FROM_SENSOR_FIXED_FIELD_BEHAVIOUR",
    "camera_holder_is_world_origin",
    "panorama_center_is_calibration_origin",
    "single_stitched_360_image_sufficient",
    "original_individual_observations_required_for_measured_authority",
    "automatic_world_registration_available",
    "METHOD_VALIDATION_ONLY_NOT_USER_REQUIREMENT",
    "DETERMINISTIC_CLASSICAL_LOCAL_FEATURE_GEOMETRY",
    "structural_signature_hash_is_registration_proof",
    "visual_similarity_is_relation_proof",
    "registration_uncertainty_required",
    "camera_system_response_proven",
    "lens_only_vignetting_proven",
    "calibration_promoted",
    "correction_authorized",
    "scientific_writeback_allowed",
]:
    assert needle in code, f"missing world-separation invariant: {needle}"

pending_export = (JAVA / "ResearchPendingJsonExportStoreV01.kt").read_text()

for needle in [
    "Export Observation-World Field Separation v0.1 · JSON",
    "launchObservationWorldFieldSeparationExport",
    "REQUEST_SAVE_OBSERVATION_WORLD_FIELD_SEPARATION = 4129",
    "currentObservationWorldProfiles",
    "ResearchPendingJsonExportStoreV01.save",
    "SOURCE/SENSOR SPACE, WORLD/SCENE SPACE en VIEW/OUTPUT SPACE",
]:
    assert needle in main, f"missing Android world-separation binding: {needle}"

for needle in [
    "OBSERVATION_WORLD_FIELD_SEPARATION",
    "copyFrozenTo",
    "sha256",
]:
    assert needle in pending_export, f"missing frozen world-separation export invariant: {needle}"

for forbidden in [
    '.put("camera_holder_is_world_origin", true)',
    '.put("panorama_center_is_calibration_origin", true)',
    '.put("single_stitched_360_image_sufficient", true)',
    '.put("automatic_world_registration_available", true)',
    '.put("structural_signature_hash_is_registration_proof", true)',
    '.put("dominant_edge_orientation_is_registration_proof", true)',
    '.put("visual_similarity_is_relation_proof", true)',
    '.put("camera_system_response_proven", true)',
    '.put("lens_only_vignetting_proven", true)',
    '.put("calibration_promoted", true)',
    '.put("correction_authorized", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert forbidden not in code, f"forbidden world-separation promotion: {forbidden}"

for token in [
    "tensorflow",
    "pytorch",
    "onnxruntime",
    "neural network",
    "generative model",
]:
    assert token not in code.lower(), f"banned runtime/model token: {token}"

assert state["coordinate_spaces"]["world_scene_space"]["registration_status"] == "UNREGISTERED_V0_1"
assert state["coordinate_spaces"]["world_scene_space"]["camera_holder_is_world_origin"] is False
assert state["coordinate_spaces"]["world_scene_space"]["panorama_center_is_calibration_origin"] is False
assert state["coordinate_spaces"]["view_output_space"]["stitched_panorama_is_source_evidence"] is False
assert state["natural_self_calibration"]["normal_user_calibration_required"] is False
assert state["natural_self_calibration"]["flat_field_required_for_normal_use"] is False
assert state["natural_self_calibration"]["360_observation_sequence_allowed"] is True
assert state["natural_self_calibration"]["single_stitched_360_image_sufficient"] is False
assert state["future_registration_contract"]["ai_ml_neural_generative_allowed"] is False
assert state["promotion_boundary"]["calibration_promoted"] is False
assert state["promotion_boundary"]["correction_authorized"] is False
assert state["promotion_boundary"]["scientific_writeback_allowed"] is False

for phrase in [
    "A field behaviour may be called sensor-fixed only after it remains tied to source/sensor coordinates while independently related world structure moves across those coordinates.",
    "The person holding the camera is not the scientific origin.",
    "v0.1 does not yet perform that registration.",
]:
    assert phrase.lower() in readme.lower(), f"missing documentation boundary: {phrase}"

print("Observation-World Field Separation v0.1 integrity: PASS")

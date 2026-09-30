#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

code = (JAVA / "FieldResponseRepeatabilityV01.kt").read_text()
main = (JAVA / "MainActivity.kt").read_text()
readme = (ROOT / "docs/research/field-response-repeatability-v0.1/README.md").read_text()
state = json.loads(
    (ROOT / "state/FIELD_RESPONSE_REPEATABILITY_STATE_2026-09-30.json").read_text()
)

for needle in [
    "D.RAW/FieldResponseRepeatability/0.1",
    "MIN_OBSERVATIONS = 3",
    "MEDIAN_OF_POSITIVE_ANNULUS_P50",
    "log2(annulus_p50/per_observation_scalar)",
    "log2(sector_p50/annulus_p50)",
    "log2(phase_p50/annulus_p50)",
    "USER_GROUPING_HINT_ONLY",
    "NON_AUTHORITY_GROUPING_HINT",
    "radial_shape_pearson",
    "cross_observation_mad_ev",
    "repeatable_camera_system_response_proven",
    "lens_only_vignetting_proven",
    "scene_illumination_separated",
    "sensor_angular_response_separated",
    "calibration_promoted",
    "correction_gain_allowed",
    "automatic_threshold_or_winner_used",
    "scientific_writeback_allowed",
]:
    assert needle in code, f"missing repeatability invariant: {needle}"

for needle in [
    "Multi-observation · Field Response v0.1",
    "Analyseer alle geselecteerde bronnen universeel",
    "Export Field Response Repeatability v0.1 · JSON",
    "launchFieldResponseRepeatabilityExport",
    "REQUEST_SAVE_FIELD_RESPONSE_REPEATABILITY = 4128",
    "currentMeasuredFieldCharts",
    "source-SHA set veranderde",
    "fieldResponseRepeatabilityAnalysisOperationKey",
    "startBackgroundOperation",
    "backgroundOperationStatusView",
    "Analyse actief · nog ",
    "force = true",
    "Alleen DNG-observaties met een werkelijk gemeten PR96",
]:
    assert needle in main, f"missing Android repeatability binding: {needle}"

for forbidden in [
    '.put("same_physical_camera_proven", true)',
    '.put("same_physical_lens_proven", true)',
    '.put("repeatable_camera_system_response_proven", true)',
    '.put("lens_only_vignetting_proven", true)',
    '.put("scene_illumination_separated", true)',
    '.put("sensor_angular_response_separated", true)',
    '.put("calibration_promoted", true)',
    '.put("correction_gain_allowed", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert forbidden not in code, f"forbidden field-response promotion: {forbidden}"

for token in [
    "tensorflow",
    "pytorch",
    "onnxruntime",
    "neural network",
    "generative model",
]:
    assert token not in code.lower(), f"banned runtime/model token: {token}"

assert state["minimum_distinct_measured_observations"] == 3
assert state["normalization"]["exposure_metadata_used"] is False
assert state["normalization"]["white_balance_used"] is False
assert state["normalization"]["gain_map_used"] is False
assert state["normalization"]["scene_content_removed"] is False
assert state["metrics"]["automatic_threshold_or_winner"] is False
assert state["relation_context"]["same_physical_camera_proven"] is False
assert state["relation_context"]["same_physical_lens_proven"] is False
assert state["interpretation"]["calibration_promoted"] is False
assert state["interpretation"]["correction_gain_allowed"] is False
assert state["invariants"]["scientific_writeback_allowed"] is False
assert state["android_ui"]["batch_progress_indicator"]["implemented"] is True
assert state["android_ui"]["batch_progress_indicator"]["elapsed_chronometer"] is True
assert state["android_ui"]["batch_progress_indicator"]["operation_key_scoped_to_selected_source_set"] is True

for phrase in [
    "This is not a lens-vignetting calibration.",
    "Selecting three files together is not proof that they came from the same physical camera or lens.",
    "Thresholds for a stronger controlled relation experiment must not be invented from the same three scenes after seeing the answer.",
]:
    assert phrase.lower() in readme.lower(), f"missing documentation boundary: {phrase}"

print("Field Response Repeatability v0.1 integrity: PASS")

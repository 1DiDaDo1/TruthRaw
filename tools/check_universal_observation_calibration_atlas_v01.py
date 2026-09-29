#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

atlas = (JAVA / "UniversalObservationCalibrationAtlasV01.kt").read_text()
profiler = (JAVA / "UniversalSourceProfiler.kt").read_text()
main = (JAVA / "MainActivity.kt").read_text()
readme = (ROOT / "docs/research/universal-observation-calibration-atlas-v0.1/README.md").read_text()
state = json.loads(
    (ROOT / "state/UNIVERSAL_OBSERVATION_CALIBRATION_ATLAS_STATE_2026-09-30.json").read_text()
)

for needle in [
    "D.RAW/UniversalObservationCalibrationAtlas/0.1",
    "OBSERVE_WHAT_IS_PRESENT_BEFORE_ASKING_WHO_PRODUCED_IT",
    "camera_identity_required",
    "lens_identity_required",
    "prior_user_calibration_required",
    "raw_format_identity_may_define_scientific_truth",
    "APPEARANCE_DERIVED_ONLY",
    "SOURCE_AND_MEASUREMENT_SIDE",
    "SOURCE_METADATA_BOUND_COLOUR_AVAILABLE",
    "white_balance_is_not_spectral_calibration",
    "three_channel_rgb_proves_full_spectrum",
    "COMPOSITE_SCENE_LENS_SENSOR_OBSERVATION_ONLY",
    "lens_only_vignetting_proven",
    "camera_system_relative_illumination_calibrated",
    "sfr_mtf_psf_calibration_attached",
    "synthetic_motion_blur_authority",
    "optional_calibration_observation_pack",
    "normal_user_must_calibrate_camera",
    "float64_branch_sensitive_compute_preserved",
    "controlled_float32_scientific_storage_preserved",
    "black_level_is_zero_line",
    "measured_support_overpaint_allowed",
    "scientific_writeback_allowed",
]:
    assert needle in atlas, f"missing atlas invariant: {needle}"

for needle in [
    "UniversalObservationCalibrationAtlasV01.describe",
    "universal_observation_calibration_atlas",
    "OPAQUE_RAW_OR_IMAGE_CONTAINER",
    "TIFF_CONTAINER_METADATA_PARSE_FAILED",
]:
    assert needle in profiler, f"missing profiler binding: {needle}"

assert profiler.count("UniversalObservationCalibrationAtlasV01.describe") >= 3

for needle in [
    "Export Universal Observation & Calibration Atlas v0.1 · JSON",
    "launchUniversalCalibrationAtlasExport",
    "REQUEST_SAVE_UNIVERSAL_CALIBRATION_ATLAS = 4127",
    "_draw_universal_observation_calibration_atlas_v0_1.json",
]:
    assert needle in main, f"missing Android export binding: {needle}"

for banned in [
    '.put("camera_identity_required", true)',
    '.put("lens_identity_required", true)',
    '.put("prior_user_calibration_required", true)',
    '.put("camera_model_routing_used", true)',
    '.put("vendor_mapping_used", true)',
    '.put("ai_ml_neural_generative_used", true)',
    '.put("automatic_light_falloff_correction_allowed", true)',
    '.put("automatic_colour_correction_from_atlas_allowed", true)',
    '.put("deconvolution_authorized", true)',
    '.put("scientific_writeback_allowed", true)',
]:
    assert banned not in atlas, f"forbidden promoted claim: {banned}"

for token in [
    "tensorflow",
    "pytorch",
    "onnxruntime",
    "neural network",
    "generative model",
]:
    assert token not in atlas.lower(), f"banned runtime/model token: {token}"

assert state["universal_input"]["camera_identity_required"] is False
assert state["universal_input"]["lens_identity_required"] is False
assert state["universal_input"]["prior_user_calibration_required"] is False
assert state["optional_calibration_observation_pack"]["required_for_universal_intake"] is False
assert state["illumination"]["automatic_light_falloff_correction_allowed"] is False
assert state["colour"]["automatic_colour_correction_allowed"] is False
assert state["optical_support"]["deconvolution_authorized"] is False
assert state["precision"]["float64_branch_sensitive_compute_preserved"] is True
assert state["precision"]["controlled_float32_scientific_storage_preserved"] is True
assert state["zero_line_truthrange"]["black_level_is_zero_line"] is False
assert state["restoration"]["measured_support_overpaint_allowed"] is False
assert state["invariants"]["scientific_writeback_allowed"] is False
assert state["integration"]["json_export_added"] is True

for phrase in [
    "Calibration is extra evidence, not an entrance requirement.",
    "universal intake does not mean universal hallucination",
    "BlackLevel is not the Zero-Line",
]:
    assert phrase.lower() in readme.lower(), f"missing documentation boundary: {phrase}"

print("Universal Observation & Calibration Atlas v0.1 integrity: PASS")

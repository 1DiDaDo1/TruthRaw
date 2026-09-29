#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
base = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

selector = (base / "UniversalObservationModelSelectionV01.kt").read_text()
lattice = (base / "RasterIndependentSampleLatticeV01.kt").read_text()
profiler = (base / "UniversalSourceProfiler.kt").read_text()
main = (base / "MainActivity.kt").read_text()
readme = (ROOT / "docs/research/universal-observation-model-selection-v0.1/README.md").read_text()
state_text = (ROOT / "state/UNIVERSAL_OBSERVATION_MODEL_SELECTION_STATE_2026-09-29.json").read_text()
state = json.loads(state_text)

for needle in [
    '"D.RAW/UniversalObservationModelSelection/0.1"',
    '"PROSPECTIVE_AUDIT_POLICY_AVAILABLE"',
    '"OBSERVATION_DERIVED_MODEL_BANK_POLICY_AUDIT_ONLY"',
    '"heldout_target_used_for_selection"',
    '"holdout_error_used_for_selection"',
    '"lens_calibration_used"',
    '"camera_model_used"',
    '"vendor_mapping_used"',
    '"AFFINE_PLANE_V0_1_RESEARCH_CANDIDATE"',
    '"DIRECTIONAL_LINE_RESEARCH_CANDIDATE"',
    '"LOW_ORDER_CURVATURE_RESEARCH_CANDIDATE"',
    '"NO_RECONSTRUCTION_CANDIDATE"',
    '"existing_2026_09_29_holdout_is_development_evidence_only"',
    '"independent_new_capture_required_for_selector_validation"',
    '"affine_plane_promoted", false',
    '"candidate_applied", false',
    '"scientific_writeback_allowed", false',
]:
    assert needle in selector, f"missing selector invariant: {needle}"

for needle in [
    '"source_raster_role"',
    '"EXACT_MEASURED_ANCHOR_GEOMETRY_AND_FULL_RESOLUTION_SOURCE_SUPPORT"',
    '"source_raster_used_only_for_noise", false',
    '"source_raster_preserves_full_resolution_structure_geometry_and_authority"',
    '"scientific_solution_space_resolution_decoupled_from_source_sampling"',
    '"output_projection_resolution_decoupled_from_source_sampling"',
    '"new_raster_may_refine_measurement_space_without_replacing_measurement_history"',
]:
    assert needle in lattice, f"missing raster/lattice law: {needle}"

for needle in [
    'UniversalObservationModelSelectionV01.analyze',
    '"universal_observation_model_selection_v0_1"',
    '"universal_observation_model_selection"',
]:
    assert needle in profiler, f"missing intake integration: {needle}"

for needle in [
    'Originele resolutie is NIET alleen een noise-raster',
    'Universal Observation Model Selection v0.1 · PROSPECTIVE',
    'lensCalibration=false · cameraModel=false · vendorMap=false',
    'een nieuwe onafhankelijke capture is vereist voor validatie',
    'Export Universal Observation Model Selection v0.1 · JSON',
    'REQUEST_SAVE_OBSERVATION_MODEL_SELECTION = 4122',
    'heldout_target_used_for_selection',
    'holdout_error_used_for_selection',
]:
    assert needle in main, f"missing UI contract: {needle}"

for needle in [
    "The original source resolution is not merely a grid for noise analysis.",
    "The source raster determines where D.RAW measured. It does not determine the raster on which D.RAW must think.",
    "A new raster may refine the measurement space without replacing the measurement history.",
    "No target leakage",
    "per-lens noise calibration",
]:
    assert needle in readme, f"missing architecture documentation: {needle}"

assert state["architecture"]["universal_input_required"] is True
assert state["architecture"]["lens_specific_noise_calibration_required"] is False
assert state["architecture"]["source_raster_used_only_for_noise"] is False
assert state["selector"]["heldout_target_used_for_selection"] is False
assert state["selector"]["holdout_error_used_for_selection"] is False
assert state["selector"]["affine_plane_promoted"] is False
assert state["selector"]["computed_before_anchor_holdout_target_reveal"] is True
assert state["selector"]["anchor_holdout_runtime_dependency"] is False
assert state["selector"]["export_contains_holdout_outcomes"] is False
assert state["selector"]["machine_readable_export"] == (
    "Export Universal Observation Model Selection v0.1 · JSON"
)
assert state["selector"]["independent_new_capture_required_for_selector_validation"] is True
assert state["safety"]["measured_anchors_modified"] is False
assert state["safety"]["unanchored_values_promoted_to_measured"] is False
assert state["safety"]["candidate_applied"] is False
assert state["safety"]["scientific_writeback_allowed"] is False

for banned_result_key in [
    "solver_mae",
    "baseline_mae",
    "solver_rmse",
    "baseline_rmse",
    "solver_lower_abs_error",
    "baseline_lower_abs_error",
    "solver_combined_z",
    "actual",
    "target_variance",
]:
    assert banned_result_key not in selector, (
        f"holdout-result leakage into selector source: {banned_result_key}"
    )

selector_call = profiler.index("UniversalObservationModelSelectionV01.analyze")
holdout_call = profiler.index("AnchorConstrainedLocalReconstructionAudit.analyze")
assert selector_call < holdout_call, (
    "prospective selector must be computed before holdout target reveal/scoring"
)
assert "anchorAudit =" not in profiler[selector_call:holdout_call]
assert '"computed_before_anchor_holdout_target_reveal"' in selector
assert '"anchor_holdout_runtime_dependency",\n                false' in selector

lower = (selector + "\n" + lattice + "\n" + profiler).lower()
for banned in [
    "tensorflow",
    "pytorch",
    "onnx",
    "neural network",
    "generative model",
]:
    assert banned not in lower, f"banned learned-runtime token: {banned}"

# Guard against accidental device/lens routing in the selector itself.
for banned in [
    "honor",
    "magic8",
    "cameraid",
    "physicalcamera",
    "focal_length_mm",
    "lens_role",
]:
    assert banned not in selector.lower(), f"device/lens-specific selector dependency: {banned}"

print("Universal Observation Model Selection v0.1 integrity: PASS")

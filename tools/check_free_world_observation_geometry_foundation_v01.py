#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

files = {
    "feature": (JAVA / "DeterministicLocalFeatureGeometryV01.kt").read_text(),
    "pair": (JAVA / "DeterministicLocalFeaturePairGeometryV01.kt").read_text(),
    "graph": (JAVA / "FreeWorldObservationGraphV01.kt").read_text(),
    "world": (JAVA / "RelativeWorldCoordinateHypothesisV01.kt").read_text(),
    "campaign": (JAVA / "MultiObservationCampaignV01.kt").read_text(),
    "atlas": (JAVA / "NaturalSelfCalibrationAtlasV01.kt").read_text(),
    "decomposition": (JAVA / "WorldSensorFieldDecompositionScaffoldV01.kt").read_text(),
    "optics": (JAVA / "OpticalSupportAtlasV01.kt").read_text(),
    "colour": (JAVA / "ColourRelationAtlasV01.kt").read_text(),
    "temporal": (JAVA / "TemporalObservationRelationV01.kt").read_text(),
    "uncertainty": (JAVA / "FreeWorldUncertaintyTransportV01.kt").read_text(),
    "query": (JAVA / "FreeWorldContinuousQueryContractV01.kt").read_text(),
    "restoration": (JAVA / "ConservationRestorationAuthorityRuntimeV01.kt").read_text(),
    "foundation": (JAVA / "FreeWorldObservationGeometryFoundationV01.kt").read_text(),
    "frontside": (JAVA / "FrontsideSceneInspector.kt").read_text(),
    "main": (JAVA / "MainActivity.kt").read_text(),
}
state = json.loads(
    (ROOT / "state/FREE_WORLD_OBSERVATION_GEOMETRY_FOUNDATION_STATE_2026-09-30.json").read_text()
)
readme = (
    ROOT / "docs/research/free-world-observation-geometry-foundation-v0.1/README.md"
).read_text()

required = {
    "feature": [
        "HARRIS_STRUCTURE_TENSOR",
        "ORIENTATION_NORMALIZED_BINARY_INTENSITY_PAIRS",
        "APPEARANCE_DERIVED_ONLY",
        "is_world_registration_proof",
    ],
    "pair": [
        "HAMMING_128",
        "BEST_LE_0_8_SECOND",
        "AFFINE_2D_APPEARANCE_HYPOTHESIS",
        "ROBUST_AFFINE",
        "same_world_structure_proven",
        "registration_promoted",
    ],
    "graph": [
        "source_sha256",
        "FRONTSIDE_PAIR_GEOMETRY_CANDIDATE",
        "source_evidence_merged",
        "camera_or_lens_identity_is_graph_key",
    ],
    "world": [
        "numeric_gauge_has_physical_origin_authority",
        "numeric_gauge_has_camera_center_authority",
        "numeric_gauge_has_panorama_center_authority",
        "RELATIVE_2D_FRONT_SIDE_GRAPH_GAUGE",
    ],
    "campaign": [
        "MULTI_LENS_OVERLAP",
        "RAW_360_SEQUENCE",
        "STOP_MOTION_SEQUENCE",
        "stitched_panorama_counts_as_independent_physical_observation",
    ],
    "atlas": [
        "field_response_axis",
        "cfa_phase_axis",
        "colour_axis",
        "optical_support_axis",
        "dark_noise_axis",
        "temporal_axis",
    ],
    "decomposition": [
        "SEPARATE_WORLD_FIXED_FROM_SENSOR_FIXED_FIELD_BEHAVIOUR",
        "world_fixed_component_estimated",
        "sensor_fixed_component_estimated",
    ],
    "optics": [
        "SFR",
        "MTF",
        "PSF",
        "FIELD_CURVATURE",
        "CHROMATIC_DISPLACEMENT",
        "inverse_optics_authorized",
    ],
    "colour": [
        "multi_illuminant_candidate_minimum",
        "held_out_validation_required",
        "three_channel_rgb_equals_spectral_truth",
    ],
    "temporal": [
        "physical_sequence_relation_proven",
        "synthetic_frame_counts_as_physical_evidence",
        "stop_motion_sequence_supported_as_future_relation_type",
    ],
    "uncertainty": [
        "AXIS_SEPARATED_UNCERTAINTY_TRANSPORT_AVAILABLE",
        "uncertainty_axes_may_be_collapsed_to_single_confidence",
        "missing_uncertainty_becomes_zero",
    ],
    "query": [
        "QUERY_CONTRACT_AVAILABLE_NO_PIXEL_SOLVER",
        "VALUE_OR_UNKNOWN",
        "PROVENANCE_ROOTS",
        "source_raster_is_world_resolution_limit",
        "interpolation_may_create_measured_samples",
    ],
    "restoration": [
        "ORIGINAL_MEASURED_SUPPORT",
        "LOSS_COMPENSATION_RECONSTRUCTED",
        "UNRESOLVED_LOSS",
        "loss_compensation_becomes_measured",
    ],
    "foundation": [
        "FreeWorldObservationGraphV01.build",
        "RelativeWorldCoordinateHypothesisV01.build",
        "NaturalSelfCalibrationAtlasV01.build",
        "FreeWorldContinuousQueryContractV01.describe",
        "ConservationRestorationAuthorityRuntimeV01.describe",
    ],
    "frontside": [
        "deterministic_local_feature_geometry_v0_1",
        "DeterministicLocalFeatureGeometryV01.extract",
    ],
    "main": [
        "Export Free World Observation Geometry Foundation v0.1 · JSON",
        "launchFreeWorldFoundationExport",
        "REQUEST_SAVE_FREE_WORLD_FOUNDATION = 4130",
        "graph identity veranderde",
    ],
}

for name, needles in required.items():
    content = files[name]
    for needle in needles:
        assert needle in content, f"{name}: missing {needle}"

joined = "\n".join(files.values()).lower()
for token in [
    "tensorflow",
    "pytorch",
    "onnxruntime",
    "com.google.mlkit",
]:
    assert token not in joined, f"banned runtime/model token: {token}"

for name in [
    "world_registration_promoted",
    "camera_system_response_proven",
    "lens_only_vignetting_proven",
    "calibration_promoted",
    "correction_authorized",
    "deconvolution_authorized",
    "scientific_writeback_allowed",
]:
    assert state["promotion_firewall"][name] is False

assert state["continuous_query"]["pixel_solver_implemented"] is False
assert state["continuous_query"]["source_raster_is_world_resolution_limit"] is False
assert state["relative_world_coordinates"]["camera_holder_is_world_origin"] is False
assert state["relative_world_coordinates"]["panorama_center_is_world_origin"] is False
assert state["campaign"]["stitched_panorama_is_independent_evidence"] is False
assert state["invariants"]["creates_new_evidence"] is False
assert state["invariants"]["scientific_writeback_allowed"] is False
assert state["inherited_capture_usability"]["universal_physical_capture_timer_seconds"] == 5

for phrase in [
    "Implement the safe architecture first; validate promotion later.",
    "A feature is never sensor evidence",
    "The lexicographically first source SHA may be chosen as a numeric gauge only.",
    "Loss compensation never becomes measured.",
]:
    assert phrase.lower() in readme.lower(), f"README missing: {phrase}"

print("Free World Observation Geometry Foundation v0.1 integrity: PASS")

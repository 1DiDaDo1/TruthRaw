#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
JAVA = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui"

files = {
    "feature": (JAVA / "DeterministicLocalFeatureGeometryV01.kt").read_text(),
    "pair": (JAVA / "DeterministicLocalFeaturePairGeometryV01.kt").read_text(),
    "controlled_rotation_constrained": (JAVA / "ControlledRotationConstrainedGeometryV01.kt").read_text(),
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
    "typed_query": (JAVA / "FreeWorldContinuousQueryApiV01.kt").read_text(),
    "view": (JAVA / "ViewAppearanceStateV01.kt").read_text(),
    "calibration_record": (JAVA / "CalibrationObservationRecordV01.kt").read_text(),
    "firewall": (JAVA / "ResearchPromotionFirewallV01.kt").read_text(),
    "tracks": (JAVA / "FreeWorldFeatureTrackHypothesesV01.kt").read_text(),
    "cycles": (JAVA / "ObservationGraphCycleConsistencyV01.kt").read_text(),
    "track_projection": (JAVA / "RelativeWorldFeatureTrackProjectionV01.kt").read_text(),
    "geometry_readiness": (JAVA / "GeometryValidationReadinessV01.kt").read_text(),
    "query_ledger": (JAVA / "FreeWorldQuerySupportLedgerV01.kt").read_text(),
    "anchor_resolver": (JAVA / "SourceLatticeExactAnchorResolverV01.kt").read_text(),
    "calibration_validator": (JAVA / "CalibrationObservationRecordValidatorV01.kt").read_text(),
    "world_source_bridge": (JAVA / "FreeWorldSourceLatticeBridgeContractV01.kt").read_text(),
    "fail_closed_solver": (JAVA / "FailClosedFreeWorldContinuousQuerySolverV01.kt").read_text(),
    "capability_matrix": (JAVA / "FreeWorldCapabilityMatrixV01.kt").read_text(),
    "components": (JAVA / "FreeWorldObservationComponentsV01.kt").read_text(),
    "component_world": (JAVA / "ComponentRelativeWorldCoordinateHypothesesV01.kt").read_text(),
    "pair_model_bank": (JAVA / "DeterministicPairGeometryModelBankV01.kt").read_text(),
    "pair_model_bank_set": (JAVA / "PairGeometryModelBankSetV01.kt").read_text(),
    "validation_campaign": (JAVA / "BundledPhysicalValidationCampaignV01.kt").read_text(),
    "gate_registry": (JAVA / "ScientificPromotionGateRegistryV01.kt").read_text(),
    "lineage": (JAVA / "FreeWorldEvidenceLineageManifestV01.kt").read_text(),
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
        "rotation_support_keypoints",
        "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_DIAGNOSTIC_ONLY",
        "primary_pair_geometry_replaced",
    ],
    "pair": [
        "HAMMING_128",
        "BEST_LE_0_8_SECOND",
        "AFFINE_2D_APPEARANCE_HYPOTHESIS",
        "ROBUST_AFFINE",
        "same_world_structure_proven",
        "registration_promoted",
    ],
    "controlled_rotation_constrained": [
        "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_AUDIT_SET_AVAILABLE",
        "EXPLICIT_CONTROLLED_ROTATION_RELATION_RECORD",
        "nominal_relation_used_as_geometry_constraint",
        "unconstrained_affine_may_override_nominal_relation",
        "shear_allowed",
        "anisotropic_scale_allowed",
        "reflection_allowed",
        "projective_terms_allowed",
        "registration_adjusted_field_solver_executed",
        "world_registration_promoted",
        "field_response_calibration_promoted",
        "correction_authorized",
        "scientific_writeback_allowed",
        "rotation_support_keypoints_preferred",
        "rotation_support_changes_primary_pair_geometry",
        "ROTATION_SUPPORT_KEYPOINTS",
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
        "CalibrationObservationRecordV01.describeContract",
        "ViewAppearanceStateV01.describe",
        "ResearchPromotionFirewallV01.audit",
        "ControlledRotationConstrainedGeometryV01.evaluate",
        "FreeWorldFeatureTrackHypothesesV01.build",
        "ObservationGraphCycleConsistencyV01.evaluate",
        "RelativeWorldFeatureTrackProjectionV01.build",
        "GeometryValidationReadinessV01.describe",
        "FreeWorldQuerySupportLedgerV01.build",
        "FreeWorldSourceLatticeBridgeContractV01.describe",
        "FreeWorldCapabilityMatrixV01.describe",
        "FreeWorldObservationComponentsV01.build",
        "ComponentRelativeWorldCoordinateHypothesesV01.build",
        "PairGeometryModelBankSetV01.build",
        "FreeWorldEvidenceLineageManifestV01.build",
        "BundledPhysicalValidationCampaignV01.describe",
        "ScientificPromotionGateRegistryV01.describe",
        "FailClosedFreeWorldContinuousQuerySolverV01",
        "ConservationRestorationAuthorityRuntimeV01.describe",
    ],
    "typed_query": [
        "FreeWorldContinuousQueryRequestV01",
        "FreeWorldContinuousQueryResultV01",
        "FreeWorldContinuousQuerySolverV01",
        "FreeWorldAuthorityV01",
    ],
    "view": [
        "VIEW_APPEARANCE_BOUNDARY_AVAILABLE",
        "pupil_or_adaptation_model_may_change_scientific_master",
        "halation_or_grain_synthesis_is_measured_evidence",
    ],
    "calibration_record": [
        "CALIBRATION_OBSERVATION_RECORD_CONTRACT_AVAILABLE",
        "SOURCE_SHA256_ROOTS",
        "camera_model_name_is_key",
        "required_for_raw_intake",
    ],
    "firewall": [
        "RESEARCH_PROMOTION_FIREWALL_PASS",
        "RESEARCH_PROMOTION_FIREWALL_BLOCK",
        "FORBIDDEN_RESEARCH_PROMOTION_TRUE",
        "export_safe_under_current_research_contract",
        "registration_promoted",
        "world_point_estimate_promoted",
        "automatic_colour_correction_applied",
        "temporal_fusion_applied",
        "restoration_applied",
    ],
    "tracks": [
        "FEATURE_TRACK_HYPOTHESES_AVAILABLE",
        "APPEARANCE_DERIVED_TRACK_HYPOTHESIS_ONLY",
        "same_physical_world_point_proven",
        "track_conflict_may_be_silently_discarded",
    ],
    "cycles": [
        "DESCRIPTIVE_CYCLE_CONSISTENCY_AVAILABLE",
        "DIRECT_A_TO_C_VS_A_TO_B_TO_C",
        "automatic_consistency_threshold_used",
        "cycle_consistency_is_same_world_proof",
    ],
    "track_projection": [
        "RELATIVE_WORLD_TRACK_PROJECTIONS_AVAILABLE",
        "graph_gauge_rms_dispersion",
        "same_physical_world_point_proven",
        "world_point_estimate_promoted",
    ],
    "geometry_readiness": [
        "GEOMETRY_VALIDATION_MACHINERY_AVAILABLE",
        "held_out_observation_validation_required",
        "world_vs_sensor_field_separation_validation_required",
    ],
    "query_ledger": [
        "QUERY_SUPPORT_LEDGER_AVAILABLE",
        "unsupported_query_must_return_unknown",
        "world_to_source_registration_promoted",
    ],
    "anchor_resolver": [
        "SourceLatticeExactAnchorResolverV01",
        "UNANCHORED_LATTICE_POSITION_UNKNOWN",
        "MEASURED_ANCHOR_PROVIDER_BINDING_MISMATCH",
        "interpolationPerformed = false",
    ],
    "calibration_validator": [
        "CALIBRATION_OBSERVATION_RECORD_VALID",
        "SOURCE_SHA256_ROOTS_REQUIRED",
        "FORBIDDEN_IDENTITY_KEY_",
        "record_validation_promotes_calibration",
    ],
    "world_source_bridge": [
        "BRIDGE_CONTRACT_AVAILABLE_NO_ADMITTED_WORLD_TO_SOURCE_MAP",
        "VALIDATED_EXPLICIT_WORLD_TO_SOURCE_RELATION_WITH_UNCERTAINTY",
        "appearance_pair_geometry_is_sufficient",
        "world_to_source_bridge_admitted",
    ],
    "fail_closed_solver": [
        "FailClosedFreeWorldContinuousQuerySolverV01",
        "UNKNOWN_NO_ADMITTED_WORLD_TO_SOURCE_BRIDGE",
        "UNKNOWN_NO_ADMITTED_CONTINUOUS_PIXEL_SOLVER",
        "FreeWorldAuthorityV01.UNKNOWN",
    ],
    "capability_matrix": [
        "CAPABILITY_MATRIX_AVAILABLE",
        "implemented_does_not_mean_validated",
        "MEASURED_ONLY_AT_EXACT_ADMITTED_ANCHORS",
        "TYPED_ABI_AND_FAIL_CLOSED_RUNTIME_IMPLEMENTED",
    ],
    "components": [
        "OBSERVATION_COMPONENTS_AVAILABLE",
        "selection_set_equals_one_world",
        "component_is_same_physical_scene_proven",
        "ISOLATED_OBSERVATION",
    ],
    "component_world": [
        "COMPONENT_RELATIVE_WORLD_HYPOTHESES_AVAILABLE",
        "cross_component_transform_exists",
        "disconnected_components_may_share_numeric_gauge",
        "component_connectivity_is_same_world_proof",
    ],
    "pair_model_bank": [
        "PAIR_GEOMETRY_MODEL_BANK_AVAILABLE",
        "TRANSLATION_2D",
        "SIMILARITY_2D",
        "AFFINE_2D",
        "HOMOGRAPHY_2D_PROJECTIVE",
        "automatic_model_winner_used",
        "lowest_residual_model_is_physical_truth",
        "pure_3d_rotation_model_implemented",
        "scientific_model_promoted",
    ],
    "pair_model_bank_set": [
        "PAIR_GEOMETRY_MODEL_BANK_SET_AVAILABLE",
        "models_may_be_ranked_as_physical_truth",
        "future_validation_must_be_held_out",
    ],
    "validation_campaign": [
        "VALIDATION_CAMPAIGN_CONTRACT_AVAILABLE",
        "NATURAL_OVERLAP_SAME_ROUTE",
        "CROSS_OPTICAL_ROUTE_OVERLAP",
        "RAW_360_SEQUENCE",
        "COLOUR_MULTI_ILLUMINANT",
        "normal_user_calibration_required",
    ],
    "gate_registry": [
        "PROMOTION_GATE_REGISTRY_AVAILABLE",
        "WORLD_TO_SOURCE_BRIDGE",
        "CONTINUOUS_FREE_WORLD_SOLVER",
        "SCIENTIFIC_CORRECTION_OR_WRITEBACK",
        "automatic_promotion_allowed",
    ],
    "lineage": [
        "DERIVED_LINEAGE_MANIFEST_AVAILABLE",
        "source_sha256_roots",
        "manifest_identity_is_physical_evidence_root",
        "derived_objects_may_merge_source_authority",
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

assert state["continuous_query"]["typed_kotlin_abi_implemented"] is True
assert state["continuous_query"]["pixel_solver_implemented"] is False
assert state["continuous_query"]["source_lattice_exact_anchor_resolver_implemented"] is True
assert state["continuous_query"]["unanchored_lattice_position_returns_unknown"] is True
assert state["continuous_query"]["world_to_source_bridge_admitted"] is False
assert state["continuous_query"]["fail_closed_world_runtime_implemented"] is True
assert state["continuous_query"]["unadmitted_world_query_returns_unknown"] is True
assert state["capability_matrix"]["implemented"] is True
assert state["capability_matrix"]["separates_implementation_from_scientific_promotion"] is True
assert state["observation_component_policy"]["component_partition_implemented"] is True
assert state["observation_component_policy"]["connected_component_equals_same_world_proof"] is False
assert state["observation_component_policy"]["selection_set_equals_one_world"] is False
assert state["observation_component_policy"]["cross_component_transform_exists"] is False
assert state["relative_world_coordinates"]["component_aware_multi_gauge_implemented"] is True
assert state["pair_geometry"]["model_bank_implemented"] is True
assert state["pair_geometry"]["automatic_model_winner_used"] is False
assert state["pair_geometry"]["lowest_residual_model_is_physical_truth"] is False
assert state["pair_geometry"]["pure_3d_rotation_model_implemented"] is False
assert state["geometry_consistency"]["multi_observation_feature_tracks_implemented"] is True
assert state["geometry_consistency"]["automatic_consistency_threshold_used"] is False
assert state["geometry_consistency"]["same_world_structure_proven"] is False
assert state["calibration_observation_record"]["validator_implemented"] is True
assert state["calibration_observation_record"]["validation_promotes_calibration"] is False
assert state["validation_campaign_contract"]["implemented"] is True
assert state["validation_campaign_contract"]["normal_user_calibration_required"] is False
assert state["scientific_gate_registry"]["automatic_promotion_allowed"] is False
assert state["evidence_lineage_manifest"]["derived_manifest_is_evidence_root"] is False
assert state["evidence_lineage_manifest"]["derived_graph_is_evidence_root"] is False
assert state["build_memory_fix"]["scientific_runtime_behavior_changed"] is False
assert state["research_promotion_firewall"]["implemented"] is True
assert state["calibration_observation_record"]["required_for_normal_intake"] is False
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
    "The lexicographically first source SHA may be chosen as a",
    "Loss compensation never becomes measured.",
    "Any position between source anchors returns UNKNOWN.",
    "Physical truth is intentionally",
    "Implemented does not mean validated.",
    "No phone test is required merely to preserve these ideas",
    "Selected files are no longer even implicitly treated as one connected world.",
    "No transform is invented between disconnected components.",
    "The same robust appearance-derived inlier correspondences are now fitted with multiple deterministic geometry families",
    "No candidate is selected as a winner.",
    "The implementation-first objective is now complete in code.",
    "One Free World may relate many sealed observations",
]:
    assert phrase.lower() in readme.lower(), f"README missing: {phrase}"

print("Free World Observation Geometry Foundation v0.1 integrity: PASS")

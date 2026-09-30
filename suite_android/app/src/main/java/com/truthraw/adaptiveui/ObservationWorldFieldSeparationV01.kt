package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Observation-World Field Separation v0.1.
 *
 * This is the identity-independent coordinate/authority foundation for later
 * natural self-calibration. It does not yet perform world registration.
 *
 * The purpose is to prevent sensor-space, world/scene-space and view/output
 * space from being conflated while D.RAW develops a deterministic classical
 * matching path.
 */
object ObservationWorldFieldSeparationV01 {
    const val SCHEMA = "D.RAW/ObservationWorldFieldSeparation/0.1"

    fun evaluate(profiles: List<JSONObject>): JSONObject {
        val byRoot = linkedMapOf<String, JSONObject>()
        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isNotBlank()) {
                byRoot.putIfAbsent(sha, profile)
            }
        }

        val observations = JSONArray()
        var frontsideReady = 0
        var measuredFieldReady = 0
        var naturalGeometryReady = 0

        for ((sha, profile) in byRoot) {
            val scene = profile.optJSONObject("scene_analysis") ?: JSONObject()
            val geometry =
                scene.optJSONObject("geometry_readiness") ?: JSONObject()
            val field =
                profile.optJSONObject("observation_optical_field_chart")
                    ?: JSONObject()
            val signal =
                field.optJSONObject("measured_composite_field_signal")
                    ?: JSONObject()

            val frontsideAvailable =
                scene.optString("status") ==
                    "FRONTSIDE_STRUCTURAL_INSPECTION_AVAILABLE"
            val fieldAvailable =
                field.optString("status") == "FIELD_CHART_AVAILABLE" &&
                    signal.optString("status") ==
                    "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            val geometryCandidate =
                geometry.optBoolean(
                    "natural_feature_geometry_candidate",
                    false,
                )

            if (frontsideAvailable) frontsideReady++
            if (fieldAvailable) measuredFieldReady++
            if (geometryCandidate) naturalGeometryReady++

            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put(
                        "source_class",
                        profile.optString(
                            "scientific_source_class",
                            "UNKNOWN",
                        ),
                    )
                    .put("frontside_available", frontsideAvailable)
                    .put(
                        "frontside_authority",
                        if (frontsideAvailable) {
                            "APPEARANCE_DERIVED_ONLY"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put(
                        "natural_feature_geometry_candidate",
                        geometryCandidate,
                    )
                    .put(
                        "measured_sensor_field_available",
                        fieldAvailable,
                    )
                    .put(
                        "sensor_field_authority",
                        if (fieldAvailable) {
                            "MEASURED_SOURCE_SAMPLES_SCENE_LENS_SENSOR_COMPOSITE"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put(
                        "structural_feature_signature_sha256",
                        scene.optString(
                            "structural_feature_signature_sha256",
                            "",
                        ).ifBlank { JSONObject.NULL },
                    ),
            )
        }

        val enoughForFutureNaturalRegistration =
            naturalGeometryReady >= 2 &&
                frontsideReady >= 2

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (byRoot.size >= 2) {
                    "COORDINATE_AUTHORITY_SEPARATION_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("observation_count", byRoot.size)
            .put("observations", observations)
            .put(
                "coordinate_spaces",
                JSONObject()
                    .put(
                        "source_sensor_space",
                        JSONObject()
                            .put(
                                "role",
                                "SEALED_MEASUREMENT_GEOMETRY_AND_SENSOR_RELATIVE_FIELD",
                            )
                            .put(
                                "coordinates",
                                JSONArray()
                                    .put("SOURCE_XY")
                                    .put("RHO")
                                    .put("AZIMUTH")
                                    .put("RADIAL_BASIS")
                                    .put("TANGENTIAL_BASIS"),
                            )
                            .put(
                                "measured_field_observation_count",
                                measuredFieldReady,
                            )
                            .put(
                                "camera_identity_required",
                                false,
                            )
                            .put(
                                "lens_identity_required",
                                false,
                            )
                            .put(
                                "world_scene_content_removed",
                                false,
                            ),
                    )
                    .put(
                        "world_scene_space",
                        JSONObject()
                            .put(
                                "role",
                                "RELATIVE_WORLD_STRUCTURE_RELATION_BETWEEN_OBSERVATIONS",
                            )
                            .put(
                                "registration_status",
                                "UNREGISTERED_V0_1",
                            )
                            .put(
                                "frontside_geometry_candidate_count",
                                naturalGeometryReady,
                            )
                            .put(
                                "future_natural_registration_prerequisite_met",
                                enoughForFutureNaturalRegistration,
                            )
                            .put(
                                "automatic_pair_matching_implemented",
                                false,
                            )
                            .put(
                                "same_world_structure_proven",
                                false,
                            )
                            .put(
                                "absolute_metric_scale_proven",
                                false,
                            )
                            .put(
                                "camera_holder_is_world_origin",
                                false,
                            )
                            .put(
                                "panorama_center_is_calibration_origin",
                                false,
                            )
                            .put(
                                "user_identity_or_position_required",
                                false,
                            ),
                    )
                    .put(
                        "view_output_space",
                        JSONObject()
                            .put(
                                "role",
                                "DERIVED_VIEW_PROJECTION_AND_APPEARANCE",
                            )
                            .put(
                                "may_define_sensor_calibration",
                                false,
                            )
                            .put(
                                "may_create_measured_evidence",
                                false,
                            )
                            .put(
                                "stitched_panorama_is_source_evidence",
                                false,
                            ),
                    ),
            )
            .put(
                "natural_self_calibration",
                JSONObject()
                    .put(
                        "target",
                        "SEPARATE_WORLD_FIXED_FROM_SENSOR_FIXED_FIELD_BEHAVIOUR",
                    )
                    .put(
                        "normal_user_calibration_required",
                        false,
                    )
                    .put(
                        "flat_field_required_for_normal_use",
                        false,
                    )
                    .put(
                        "camera_or_lens_name_used_as_key",
                        false,
                    )
                    .put(
                        "natural_overlapping_observations_allowed",
                        true,
                    )
                    .put(
                        "360_observation_sequence_allowed",
                        true,
                    )
                    .put(
                        "single_stitched_360_image_sufficient",
                        false,
                    )
                    .put(
                        "original_individual_observations_required_for_measured_authority",
                        true,
                    )
                    .put(
                        "automatic_world_registration_available",
                        false,
                    )
                    .put(
                        "world_fixed_component_estimated",
                        false,
                    )
                    .put(
                        "sensor_fixed_component_estimated",
                        false,
                    ),
            )
            .put(
                "controlled_validation",
                JSONObject()
                    .put(
                        "purpose",
                        "METHOD_VALIDATION_ONLY_NOT_USER_REQUIREMENT",
                    )
                    .put(
                        "flat_field_rotation_experiment_optional",
                        true,
                    )
                    .put(
                        "preferred_orientation_degrees",
                        JSONArray().put(0).put(90).put(180).put(270),
                    )
                    .put(
                        "result_may_be_hardcoded_by_device_name",
                        false,
                    )
                    .put(
                        "result_may_be_promoted_without_cross_source_validation",
                        false,
                    ),
            )
            .put(
                "registration_contract",
                JSONObject()
                    .put(
                        "future_algorithm_family",
                        "DETERMINISTIC_CLASSICAL_LOCAL_FEATURE_GEOMETRY",
                    )
                    .put(
                        "ai_ml_neural_generative_allowed",
                        false,
                    )
                    .put(
                        "structural_signature_hash_is_registration_proof",
                        false,
                    )
                    .put(
                        "dominant_edge_orientation_is_registration_proof",
                        false,
                    )
                    .put(
                        "visual_similarity_is_relation_proof",
                        false,
                    )
                    .put(
                        "pair_geometry_must_be_explicit_and_inspectable",
                        true,
                    )
                    .put(
                        "outlier_rejection_must_be_deterministic",
                        true,
                    )
                    .put(
                        "registration_uncertainty_required",
                        true,
                    ),
            )
            .put(
                "promotion_boundary",
                JSONObject()
                    .put(
                        "camera_system_response_proven",
                        false,
                    )
                    .put(
                        "lens_only_vignetting_proven",
                        false,
                    )
                    .put(
                        "scene_illumination_separated",
                        false,
                    )
                    .put(
                        "sensor_angular_response_separated",
                        false,
                    )
                    .put(
                        "calibration_promoted",
                        false,
                    )
                    .put(
                        "correction_authorized",
                        false,
                    )
                    .put(
                        "scientific_writeback_allowed",
                        false,
                    ),
            )
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("new_measured_samples_created", false)
            .put("creates_new_evidence", false)
            .put("uses_ai_or_learned_model", false)
            .put("scientific_writeback_allowed", false)
    }
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Executable scaffold for future world-fixed versus sensor-fixed separation.
 *
 * It computes readiness only. No field component is estimated and no
 * calibration/correction can be promoted from this module.
 */
object WorldSensorFieldDecompositionScaffoldV01 {
    const val SCHEMA = "D.RAW/WorldSensorFieldDecompositionScaffold/0.1"

    fun describe(
        profiles: List<JSONObject>,
        graph: JSONObject,
    ): JSONObject {
        val measuredRoots = linkedSetOf<String>()
        for (profile in profiles) {
            val field =
                profile.optJSONObject(
                    "observation_optical_field_chart",
                ) ?: continue
            val signal =
                field.optJSONObject(
                    "measured_composite_field_signal",
                ) ?: continue
            if (
                field.optString("status") == "FIELD_CHART_AVAILABLE" &&
                signal.optString("status") ==
                "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            ) {
                profile.optString("source_sha256")
                    .takeIf { it.isNotBlank() }
                    ?.let(measuredRoots::add)
            }
        }

        val edges =
            graph.optJSONArray("edges") ?: JSONArray()
        var geometryCandidates = 0
        var geometryWithTwoMeasuredFields = 0
        val readiness = JSONArray()
        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val left =
                edge.optString("left_source_sha256")
            val right =
                edge.optString("right_source_sha256")
            val pair =
                edge.optJSONObject("pair_geometry") ?: JSONObject()
            val candidate =
                pair.optString("status") ==
                    "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
            if (candidate) geometryCandidates++
            val bothMeasured =
                left in measuredRoots &&
                    right in measuredRoots
            if (candidate && bothMeasured) {
                geometryWithTwoMeasuredFields++
            }
            readiness.put(
                JSONObject()
                    .put("left_source_sha256", left)
                    .put("right_source_sha256", right)
                    .put(
                        "appearance_geometry_candidate_available",
                        candidate,
                    )
                    .put(
                        "both_measured_sensor_fields_available",
                        bothMeasured,
                    )
                    .put(
                        "eligible_for_future_controlled_decomposition_validation",
                        candidate && bothMeasured,
                    )
                    .put(
                        "decomposition_estimated",
                        false,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "DECOMPOSITION_SCAFFOLD_AVAILABLE",
            )
            .put(
                "target",
                "SEPARATE_WORLD_FIXED_FROM_SENSOR_FIXED_FIELD_BEHAVIOUR",
            )
            .put(
                "controlled_rotation_candidate_solver",
                "FieldResponseRotationSeparationCandidateV01",
            )
            .put(
                "measured_sensor_field_observation_count",
                measuredRoots.size,
            )
            .put(
                "appearance_geometry_candidate_edge_count",
                geometryCandidates,
            )
            .put(
                "candidate_edge_with_two_measured_fields_count",
                geometryWithTwoMeasuredFields,
            )
            .put("pair_readiness", readiness)
            .put(
                "future_model",
                JSONObject()
                    .put(
                        "observation_log_signal_concept",
                        "WORLD_SCENE_COMPONENT_PLUS_SENSOR_FIXED_COMPONENT_PLUS_UNRESOLVED_TERMS",
                    )
                    .put(
                        "world_component_coordinate_system",
                        "RELATIVE_WORLD_SCENE_SPACE",
                    )
                    .put(
                        "sensor_component_coordinate_system",
                        "SOURCE_SENSOR_SPACE",
                    )
                    .put(
                        "appearance_view_coordinate_system_may_define_either",
                        false,
                    )
                    .put(
                        "requires_validated_inter_observation_geometry",
                        true,
                    )
                    .put(
                        "requires_multiple_sensor_positions_for_same_world_support",
                        true,
                    )
                    .put(
                        "uncertainty_must_remain_axis_separated",
                        true,
                    ),
            )
            .put("world_fixed_component_estimated", false)
            .put("sensor_fixed_component_estimated", false)
            .put("scene_illumination_separated", false)
            .put("sensor_angular_response_separated", false)
            .put("lens_only_vignetting_proven", false)
            .put("camera_system_response_proven", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("source_sample_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

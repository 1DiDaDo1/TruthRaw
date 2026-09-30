package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Future world-space residual decomposition contract.
 *
 * The same physical structure must first be validated across independent
 * observations before world-fixed, sensor-fixed and temporal residual classes
 * can be estimated.
 */
object WorldSpaceNoiseSeparationV01 {
    const val SCHEMA = "D.RAW/WorldSpaceNoiseSeparation/0.1"

    fun describe(
        decomposition: JSONObject,
        temporal: JSONObject,
        geometry: JSONObject,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "WORLD_SPACE_NOISE_SEPARATION_CONTRACT_AVAILABLE")
            .put(
                "candidate_classes",
                JSONArray()
                    .put("WORLD_FIXED_RADIOMETRIC_STRUCTURE")
                    .put("SENSOR_FIXED_PATTERN")
                    .put("TEMPORAL_RANDOM_RESIDUAL")
                    .put("VIEW_DEPENDENT_REFLECTION_OR_SPECULAR")
                    .put("MOTION_OR_OCCLUSION")
                    .put("OPTICAL_FIELD_DEPENDENT")
                    .put("UNKNOWN_RESIDUAL"),
            )
            .put(
                "readiness",
                JSONObject()
                    .put(
                        "world_sensor_component_estimated",
                        decomposition.optBoolean("world_fixed_component_estimated", false) ||
                            decomposition.optBoolean("sensor_fixed_component_estimated", false),
                    )
                    .put(
                        "physical_sequence_relation_proven",
                        temporal.optBoolean("physical_sequence_order_proven", false),
                    )
                    .put(
                        "depth_or_visibility_estimated",
                        geometry.optBoolean("parallax_depth_estimated", false) ||
                            geometry.optBoolean("visibility_graph_estimated", false),
                    ),
            )
            .put(
                "candidate_solver",
                "WorldSpaceResidualCandidateSolverV01",
            )
            .put("world_fixed_signal_estimated", false)
            .put("sensor_fixed_pattern_estimated", false)
            .put("temporal_random_residual_estimated", false)
            .put("view_dependent_component_estimated", false)
            .put("motion_occlusion_component_estimated", false)
            .put("unknown_residual_may_be_declared_noise", false)
            .put("world_space_denoise_applied", false)
            .put("multi_observation_reconstruction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only relation scaffold for future multi-observation residual analysis.
 *
 * It does not align samples or compute residuals. It only records which sealed
 * observations could participate once world/source and radiometric relations
 * are independently validated.
 */
object MultiObservationResidualRelationV01 {
    const val SCHEMA = "D.RAW/MultiObservationResidualRelation/0.1"

    fun build(
        profiles: List<JSONObject>,
        graph: JSONObject,
        temporalFootprint: JSONObject,
    ): JSONObject {
        val observations = JSONArray()
        val seen = linkedSetOf<String>()
        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue
            val context =
                profile.optJSONObject("physical_observation_noise_context_v0_1")
                    ?: JSONObject()
            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put(
                        "physical_noise_context_status",
                        context.optString("status", "UNKNOWN"),
                    )
                    .put(
                        "measured_payload_summary_available",
                        context
                            .optJSONObject("measured_payload_summary")
                            ?.optBoolean("available", false)
                            ?: false,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "MULTI_OBSERVATION_RESIDUAL_RELATION_SCAFFOLD_AVAILABLE")
            .put("observation_count", observations.length())
            .put("observations", observations)
            .put(
                "relation_requirements",
                JSONArray()
                    .put("VALIDATED_WORLD_TO_SOURCE_RELATION")
                    .put("RADIOMETRIC_RELATION_OR_EXPLICIT_NORMALIZATION_AUTHORITY")
                    .put("SOURCE_BOUND_TEMPORAL_RELATION")
                    .put("VISIBILITY_OCCLUSION_HANDLING")
                    .put("AXIS_SEPARATED_UNCERTAINTY")
                    .put("HELD_OUT_VALIDATION"),
            )
            .put(
                "current_relation_context",
                JSONObject()
                    .put(
                        "appearance_graph_schema",
                        graph.optString("schema", "UNKNOWN"),
                    )
                    .put("appearance_graph_is_world_registration", false)
                    .put(
                        "temporal_footprint_schema",
                        temporalFootprint.optString("schema", "UNKNOWN"),
                    )
                    .put(
                        "physical_sequence_relation_proven",
                        temporalFootprint.optBoolean("physical_sequence_order_proven", false),
                    ),
            )
            .put("residuals_computed", false)
            .put("world_fixed_signal_estimated", false)
            .put("sensor_fixed_pattern_estimated", false)
            .put("temporal_random_residual_estimated", false)
            .put("view_dependent_component_estimated", false)
            .put("motion_occlusion_component_estimated", false)
            .put("unknown_residual_preserved", true)
            .put("noise_reduction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
}

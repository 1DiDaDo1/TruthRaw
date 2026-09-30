package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Keeps uncertainty dimensions separate instead of collapsing them into one
 * confidence score.
 */
object FreeWorldUncertaintyTransportV01 {
    const val SCHEMA = "D.RAW/FreeWorldUncertaintyTransport/0.1"

    fun describe(
        graph: JSONObject,
        atlas: JSONObject,
    ): JSONObject {
        val geometryEdges = JSONArray()
        val edges = graph.optJSONArray("edges") ?: JSONArray()
        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val pair =
                edge.optJSONObject("pair_geometry") ?: JSONObject()
            val uncertainty =
                pair.optJSONObject("uncertainty") ?: JSONObject()
            geometryEdges.put(
                JSONObject()
                    .put(
                        "left_source_sha256",
                        edge.optString("left_source_sha256"),
                    )
                    .put(
                        "right_source_sha256",
                        edge.optString("right_source_sha256"),
                    )
                    .put(
                        "pair_geometry_status",
                        pair.optString("status", "UNKNOWN"),
                    )
                    .put(
                        "geometry_rms_normalized",
                        uncertainty.opt(
                            "rms_normalized",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "geometry_p95_normalized",
                        uncertainty.opt(
                            "p95_normalized",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "absolute_world_scale_uncertainty",
                        uncertainty.opt(
                            "absolute_world_scale_uncertainty",
                        ) ?: "UNKNOWN",
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "AXIS_SEPARATED_UNCERTAINTY_TRANSPORT_AVAILABLE",
            )
            .put(
                "axes",
                JSONArray()
                    .put("SOURCE_MEASUREMENT")
                    .put("RADIOMETRIC_RESPONSE")
                    .put("NOISE_COMPONENT")
                    .put("NOISE_COVARIANCE_OR_PSD")
                    .put("RECONSTRUCTION")
                    .put("PAIR_GEOMETRY")
                    .put("GEOMETRY_DEPTH_VISIBILITY")
                    .put("WORLD_RELATION")
                    .put("FIELD_RESPONSE")
                    .put("COLOUR")
                    .put("OPTICAL_SUPPORT")
                    .put("TEMPORAL")
                    .put("RESTORATION")
                    .put("LIGHT_TRANSPORT")
                    .put("APPEARANCE"),
            )
            .put("geometry_edges", geometryEdges)
            .put(
                "atlas_axis_status",
                JSONObject()
                    .put(
                        "field_response",
                        atlas.optJSONObject("field_response_axis")
                            ?: JSONObject(),
                    )
                    .put(
                        "cfa_phase",
                        atlas.optJSONObject("cfa_phase_axis")
                            ?: JSONObject(),
                    )
                    .put(
                        "colour",
                        atlas.optJSONObject("colour_axis")
                            ?: JSONObject(),
                    )
                    .put(
                        "optical_support",
                        atlas.optJSONObject("optical_support_axis")
                            ?: JSONObject(),
                    )
                    .put(
                        "dark_noise",
                        atlas.optJSONObject("dark_noise_axis")
                            ?: JSONObject(),
                    )
                    .put(
                        "temporal",
                        atlas.optJSONObject("temporal_axis")
                            ?: JSONObject(),
                    ),
            )
            .put(
                "transport_law",
                JSONObject()
                    .put(
                        "uncertainty_axes_may_be_collapsed_to_single_confidence",
                        false,
                    )
                    .put(
                        "missing_uncertainty_becomes_zero",
                        false,
                    )
                    .put(
                        "unknown_axis_is_valid_state",
                        true,
                    )
                    .put(
                        "appearance_confidence_may_upgrade_measurement_authority",
                        false,
                    )
                    .put(
                        "high_precision_numeric_storage_implies_low_uncertainty",
                        false,
                    )
                    .put(
                        "unknown_noise_covariance_may_be_assumed_zero",
                        false,
                    )
                    .put(
                        "geometry_confidence_may_upgrade_radiometric_authority",
                        false,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

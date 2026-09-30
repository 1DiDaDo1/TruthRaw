package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Applies the deterministic pair-geometry model bank to every candidate edge
 * in the current observation graph.
 */
object PairGeometryModelBankSetV01 {
    const val SCHEMA = "D.RAW/PairGeometryModelBankSet/0.1"

    fun build(graph: JSONObject): JSONObject {
        val edges = graph.optJSONArray("edges") ?: JSONArray()
        val reports = JSONArray()
        var availableCount = 0

        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val pair =
                edge.optJSONObject("pair_geometry") ?: continue
            val report =
                DeterministicPairGeometryModelBankV01.evaluate(pair)
            if (
                report.optString("status") ==
                "PAIR_GEOMETRY_MODEL_BANK_AVAILABLE"
            ) {
                availableCount++
            }
            reports.put(
                JSONObject()
                    .put(
                        "left_source_sha256",
                        edge.optString("left_source_sha256"),
                    )
                    .put(
                        "right_source_sha256",
                        edge.optString("right_source_sha256"),
                    )
                    .put("model_bank", report),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "PAIR_GEOMETRY_MODEL_BANK_SET_AVAILABLE",
            )
            .put("edge_report_count", reports.length())
            .put(
                "available_model_bank_count",
                availableCount,
            )
            .put("edges", reports)
            .put(
                "selection_policy",
                JSONObject()
                    .put(
                        "automatic_model_winner_used",
                        false,
                    )
                    .put(
                        "models_may_be_ranked_as_physical_truth",
                        false,
                    )
                    .put(
                        "future_validation_must_be_held_out",
                        true,
                    ),
            )
            .put("world_registration_promoted", false)
            .put("calibration_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Builds the exact candidate set that a future world-vs-sensor field
 * decomposition experiment may consume.
 *
 * It selects nothing by camera/lens identity and performs no decomposition.
 */
object FieldResponseSeparationCandidateSetV01 {
    const val SCHEMA =
        "D.RAW/FieldResponseSeparationCandidateSet/0.1"

    fun build(
        profiles: List<JSONObject>,
        graph: JSONObject,
        modelBankSet: JSONObject,
    ): JSONObject {
        val measuredRoots = linkedSetOf<String>()
        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            val field =
                profile.optJSONObject(
                    "observation_optical_field_chart",
                ) ?: continue
            val signal =
                field.optJSONObject(
                    "measured_composite_field_signal",
                ) ?: continue
            if (
                sha.isNotBlank() &&
                field.optString("status") ==
                    "FIELD_CHART_AVAILABLE" &&
                signal.optString("status") ==
                    "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            ) {
                measuredRoots += sha
            }
        }

        val modelBanks =
            linkedMapOf<Pair<String, String>, JSONObject>()
        val bankEdges =
            modelBankSet.optJSONArray("edges")
                ?: JSONArray()
        for (i in 0 until bankEdges.length()) {
            val item = bankEdges.optJSONObject(i) ?: continue
            val a = item.optString("left_source_sha256")
            val b = item.optString("right_source_sha256")
            if (a.isBlank() || b.isBlank()) continue
            modelBanks[a to b] =
                item.optJSONObject("model_bank")
                    ?: JSONObject()
        }

        val candidates = JSONArray()
        val edges = graph.optJSONArray("edges") ?: JSONArray()
        var eligibleCount = 0

        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val left = edge.optString("left_source_sha256")
            val right = edge.optString("right_source_sha256")
            val pair =
                edge.optJSONObject("pair_geometry")
                    ?: JSONObject()
            val pairCandidate =
                pair.optString("status") ==
                    "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
            val bothMeasured =
                left in measuredRoots &&
                    right in measuredRoots
            val bank =
                modelBanks[left to right]
                    ?: modelBanks[right to left]
                    ?: JSONObject()
            val bankAvailable =
                bank.optString("status") ==
                    "PAIR_GEOMETRY_MODEL_BANK_AVAILABLE"

            val eligible =
                pairCandidate &&
                    bothMeasured &&
                    bankAvailable
            if (eligible) eligibleCount++

            candidates.put(
                JSONObject()
                    .put("left_source_sha256", left)
                    .put("right_source_sha256", right)
                    .put(
                        "appearance_pair_geometry_candidate",
                        pairCandidate,
                    )
                    .put(
                        "both_measured_field_charts_available",
                        bothMeasured,
                    )
                    .put(
                        "geometry_model_bank_available",
                        bankAvailable,
                    )
                    .put(
                        "eligible_for_future_field_separation_experiment",
                        eligible,
                    )
                    .put(
                        "field_separation_performed",
                        false,
                    )
                    .put(
                        "camera_or_lens_identity_used",
                        false,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "FIELD_SEPARATION_CANDIDATE_SET_AVAILABLE",
            )
            .put(
                "measured_field_observation_count",
                measuredRoots.size,
            )
            .put("pair_candidate_count", candidates.length())
            .put(
                "eligible_future_experiment_pair_count",
                eligibleCount,
            )
            .put("pairs", candidates)
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "world_fixed_component_estimated",
                        false,
                    )
                    .put(
                        "sensor_fixed_component_estimated",
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
                        "lens_only_vignetting_proven",
                        false,
                    )
                    .put(
                        "camera_system_response_proven",
                        false,
                    )
                    .put("calibration_promoted", false)
                    .put("correction_authorized", false),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Dark Chroma Stability v0.6.
 *
 * v0.6 adds exact sampled N2 structure/censor support geometry to the
 * device-validated v0.5 refinement. It is a measurement layer only:
 * no distance threshold is interpreted as permission and no existing
 * v0.4/v0.5 protection is reduced.
 */
object DarkChromaStabilityV06Audit {
    fun analyze(
        sourceSha256: String,
        v03: JSONObject?,
        v05: JSONObject?,
        supportDistance: JSONObject?,
    ): JSONObject {
        if (
            v03 == null ||
            v03.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.3" ||
            v03.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "V0_3_BINDING_MISMATCH")
        }
        if (
            v05 == null ||
            v05.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.5" ||
            v05.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "V0_5_BINDING_MISMATCH")
        }

        val globalStateV03 =
            v03.optString("global_information_state", "UNKNOWN")
        val globallyDarkUninformative =
            globalStateV03.startsWith("DARK_UNINFORMATIVE")

        val distanceStatus =
            supportDistance?.optString("status", "UNKNOWN") ?: "UNKNOWN"
        val distanceAvailable =
            supportDistance != null &&
                distanceStatus == "AUDIT_ONLY_DISTANCE_BINDING_AVAILABLE" &&
                supportDistance.optString("source_sha256") == sourceSha256 &&
                supportDistance.optBoolean("distance_binding_available", false) &&
                supportDistance.optBoolean(
                    "exact_sample_coordinates_recorded",
                    false,
                ) &&
                !supportDistance.optBoolean(
                    "unsampled_pixels_inferred",
                    true,
                ) &&
                !supportDistance.optBoolean(
                    "scalar_probability_created",
                    true,
                ) &&
                !supportDistance.optBoolean("can_reduce_protection", true) &&
                !supportDistance.optBoolean("can_enable_correction", true)

        val distanceSkipped =
            distanceStatus == "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE"

        val v05Global =
            v05.optJSONObject("global") ?: JSONObject()
        val distanceGlobal =
            supportDistance?.optJSONObject("global") ?: JSONObject()
        val queries =
            supportDistance?.optJSONArray("queries") ?: JSONArray()

        val visibleCandidates =
            v05Global.optLong("visible_candidate_tiles", 0L)
        val queryCount =
            distanceGlobal.optLong("query_count", 0L)
        val structureInside =
            distanceGlobal.optLong(
                "structure_inside_rect_candidates",
                0L,
            )
        val zeroByRadius =
            distanceGlobal.optJSONArray(
                "center_zero_structure_candidates_r8_r16_r32_r64",
            ) ?: JSONArray()

        val outQueries = JSONArray()
        for (i in 0 until queries.length()) {
            val q = queries.optJSONObject(i) ?: continue
            outQueries.put(
                JSONObject()
                    .put("id", q.optInt("id"))
                    .put("frontside_x", q.optInt("frontside_x"))
                    .put("frontside_y", q.optInt("frontside_y"))
                    .put("source_rect", q.optJSONArray("source_rect"))
                    .put(
                        "center_structure_fraction",
                        q.optJSONArray("center_structure_fraction"),
                    )
                    .put(
                        "rect_margin_structure_fraction",
                        q.optJSONArray(
                            "rect_margin_structure_fraction",
                        ),
                    )
                    .put(
                        "nearest_structure_from_center",
                        q.opt("nearest_structure_from_center"),
                    )
                    .put(
                        "nearest_structure_to_rect",
                        q.opt("nearest_structure_to_rect"),
                    )
                    .put(
                        "nearest_censored_from_center",
                        q.opt("nearest_censored_from_center"),
                    )
                    .put(
                        "nearest_censored_to_rect",
                        q.opt("nearest_censored_to_rect"),
                    )
                    .put(
                        "nearest_censor_boundary_from_center",
                        q.opt(
                            "nearest_censor_boundary_from_center",
                        ),
                    )
                    .put(
                        "nearest_censor_boundary_to_rect",
                        q.opt("nearest_censor_boundary_to_rect"),
                    )
                    .put(
                        "v0_6_state",
                        when {
                            globallyDarkUninformative ->
                                "BLOCKED_DARK_UNINFORMATIVE"
                            !distanceAvailable ->
                                "SAMPLE_SUPPORT_DISTANCE_UNAVAILABLE"
                            else ->
                                "SAMPLE_SUPPORT_DISTANCE_MEASURED_AUDIT_ONLY"
                        },
                    )
                    .put("can_reduce_protection", false)
                    .put("can_enable_correction", false)
                    .put("chroma_correction_supported", false)
                    .put("candidate_applied", false),
            )
        }

        return JSONObject()
            .put(
                "schema",
                "D.RAW/Frontside/DarkChromaStability/0.6",
            )
            .put(
                "status",
                when {
                    distanceAvailable ->
                        "AUDIT_ONLY_SAMPLE_SUPPORT_DISTANCE_AVAILABLE"
                    distanceSkipped ->
                        "AUDIT_ONLY_SAMPLE_SUPPORT_DISTANCE_SKIPPED"
                    else ->
                        "AUDIT_ONLY_SAMPLE_SUPPORT_DISTANCE_FAIL_CLOSED"
                },
            )
            .put("source_sha256", sourceSha256)
            .put(
                "authority",
                "APPEARANCE_DERIVED_PLUS_EXACT_SAMPLED_N2_SUPPORT_GEOMETRY_DIAGNOSTIC_ONLY",
            )
            .put("v0_3_global_information_state", globalStateV03)
            .put(
                "global_dark_uninformative_preserved",
                globallyDarkUninformative,
            )
            .put("distance_binding_available", distanceAvailable)
            .put("distance_binding_skipped", distanceSkipped)
            .put(
                "distance_skip_reason",
                if (distanceSkipped) {
                    supportDistance?.optString("reason", "") ?: ""
                } else {
                    ""
                },
            )
            .put(
                "support_point_stream_sha256",
                supportDistance?.optString(
                    "support_point_stream_sha256",
                    "",
                ) ?: "",
            )
            .put(
                "sidecar_json_sha256",
                supportDistance?.optString(
                    "sidecar_json_sha256",
                    "",
                ) ?: "",
            )
            .put(
                "global",
                JSONObject()
                    .put(
                        "visible_candidate_tiles",
                        visibleCandidates,
                    )
                    .put(
                        "distance_bound_visible_candidate_tiles",
                        queryCount,
                    )
                    .put(
                        "structure_inside_rect_candidates",
                        structureInside,
                    )
                    .put(
                        "center_zero_structure_candidates_r8_r16_r32_r64",
                        zeroByRadius,
                    )
                    .put(
                        "nearest_center_structure_distance_px",
                        distanceGlobal.optJSONObject(
                            "nearest_center_structure_distance_px",
                        ) ?: JSONObject(),
                    )
                    .put(
                        "nearest_rect_structure_distance_px",
                        distanceGlobal.optJSONObject(
                            "nearest_rect_structure_distance_px",
                        ) ?: JSONObject(),
                    )
                    .put(
                        "v0_5_aggregate_parity_verified",
                        supportDistance?.optBoolean(
                            "v0_5_aggregate_parity_verified",
                            false,
                        ) ?: false,
                    )
                    .put(
                        "chroma_correction_supported_tiles",
                        0,
                    ),
            )
            .put(
                "distance_contract",
                JSONObject()
                    .put(
                        "exact_sample_coordinates_recorded",
                        distanceAvailable,
                    )
                    .put("sample_grid_evidence_only", true)
                    .put("unsampled_pixels_inferred", false)
                    .put("distance_metrics_are_probability", false)
                    .put("distance_threshold_admitted", false)
                    .put("distance_can_reduce_protection", false)
                    .put("distance_can_enable_correction", false)
                    .put(
                        "dark_uninformative_can_be_overridden",
                        false,
                    )
                    .put("chroma_correction_supported", false)
                    .put("private_ab_delta_allowed", false)
                    .put(
                        "next_required_gate",
                        "DEVICE_VALIDATE_EXACT_SAMPLE_SUPPORT_DISTANCE_BEFORE_ANY_DISTANCE_THRESHOLD_RESEARCH",
                    ),
            )
            .put("queries", outQueries)
            .put(
                "single_observation_contract",
                JSONObject()
                    .put("source_observation_count", 1)
                    .put("selected_source_sha256", sourceSha256)
                    .put("other_physical_lenses_used", false)
                    .put("temporal_frames_used", false)
                    .put("burst_used", false)
                    .put("multi_observation_fusion_allowed", false),
            )
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("replacement_colour_estimated", false)
            .put("pixel_value_replacement_proposed", false)
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put(
                "schema",
                "D.RAW/Frontside/DarkChromaStability/0.6",
            )
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "UNKNOWN")
            .put("distance_binding_available", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Multi-observation campaign envelope. It supports ordinary overlap, multi-lens
 * capture and future 360 sequences without making the photographer or panorama
 * center a world origin.
 */
object MultiObservationCampaignV01 {
    const val SCHEMA = "D.RAW/MultiObservationCampaign/0.1"

    fun describe(profiles: List<JSONObject>): JSONObject {
        val roots = JSONArray()
        val members = JSONArray()
        val seen = linkedSetOf<String>()

        for ((selectionIndex, profile) in profiles.withIndex()) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue
            roots.put(sha)
            val metadata =
                profile.optJSONObject("source_metadata") ?: JSONObject()
            members.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put("ui_selection_index", selectionIndex)
                    .put(
                        "ui_selection_index_is_capture_order",
                        false,
                    )
                    .put(
                        "capture_time_text_hint",
                        metadata.opt(
                            "capture_time_preferred_text",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "focal_length_metadata_hint_mm",
                        metadata.opt("focal_length_mm")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "focal_length_hint_defines_lens_identity",
                        false,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (seen.size >= 2) {
                    "MULTI_OBSERVATION_CAMPAIGN_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("observation_roots", roots)
            .put("members", members)
            .put(
                "campaign_types_allowed",
                JSONArray()
                    .put("ORDINARY_OVERLAP")
                    .put("MULTI_LENS_OVERLAP")
                    .put("ROTATION_SEQUENCE")
                    .put("RAW_360_SEQUENCE")
                    .put("STOP_MOTION_SEQUENCE")
                    .put("CONTROLLED_CALIBRATION_SEQUENCE"),
            )
            .put(
                "origin_policy",
                JSONObject()
                    .put("camera_holder_is_world_origin", false)
                    .put("tripod_position_is_world_origin", false)
                    .put("panorama_center_is_world_origin", false)
                    .put("first_frame_is_absolute_world_origin", false)
                    .put(
                        "relative_graph_origin_may_be_chosen_for_numeric_convenience",
                        true,
                    )
                    .put(
                        "relative_graph_origin_has_physical_authority",
                        false,
                    ),
            )
            .put(
                "multi_lens_policy",
                JSONObject()
                    .put(
                        "different_optical_routes_may_share_world_structure",
                        true,
                    )
                    .put(
                        "different_optical_routes_may_share_one_field_calibration_automatically",
                        false,
                    )
                    .put(
                        "camera_or_lens_name_required",
                        false,
                    ),
            )
            .put(
                "panorama_policy",
                JSONObject()
                    .put(
                        "stitched_panorama_allowed_as_derived_view",
                        true,
                    )
                    .put(
                        "stitched_panorama_counts_as_independent_physical_observation",
                        false,
                    )
                    .put(
                        "original_sealed_observations_remain_roots",
                        true,
                    ),
            )
            .put("physical_timing_relation_proven", false)
            .put("world_registration_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

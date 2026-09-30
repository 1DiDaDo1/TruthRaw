package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Downstream viewing/appearance boundary.
 *
 * Human-vision, display, cinema/film-like rendering and output-acutance
 * parameters may change presentation but never the Free World scientific state.
 */
object ViewAppearanceStateV01 {
    const val SCHEMA = "D.RAW/ViewAppearanceState/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "VIEW_APPEARANCE_BOUNDARY_AVAILABLE")
            .put(
                "optional_viewing_inputs",
                JSONArray()
                    .put("DISPLAY_PEAK_LUMINANCE")
                    .put("DISPLAY_BLACK_LEVEL")
                    .put("DISPLAY_PIXEL_DENSITY")
                    .put("VIEWING_DISTANCE")
                    .put("AMBIENT_ILLUMINANCE")
                    .put("VIEWING_ADAPTATION")
                    .put("OUTPUT_SIZE")
                    .put("OUTPUT_ACUTANCE"),
            )
            .put(
                "human_vision_policy",
                JSONObject()
                    .put(
                        "pupil_or_adaptation_model_may_change_scientific_master",
                        false,
                    )
                    .put(
                        "visual_angle_acutance_may_change_scientific_master",
                        false,
                    )
                    .put(
                        "viewing_distance_is_sensor_calibration",
                        false,
                    ),
            )
            .put(
                "film_cinema_policy",
                JSONObject()
                    .put(
                        "tone_or_density_curve_is_appearance_only",
                        true,
                    )
                    .put(
                        "halation_or_grain_synthesis_is_measured_evidence",
                        false,
                    )
                    .put(
                        "cinematic_motion_blur_synthesis_is_physical_capture_evidence",
                        false,
                    )
                    .put(
                        "appearance_transform_must_remain_reversible_from_scientific_state",
                        true,
                    ),
            )
            .put(
                "output_policy",
                JSONObject()
                    .put(
                        "display_target_may_change_scene_state",
                        false,
                    )
                    .put(
                        "output_resolution_may_change_scene_authority",
                        false,
                    )
                    .put(
                        "output_acutance_is_optical_frequency_recovery",
                        false,
                    ),
            )
            .put("appearance_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

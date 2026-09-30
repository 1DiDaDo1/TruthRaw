package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Downstream perceptual-noise presentation contract.
 *
 * It may change only VIEW/APPEARANCE. Scientific noise estimates and Scientific
 * Master remain untouched.
 */
object PerceptualNoiseAppearanceV01 {
    const val SCHEMA = "D.RAW/PerceptualNoiseAppearance/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PERCEPTUAL_NOISE_APPEARANCE_CONTRACT_AVAILABLE")
            .put(
                "viewing_dependencies",
                JSONArray()
                    .put("DISPLAY_PEAK_LUMINANCE")
                    .put("DISPLAY_BLACK_LEVEL")
                    .put("VIEWING_DISTANCE")
                    .put("DISPLAY_PIXEL_DENSITY")
                    .put("AMBIENT_ILLUMINANCE")
                    .put("VIEWING_ADAPTATION")
                    .put("OUTPUT_SIZE")
                    .put("SPATIAL_FREQUENCY_SENSITIVITY")
                    .put("LUMINANCE_CHROMA_SENSITIVITY"),
            )
            .put(
                "visual_angle_candidate_runtime",
                "PerceptualNoiseVisibilityCandidateV01",
            )
            .put("scientific_noise_state_may_be_changed", false)
            .put("scientific_master_may_be_changed", false)
            .put("perceptual_noise_visibility_mapping_allowed", true)
            .put("appearance_only_denoise_allowed", true)
            .put("appearance_result_may_be_called_measured", false)
            .put("appearance_result_may_create_new_evidence", false)
            .put("appearance_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

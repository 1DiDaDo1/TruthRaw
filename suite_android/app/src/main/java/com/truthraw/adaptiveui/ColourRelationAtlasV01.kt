package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Executable identity-independent colour-relation contract.
 */
object ColourRelationAtlasV01 {
    const val SCHEMA = "D.RAW/ColourRelationAtlas/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "COLOUR_RELATION_ATLAS_CONTRACT_AVAILABLE")
            .put(
                "authority_axes",
                JSONArray()
                    .put("SOURCE_METADATA_COLOUR")
                    .put("OBSERVATION_VISIBLE_COLOUR")
                    .put("EMPIRICAL_REFERENCE_TARGET")
                    .put("ILLUMINANT_CHARACTERIZATION")
                    .put("SPECTRAL_CALIBRATION"),
            )
            .put(
                "multi_illuminant_candidate_minimum",
                2,
            )
            .put(
                "reference_target_required_for_empirical_promotion",
                true,
            )
            .put(
                "held_out_validation_required",
                true,
            )
            .put(
                "white_balance_equals_illuminant_spectrum",
                false,
            )
            .put(
                "three_channel_rgb_equals_spectral_truth",
                false,
            )
            .put(
                "frontside_visible_colour_is_colorimetric_calibration",
                false,
            )
            .put("empirical_relation_attached", false)
            .put("spectral_calibration_attached", false)
            .put("calibration_promoted", false)
            .put("automatic_colour_correction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

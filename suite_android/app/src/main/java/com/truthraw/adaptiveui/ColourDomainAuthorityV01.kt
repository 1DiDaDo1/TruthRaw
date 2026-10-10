package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Scientific authority boundary for camera-RGB -> tristimulus colour relations.
 *
 * A finite RGB triplet and a numerically valid 3x3 transform do not prove that
 * the observed spectrum lies inside the calibration domain of that transform.
 * This is especially important for narrow-band / unusual emissive spectra.
 */
object ColourDomainAuthorityV01 {
    const val SCHEMA = "D.RAW/ColourDomainAuthority/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "COLOUR_DOMAIN_AUTHORITY_CONTRACT_AVAILABLE")
            .put(
                "authority_axes",
                JSONArray()
                    .put("CAMERA_RGB_MEASUREMENT_SUPPORT")
                    .put("COLOUR_RELATION_CALIBRATION_DOMAIN")
                    .put("COLOUR_TRANSFORM_UNCERTAINTY")
                    .put("SPECTRAL_DOMAIN_SUPPORT")
                    .put("RADIOMETRIC_RELIABILITY")
                    .put("CENSOR_STATE"),
            )
            .put("fixed_3x3_is_universal_spectral_truth", false)
            .put("finite_rgb_implies_known_human_chromaticity", false)
            .put("matrix_numerically_valid_implies_domain_valid", false)
            .put("calibration_distribution_binding_required", true)
            .put("narrowband_emissive_out_of_domain_may_be_unknown", true)
            .put("unknown_spectral_domain_may_be_assumed_in_domain", false)
            .put("missing_colour_covariance_may_be_assumed_zero", false)
            .put("appearance_match_may_promote_colour_authority", false)
            .put("emissive_object_semantics_may_define_spectrum", false)
            .put("automatic_emissive_colour_correction_authorized", false)
            .put("colour_domain_calibration_promoted", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

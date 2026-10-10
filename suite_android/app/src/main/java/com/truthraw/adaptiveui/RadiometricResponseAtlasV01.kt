package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Identity-independent radiometric response foundation.
 *
 * This contract separates source exposure/gain metadata, sensor black/white
 * coding bounds, scientific Zero-Line and an eventual measured OECF/linearity
 * relation. Metadata may describe capture context but cannot by itself prove a
 * radiometric response curve or pre-saturation reliability boundary.
 */
object RadiometricResponseAtlasV01 {
    const val SCHEMA = "D.RAW/RadiometricResponseAtlas/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        var exposureHintCount = 0
        var gainHintCount = 0
        var blackWhiteBoundCount = 0
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue

            val metadata = profile.optJSONObject("source_metadata") ?: JSONObject()
            val raster =
                profile.optJSONObject("primary_raw_raster")
                    ?: profile.optJSONObject("raw_raster")
                    ?: JSONObject()

            val exposure =
                finitePositive(metadata, "exposure_time_seconds")
                    ?: finitePositive(metadata, "exposure_seconds")
            val iso = finitePositive(metadata, "iso")
            val blackPresent =
                raster.has("black_level") && raster.opt("black_level") != JSONObject.NULL
            val whitePresent =
                raster.has("white_level") && raster.opt("white_level") != JSONObject.NULL

            if (exposure != null) exposureHintCount++
            if (iso != null) gainHintCount++
            if (blackPresent && whitePresent) blackWhiteBoundCount++

            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put("exposure_time_seconds", exposure ?: JSONObject.NULL)
                    .put("iso_metadata", iso ?: JSONObject.NULL)
                    .put("black_level_present", blackPresent)
                    .put("white_level_present", whitePresent)
                    .put("capture_context_authority", "SOURCE_METADATA_PROVENANCE_WHEN_PRESENT")
                    .put("radiometric_response_measured", false)
                    .put("pre_saturation_reliability_proven", false),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "RADIOMETRIC_RESPONSE_FOUNDATION_AVAILABLE")
            .put("observation_count", seen.size)
            .put("exposure_metadata_observation_count", exposureHintCount)
            .put("gain_metadata_observation_count", gainHintCount)
            .put("black_white_bound_observation_count", blackWhiteBoundCount)
            .put(
                "response_axes",
                JSONArray()
                    .put("FOCAL_PLANE_EXPOSURE_RELATION")
                    .put("EFFECTIVE_GAIN")
                    .put("BLACK_OFFSET")
                    .put("SATURATION_CENSOR_BOUND")
                    .put("PRE_SATURATION_RELIABILITY")
                    .put("LINEARITY")
                    .put("OECF_OR_EQUIVALENT"),
            )
            .put(
                "separation_law",
                JSONObject()
                    .put("black_level_equals_zero_line", false)
                    .put("metadata_iso_equals_measured_gain", false)
                    .put("exposure_metadata_proves_linearity", false)
                    .put("white_level_proves_scene_clipping_point", false)
                    .put("white_level_proves_pre_saturation_reliability", false)
                    .put("finite_uncensored_code_proves_reliable_linearity", false)
                    .put("zero_line_may_be_derived_from_black_level_alone", false),
            )
            .put(
                "candidate_solver",
                "RadiometricResponseCandidateSolverV01",
            )
            .put(
                "reliability_envelope_candidate",
                RadiometricReliabilityEnvelopeV01.SCHEMA,
            )
            .put("oecf_measurement_attached", false)
            .put("effective_gain_calibrated", false)
            .put("response_linearity_proven", false)
            .put("pre_saturation_reliability_proven", false)
            .put("saturation_model_promoted", false)
            .put("radiometric_calibration_promoted", false)
            .put("automatic_radiometric_correction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun finitePositive(
        obj: JSONObject,
        key: String,
    ): Double? {
        val value = obj.optDouble(key, Double.NaN)
        return value.takeIf { it.isFinite() && it > 0.0 }
    }
}

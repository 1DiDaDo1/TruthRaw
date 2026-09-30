package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Source-bound temporal footprint foundation.
 *
 * Exposure integration may be known from metadata while capture ordering,
 * rolling-shutter readout and motion remain separate unknowns.
 */
object TemporalFootprintV01 {
    const val SCHEMA = "D.RAW/TemporalFootprint/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        var exposureKnown = 0
        var captureTimeHintKnown = 0
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue
            val metadata = profile.optJSONObject("source_metadata") ?: JSONObject()
            val exposure =
                finitePositive(metadata, "exposure_time_seconds")
                    ?: finitePositive(metadata, "exposure_seconds")
            val captureTime =
                metadata.opt("capture_time_preferred_text")
                    ?: JSONObject.NULL

            if (exposure != null) exposureKnown++
            if (captureTime != JSONObject.NULL) captureTimeHintKnown++

            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put("integration_duration_seconds", exposure ?: JSONObject.NULL)
                    .put(
                        "integration_duration_authority",
                        if (exposure != null) {
                            "SOURCE_METADATA_BOUND_CAPTURE_DURATION"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put("capture_time_text_hint", captureTime)
                    .put(
                        "capture_time_hint_authority",
                        if (captureTime != JSONObject.NULL) {
                            "SOURCE_METADATA_BOUND_HINT"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put("readout_start", "UNKNOWN")
                    .put("readout_end", "UNKNOWN")
                    .put("rolling_shutter_row_time", "UNKNOWN")
                    .put("motion_path", "UNKNOWN"),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "TEMPORAL_FOOTPRINT_FOUNDATION_AVAILABLE")
            .put("observation_count", seen.size)
            .put("integration_duration_known_count", exposureKnown)
            .put("capture_time_hint_known_count", captureTimeHintKnown)
            .put(
                "candidate_solver",
                "TemporalSequenceCandidateSolverV01",
            )
            .put("physical_sequence_order_proven", false)
            .put("rolling_shutter_readout_proven", false)
            .put("motion_path_proven", false)
            .put("occlusion_time_relation_proven", false)
            .put("synthetic_intermediate_is_physical_observation", false)
            .put("temporal_fusion_applied", false)
            .put("multi_frame_scientific_fusion_applied", false)
            .put("observations", observations)
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

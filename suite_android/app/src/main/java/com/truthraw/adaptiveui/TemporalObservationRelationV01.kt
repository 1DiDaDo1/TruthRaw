package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Temporal/multiview relation scaffold for physical observation sequences.
 */
object TemporalObservationRelationV01 {
    const val SCHEMA = "D.RAW/TemporalObservationRelation/0.1"

    fun describe(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        var timeHintCount = 0
        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank()) continue
            val metadata =
                profile.optJSONObject("source_metadata") ?: JSONObject()
            val time =
                metadata.opt("capture_time_preferred_text")
                    ?: JSONObject.NULL
            if (time != JSONObject.NULL) timeHintCount++
            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put("capture_time_text_hint", time)
                    .put(
                        "capture_time_hint_authority",
                        if (time != JSONObject.NULL) {
                            "SOURCE_METADATA_BOUND_HINT"
                        } else {
                            "UNKNOWN"
                        },
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "TEMPORAL_RELATION_SCAFFOLD_AVAILABLE",
            )
            .put("observations", observations)
            .put(
                "capture_time_hint_count",
                timeHintCount,
            )
            .put(
                "physical_sequence_relation_proven",
                false,
            )
            .put(
                "rolling_shutter_readout_proven",
                false,
            )
            .put("motion_path_proven", false)
            .put(
                "synthetic_frame_counts_as_physical_evidence",
                false,
            )
            .put(
                "virtual_exposure_counts_as_physical_evidence",
                false,
            )
            .put(
                "stop_motion_sequence_supported_as_future_relation_type",
                true,
            )
            .put(
                "raw_360_sequence_supported_as_future_relation_type",
                true,
            )
            .put("temporal_fusion_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Source-bound physical observation context for future noise decomposition.
 *
 * This object deliberately copies only already-admitted source metadata and
 * measured backside summary observations. It does not classify any observed
 * variation as noise.
 */
object PhysicalObservationNoiseContextV01 {
    const val SCHEMA = "D.RAW/PhysicalObservationNoiseContext/0.1"

    fun describe(
        sourceSha256: String,
        metadata: JSONObject?,
        raster: JSONObject?,
        backsideSignalSupport: JSONObject?,
        opticalFieldChart: JSONObject?,
    ): JSONObject {
        val m = metadata ?: JSONObject()
        val r = raster ?: JSONObject()
        val backside = backsideSignalSupport ?: JSONObject()
        val field = opticalFieldChart ?: JSONObject()

        val measuredBackside =
            backside.optString("status") ==
                "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
        val statistics =
            if (measuredBackside) {
                backside.optJSONObject("statistics") ?: JSONObject()
            } else {
                JSONObject()
            }
        val phases =
            if (measuredBackside) {
                backside.optJSONArray("cfa_phase_summary") ?: JSONArray()
            } else {
                JSONArray()
            }
        val fieldSignal =
            field.optJSONObject("measured_composite_field_signal")
                ?: JSONObject().put("status", "UNKNOWN")
        val sparseGrid =
            backside.optJSONObject("sparse_measured_sample_grid")
                ?: JSONObject().put("status", "UNKNOWN")

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (measuredBackside) {
                    "SOURCE_BOUND_NOISE_CONTEXT_AVAILABLE"
                } else {
                    "SOURCE_BOUND_CONTEXT_WITHOUT_MEASURED_PAYLOAD_SUMMARY"
                },
            )
            .put("source_sha256", sourceSha256)
            .put(
                "capture_context",
                JSONObject()
                    .put(
                        "exposure_time_seconds",
                        finitePositive(m, "exposure_time_seconds")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "iso_metadata",
                        finitePositive(m, "iso") ?: JSONObject.NULL,
                    )
                    .put(
                        "f_number",
                        finitePositive(m, "f_number") ?: JSONObject.NULL,
                    )
                    .put(
                        "capture_context_authority",
                        "SOURCE_METADATA_PROVENANCE_WHEN_PRESENT",
                    ),
            )
            .put(
                "coding_context",
                JSONObject()
                    .put("black_level", r.opt("black_level") ?: JSONObject.NULL)
                    .put("white_level", r.opt("white_level") ?: JSONObject.NULL)
                    .put("bits_per_sample", r.opt("bits_per_sample") ?: JSONObject.NULL)
                    .put("sample_format", r.opt("sample_format") ?: JSONObject.NULL)
                    .put("cfa_pattern", r.opt("cfa_pattern") ?: JSONObject.NULL)
                    .put("black_level_equals_zero_line", false)
                    .put("coding_bounds_are_noise_model", false),
            )
            .put(
                "measured_payload_summary",
                JSONObject()
                    .put("available", measuredBackside)
                    .put(
                        "authority",
                        if (measuredBackside) {
                            "SOURCE_PAYLOAD_MEASURED_WITHIN_SELECTED_SOURCE"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put(
                        "sample_count",
                        if (measuredBackside) {
                            backside.opt("sample_count") ?: JSONObject.NULL
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put("statistics", statistics)
                    .put("cfa_phase_summary", phases)
                    .put(
                        "signal_support_state",
                        backside.optString("signal_support_state", "UNKNOWN"),
                    ),
            )
            .put(
                "sparse_measured_source_grid",
                JSONObject()
                    .put("status", sparseGrid.optString("status", "UNKNOWN"))
                    .put(
                        "sample_count",
                        sparseGrid.opt("sample_count") ?: JSONObject.NULL,
                    )
                    .put(
                        "authority",
                        sparseGrid.optString("authority", "UNKNOWN"),
                    )
                    .put("interpolation_performed", false),
            )
            .put(
                "measured_field_context",
                JSONObject()
                    .put(
                        "status",
                        fieldSignal.optString("status", "UNKNOWN"),
                    )
                    .put(
                        "authority",
                        fieldSignal.optString("authority", "UNKNOWN"),
                    )
                    .put(
                        "sample_count",
                        fieldSignal.opt("sample_count") ?: JSONObject.NULL,
                    ),
            )
            .put(
                "interpretation_boundary",
                JSONObject()
                    .put("observed_stddev_is_temporal_noise", false)
                    .put("observed_chroma_variation_is_noise", false)
                    .put("cfa_phase_difference_is_sensor_calibration", false)
                    .put("field_variation_is_lens_vignetting", false)
                    .put("single_observation_can_separate_fixed_pattern", false)
                    .put("single_observation_can_separate_shot_and_read_noise", false)
                    .put("unknown_residual_is_noise", false),
            )
            .put("noise_component_decomposition_performed", false)
            .put("noise_reduction_applied", false)
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

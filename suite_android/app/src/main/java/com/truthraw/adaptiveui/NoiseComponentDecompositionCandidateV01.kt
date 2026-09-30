package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Candidate decomposition of repeated-observation noise summary points.
 *
 * Expected payload points contain signal_mean and temporal_variance. Optional
 * fixed-pattern summaries remain separate and are never inferred when absent.
 */
object NoiseComponentDecompositionCandidateV01 {
    const val SCHEMA = "D.RAW/NoiseComponentDecompositionCandidate/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val points = ArrayList<JSONObject>()
        val roots = linkedSetOf<String>()
        for (record in records) {
            if (
                record.optString("axis_scope") !=
                "NOISE_COMPONENT_SEPARATION"
            ) {
                continue
            }
            val validation = CalibrationObservationRecordValidatorV01.validate(record)
            if (
                validation.optString("status") !=
                "CALIBRATION_OBSERVATION_RECORD_VALID"
            ) {
                continue
            }
            val rs = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            val arr = payload.optJSONArray("measurement_points") ?: continue
            for (i in 0 until arr.length()) arr.optJSONObject(i)?.let(points::add)
        }

        val train = points.filter { it.optString("role", "TRAIN") == "TRAIN" }
        val held = points.filter { it.optString("role") == "HELD_OUT" }

        val x = ArrayList<Double>()
        val y = ArrayList<Double>()
        for (p in train) {
            val mean = p.optDouble("signal_mean", Double.NaN)
            val variance = p.optDouble("temporal_variance", Double.NaN)
            if (
                mean.isFinite() && mean >= 0.0 &&
                variance.isFinite() && variance >= 0.0
            ) {
                x += mean
                y += variance
            }
        }
        if (x.size < 3) {
            return unavailable("INSUFFICIENT_SIGNAL_VARIANCE_POINTS", roots)
        }

        val fit = ResearchMathV01.linearFit(x, y)
            ?: return unavailable("SHOT_READ_VARIANCE_FIT_FAILED", roots)

        val shotSlope =
            fit.slope.takeIf { it.isFinite() && it >= 0.0 }
        val readVariance =
            fit.intercept.takeIf { it.isFinite() && it >= 0.0 }

        val heldActual = ArrayList<Double>()
        val heldPred = ArrayList<Double>()
        for (p in held) {
            val mean = p.optDouble("signal_mean", Double.NaN)
            val variance = p.optDouble("temporal_variance", Double.NaN)
            if (
                mean.isFinite() && mean >= 0.0 &&
                variance.isFinite() && variance >= 0.0
            ) {
                heldActual += variance
                heldPred += fit.slope * mean + fit.intercept
            }
        }

        fun medianOptional(key: String): Any {
            val values =
                points.mapNotNull {
                    it.optDouble(key, Double.NaN)
                        .takeIf { v -> v.isFinite() && v >= 0.0 }
                }
            return ResearchMathV01.median(values) ?: JSONObject.NULL
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "NOISE_COMPONENT_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("training_point_count", x.size)
            .put("held_out_point_count", heldActual.size)
            .put(
                "temporal_variance_candidate",
                JSONObject()
                    .put("model", "VARIANCE=SLOPE*SIGNAL+INTERCEPT")
                    .put("shot_slope_candidate", shotSlope ?: JSONObject.NULL)
                    .put("read_variance_candidate", readVariance ?: JSONObject.NULL)
                    .put("training_rmse", fit.rmse)
                    .put(
                        "held_out_rmse",
                        ResearchMathV01.rmse(heldActual, heldPred)
                            ?: JSONObject.NULL,
                    ),
            )
            .put(
                "fixed_pattern_summary_candidates",
                JSONObject()
                    .put("spatial_fixed_variance_median", medianOptional("spatial_fixed_variance"))
                    .put("row_variance_median", medianOptional("row_variance"))
                    .put("column_variance_median", medianOptional("column_variance"))
                    .put("cfa_phase_variance_median", medianOptional("cfa_phase_variance")),
            )
            .put("dark_current_component_separated", false)
            .put("dsnu_component_separated", false)
            .put("prnu_component_separated", false)
            .put("row_column_component_separated", false)
            .put("unknown_residual_preserved", true)
            .put("noise_component_decomposition_performed", false)
            .put("noise_component_calibration_promoted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun unavailable(
        reason: String,
        roots: Set<String>,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("noise_component_decomposition_performed", false)
            .put("noise_component_calibration_promoted", false)
            .put("noise_reduction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

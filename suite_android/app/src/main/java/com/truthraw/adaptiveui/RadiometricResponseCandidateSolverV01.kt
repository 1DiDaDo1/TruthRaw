package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Deterministic candidate fit for a relation-based radiometric observation
 * record. It never promotes an OECF, gain or correction.
 */
object RadiometricResponseCandidateSolverV01 {
    const val SCHEMA = "D.RAW/RadiometricResponseCandidate/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val points = ArrayList<JSONObject>()
        val roots = linkedSetOf<String>()
        for (record in records) {
            if (record.optString("axis_scope") != "RADIOMETRIC_RESPONSE") continue
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
            for (i in 0 until arr.length()) {
                arr.optJSONObject(i)?.let(points::add)
            }
        }

        val train = points.filter {
            it.optString("role", "TRAIN") == "TRAIN" &&
                !it.optBoolean("censored", false)
        }
        val held = points.filter {
            it.optString("role") == "HELD_OUT" &&
                !it.optBoolean("censored", false)
        }

        fun xs(list: List<JSONObject>): List<Double> =
            list.mapNotNull {
                it.optDouble("relative_exposure", Double.NaN)
                    .takeIf { v -> v.isFinite() && v >= 0.0 }
            }
        fun ys(list: List<JSONObject>): List<Double> =
            list.mapNotNull {
                it.optDouble("normalized_signal", Double.NaN)
                    .takeIf { v -> v.isFinite() }
            }

        val xTrain = xs(train)
        val yTrain = ys(train)
        if (
            train.size < 3 ||
            xTrain.size != train.size ||
            yTrain.size != train.size
        ) {
            return unavailable("INSUFFICIENT_VALID_TRAINING_POINTS", roots)
        }

        val fit = ResearchMathV01.linearFit(xTrain, yTrain)
            ?: return unavailable("RADIOMETRIC_LINEAR_FIT_FAILED", roots)

        val sorted =
            train.mapNotNull {
                val x = it.optDouble("relative_exposure", Double.NaN)
                val y = it.optDouble("normalized_signal", Double.NaN)
                if (x.isFinite() && y.isFinite()) x to y else null
            }.sortedBy { it.first }
        var monotonic = true
        for (i in 1 until sorted.size) {
            if (sorted[i].second < sorted[i - 1].second) {
                monotonic = false
                break
            }
        }

        val heldActual = ArrayList<Double>()
        val heldPred = ArrayList<Double>()
        for (p in held) {
            val x = p.optDouble("relative_exposure", Double.NaN)
            val y = p.optDouble("normalized_signal", Double.NaN)
            if (!x.isFinite() || !y.isFinite()) continue
            heldActual += y
            heldPred += fit.slope * x + fit.intercept
        }
        val heldRmse =
            ResearchMathV01.rmse(heldActual, heldPred)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "RADIOMETRIC_RESPONSE_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("training_point_count", train.size)
            .put("held_out_point_count", heldActual.size)
            .put(
                "linear_candidate",
                JSONObject()
                    .put("slope", fit.slope)
                    .put("intercept", fit.intercept)
                    .put("training_rmse", fit.rmse)
                    .put("held_out_rmse", heldRmse ?: JSONObject.NULL)
                    .put("monotonic_training_relation", monotonic),
            )
            .put("oecf_piecewise_points", JSONArray(sorted.map {
                JSONArray().put(it.first).put(it.second)
            }))
            .put("response_linearity_proven", false)
            .put("effective_gain_calibrated", false)
            .put("radiometric_calibration_promoted", false)
            .put("automatic_radiometric_correction_applied", false)
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
            .put("radiometric_calibration_promoted", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Evidence-bound pre-saturation / near-censor reliability foundation.
 *
 * This object deliberately does NOT use a percentage of WhiteLevel as a
 * reliability threshold. It only summarizes explicitly admitted radiometric
 * relation records and keeps observed uncensored support separate from a
 * censor-transition bracket. Neither an uncensored code value nor a finite
 * decoded value proves linearity or chromatic reliability.
 */
object RadiometricReliabilityEnvelopeV01 {
    const val SCHEMA = "D.RAW/RadiometricReliabilityEnvelope/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val roots = linkedSetOf<String>()
        val uncensored = ArrayList<Double>()
        val censored = ArrayList<Double>()
        var admittedRecordCount = 0
        var validPointCount = 0

        for (record in records) {
            if (record.optString("axis_scope") != "RADIOMETRIC_RESPONSE") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "RADIOMETRIC_RESPONSE",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            admittedRecordCount++

            val recordRoots = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until recordRoots.length()) {
                recordRoots.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }

            val points =
                record.optJSONObject("axis_payload")
                    ?.optJSONArray("measurement_points") ?: continue
            for (i in 0 until points.length()) {
                val point = points.optJSONObject(i) ?: continue
                val exposure = point.optDouble("relative_exposure", Double.NaN)
                val signal = point.optDouble("normalized_signal", Double.NaN)
                if (!exposure.isFinite() || exposure < 0.0 || !signal.isFinite()) continue
                validPointCount++
                if (point.optBoolean("censored", false)) {
                    censored += exposure
                } else {
                    uncensored += exposure
                }
            }
        }

        if (validPointCount == 0 || uncensored.isEmpty()) {
            return unavailable(
                reason = "NO_ADMITTED_UNCENSORED_RADIOMETRIC_SUPPORT",
                roots = roots,
                admittedRecordCount = admittedRecordCount,
                validPointCount = validPointCount,
            )
        }

        val lastUncensored = uncensored.maxOrNull()!!
        val firstCensoredAbove = censored.filter { it > lastUncensored }.minOrNull()
        val transitionBracket =
            if (firstCensoredAbove != null) {
                JSONObject()
                    .put("lower_observed_uncensored_relative_exposure", lastUncensored)
                    .put("upper_observed_censored_relative_exposure", firstCensoredAbove)
                    .put("status", "OBSERVED_CENSOR_TRANSITION_BRACKET")
            } else {
                JSONObject()
                    .put("lower_observed_uncensored_relative_exposure", lastUncensored)
                    .put("upper_observed_censored_relative_exposure", JSONObject.NULL)
                    .put("status", "UPPER_CENSOR_TRANSITION_UNKNOWN")
            }

        return base(roots)
            .put("status", "RADIOMETRIC_RELIABILITY_EVIDENCE_AVAILABLE")
            .put("admitted_record_count", admittedRecordCount)
            .put("valid_measurement_point_count", validPointCount)
            .put("uncensored_point_count", uncensored.size)
            .put("censored_point_count", censored.size)
            .put("observed_uncensored_relative_exposure_max", lastUncensored)
            .put("censor_transition", transitionBracket)
            .put("response_linearity_proven", false)
            .put("pre_saturation_reliability_boundary_proven", false)
            .put("chromatic_reliability_near_saturation_proven", false)
            .put("automatic_reliability_weighting_applied", false)
    }

    private fun unavailable(
        reason: String,
        roots: Set<String>,
        admittedRecordCount: Int,
        validPointCount: Int,
    ): JSONObject =
        base(roots)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("admitted_record_count", admittedRecordCount)
            .put("valid_measurement_point_count", validPointCount)
            .put("response_linearity_proven", false)
            .put("pre_saturation_reliability_boundary_proven", false)
            .put("chromatic_reliability_near_saturation_proven", false)
            .put("automatic_reliability_weighting_applied", false)

    private fun base(roots: Set<String>): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("white_level_percentage_threshold_used", false)
            .put("white_level_alone_proves_reliability", false)
            .put("finite_value_alone_proves_reliability", false)
            .put("uncensored_value_alone_proves_linearity", false)
            .put("observed_support_is_promoted_calibration", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

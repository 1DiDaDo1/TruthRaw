package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Relation-based noise power spectrum measurement candidate.
 *
 * The spectrum is supplied by an explicit controlled measurement record. This
 * object aggregates it deterministically but does not infer an NPS from a
 * single ordinary photograph.
 */
object NoiseSpectrumMeasurementCandidateV01 {
    const val SCHEMA = "D.RAW/NoiseSpectrumMeasurementCandidate/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val roots = linkedSetOf<String>()
        val grouped =
            linkedMapOf<String, MutableMap<Double, MutableList<Double>>>()
        var rawBinCount = 0

        for (record in records) {
            val axis = record.optString("axis_scope")
            if (
                axis != "NOISE_COMPONENT_SEPARATION" &&
                axis != "DARK_NOISE_OFFSET"
            ) {
                continue
            }
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = axis,
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            val rs = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            val bins = payload.optJSONArray("nps_bins") ?: continue
            for (i in 0 until bins.length()) {
                val b = bins.optJSONObject(i) ?: continue
                val frequency =
                    b.optDouble("frequency_cycles_per_pixel", Double.NaN)
                val power = b.optDouble("power", Double.NaN)
                val channel = b.optString("channel_or_phase", "UNSPECIFIED")
                if (
                    !frequency.isFinite() || frequency < 0.0 ||
                    !power.isFinite() || power < 0.0
                ) {
                    continue
                }
                grouped
                    .getOrPut(channel) { linkedMapOf() }
                    .getOrPut(frequency) { ArrayList() }
                    .add(power)
                rawBinCount++
            }
        }

        if (rawBinCount == 0) {
            return JSONObject()
                .put("schema", SCHEMA)
                .put("status", "UNKNOWN_FAIL_CLOSED")
                .put("reason", "NO_CONTROLLED_NPS_MEASUREMENT_BINS")
                .put("noise_spectrum_promoted", false)
                .put("scientific_writeback_allowed", false)
        }

        val channels = JSONArray()
        for ((channel, byFrequency) in grouped) {
            val curve = JSONArray()
            for ((frequency, powers) in byFrequency.toSortedMap()) {
                val median =
                    ResearchMathV01.median(powers) ?: continue
                curve.put(
                    JSONObject()
                        .put("frequency_cycles_per_pixel", frequency)
                        .put("median_power", median)
                        .put("measurement_count", powers.size),
                )
            }
            channels.put(
                JSONObject()
                    .put("channel_or_phase", channel)
                    .put("curve", curve),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "NOISE_SPECTRUM_MEASUREMENT_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("raw_measurement_bin_count", rawBinCount)
            .put("channels", channels)
            .put("ordinary_single_frame_nps_inferred", false)
            .put("noise_spectrum_promoted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

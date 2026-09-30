package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Candidate temporal ordering/readout description from explicit relation
 * records. Metadata timestamps alone never prove physical sequence membership.
 */
object TemporalSequenceCandidateSolverV01 {
    const val SCHEMA = "D.RAW/TemporalSequenceCandidate/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val observations = ArrayList<JSONObject>()
        val roots = linkedSetOf<String>()
        for (record in records) {
            if (record.optString("axis_scope") != "TEMPORAL_FOOTPRINT") continue
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
            val arr = payload.optJSONArray("observations") ?: continue
            for (i in 0 until arr.length()) arr.optJSONObject(i)?.let(observations::add)
        }

        if (observations.size < 2) {
            return unavailable("AT_LEAST_TWO_TEMPORAL_OBSERVATIONS_REQUIRED", roots)
        }

        val indexed =
            observations.mapNotNull { o ->
                val index = o.optInt("sequence_index", Int.MIN_VALUE)
                val sha = o.optString("source_sha256")
                if (index == Int.MIN_VALUE || sha.isBlank()) null else index to o
            }
        if (indexed.size != observations.size) {
            return unavailable("SEQUENCE_INDEX_OR_SOURCE_ROOT_MISSING", roots)
        }
        if (indexed.map { it.first }.toSet().size != indexed.size) {
            return unavailable("DUPLICATE_SEQUENCE_INDEX", roots)
        }

        val sorted = indexed.sortedBy { it.first }
        val intervals = JSONArray()
        var timestampPairCount = 0
        for (i in 1 until sorted.size) {
            val a = sorted[i - 1].second
            val b = sorted[i].second
            val ta = a.optDouble("timestamp_seconds", Double.NaN)
            val tb = b.optDouble("timestamp_seconds", Double.NaN)
            if (ta.isFinite() && tb.isFinite()) {
                val dt = tb - ta
                intervals.put(dt)
                timestampPairCount++
            }
        }

        val allReadoutKnown =
            sorted.all {
                val v = it.second.optDouble("readout_duration_seconds", Double.NaN)
                v.isFinite() && v > 0.0
            }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "TEMPORAL_SEQUENCE_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("observation_count", sorted.size)
            .put("timestamp_interval_count", timestampPairCount)
            .put("timestamp_intervals_seconds", intervals)
            .put("explicit_sequence_index_relation_available", true)
            .put("readout_duration_available_for_all", allReadoutKnown)
            .put("rolling_shutter_candidate_available", allReadoutKnown)
            .put("motion_path_proven", false)
            .put("occlusion_time_relation_proven", false)
            .put("physical_sequence_relation_proven", false)
            .put("temporal_relation_promoted", false)
            .put("temporal_fusion_applied", false)
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
            .put("physical_sequence_relation_proven", false)
            .put("temporal_relation_promoted", false)
            .put("scientific_writeback_allowed", false)
}

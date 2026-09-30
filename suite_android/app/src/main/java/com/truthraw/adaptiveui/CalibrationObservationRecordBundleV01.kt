package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Fail-closed JSON bundle reader for optional relation-based calibration and
 * physical-observation records.
 */
object CalibrationObservationRecordBundleV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationRecordBundle/0.1"

    fun parse(text: String): JSONObject {
        val source = text.trim()
        if (source.isEmpty()) return unavailable("EMPTY_BUNDLE_TEXT")

        val records =
            runCatching {
                when {
                    source.startsWith("[") -> JSONArray(source)
                    source.startsWith("{") -> {
                        val root = JSONObject(source)
                        root.optJSONArray("records")
                            ?: JSONArray().put(root)
                    }
                    else -> null
                }
            }.getOrNull()
                ?: return unavailable("INVALID_JSON_BUNDLE")

        val valid = JSONArray()
        val invalid = JSONArray()
        for (i in 0 until records.length()) {
            val record = records.optJSONObject(i)
            if (record == null) {
                invalid.put(
                    JSONObject()
                        .put("index", i)
                        .put("reason", "RECORD_IS_NOT_OBJECT"),
                )
                continue
            }
            val validation =
                CalibrationObservationRecordValidatorV01.validate(record)
            if (
                validation.optString("status") ==
                "CALIBRATION_OBSERVATION_RECORD_VALID"
            ) {
                valid.put(record)
            } else {
                invalid.put(
                    JSONObject()
                        .put("index", i)
                        .put("validation", validation),
                )
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (valid.length() > 0) {
                    "CALIBRATION_OBSERVATION_RECORD_BUNDLE_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("valid_record_count", valid.length())
            .put("invalid_record_count", invalid.length())
            .put("records", valid)
            .put("invalid_records", invalid)
            .put("invalid_records_silently_accepted", false)
            .put("identity_profile_routing_used", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    fun validRecords(text: String): List<JSONObject> {
        val parsed = parse(text)
        val records = parsed.optJSONArray("records") ?: return emptyList()
        val out = ArrayList<JSONObject>()
        for (i in 0 until records.length()) {
            records.optJSONObject(i)?.let(out::add)
        }
        return out
    }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("valid_record_count", 0)
            .put("records", JSONArray())
            .put("calibration_promoted", false)
            .put("scientific_writeback_allowed", false)
}

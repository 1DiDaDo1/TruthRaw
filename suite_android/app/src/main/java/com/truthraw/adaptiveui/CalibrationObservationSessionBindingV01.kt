package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Binds relation-based records to the observations in the current Free World
 * session. A syntactically valid record from another data set must never drive
 * the current session's numeric candidates.
 */
object CalibrationObservationSessionBindingV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationSessionBinding/0.1"

    data class Result(
        val records: List<JSONObject>,
        val report: JSONObject,
    )

    fun bind(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
    ): Result {
        val activeRoots = linkedSetOf<String>()
        val aliasToProcessingRoot = linkedMapOf<String, String>()

        for (profile in profiles) {
            val processing =
                profile.optString("source_sha256").trim().lowercase()
            if (processing.matches(Regex("[0-9a-f]{64}"))) {
                activeRoots += processing
                aliasToProcessingRoot[processing] = processing
            }
            val upstream =
                profile.optString("upstream_sealed_source_sha256")
                    .trim()
                    .lowercase()
            if (
                processing.matches(Regex("[0-9a-f]{64}")) &&
                upstream.matches(Regex("[0-9a-f]{64}"))
            ) {
                activeRoots += upstream
                aliasToProcessingRoot[upstream] = processing
            }
        }

        val accepted = ArrayList<JSONObject>()
        val rejected = JSONArray()

        for (raw in records) {
            val record =
                CalibrationObservationRecordIdentityV01.normalize(raw)
            val roots =
                record.optJSONArray("source_sha256_roots") ?: JSONArray()
            val missing = JSONArray()
            val processingAliases = JSONArray()

            for (i in 0 until roots.length()) {
                val root = roots.optString(i).trim().lowercase()
                if (root !in activeRoots) {
                    missing.put(root)
                } else {
                    processingAliases.put(
                        aliasToProcessingRoot[root] ?: root,
                    )
                }
            }

            if (roots.length() > 0 && missing.length() == 0) {
                record.put(
                    "session_processing_source_sha256_roots",
                    processingAliases,
                )
                record.put(
                    "session_binding_status",
                    "BOUND_TO_ACTIVE_OBSERVATION_SET",
                )
                accepted += record
            } else {
                rejected.put(
                    JSONObject()
                        .put(
                            "record_identity_sha256",
                            CalibrationObservationRecordIdentityV01.identity(
                                record,
                            ),
                        )
                        .put(
                            "reason",
                            if (roots.length() == 0) {
                                "NO_SOURCE_ROOTS"
                            } else {
                                "SOURCE_ROOT_NOT_IN_ACTIVE_SESSION"
                            },
                        )
                        .put("missing_source_sha256_roots", missing),
                )
            }
        }

        val active = JSONArray()
        activeRoots.sorted().forEach(active::put)

        return Result(
            records = accepted,
            report =
                JSONObject()
                    .put("schema", SCHEMA)
                    .put(
                        "status",
                        if (rejected.length() == 0) {
                            "ALL_RECORDS_BOUND_TO_ACTIVE_SESSION"
                        } else {
                            "SESSION_BINDING_FILTERED_RECORDS"
                        },
                    )
                    .put("active_source_sha256_roots", active)
                    .put("input_record_count", records.size)
                    .put("bound_record_count", accepted.size)
                    .put("rejected_record_count", rejected.length())
                    .put("rejected_records", rejected)
                    .put("cross_session_record_use_allowed", false)
                    .put("binding_promotes_calibration", false)
                    .put("scientific_writeback_allowed", false),
        )
    }
}

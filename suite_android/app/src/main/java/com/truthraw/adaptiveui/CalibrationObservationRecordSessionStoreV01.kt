package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * Private, session-scoped persistence for potentially large relation records.
 *
 * Only the opaque session id is stored in Activity state. Records stay in the
 * app cache and are never promoted to evidence by persistence.
 */
object CalibrationObservationRecordSessionStoreV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationRecordSessionStore/0.1"

    fun save(
        cacheDir: File,
        sessionId: String,
        records: List<JSONObject>,
    ): Boolean =
        runCatching {
            require(sessionId.matches(Regex("[0-9a-fA-F-]{16,64}")))
            val dir =
                File(
                    cacheDir,
                    "calibration_observation_record_sessions",
                ).apply { mkdirs() }
            val target = File(dir, "$sessionId.json")
            val temp = File(dir, "$sessionId.tmp")
            val array = JSONArray()
            records.forEach { array.put(JSONObject(it.toString())) }
            temp.writeText(
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("records", array)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false)
                    .toString(),
            )
            if (target.exists()) target.delete()
            check(temp.renameTo(target))
        }.isSuccess

    fun load(
        cacheDir: File,
        sessionId: String,
    ): List<JSONObject> =
        runCatching {
            val file =
                File(
                    File(
                        cacheDir,
                        "calibration_observation_record_sessions",
                    ),
                    "$sessionId.json",
                )
            if (!file.isFile) return emptyList()
            val root = JSONObject(file.readText())
            if (root.optString("schema") != SCHEMA) return emptyList()
            val array = root.optJSONArray("records") ?: return emptyList()
            buildList {
                for (i in 0 until array.length()) {
                    val raw = array.optJSONObject(i) ?: continue
                    val normalized =
                        CalibrationObservationRecordIdentityV01.normalize(raw)
                    val validation =
                        CalibrationObservationRecordValidatorV01
                            .validate(normalized)
                    if (
                        validation.optString("status") ==
                        "CALIBRATION_OBSERVATION_RECORD_VALID"
                    ) {
                        add(normalized)
                    }
                }
            }
        }.getOrElse { emptyList() }

    fun clear(
        cacheDir: File,
        sessionId: String,
    ) {
        runCatching {
            File(
                File(
                    cacheDir,
                    "calibration_observation_record_sessions",
                ),
                "$sessionId.json",
            ).delete()
        }
        runCatching {
            File(
                File(
                    cacheDir,
                    "calibration_observation_record_sessions",
                ),
                "$sessionId.tmp",
            ).delete()
        }
    }
}

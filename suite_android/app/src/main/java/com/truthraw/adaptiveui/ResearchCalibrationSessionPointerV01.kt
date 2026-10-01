package com.truthraw.adaptiveui

import java.io.File
import org.json.JSONObject

/**
 * Durable pointer to the current research relation-record session.
 *
 * The relation records themselves remain validated Calibration Observation
 * Records. This pointer only lets a recreated Research workbench recover the
 * same private record session after process loss or an external document
 * picker. It creates no evidence and carries no promotion authority.
 */
object ResearchCalibrationSessionPointerV01 {
    const val SCHEMA =
        "D.RAW/ResearchCalibrationSessionPointer/0.1"
    private const val FILE_NAME =
        "draw_research_calibration_session_pointer_v0_1.json"

    fun save(
        filesDir: File,
        sessionId: String,
    ) {
        if (
            !sessionId.matches(
                Regex(
                    "[0-9a-fA-F-]{16,64}",
                ),
            )
        ) {
            return
        }
        val target =
            File(
                filesDir,
                FILE_NAME,
            )
        val temp =
            File(
                filesDir,
                FILE_NAME +
                    ".tmp",
            )
        val root =
            JSONObject()
                .put("schema", SCHEMA)
                .put(
                    "session_id",
                    sessionId,
                )
                .put(
                    "creates_new_evidence",
                    false,
                )
                .put(
                    "scientific_writeback_allowed",
                    false,
                )
        runCatching {
            temp.writeText(
                root.toString(),
                Charsets.UTF_8,
            )
            if (
                target.exists() &&
                !target.delete()
            ) {
                error(
                    "Could not replace research calibration pointer.",
                )
            }
            if (!temp.renameTo(target)) {
                target.writeText(
                    root.toString(),
                    Charsets.UTF_8,
                )
                temp.delete()
            }
        }.onFailure {
            temp.delete()
        }
    }

    fun load(
        filesDir: File,
    ): String? =
        runCatching {
            val file =
                File(
                    filesDir,
                    FILE_NAME,
                )
            if (!file.isFile) {
                return@runCatching null
            }
            val root =
                JSONObject(
                    file.readText(
                        Charsets.UTF_8,
                    ),
                )
            if (
                root.optString("schema") !=
                SCHEMA ||
                root.optBoolean(
                    "creates_new_evidence",
                    true,
                ) ||
                root.optBoolean(
                    "scientific_writeback_allowed",
                    true,
                )
            ) {
                return@runCatching null
            }
            root.optString(
                "session_id",
            ).takeIf {
                it.matches(
                    Regex(
                        "[0-9a-fA-F-]{16,64}",
                    ),
                )
            }
        }.getOrNull()
}

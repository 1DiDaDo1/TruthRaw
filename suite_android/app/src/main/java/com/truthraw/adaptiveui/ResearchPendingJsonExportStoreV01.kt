package com.truthraw.adaptiveui

import java.io.File
import org.json.JSONObject

object ResearchPendingJsonExportStoreV01 {
    const val SCHEMA = "D.RAW/ResearchPendingJsonExportStore/0.1"
    const val FREE_WORLD_FOUNDATION = "FREE_WORLD_FOUNDATION"
    const val FIELD_RESPONSE_REPEATABILITY = "FIELD_RESPONSE_REPEATABILITY"
    const val OBSERVATION_WORLD_FIELD_SEPARATION = "OBSERVATION_WORLD_FIELD_SEPARATION"
    private const val DIR_NAME = "draw_pending_research_json_exports_v0_1"

    fun save(filesDir: File, key: String, reportText: String) {
        val dir = File(filesDir, DIR_NAME).apply { mkdirs() }
        val wrapper = JSONObject()
            .put("schema", SCHEMA)
            .put("key", key)
            .put("report_text", reportText)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
        val target = File(dir, safeName(key) + ".json")
        val temp = File(dir, target.name + ".tmp")
        temp.writeText(wrapper.toString(), Charsets.UTF_8)
        if (target.exists() && !target.delete()) {
            temp.delete()
            error("Could not replace pending research export.")
        }
        if (!temp.renameTo(target)) {
            target.writeText(wrapper.toString(), Charsets.UTF_8)
            temp.delete()
        }
    }

    fun load(filesDir: File, key: String): String? {
        val file = File(File(filesDir, DIR_NAME), safeName(key) + ".json")
        if (!file.isFile) return null
        val wrapper = runCatching {
            JSONObject(file.readText(Charsets.UTF_8))
        }.getOrNull() ?: return null
        if (wrapper.optString("schema") != SCHEMA ||
            wrapper.optString("key") != key ||
            wrapper.optBoolean("creates_new_evidence", true) ||
            wrapper.optBoolean("scientific_writeback_allowed", true)
        ) {
            return null
        }
        return wrapper.optString("report_text").takeIf { it.isNotBlank() }
    }

    fun clear(filesDir: File, key: String) {
        runCatching {
            File(File(filesDir, DIR_NAME), safeName(key) + ".json").delete()
        }
    }

    private fun safeName(value: String): String =
        value.replace(Regex("[^A-Za-z0-9._-]"), "_")
}

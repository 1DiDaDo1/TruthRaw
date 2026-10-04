package com.truthraw.adaptiveui

import java.io.File
import java.security.MessageDigest
import org.json.JSONObject

/**
 * Private cache for deterministic, derived Research profiler stages.
 *
 * Cache hits may skip repeated expensive diagnostics after a process restart,
 * but never replace source hashing, never become evidence and never authorize
 * Scientific Master writeback. Each entry is bound to source SHA, stage id and
 * an explicit input fingerprint so changed inputs fail closed to recomputation.
 */
object ResearchProfileStageCacheV01 {
    const val SCHEMA = "D.RAW/ResearchProfileStageCache/0.1"
    private const val DIR_NAME = "draw_research_profile_stage_cache_v0_1"
    private const val N2_R12_STAGE =
        "N2_LOCAL_SPATIAL_V01_R12_CENTER_EXCLUDED_PROFILE"
    private const val N2_R12_IMPLEMENTATION_EPOCH =
        "CENTER_EXCLUDED_FIXED_TOPOLOGY_V01_SCIENTIFIC_MASTER_EXACT_GAUGE_V03_COLD_VALIDATION_V02"
    private val lock = Any()

    fun load(
        filesDir: File,
        sourceSha256: String,
        stageId: String,
        inputFingerprint: String,
    ): JSONObject? = synchronized(lock) {
        val file = fileFor(filesDir, sourceSha256, stageId)
        if (!file.isFile) return@synchronized null
        val wrapper =
            runCatching {
                JSONObject(file.readText(Charsets.UTF_8))
            }.getOrNull()
                ?: return@synchronized null

        if (
            wrapper.optString("schema") != SCHEMA ||
            wrapper.optString("source_sha256") != sourceSha256 ||
            wrapper.optString("stage_id") != stageId ||
            wrapper.optString("input_fingerprint") != inputFingerprint ||
            wrapper.optBoolean("creates_new_evidence", true) ||
            wrapper.optBoolean("scientific_writeback_allowed", true)
        ) {
            return@synchronized null
        }

        val requiredImplementationEpoch = implementationEpoch(stageId)
        if (
            requiredImplementationEpoch != null &&
            wrapper.optString("implementation_epoch") != requiredImplementationEpoch
        ) {
            return@synchronized null
        }

        wrapper.optJSONObject("result")
            ?.let { JSONObject(it.toString()) }
    }

    fun save(
        filesDir: File,
        sourceSha256: String,
        stageId: String,
        inputFingerprint: String,
        result: JSONObject,
    ) = synchronized(lock) {
        val wrapper =
            JSONObject()
                .put("schema", SCHEMA)
                .put("source_sha256", sourceSha256)
                .put("stage_id", stageId)
                .put("input_fingerprint", inputFingerprint)
                .put("result", result)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)

        implementationEpoch(stageId)?.let {
            wrapper.put("implementation_epoch", it)
        }

        val target = fileFor(filesDir, sourceSha256, stageId)
        target.parentFile?.mkdirs()
        val temp = File(target.parentFile, target.name + ".tmp")
        val text = wrapper.toString()
        temp.writeText(text, Charsets.UTF_8)
        if (target.exists() && !target.delete()) {
            temp.delete()
            return@synchronized
        }
        if (!temp.renameTo(target)) {
            target.writeText(text, Charsets.UTF_8)
            temp.delete()
        }
    }

    fun fingerprint(vararg parts: String?): String {
        val digest = MessageDigest.getInstance("SHA-256")
        for (part in parts) {
            val bytes = (part ?: "<null>").toByteArray(Charsets.UTF_8)
            digest.update(bytes.size.toString().toByteArray(Charsets.UTF_8))
            digest.update(0.toByte())
            digest.update(bytes)
            digest.update(0.toByte())
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun implementationEpoch(stageId: String): String? =
        if (stageId == N2_R12_STAGE) {
            N2_R12_IMPLEMENTATION_EPOCH
        } else {
            null
        }

    private fun fileFor(
        filesDir: File,
        sourceSha256: String,
        stageId: String,
    ): File =
        File(
            File(filesDir, DIR_NAME),
            safe(sourceSha256) + "__" + safe(stageId) + ".json",
        )

    private fun safe(value: String): String =
        value.replace(Regex("[^A-Za-z0-9._-]"), "_")
}

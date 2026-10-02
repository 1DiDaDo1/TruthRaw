package com.truthraw.adaptiveui

import android.app.ActivityManager
import android.content.Context
import android.os.Debug
import android.os.PowerManager
import java.io.File
import java.security.MessageDigest
import org.json.JSONObject

/**
 * Durable orchestration journal for the long multi-RAW Research batch.
 *
 * This journal is diagnostic only. It stores lifecycle/progress/resource facts,
 * never RAW payload bytes and never Scientific Master sample values.
 */
object ResearchBatchJournalV02 {
    const val SCHEMA = "D.RAW/ResearchBatchJournal/0.2"
    private const val DIR_NAME = "draw_research_batch_journal_v0_2"
    private val lock = Any()

    fun begin(
        context: Context,
        operationKey: String,
        jobs: List<RawJob>,
        redelivered: Boolean,
    ) = synchronized(lock) {
        val now = System.currentTimeMillis()
        val root =
            readInternal(context, operationKey)
                ?.takeIf { sameJobSet(it, jobs) }
                ?: JSONObject()
                    .put("schema", SCHEMA)
                    .put("operation_key", operationKey)
                    .put("created_at_wall_ms", now)
                    .put("attempt_count", 0)
                    .put("redelivery_count", 0)
                    .put("jobs", JSONObject())

        root
            .put("updated_at_wall_ms", now)
            .put("attempt_started_at_wall_ms", now)
            .put("terminal", false)
            .put("status", "RUNNING")
            .put(
                "run_mode",
                if (redelivered) {
                    "REDELIVERED_RESUME"
                } else {
                    "FRESH_USER_RUN"
                },
            )
            .put("attempt_count", root.optInt("attempt_count", 0) + 1)
            .put("service_heartbeat_wall_ms", now)
            .put("system", systemSample(context))
            .put(
                "previous_process_exit",
                AndroidExitForensicsV01.latest(context) ?: JSONObject.NULL,
            )

        if (redelivered) {
            root.put(
                "redelivery_count",
                root.optInt("redelivery_count", 0) + 1,
            )
        } else {
            // A new explicit Research run is a fresh measurement attempt.
            // Old terminal/job-stage facts belong to the previous attempt and
            // must not make the UI or export path believe that new work has
            // already completed. Redelivery is the only resume path.
            root.remove("current_job_id")
            root.remove("current_job_index")
            root.remove("current_job_total")
            root.remove("current_stage")
            root.remove("terminal_message")
        }

        val map =
            root.optJSONObject("jobs")
                ?: JSONObject().also { root.put("jobs", it) }

        jobs.forEachIndexed { index, job ->
            val existing =
                map.optJSONObject(job.id)
                    ?: JSONObject()
            existing
                .put("job_id", job.id)
                .put("index", index + 1)
                .put("display_name", job.source.displayName)
                .put("source_uri", job.source.uri.toString())

            if (redelivered) {
                existing.put(
                    "status",
                    existing.optString("status", "PENDING"),
                )
            } else {
                existing
                    .put("status", "PENDING")
                    .put("updated_at_wall_ms", now)
                existing.remove("stage")
                existing.remove("stage_started_at_wall_ms")
                existing.remove("detail")
                existing.remove("failure")
                existing.remove("source_sha256")
                existing.remove("completed_at_wall_ms")
            }
            map.put(job.id, existing)
        }

        write(context, operationKey, root)
    }

    fun stage(
        context: Context,
        operationKey: String,
        job: RawJob,
        index: Int,
        total: Int,
        stage: String,
        detail: String? = null,
    ) = mutate(context, operationKey) { root ->
        val now = System.currentTimeMillis()
        val state =
            jobState(root, job)
        val previousStage =
            state.optString(
                "stage",
                "",
            )
        state
            .put("index", index + 1)
            .put("total", total)
            .put("status", "RUNNING")
            .put("stage", stage)
            .put("updated_at_wall_ms", now)

        if (previousStage != stage) {
            state.put("stage_started_at_wall_ms", now)
        }
        if (!detail.isNullOrBlank()) {
            state.put("detail", detail)
        }

        root
            .put("current_job_id", job.id)
            .put("current_job_index", index + 1)
            .put("current_job_total", total)
            .put("current_stage", stage)
            .put("service_heartbeat_wall_ms", now)
            .put("system", systemSample(context))
    }

    fun heartbeat(
        context: Context,
        operationKey: String,
    ) = mutate(context, operationKey) { root ->
        root
            .put("service_heartbeat_wall_ms", System.currentTimeMillis())
            .put("system", systemSample(context))
    }

    fun completed(
        context: Context,
        operationKey: String,
        job: RawJob,
        sourceSha256: String?,
    ) = mutate(context, operationKey) { root ->
        val now = System.currentTimeMillis()
        val state =
            jobState(root, job)
                .put("status", "COMPLETED")
                .put("stage", "PROFILE_PERSISTED")
                .put("completed_at_wall_ms", now)
                .put("updated_at_wall_ms", now)
        if (!sourceSha256.isNullOrBlank()) {
            state.put("source_sha256", sourceSha256)
        }
        root
            .put("service_heartbeat_wall_ms", now)
            .put("system", systemSample(context))
    }

    fun failed(
        context: Context,
        operationKey: String,
        job: RawJob,
        message: String,
    ) = mutate(context, operationKey) { root ->
        val now = System.currentTimeMillis()
        jobState(root, job)
            .put("status", "FAILED")
            .put("stage", "FAILED")
            .put("failure", message)
            .put("updated_at_wall_ms", now)
        root
            .put("service_heartbeat_wall_ms", now)
            .put("system", systemSample(context))
    }

    fun finish(
        context: Context,
        operationKey: String,
        success: Boolean,
        message: String,
    ) = mutate(context, operationKey) { root ->
        val now = System.currentTimeMillis()
        val attemptStartedAt =
            root.optLong(
                "attempt_started_at_wall_ms",
                now,
            )
        root
            .put("updated_at_wall_ms", now)
            .put("service_heartbeat_wall_ms", now)
            .put("attempt_finished_at_wall_ms", now)
            .put(
                "attempt_elapsed_ms",
                (now - attemptStartedAt).coerceAtLeast(0L),
            )
            .put("terminal", true)
            .put("status", if (success) "SUCCESS" else "ERROR")
            .put("terminal_message", message)
            .put("system", systemSample(context))
    }

    fun read(
        context: Context,
        operationKey: String,
    ): JSONObject? = synchronized(lock) {
        readInternal(context, operationKey)
            ?.let { JSONObject(it.toString()) }
    }

    fun completedJobIds(
        context: Context,
        operationKey: String,
    ): Set<String> =
        jobIdsWithStatus(context, operationKey, "COMPLETED")

    fun failedJobIds(
        context: Context,
        operationKey: String,
    ): Set<String> =
        jobIdsWithStatus(context, operationKey, "FAILED")

    private fun jobIdsWithStatus(
        context: Context,
        operationKey: String,
        status: String,
    ): Set<String> {
        val root = read(context, operationKey) ?: return emptySet()
        val jobs = root.optJSONObject("jobs") ?: return emptySet()
        val out = linkedSetOf<String>()
        val keys = jobs.keys()
        while (keys.hasNext()) {
            val id = keys.next()
            if (jobs.optJSONObject(id)?.optString("status") == status) {
                out += id
            }
        }
        return out
    }

    private inline fun mutate(
        context: Context,
        operationKey: String,
        block: (JSONObject) -> Unit,
    ) = synchronized(lock) {
        val root =
            readInternal(context, operationKey)
                ?: JSONObject()
                    .put("schema", SCHEMA)
                    .put("operation_key", operationKey)
                    .put("created_at_wall_ms", System.currentTimeMillis())
                    .put("jobs", JSONObject())
        block(root)
        root.put("updated_at_wall_ms", System.currentTimeMillis())
        write(context, operationKey, root)
    }

    private fun jobState(
        root: JSONObject,
        job: RawJob,
    ): JSONObject {
        val jobs =
            root.optJSONObject("jobs")
                ?: JSONObject().also { root.put("jobs", it) }
        val state =
            jobs.optJSONObject(job.id)
                ?: JSONObject()
                    .put("job_id", job.id)
                    .put("display_name", job.source.displayName)
                    .put("source_uri", job.source.uri.toString())
        jobs.put(job.id, state)
        return state
    }

    private fun sameJobSet(
        root: JSONObject,
        jobs: List<RawJob>,
    ): Boolean {
        val stored = root.optJSONObject("jobs") ?: return false
        if (stored.length() != jobs.size) return false
        return jobs.all { stored.has(it.id) }
    }

    private fun systemSample(
        context: Context,
    ): JSONObject {
        val runtime = Runtime.getRuntime()
        val activityManager =
            context.getSystemService(ActivityManager::class.java)
        val memoryInfo = ActivityManager.MemoryInfo()
        runCatching { activityManager?.getMemoryInfo(memoryInfo) }
        val powerManager =
            context.getSystemService(PowerManager::class.java)

        return JSONObject()
            .put("java_heap_used_bytes", runtime.totalMemory() - runtime.freeMemory())
            .put("java_heap_total_bytes", runtime.totalMemory())
            .put("java_heap_max_bytes", runtime.maxMemory())
            .put("native_heap_allocated_bytes", Debug.getNativeHeapAllocatedSize())
            .put("process_pss_kb", Debug.getPss())
            .put("system_avail_mem_bytes", memoryInfo.availMem)
            .put("system_low_memory", memoryInfo.lowMemory)
            .put("device_interactive", powerManager?.isInteractive ?: false)
    }

    private fun readInternal(
        context: Context,
        operationKey: String,
    ): JSONObject? {
        val file = fileFor(context, operationKey)
        if (!file.isFile) return null
        return runCatching {
            JSONObject(file.readText(Charsets.UTF_8))
        }.getOrNull()
            ?.takeIf {
                it.optString("schema") == SCHEMA &&
                    it.optString("operation_key") == operationKey
            }
    }

    private fun write(
        context: Context,
        operationKey: String,
        root: JSONObject,
    ) {
        val target = fileFor(context, operationKey)
        target.parentFile?.mkdirs()
        val temp = File(target.parentFile, target.name + ".tmp")
        val text = root.toString()
        temp.writeText(text, Charsets.UTF_8)
        if (target.exists() && !target.delete()) {
            temp.delete()
            return
        }
        if (!temp.renameTo(target)) {
            target.writeText(text, Charsets.UTF_8)
            temp.delete()
        }
    }

    private fun fileFor(
        context: Context,
        operationKey: String,
    ): File =
        File(
            File(context.filesDir, DIR_NAME),
            sha256(operationKey) + ".json",
        )

    private fun sha256(value: String): String =
        MessageDigest.getInstance("SHA-256")
            .digest(value.toByteArray(Charsets.UTF_8))
            .joinToString("") { "%02x".format(it) }
}

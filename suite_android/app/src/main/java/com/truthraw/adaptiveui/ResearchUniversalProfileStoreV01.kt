package com.truthraw.adaptiveui

import java.io.File
import org.json.JSONObject

/**
 * Private persistence for derived UniversalSourceProfile JSON in the Research
 * workbench. This is a cache of derived diagnostics only; it is not source
 * evidence and does not modify or replace the sealed RAW.
 */
object ResearchUniversalProfileStoreV01 {
    const val SCHEMA = "D.RAW/ResearchUniversalProfileStore/0.1"
    private const val DIR_NAME =
        "draw_research_universal_profiles_v0_1"

    private fun isCurrentProfile(profile: JSONObject): Boolean =
        profile.optString("profile_cache_generation") ==
            UniversalSourceProfiler.CACHE_GENERATION

    fun save(
        filesDir: File,
        job: RawJob,
        profile: JSONObject,
    ) {
        require(isCurrentProfile(profile)) {
            "Refusing to persist a UniversalSourceProfile from a stale profiler cache generation."
        }
        val dir =
            File(
                filesDir,
                DIR_NAME,
            ).apply {
                mkdirs()
            }
        val wrapper =
            JSONObject()
                .put("schema", SCHEMA)
                .put(
                    "profile_cache_generation",
                    UniversalSourceProfiler.CACHE_GENERATION,
                )
                .put("job_id", job.id)
                .put(
                    "source_uri",
                    job.source.uri.toString(),
                )
                .put(
                    "source_sha256",
                    profile.optString(
                        "source_sha256",
                    ),
                )
                .put(
                    "profile",
                    profile,
                )
                .put(
                    "creates_new_evidence",
                    false,
                )
                .put(
                    "scientific_writeback_allowed",
                    false,
                )

        atomicWrite(
            target =
                File(
                    dir,
                    safeName(job.id) +
                        ".json",
                ),
            text =
                wrapper.toString(),
        )
    }

    fun loadForJob(
        filesDir: File,
        job: RawJob,
    ): JSONObject? {
        val file =
            File(
                File(
                    filesDir,
                    DIR_NAME,
                ),
                safeName(job.id) +
                    ".json",
            )
        if (!file.isFile) {
            return null
        }
        val wrapper =
            runCatching {
                JSONObject(
                    file.readText(
                        Charsets.UTF_8,
                    ),
                )
            }.getOrNull()
                ?: return null
        if (
            wrapper.optString("schema") !=
            SCHEMA ||
            wrapper.optString("profile_cache_generation") !=
            UniversalSourceProfiler.CACHE_GENERATION ||
            wrapper.optString("job_id") !=
            job.id ||
            wrapper.optString("source_uri") !=
            job.source.uri.toString() ||
            wrapper.optBoolean(
                "creates_new_evidence",
                true,
            ) ||
            wrapper.optBoolean(
                "scientific_writeback_allowed",
                true,
            )
        ) {
            return null
        }
        val profile =
            wrapper.optJSONObject("profile")
                ?: return null
        if (
            profile.optString("status") !=
            "AUTO_PROFILED_IN_FULL_DRAW_SUITE" ||
            !isCurrentProfile(profile) ||
            profile.optString(
                "source_sha256",
            ).isBlank()
        ) {
            return null
        }
        return JSONObject(
            profile.toString(),
        )
    }

    fun load(
        filesDir: File,
        session: BatchSession,
    ): Map<String, JSONObject> {
        val dir =
            File(
                filesDir,
                DIR_NAME,
            )
        if (!dir.isDirectory) {
            return emptyMap()
        }

        val out =
            linkedMapOf<String, JSONObject>()
        for (job in session.jobs) {
            val file =
                File(
                    dir,
                    safeName(job.id) +
                        ".json",
                )
            if (!file.isFile) {
                continue
            }

            val wrapper =
                runCatching {
                    JSONObject(
                        file.readText(
                            Charsets.UTF_8,
                        ),
                    )
                }.getOrNull()
                    ?: continue
            if (
                wrapper.optString("schema") !=
                SCHEMA ||
                wrapper.optString("profile_cache_generation") !=
                UniversalSourceProfiler.CACHE_GENERATION ||
                wrapper.optString("job_id") !=
                job.id ||
                wrapper.optString("source_uri") !=
                job.source.uri.toString() ||
                wrapper.optBoolean(
                    "creates_new_evidence",
                    true,
                ) ||
                wrapper.optBoolean(
                    "scientific_writeback_allowed",
                    true,
                )
            ) {
                continue
            }

            val profile =
                wrapper.optJSONObject("profile")
                    ?: continue
            if (
                profile.optString("status") !=
                "AUTO_PROFILED_IN_FULL_DRAW_SUITE" ||
                !isCurrentProfile(profile) ||
                profile.optString(
                    "source_sha256",
                ).isBlank()
            ) {
                continue
            }
            out[job.id] =
                JSONObject(
                    profile.toString(),
                )
        }
        return out
    }

    fun remove(
        filesDir: File,
        jobId: String,
    ): Boolean {
        val file =
            File(
                File(
                    filesDir,
                    DIR_NAME,
                ),
                safeName(jobId) +
                    ".json",
            )
        if (!file.exists()) {
            return true
        }
        return runCatching {
            file.delete() || !file.exists()
        }.getOrDefault(false)
    }

    fun clear(
        filesDir: File,
    ) {
        val dir =
            File(
                filesDir,
                DIR_NAME,
            )
        if (!dir.exists()) {
            return
        }
        runCatching {
            dir.listFiles()
                ?.forEach {
                    it.delete()
                }
            dir.delete()
        }
    }

    private fun atomicWrite(
        target: File,
        text: String,
    ) {
        val temp =
            File(
                target.parentFile,
                target.name +
                    ".tmp",
            )
        temp.writeText(
            text,
            Charsets.UTF_8,
        )
        if (
            target.exists() &&
            !target.delete()
        ) {
            temp.delete()
            error(
                "Could not replace research profile cache.",
            )
        }
        if (!temp.renameTo(target)) {
            target.writeText(
                text,
                Charsets.UTF_8,
            )
            temp.delete()
        }
    }

    private fun safeName(
        value: String,
    ): String =
        value.replace(
            Regex(
                "[^A-Za-z0-9._-]",
            ),
            "_",
        )
}

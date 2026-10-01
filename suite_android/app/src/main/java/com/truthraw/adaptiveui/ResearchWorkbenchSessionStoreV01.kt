package com.truthraw.adaptiveui

import android.net.Uri
import java.io.File
import org.json.JSONArray
import org.json.JSONObject

/**
 * Private lightweight persistence for the Research/Multi-observation selection.
 *
 * Only document handles and ingress metadata are stored. RAW payload bytes are
 * never copied into this store. Imported document URIs rely on the persistable
 * read permission already requested by RawIngress.
 *
 * This exists so an Activity recreation or process-pressure restart cannot
 * silently erase a four-RAW research selection while universal profiling is
 * running.
 */
object ResearchWorkbenchSessionStoreV01 {
    const val SCHEMA = "D.RAW/ResearchWorkbenchSessionStore/0.1"
    private const val FILE_NAME =
        "draw_research_workbench_session_v0_1.json"

    fun save(
        cacheDir: File,
        session: BatchSession,
    ) {
        val root =
            JSONObject()
                .put("schema", SCHEMA)
                .put(
                    "route",
                    session.route.name,
                )
                .put(
                    "jobs",
                    JSONArray().also { array ->
                        for (job in session.jobs) {
                            array.put(jobJson(job))
                        }
                    },
                )
                .put(
                    "raw_payload_bytes_stored",
                    false,
                )
                .put(
                    "scientific_authority_changed",
                    false,
                )

        val target = File(cacheDir, FILE_NAME)
        val temp = File(cacheDir, FILE_NAME + ".tmp")
        runCatching {
            temp.writeText(
                root.toString(),
                Charsets.UTF_8,
            )
            if (target.exists() && !target.delete()) {
                error("Could not replace research session store")
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
        cacheDir: File,
    ): BatchSession? =
        runCatching {
            val file =
                File(
                    cacheDir,
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
                SCHEMA
            ) {
                return@runCatching null
            }
            val array =
                root.optJSONArray("jobs")
                    ?: return@runCatching null
            val jobs =
                buildList {
                    for (
                        index in
                        0 until array.length()
                    ) {
                        val item =
                            array.optJSONObject(
                                index,
                            ) ?: continue
                        parseJob(item)
                            ?.let(::add)
                    }
                }
            if (jobs.isEmpty()) {
                return@runCatching null
            }
            val route =
                runCatching {
                    InputRoute.valueOf(
                        root.optString(
                            "route",
                            InputRoute.BATCH_INDEPENDENT.name,
                        ),
                    )
                }.getOrDefault(
                    if (jobs.size == 1) {
                        InputRoute.SINGLE_ONE_OUTPUT
                    } else {
                        InputRoute.BATCH_INDEPENDENT
                    },
                )
            BatchSession(
                jobs = jobs,
                route = route,
            )
        }.getOrNull()

    fun clear(
        cacheDir: File,
    ) {
        runCatching {
            File(
                cacheDir,
                FILE_NAME,
            ).delete()
        }
        runCatching {
            File(
                cacheDir,
                FILE_NAME + ".tmp",
            ).delete()
        }
    }

    private fun jobJson(
        job: RawJob,
    ): JSONObject {
        val source = job.source
        return JSONObject()
            .put("job_id", job.id)
            .put("job_state", job.state.name)
            .put(
                "uri",
                source.uri.toString(),
            )
            .put(
                "display_name",
                source.displayName,
            )
            .put(
                "declared_size_bytes",
                source.declaredSizeBytes
                    ?: JSONObject.NULL,
            )
            .put(
                "mime_type",
                source.mimeType
                    ?: JSONObject.NULL,
            )
            .put(
                "source_route",
                source.sourceRoute.name,
            )
            .put(
                "acquisition_evidence_path",
                source.acquisitionEvidencePath
                    ?: JSONObject.NULL,
            )
            .put(
                "upstream_sealed_source_sha256",
                source.upstreamSealedSourceSha256
                    ?: JSONObject.NULL,
            )
            .put(
                "upstream_source_role",
                source.upstreamSourceRole
                    ?: JSONObject.NULL,
            )
            .put(
                "verified_camera5_truthnegative_200mp_envelope",
                source.verifiedCamera5TruthNegative200MpEnvelope,
            )
            .put(
                "acquisition_evidence_sha256",
                source.acquisitionEvidenceSha256
                    ?: JSONObject.NULL,
            )
    }

    private fun parseJob(
        item: JSONObject,
    ): RawJob? {
        val id =
            item.optString("job_id")
                .takeIf {
                    it.isNotBlank()
                } ?: return null
        val uriText =
            item.optString("uri")
                .takeIf {
                    it.isNotBlank()
                } ?: return null
        val displayName =
            item.optString(
                "display_name",
                "RAW",
            )
        val mimeType =
            item.optStringOrNull(
                "mime_type",
            )
        val sourceRoute =
            runCatching {
                SourceIngressRoute.valueOf(
                    item.optString(
                        "source_route",
                        SourceIngressRoute.IMPORTED_FILE.name,
                    ),
                )
            }.getOrDefault(
                SourceIngressRoute.IMPORTED_FILE,
            )
        val state =
            runCatching {
                JobState.valueOf(
                    item.optString(
                        "job_state",
                        JobState.READY.name,
                    ),
                )
            }.getOrDefault(
                JobState.READY,
            )

        return RawJob(
            id = id,
            source =
                RawHandle(
                    uri =
                        Uri.parse(
                            uriText,
                        ),
                    displayName =
                        displayName,
                    declaredSizeBytes =
                        if (
                            item.has(
                                "declared_size_bytes",
                            ) &&
                            !item.isNull(
                                "declared_size_bytes",
                            )
                        ) {
                            item.optLong(
                                "declared_size_bytes",
                            )
                        } else {
                            null
                        },
                    mimeType = mimeType,
                    sourceRoute =
                        sourceRoute,
                    acquisitionEvidencePath =
                        item.optStringOrNull(
                            "acquisition_evidence_path",
                        ),
                    upstreamSealedSourceSha256 =
                        item.optStringOrNull(
                            "upstream_sealed_source_sha256",
                        ),
                    upstreamSourceRole =
                        item.optStringOrNull(
                            "upstream_source_role",
                        ),
                    verifiedCamera5TruthNegative200MpEnvelope =
                        item.optBoolean(
                            "verified_camera5_truthnegative_200mp_envelope",
                            false,
                        ),
                    acquisitionEvidenceSha256 =
                        item.optStringOrNull(
                            "acquisition_evidence_sha256",
                        ),
                ),
            state = state,
        )
    }

    private fun JSONObject.optStringOrNull(
        key: String,
    ): String? =
        if (
            has(key) &&
            !isNull(key)
        ) {
            optString(key)
                .takeIf {
                    it.isNotBlank()
                }
        } else {
            null
        }
}

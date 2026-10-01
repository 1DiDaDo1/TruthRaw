package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import java.io.File
import java.security.MessageDigest
import org.json.JSONObject

/**
 * Disk-backed handoff for a prepared Research JSON export.
 *
 * ACTION_CREATE_DOCUMENT can move MainActivity to the background, recreate it,
 * or kill the process after the document provider has already created an empty
 * destination file. Keeping the prepared report only in an Activity String is
 * therefore unsafe.
 *
 * This store freezes the already safety-checked JSON snapshot before launching
 * the picker, records its SHA-256 and byte length, and later streams those exact
 * bytes to the chosen destination. The expensive Foundation graph is not
 * recomputed after returning from the picker.
 */
object ResearchPendingJsonExportStoreV01 {
    const val SCHEMA =
        "D.RAW/ResearchPendingJsonExportStore/0.2"
    private const val DIR_NAME =
        "draw_pending_research_json_exports_v0_2"

    const val FREE_WORLD_FOUNDATION =
        "FREE_WORLD_FOUNDATION"
    const val FIELD_RESPONSE_REPEATABILITY =
        "FIELD_RESPONSE_REPEATABILITY"
    const val OBSERVATION_WORLD_FIELD_SEPARATION =
        "OBSERVATION_WORLD_FIELD_SEPARATION"

    data class FrozenPayload(
        val file: File,
        val byteLength: Long,
        val sha256: String,
    )

    data class CopyResult(
        val byteLength: Long,
        val sha256: String,
    )

    fun save(
        filesDir: File,
        key: String,
        reportText: String,
    ): FrozenPayload {
        val dir =
            File(
                filesDir,
                DIR_NAME,
            ).apply {
                mkdirs()
            }
        val payload =
            File(
                dir,
                safeName(key) +
                    ".payload.json",
            )
        val payloadTemp =
            File(
                dir,
                payload.name +
                    ".tmp",
            )
        payloadTemp.writeText(
            reportText,
            Charsets.UTF_8,
        )
        replaceAtomic(
            temp = payloadTemp,
            target = payload,
        )

        val digest =
            sha256(payload)
        val meta =
            JSONObject()
                .put("schema", SCHEMA)
                .put("key", key)
                .put(
                    "byte_length",
                    payload.length(),
                )
                .put(
                    "sha256",
                    digest,
                )
                .put(
                    "snapshot_frozen_before_document_picker",
                    true,
                )
                .put(
                    "payload_recomputed_after_picker",
                    false,
                )
                .put(
                    "creates_new_evidence",
                    false,
                )
                .put(
                    "scientific_writeback_allowed",
                    false,
                )

        val metaFile =
            File(
                dir,
                safeName(key) +
                    ".meta.json",
            )
        val metaTemp =
            File(
                dir,
                metaFile.name +
                    ".tmp",
            )
        metaTemp.writeText(
            meta.toString(),
            Charsets.UTF_8,
        )
        replaceAtomic(
            temp = metaTemp,
            target = metaFile,
        )

        return FrozenPayload(
            file = payload,
            byteLength = payload.length(),
            sha256 = digest,
        )
    }

    fun frozen(
        filesDir: File,
        key: String,
    ): FrozenPayload? {
        val dir =
            File(
                filesDir,
                DIR_NAME,
            )
        val payload =
            File(
                dir,
                safeName(key) +
                    ".payload.json",
            )
        val metaFile =
            File(
                dir,
                safeName(key) +
                    ".meta.json",
            )
        if (
            !payload.isFile ||
            !metaFile.isFile
        ) {
            return null
        }

        val meta =
            runCatching {
                JSONObject(
                    metaFile.readText(
                        Charsets.UTF_8,
                    ),
                )
            }.getOrNull()
                ?: return null
        if (
            meta.optString("schema") !=
            SCHEMA ||
            meta.optString("key") !=
            key ||
            !meta.optBoolean(
                "snapshot_frozen_before_document_picker",
                false,
            ) ||
            meta.optBoolean(
                "payload_recomputed_after_picker",
                true,
            ) ||
            meta.optBoolean(
                "creates_new_evidence",
                true,
            ) ||
            meta.optBoolean(
                "scientific_writeback_allowed",
                true,
            )
        ) {
            return null
        }

        val expectedBytes =
            meta.optLong(
                "byte_length",
                -1L,
            )
        val expectedSha =
            meta.optString(
                "sha256",
            )
        if (
            expectedBytes < 1L ||
            payload.length() !=
            expectedBytes ||
            expectedSha.length != 64
        ) {
            return null
        }

        val actualSha =
            sha256(payload)
        if (
            actualSha !=
            expectedSha
        ) {
            return null
        }

        return FrozenPayload(
            file = payload,
            byteLength = expectedBytes,
            sha256 = expectedSha,
        )
    }

    fun copyFrozenTo(
        filesDir: File,
        key: String,
        resolver: ContentResolver,
        destination: Uri,
    ): CopyResult {
        val frozen =
            frozen(
                filesDir,
                key,
            ) ?: error(
                "Bevroren Research JSON ontbreekt of faalt SHA-256-validatie.",
            )

        val digest =
            MessageDigest.getInstance(
                "SHA-256",
            )
        var written = 0L

        frozen.file.inputStream()
            .buffered()
            .use { input ->
                val output =
                    resolver.openOutputStream(
                        destination,
                        "w",
                    ) ?: error(
                        "Documentprovider gaf geen outputstream.",
                    )
                output.buffered()
                    .use { out ->
                        val buffer =
                            ByteArray(
                                256 * 1024,
                            )
                        while (true) {
                            val count =
                                input.read(
                                    buffer,
                                )
                            if (count <= 0) {
                                break
                            }
                            out.write(
                                buffer,
                                0,
                                count,
                            )
                            digest.update(
                                buffer,
                                0,
                                count,
                            )
                            written +=
                                count.toLong()
                        }
                        out.flush()
                    }
            }

        val writtenSha =
            digest.digest()
                .joinToString("") {
                    "%02x".format(
                        it.toInt() and 0xff,
                    )
                }

        check(
            written ==
                frozen.byteLength,
        ) {
            "Export byte length mismatch: " +
                written +
                " != " +
                frozen.byteLength
        }
        check(
            writtenSha ==
                frozen.sha256,
        ) {
            "Export SHA-256 mismatch."
        }

        return CopyResult(
            byteLength = written,
            sha256 = writtenSha,
        )
    }

    fun clear(
        filesDir: File,
        key: String,
    ) {
        val dir =
            File(
                filesDir,
                DIR_NAME,
            )
        val stem =
            safeName(key)
        for (
            suffix in listOf(
                ".payload.json",
                ".payload.json.tmp",
                ".meta.json",
                ".meta.json.tmp",
            )
        ) {
            runCatching {
                File(
                    dir,
                    stem +
                        suffix,
                ).delete()
            }
        }
    }

    private fun replaceAtomic(
        temp: File,
        target: File,
    ) {
        if (
            target.exists() &&
            !target.delete()
        ) {
            temp.delete()
            error(
                "Could not replace pending Research export.",
            )
        }
        if (!temp.renameTo(target)) {
            target.writeBytes(
                temp.readBytes(),
            )
            temp.delete()
        }
    }

    private fun sha256(
        file: File,
    ): String {
        val digest =
            MessageDigest.getInstance(
                "SHA-256",
            )
        file.inputStream()
            .buffered()
            .use { input ->
                val buffer =
                    ByteArray(
                        256 * 1024,
                    )
                while (true) {
                    val count =
                        input.read(
                            buffer,
                        )
                    if (count <= 0) {
                        break
                    }
                    digest.update(
                        buffer,
                        0,
                        count,
                    )
                }
            }
        return digest.digest()
            .joinToString("") {
                "%02x".format(
                    it.toInt() and 0xff,
                )
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

package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject
import java.io.BufferedInputStream

private const val TNC_HEADER_BYTES = 4096
private const val TNC_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val TNC_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeNativeContainerBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportAndVerify(
        sourceFd: Int,
        destinationFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): String
}

data class TruthNegativeNativeContainerMetrics(
    val width: Int,
    val height: Int,
    val tileCount: Long,
    val recordCount: Long,
    val bodyBytes: Long,
    val fileBytes: Long,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val bodySha256: String,
    val containerSha256: String,
    val nativeImportVerified: Boolean,
    val authorityRoundtripVerified: Boolean,
    val stateRoundtripVerified: Boolean,
)

sealed interface TruthNegativeNativeContainerResult {
    data class Success(
        val metrics: TruthNegativeNativeContainerMetrics,
    ) : TruthNegativeNativeContainerResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeNativeContainerResult
}

object TruthNegativeNativeContainerExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeNativeContainerResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeNativeContainerResult.Failed(
                "Native TruthNegative container vereist de admitted DNG-route.",
            )
        }

        val jsonText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeNativeContainerBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        TNC_MAX_SOURCE_BYTES,
                        TNC_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeNativeContainerResult.Failed(
            "Native TruthNegative container kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(jsonText) }.getOrNull()
            ?: return TruthNegativeNativeContainerResult.Failed(
                "Native TruthNegative container gaf geen geldig statuspakket.",
            )
        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeNativeContainerResult.Failed(
                json.optString(
                    "message",
                    "Native TruthNegative container status " + status,
                ),
            )
        }

        val metrics = TruthNegativeNativeContainerMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            tileCount = json.optLong("tileCount"),
            recordCount = json.optLong("recordCount"),
            bodyBytes = json.optLong("bodyBytes"),
            fileBytes = json.optLong("fileBytes"),
            sourceSha256 = json.optString("sourceSha256"),
            scientificMasterSha256 =
                json.optString("scientificMasterSha256"),
            authorityFieldSha256 =
                json.optString("authorityFieldSha256"),
            truthNegativeStateSha256 =
                json.optString("truthNegativeStateSha256"),
            bodySha256 = json.optString("bodySha256"),
            containerSha256 = json.optString("containerSha256"),
            nativeImportVerified =
                json.optBoolean("nativeImportVerified"),
            authorityRoundtripVerified =
                json.optBoolean("authorityRoundtripVerified"),
            stateRoundtripVerified =
                json.optBoolean("stateRoundtripVerified"),
        )

        val identityDigests = listOf(
            metrics.sourceSha256,
            metrics.scientificMasterSha256,
            metrics.authorityFieldSha256,
            metrics.truthNegativeStateSha256,
            metrics.bodySha256,
            metrics.containerSha256,
        )
        if (!metrics.nativeImportVerified ||
            !metrics.authorityRoundtripVerified ||
            !metrics.stateRoundtripVerified ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.recordCount !=
                metrics.width.toLong() *
                metrics.height.toLong() * 3L ||
            identityDigests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeNativeContainerResult.Failed(
                "Fail-closed: native container round-trip contract mismatch.",
            )
        }

        val headerOk = runCatching {
            resolver.openInputStream(destination)?.use { raw ->
                val input = BufferedInputStream(raw)
                val bytes = ByteArray(TNC_HEADER_BYTES)
                var offset = 0
                while (offset < bytes.size) {
                    val n = input.read(
                        bytes,
                        offset,
                        bytes.size - offset,
                    )
                    if (n <= 0) break
                    offset += n
                }
                if (offset != bytes.size) {
                    false
                } else {
                    val header = bytes.toString(Charsets.US_ASCII)
                    listOf(
                        "magic=DRAW_TRUTHNEGATIVE_NATIVE_CONTAINER_V01",
                        "container_version=1",
                        "schema=TruthNegativeNativeContainer/0.1",
                        "truthnegative_state_sha256=" +
                            metrics.truthNegativeStateSha256,
                        "creates_new_evidence=0",
                        "scientific_writeback_allowed=0",
                        "END_HEADER",
                    ).all(header::contains)
                }
            } ?: false
        }.getOrDefault(false)

        if (!headerOk) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeNativeContainerResult.Failed(
                "Post-write container header verification faalde.",
            )
        }

        return TruthNegativeNativeContainerResult.Success(metrics)
    }
}

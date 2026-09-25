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
    val roleUnknown: Long,
    val roleSourceMeasuredCfa: Long,
    val roleScientificReconstruction: Long,
    val roleDenseProjection: Long,
    val roleRestorationDerivative: Long,
    val authorityCalibratedEstimate: Long,
    val authorityReconstructed: Long,
    val authorityCensored: Long,
    val authorityUnknown: Long,
    val censoredR: Long,
    val censoredG: Long,
    val censoredB: Long,
    val censoredValueAboveOne: Long,
    val censoredValueAtOrBelowOne: Long,
    val censoredTileCount: Long,
    val censoredMinX: Long,
    val censoredMinY: Long,
    val censoredMaxX: Long,
    val censoredMaxY: Long,
    val censoredRawCodeBoundMin: Double,
    val censoredRawCodeBoundMax: Double,
    val censoredRawCodeBoundMismatchCount: Long,
    val uncertaintyKnownCount: Long,
    val supportKnownCount: Long,
    val boundKnownCount: Long,
    val valueNegativeCount: Long,
    val valueAboveOneCount: Long,
    val valueNonFiniteCount: Long,
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
            roleUnknown = json.optLong("roleUnknown"),
            roleSourceMeasuredCfa = json.optLong("roleSourceMeasuredCfa"),
            roleScientificReconstruction = json.optLong("roleScientificReconstruction"),
            roleDenseProjection = json.optLong("roleDenseProjection"),
            roleRestorationDerivative = json.optLong("roleRestorationDerivative"),
            authorityCalibratedEstimate = json.optLong("authorityCalibratedEstimate"),
            authorityReconstructed = json.optLong("authorityReconstructed"),
            authorityCensored = json.optLong("authorityCensored"),
            authorityUnknown = json.optLong("authorityUnknown"),
            censoredR = json.optLong("censoredR"),
            censoredG = json.optLong("censoredG"),
            censoredB = json.optLong("censoredB"),
            censoredValueAboveOne = json.optLong("censoredValueAboveOne"),
            censoredValueAtOrBelowOne = json.optLong("censoredValueAtOrBelowOne"),
            censoredTileCount = json.optLong("censoredTileCount"),
            censoredMinX = json.optLong("censoredMinX"),
            censoredMinY = json.optLong("censoredMinY"),
            censoredMaxX = json.optLong("censoredMaxX"),
            censoredMaxY = json.optLong("censoredMaxY"),
            censoredRawCodeBoundMin = json.optDouble("censoredRawCodeBoundMin"),
            censoredRawCodeBoundMax = json.optDouble("censoredRawCodeBoundMax"),
            censoredRawCodeBoundMismatchCount =
                json.optLong("censoredRawCodeBoundMismatchCount"),
            uncertaintyKnownCount = json.optLong("uncertaintyKnownCount"),
            supportKnownCount = json.optLong("supportKnownCount"),
            boundKnownCount = json.optLong("boundKnownCount"),
            valueNegativeCount = json.optLong("valueNegativeCount"),
            valueAboveOneCount = json.optLong("valueAboveOneCount"),
            valueNonFiniteCount = json.optLong("valueNonFiniteCount"),
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
            metrics.roleUnknown +
                metrics.roleSourceMeasuredCfa +
                metrics.roleScientificReconstruction +
                metrics.roleDenseProjection +
                metrics.roleRestorationDerivative != metrics.recordCount ||
            metrics.authorityCalibratedEstimate +
                metrics.authorityReconstructed +
                metrics.authorityCensored +
                metrics.authorityUnknown != metrics.recordCount ||
            metrics.censoredR + metrics.censoredG + metrics.censoredB !=
                metrics.authorityCensored ||
            metrics.censoredValueAboveOne +
                metrics.censoredValueAtOrBelowOne != metrics.authorityCensored ||
            metrics.censoredRawCodeBoundMismatchCount != 0L ||
            metrics.valueNonFiniteCount != 0L ||
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

package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val N2_SPATIAL_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val N2_SPATIAL_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeN2SpatialSidecarBridge {
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

data class TruthNegativeN2SpatialSidecarMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val tileCount: Long,
    val sampled: Long,
    val candidateCorrected: Long,
    val preserved: Long,
    val structureProtected: Long,
    val censoredProtected: Long,
    val censorBoundaryProtected: Long,
    val removedResidualEnergyFraction: Double,
    val maxAbsCorrectionStage2: Double,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val candidateSha256: String,
    val auditSha256: String,
    val spatialSha256: String,
    val jsonSha256: String,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeN2SpatialSidecarResult {
    data class Success(
        val metrics: TruthNegativeN2SpatialSidecarMetrics,
    ) : TruthNegativeN2SpatialSidecarResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeN2SpatialSidecarResult
}

object TruthNegativeN2SpatialSidecarExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeN2SpatialSidecarResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeN2SpatialSidecarResult.Failed(
                "N2 Spatial Audit vereist de volledig admitted DNG-route.",
            )
        }

        val jsonText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeN2SpatialSidecarBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        N2_SPATIAL_MAX_SOURCE_BYTES,
                        N2_SPATIAL_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeN2SpatialSidecarResult.Failed(
            "N2 Spatial Audit sidecar kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(jsonText) }.getOrNull()
            ?: return TruthNegativeN2SpatialSidecarResult.Failed(
                "N2 Spatial Audit bridge gaf geen geldig statuspakket.",
            )
        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2SpatialSidecarResult.Failed(
                json.optString(
                    "message",
                    "N2 Spatial Audit status " + status,
                ),
            )
        }

        val metrics = TruthNegativeN2SpatialSidecarMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            tileCount = json.optLong("tileCount"),
            sampled = json.optLong("sampled"),
            candidateCorrected = json.optLong("candidateCorrected"),
            preserved = json.optLong("preserved"),
            structureProtected = json.optLong("structureProtected"),
            censoredProtected = json.optLong("censoredProtected"),
            censorBoundaryProtected =
                json.optLong("censorBoundaryProtected"),
            removedResidualEnergyFraction =
                json.optDouble("removedResidualEnergyFraction"),
            maxAbsCorrectionStage2 =
                json.optDouble("maxAbsCorrectionStage2"),
            sourceSha256 = json.optString("sourceSha256"),
            scientificMasterSha256 =
                json.optString("scientificMasterSha256"),
            authorityFieldSha256 =
                json.optString("authorityFieldSha256"),
            truthNegativeStateSha256 =
                json.optString("truthNegativeStateSha256"),
            candidateSha256 = json.optString("candidateSha256"),
            auditSha256 = json.optString("auditSha256"),
            spatialSha256 = json.optString("spatialSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            postWriteVerified = json.optBoolean("postWriteVerified"),
        )

        val digests = listOf(
            metrics.sourceSha256,
            metrics.scientificMasterSha256,
            metrics.authorityFieldSha256,
            metrics.truthNegativeStateSha256,
            metrics.candidateSha256,
            metrics.auditSha256,
            metrics.spatialSha256,
            metrics.jsonSha256,
        )
        if (!metrics.postWriteVerified ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.tileCount <= 0L ||
            metrics.sampled <= 0L ||
            metrics.candidateCorrected + metrics.preserved != metrics.sampled ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2SpatialSidecarResult.Failed(
                "Fail-closed: N2 Spatial Audit sidecar-contract mismatch.",
            )
        }

        return TruthNegativeN2SpatialSidecarResult.Success(metrics)
    }
}

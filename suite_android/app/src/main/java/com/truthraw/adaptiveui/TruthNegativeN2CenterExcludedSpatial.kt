package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val N2_CE_SPATIAL_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val N2_CE_SPATIAL_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeN2CenterExcludedSpatialBridge {
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

data class TruthNegativeN2CenterExcludedSpatialMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val tileCount: Long,
    val sampled: Long,
    val v01CandidateCenters: Long,
    val predictorValid: Long,
    val predictorInvalid: Long,
    val pairsConsidered: Long,
    val pairsAccepted: Long,
    val pairsRejected: Long,
    val scalesConsidered: Long,
    val scalesAccepted: Long,
    val scalesRejected: Long,
    val centerZLe1: Long,
    val centerZ1To2: Long,
    val centerZGt2: Long,
    val combinedZLe1: Long,
    val combinedZ1To2: Long,
    val combinedZGt2: Long,
    val meanAbsResidual: Double,
    val maxAbsResidual: Double,
    val meanCenterVariance: Double,
    val meanEstimateVariance: Double,
    val meanEstimateToCenterVarianceRatio: Double,
    val maxEstimateToCenterVarianceRatio: Double,
    val maxDirectionalSigma: Double,
    val maxCrossScaleSigma: Double,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val v01CandidateSha256: String,
    val v01AuditSha256: String,
    val v01SpatialSha256: String,
    val centerExcludedAuditSha256: String,
    val jsonSha256: String,
    val v01TileParityVerified: Boolean,
    val centerOnlySigmaPrimary: Boolean,
    val combinedSigmaDiagnosticOnly: Boolean,
    val noiseIndependenceAdmitted: Boolean,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeN2CenterExcludedSpatialResult {
    data class Success(
        val metrics: TruthNegativeN2CenterExcludedSpatialMetrics,
    ) : TruthNegativeN2CenterExcludedSpatialResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeN2CenterExcludedSpatialResult
}

object TruthNegativeN2CenterExcludedSpatialExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeN2CenterExcludedSpatialResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeN2CenterExcludedSpatialResult.Failed(
                "N2 v0.2.1 Spatial Audit vereist de volledig admitted DNG-route.",
            )
        }

        val jsonText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeN2CenterExcludedSpatialBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        N2_CE_SPATIAL_MAX_SOURCE_BYTES,
                        N2_CE_SPATIAL_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeN2CenterExcludedSpatialResult.Failed(
            "N2 v0.2.1 Spatial Audit sidecar kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(jsonText) }.getOrNull()
            ?: return TruthNegativeN2CenterExcludedSpatialResult.Failed(
                "N2 v0.2.1 Spatial Audit bridge gaf geen geldig statuspakket.",
            )

        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2CenterExcludedSpatialResult.Failed(
                json.optString(
                    "message",
                    "N2 v0.2.1 Spatial Audit status $status",
                ),
            )
        }

        val metrics = TruthNegativeN2CenterExcludedSpatialMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            tileCount = json.optLong("tileCount"),
            sampled = json.optLong("sampled"),
            v01CandidateCenters = json.optLong("v01CandidateCenters"),
            predictorValid = json.optLong("predictorValid"),
            predictorInvalid = json.optLong("predictorInvalid"),
            pairsConsidered = json.optLong("pairsConsidered"),
            pairsAccepted = json.optLong("pairsAccepted"),
            pairsRejected = json.optLong("pairsRejected"),
            scalesConsidered = json.optLong("scalesConsidered"),
            scalesAccepted = json.optLong("scalesAccepted"),
            scalesRejected = json.optLong("scalesRejected"),
            centerZLe1 = json.optLong("centerZLe1"),
            centerZ1To2 = json.optLong("centerZ1To2"),
            centerZGt2 = json.optLong("centerZGt2"),
            combinedZLe1 = json.optLong("combinedZLe1"),
            combinedZ1To2 = json.optLong("combinedZ1To2"),
            combinedZGt2 = json.optLong("combinedZGt2"),
            meanAbsResidual = json.optDouble("meanAbsResidual"),
            maxAbsResidual = json.optDouble("maxAbsResidual"),
            meanCenterVariance = json.optDouble("meanCenterVariance"),
            meanEstimateVariance = json.optDouble("meanEstimateVariance"),
            meanEstimateToCenterVarianceRatio =
                json.optDouble("meanEstimateToCenterVarianceRatio"),
            maxEstimateToCenterVarianceRatio =
                json.optDouble("maxEstimateToCenterVarianceRatio"),
            maxDirectionalSigma = json.optDouble("maxDirectionalSigma"),
            maxCrossScaleSigma = json.optDouble("maxCrossScaleSigma"),
            sourceSha256 = json.optString("sourceSha256"),
            scientificMasterSha256 =
                json.optString("scientificMasterSha256"),
            authorityFieldSha256 =
                json.optString("authorityFieldSha256"),
            truthNegativeStateSha256 =
                json.optString("truthNegativeStateSha256"),
            v01CandidateSha256 =
                json.optString("v01CandidateSha256"),
            v01AuditSha256 = json.optString("v01AuditSha256"),
            v01SpatialSha256 = json.optString("v01SpatialSha256"),
            centerExcludedAuditSha256 =
                json.optString("centerExcludedAuditSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            v01TileParityVerified =
                json.optBoolean("v01TileParityVerified"),
            centerOnlySigmaPrimary =
                json.optBoolean("centerOnlySigmaPrimary"),
            combinedSigmaDiagnosticOnly =
                json.optBoolean("combinedSigmaDiagnosticOnly"),
            noiseIndependenceAdmitted =
                json.optBoolean("noiseIndependenceAdmitted"),
            postWriteVerified = json.optBoolean("postWriteVerified"),
        )

        val digests = listOf(
            metrics.sourceSha256,
            metrics.scientificMasterSha256,
            metrics.authorityFieldSha256,
            metrics.truthNegativeStateSha256,
            metrics.v01CandidateSha256,
            metrics.v01AuditSha256,
            metrics.v01SpatialSha256,
            metrics.centerExcludedAuditSha256,
            metrics.jsonSha256,
        )

        val centerZCount =
            metrics.centerZLe1 +
                metrics.centerZ1To2 +
                metrics.centerZGt2
        val combinedZCount =
            metrics.combinedZLe1 +
                metrics.combinedZ1To2 +
                metrics.combinedZGt2

        if (!metrics.postWriteVerified ||
            !metrics.v01TileParityVerified ||
            !metrics.centerOnlySigmaPrimary ||
            !metrics.combinedSigmaDiagnosticOnly ||
            metrics.noiseIndependenceAdmitted ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.tileCount <= 0L ||
            metrics.sampled <= 0L ||
            metrics.v01CandidateCenters < 0L ||
            metrics.predictorValid < 0L ||
            metrics.predictorInvalid < 0L ||
            metrics.predictorValid + metrics.predictorInvalid !=
                metrics.v01CandidateCenters ||
            centerZCount != metrics.predictorValid ||
            combinedZCount != metrics.predictorValid ||
            metrics.pairsAccepted + metrics.pairsRejected >
                metrics.pairsConsidered ||
            metrics.scalesAccepted + metrics.scalesRejected !=
                metrics.scalesConsidered ||
            !metrics.meanAbsResidual.isFinite() ||
            !metrics.maxAbsResidual.isFinite() ||
            !metrics.meanCenterVariance.isFinite() ||
            !metrics.meanEstimateVariance.isFinite() ||
            !metrics.meanEstimateToCenterVarianceRatio.isFinite() ||
            !metrics.maxEstimateToCenterVarianceRatio.isFinite() ||
            !metrics.maxDirectionalSigma.isFinite() ||
            !metrics.maxCrossScaleSigma.isFinite() ||
            metrics.meanAbsResidual < 0.0 ||
            metrics.maxAbsResidual < metrics.meanAbsResidual ||
            metrics.meanCenterVariance < 0.0 ||
            metrics.meanEstimateVariance < 0.0 ||
            metrics.meanEstimateToCenterVarianceRatio < 0.0 ||
            metrics.maxEstimateToCenterVarianceRatio <
                metrics.meanEstimateToCenterVarianceRatio ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2CenterExcludedSpatialResult.Failed(
                "Fail-closed: N2 v0.2.1 Spatial Audit sidecar-contract mismatch.",
            )
        }

        return TruthNegativeN2CenterExcludedSpatialResult.Success(metrics)
    }
}

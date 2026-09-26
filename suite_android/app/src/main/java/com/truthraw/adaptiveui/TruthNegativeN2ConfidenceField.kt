package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val N2_CONFIDENCE_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val N2_CONFIDENCE_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeN2ConfidenceFieldBridge {
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

data class TruthNegativeN2ConfidenceFieldMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val tileCount: Long,
    val sampled: Long,
    val v01CandidateCenters: Long,
    val predictorValid: Long,
    val candidateFraction: Double,
    val predictorCoverage: Double,
    val pairAcceptance: Double,
    val scaleAcceptance: Double,
    val centerZGt2Fraction: Double,
    val noCandidateTiles: Long,
    val unresolvedTiles: Long,
    val mixedTiles: Long,
    val fullyCoherentTiles: Long,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val v01CandidateSha256: String,
    val v01AuditSha256: String,
    val v01SpatialSha256: String,
    val centerExcludedAuditSha256: String,
    val confidenceFieldSha256: String,
    val jsonSha256: String,
    val exactV01ParityVerified: Boolean,
    val exactCenterExcludedParityVerified: Boolean,
    val vectorValuedNoScalarProbability: Boolean,
    val cfaPhaseDiagnosticOnly: Boolean,
    val supportDistanceAdmitted: Boolean,
    val promotionEligible: Boolean,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeN2ConfidenceFieldResult {
    data class Success(
        val metrics: TruthNegativeN2ConfidenceFieldMetrics,
    ) : TruthNegativeN2ConfidenceFieldResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeN2ConfidenceFieldResult
}

object TruthNegativeN2ConfidenceFieldExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeN2ConfidenceFieldResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeN2ConfidenceFieldResult.Failed(
                "N2 Confidence Field v0.3 vereist de volledig admitted DNG-route.",
            )
        }

        val statusText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeN2ConfidenceFieldBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        N2_CONFIDENCE_MAX_SOURCE_BYTES,
                        N2_CONFIDENCE_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeN2ConfidenceFieldResult.Failed(
            "N2 Confidence Field v0.3 kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(statusText) }.getOrNull()
            ?: return TruthNegativeN2ConfidenceFieldResult.Failed(
                "N2 Confidence Field v0.3 bridge gaf geen geldig statuspakket.",
            )

        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2ConfidenceFieldResult.Failed(
                json.optString(
                    "message",
                    "N2 Confidence Field v0.3 status $status",
                ),
            )
        }

        val metrics = TruthNegativeN2ConfidenceFieldMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            tileCount = json.optLong("tileCount"),
            sampled = json.optLong("sampled"),
            v01CandidateCenters = json.optLong("v01CandidateCenters"),
            predictorValid = json.optLong("predictorValid"),
            candidateFraction = json.optDouble("candidateFraction"),
            predictorCoverage = json.optDouble("predictorCoverage"),
            pairAcceptance = json.optDouble("pairAcceptance"),
            scaleAcceptance = json.optDouble("scaleAcceptance"),
            centerZGt2Fraction = json.optDouble("centerZGt2Fraction"),
            noCandidateTiles = json.optLong("noCandidateTiles"),
            unresolvedTiles = json.optLong("unresolvedTiles"),
            mixedTiles = json.optLong("mixedTiles"),
            fullyCoherentTiles = json.optLong("fullyCoherentTiles"),
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
            confidenceFieldSha256 =
                json.optString("confidenceFieldSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            exactV01ParityVerified =
                json.optBoolean("exactV01ParityVerified"),
            exactCenterExcludedParityVerified =
                json.optBoolean("exactCenterExcludedParityVerified"),
            vectorValuedNoScalarProbability =
                json.optBoolean("vectorValuedNoScalarProbability"),
            cfaPhaseDiagnosticOnly =
                json.optBoolean("cfaPhaseDiagnosticOnly"),
            supportDistanceAdmitted =
                json.optBoolean("supportDistanceAdmitted"),
            promotionEligible =
                json.optBoolean("promotionEligible"),
            postWriteVerified =
                json.optBoolean("postWriteVerified"),
        )

        val classTileCount =
            metrics.noCandidateTiles +
                metrics.unresolvedTiles +
                metrics.mixedTiles +
                metrics.fullyCoherentTiles
        val fractions = listOf(
            metrics.candidateFraction,
            metrics.predictorCoverage,
            metrics.pairAcceptance,
            metrics.scaleAcceptance,
            metrics.centerZGt2Fraction,
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
            metrics.confidenceFieldSha256,
            metrics.jsonSha256,
        )

        if (!metrics.postWriteVerified ||
            !metrics.exactV01ParityVerified ||
            !metrics.exactCenterExcludedParityVerified ||
            !metrics.vectorValuedNoScalarProbability ||
            !metrics.cfaPhaseDiagnosticOnly ||
            metrics.supportDistanceAdmitted ||
            metrics.promotionEligible ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.tileCount <= 0L ||
            metrics.sampled <= 0L ||
            metrics.v01CandidateCenters < 0L ||
            metrics.v01CandidateCenters > metrics.sampled ||
            metrics.predictorValid < 0L ||
            metrics.predictorValid > metrics.v01CandidateCenters ||
            classTileCount != metrics.tileCount ||
            fractions.any { !it.isFinite() || it < 0.0 || it > 1.0 } ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2ConfidenceFieldResult.Failed(
                "Fail-closed: N2 Confidence Field v0.3 contract mismatch.",
            )
        }

        return TruthNegativeN2ConfidenceFieldResult.Success(metrics)
    }
}

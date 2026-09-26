package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val N2_FACTORED_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val N2_FACTORED_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeN2FactoredConfidenceBridge {
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

data class TruthNegativeN2FactoredConfidenceMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val tileCount: Long,
    val hasCandidateTiles: Long,
    val allCandidatesPredictableTiles: Long,
    val centerOutlierFreeTiles: Long,
    val predictableAndCenterOutlierFreeTiles: Long,
    val pairRejectionFreeTiles: Long,
    val scaleRejectionFreeTiles: Long,
    val structureProtectionPresentTiles: Long,
    val censorProtectionPresentTiles: Long,
    val censorBoundaryProtectionPresentTiles: Long,
    val maxPredictorVarianceLeCenterVarianceTiles: Long,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val v01CandidateSha256: String,
    val v01AuditSha256: String,
    val v01SpatialSha256: String,
    val centerExcludedAuditSha256: String,
    val confidenceFieldSha256: String,
    val factoredStateSha256: String,
    val jsonSha256: String,
    val exactConfidenceFieldBindingVerified: Boolean,
    val vectorValuedNoScalarProbability: Boolean,
    val legacyClassNonAuthoritative: Boolean,
    val cfaPhaseDiagnosticOnly: Boolean,
    val supportDistanceAdmitted: Boolean,
    val promotionEligible: Boolean,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeN2FactoredConfidenceResult {
    data class Success(
        val metrics: TruthNegativeN2FactoredConfidenceMetrics,
    ) : TruthNegativeN2FactoredConfidenceResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeN2FactoredConfidenceResult
}

object TruthNegativeN2FactoredConfidenceExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeN2FactoredConfidenceResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeN2FactoredConfidenceResult.Failed(
                "N2 Factored Confidence v0.3.1 vereist de volledig admitted DNG-route.",
            )
        }

        val statusText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeN2FactoredConfidenceBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        N2_FACTORED_MAX_SOURCE_BYTES,
                        N2_FACTORED_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeN2FactoredConfidenceResult.Failed(
            "N2 Factored Confidence v0.3.1 kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(statusText) }.getOrNull()
            ?: return TruthNegativeN2FactoredConfidenceResult.Failed(
                "N2 Factored Confidence v0.3.1 bridge gaf geen geldig statuspakket.",
            )

        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2FactoredConfidenceResult.Failed(
                json.optString(
                    "message",
                    "N2 Factored Confidence v0.3.1 status $status",
                ),
            )
        }

        val metrics = TruthNegativeN2FactoredConfidenceMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            tileCount = json.optLong("tileCount"),
            hasCandidateTiles = json.optLong("hasCandidateTiles"),
            allCandidatesPredictableTiles =
                json.optLong("allCandidatesPredictableTiles"),
            centerOutlierFreeTiles =
                json.optLong("centerOutlierFreeTiles"),
            predictableAndCenterOutlierFreeTiles =
                json.optLong("predictableAndCenterOutlierFreeTiles"),
            pairRejectionFreeTiles =
                json.optLong("pairRejectionFreeTiles"),
            scaleRejectionFreeTiles =
                json.optLong("scaleRejectionFreeTiles"),
            structureProtectionPresentTiles =
                json.optLong("structureProtectionPresentTiles"),
            censorProtectionPresentTiles =
                json.optLong("censorProtectionPresentTiles"),
            censorBoundaryProtectionPresentTiles =
                json.optLong("censorBoundaryProtectionPresentTiles"),
            maxPredictorVarianceLeCenterVarianceTiles =
                json.optLong("maxPredictorVarianceLeCenterVarianceTiles"),
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
            factoredStateSha256 =
                json.optString("factoredStateSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            exactConfidenceFieldBindingVerified =
                json.optBoolean("exactConfidenceFieldBindingVerified"),
            vectorValuedNoScalarProbability =
                json.optBoolean("vectorValuedNoScalarProbability"),
            legacyClassNonAuthoritative =
                json.optBoolean("legacyClassNonAuthoritative"),
            cfaPhaseDiagnosticOnly =
                json.optBoolean("cfaPhaseDiagnosticOnly"),
            supportDistanceAdmitted =
                json.optBoolean("supportDistanceAdmitted"),
            promotionEligible =
                json.optBoolean("promotionEligible"),
            postWriteVerified =
                json.optBoolean("postWriteVerified"),
        )

        val counts = listOf(
            metrics.hasCandidateTiles,
            metrics.allCandidatesPredictableTiles,
            metrics.centerOutlierFreeTiles,
            metrics.predictableAndCenterOutlierFreeTiles,
            metrics.pairRejectionFreeTiles,
            metrics.scaleRejectionFreeTiles,
            metrics.structureProtectionPresentTiles,
            metrics.censorProtectionPresentTiles,
            metrics.censorBoundaryProtectionPresentTiles,
            metrics.maxPredictorVarianceLeCenterVarianceTiles,
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
            metrics.factoredStateSha256,
            metrics.jsonSha256,
        )

        if (!metrics.postWriteVerified ||
            !metrics.exactConfidenceFieldBindingVerified ||
            !metrics.vectorValuedNoScalarProbability ||
            !metrics.legacyClassNonAuthoritative ||
            !metrics.cfaPhaseDiagnosticOnly ||
            metrics.supportDistanceAdmitted ||
            metrics.promotionEligible ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.tileCount <= 0L ||
            counts.any { it < 0L || it > metrics.tileCount } ||
            metrics.predictableAndCenterOutlierFreeTiles >
                metrics.allCandidatesPredictableTiles ||
            metrics.predictableAndCenterOutlierFreeTiles >
                metrics.centerOutlierFreeTiles ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeN2FactoredConfidenceResult.Failed(
                "Fail-closed: N2 Factored Confidence v0.3.1 contract mismatch.",
            )
        }

        return TruthNegativeN2FactoredConfidenceResult.Success(metrics)
    }
}

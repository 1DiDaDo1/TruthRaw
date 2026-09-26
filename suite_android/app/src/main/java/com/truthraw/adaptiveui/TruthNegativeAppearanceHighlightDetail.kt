package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val APPEARANCE_HIGHLIGHT_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val APPEARANCE_HIGHLIGHT_MAX_LOGICAL_BYTES = 64 * 1024 * 1024
private const val APPEARANCE_HIGHLIGHT_MAX_EDGE = 192

object TruthNegativeAppearanceHighlightDetailBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportAndVerify(
        sourceFd: Int,
        destinationFd: Int,
        requestedMaxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): String
}

data class TruthNegativeAppearanceHighlightDetailMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val sampleCount: Long,
    val sourceAboveReferenceWhite: Long,
    val mappedAtPeak: Long,
    val sourceCensored: Long,
    val gamutOrDisplayClamp: Long,
    val adjacentPairs: Long,
    val sourceDistinctAdjacentPairs: Long,
    val peakCollapsedDistinctAdjacentPairs: Long,
    val brightPeakCollapsedDistinctAdjacentPairs: Long,
    val sourceGradientSum: Double,
    val mappedGradientSum: Double,
    val collapsedSourceGradientSum: Double,
    val maxCollapsedSourceGradient: Double,
    val peakCollapseFractionOfDistinctPairs: Double,
    val noHighlightHeadroom: Boolean,
    val mappedPeakCollapseObserved: Boolean,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val appearanceStateSha256: String,
    val auditSha256: String,
    val jsonSha256: String,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeAppearanceHighlightDetailResult {
    data class Success(
        val metrics: TruthNegativeAppearanceHighlightDetailMetrics,
    ) : TruthNegativeAppearanceHighlightDetailResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeAppearanceHighlightDetailResult
}

object TruthNegativeAppearanceHighlightDetailExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeAppearanceHighlightDetailResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeAppearanceHighlightDetailResult.Failed(
                "Appearance Highlight Detail v0.1 vereist de volledig admitted DNG-route.",
            )
        }

        val statusText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeAppearanceHighlightDetailBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        APPEARANCE_HIGHLIGHT_MAX_EDGE,
                        APPEARANCE_HIGHLIGHT_MAX_SOURCE_BYTES,
                        APPEARANCE_HIGHLIGHT_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeAppearanceHighlightDetailResult.Failed(
            "Appearance Highlight Detail v0.1 kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(statusText) }.getOrNull()
            ?: return TruthNegativeAppearanceHighlightDetailResult.Failed(
                "Appearance Highlight Detail v0.1 bridge gaf geen geldig statuspakket.",
            )

        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeAppearanceHighlightDetailResult.Failed(
                json.optString(
                    "message",
                    "Appearance Highlight Detail v0.1 status $status",
                ),
            )
        }

        val metrics = TruthNegativeAppearanceHighlightDetailMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            sampleCount = json.optLong("sampleCount"),
            sourceAboveReferenceWhite =
                json.optLong("sourceAboveReferenceWhite"),
            mappedAtPeak = json.optLong("mappedAtPeak"),
            sourceCensored = json.optLong("sourceCensored"),
            gamutOrDisplayClamp = json.optLong("gamutOrDisplayClamp"),
            adjacentPairs = json.optLong("adjacentPairs"),
            sourceDistinctAdjacentPairs =
                json.optLong("sourceDistinctAdjacentPairs"),
            peakCollapsedDistinctAdjacentPairs =
                json.optLong("peakCollapsedDistinctAdjacentPairs"),
            brightPeakCollapsedDistinctAdjacentPairs =
                json.optLong("brightPeakCollapsedDistinctAdjacentPairs"),
            sourceGradientSum = json.optDouble("sourceGradientSum"),
            mappedGradientSum = json.optDouble("mappedGradientSum"),
            collapsedSourceGradientSum =
                json.optDouble("collapsedSourceGradientSum"),
            maxCollapsedSourceGradient =
                json.optDouble("maxCollapsedSourceGradient"),
            peakCollapseFractionOfDistinctPairs =
                json.optDouble("peakCollapseFractionOfDistinctPairs"),
            noHighlightHeadroom =
                json.optBoolean("noHighlightHeadroom"),
            mappedPeakCollapseObserved =
                json.optBoolean("mappedPeakCollapseObserved"),
            sourceSha256 = json.optString("sourceSha256"),
            scientificMasterSha256 =
                json.optString("scientificMasterSha256"),
            authorityFieldSha256 =
                json.optString("authorityFieldSha256"),
            truthNegativeStateSha256 =
                json.optString("truthNegativeStateSha256"),
            appearanceStateSha256 =
                json.optString("appearanceStateSha256"),
            auditSha256 = json.optString("auditSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            postWriteVerified =
                json.optBoolean("postWriteVerified"),
        )

        val counts = listOf(
            metrics.sampleCount,
            metrics.sourceAboveReferenceWhite,
            metrics.mappedAtPeak,
            metrics.sourceCensored,
            metrics.gamutOrDisplayClamp,
            metrics.adjacentPairs,
            metrics.sourceDistinctAdjacentPairs,
            metrics.peakCollapsedDistinctAdjacentPairs,
            metrics.brightPeakCollapsedDistinctAdjacentPairs,
        )
        val reals = listOf(
            metrics.sourceGradientSum,
            metrics.mappedGradientSum,
            metrics.collapsedSourceGradientSum,
            metrics.maxCollapsedSourceGradient,
            metrics.peakCollapseFractionOfDistinctPairs,
        )
        val digests = listOf(
            metrics.sourceSha256,
            metrics.scientificMasterSha256,
            metrics.authorityFieldSha256,
            metrics.truthNegativeStateSha256,
            metrics.appearanceStateSha256,
            metrics.auditSha256,
            metrics.jsonSha256,
        )

        if (!metrics.postWriteVerified ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.sampleCount !=
                metrics.width.toLong() * metrics.height.toLong() ||
            counts.any { it < 0L } ||
            metrics.sourceAboveReferenceWhite > metrics.sampleCount ||
            metrics.mappedAtPeak > metrics.sampleCount ||
            metrics.sourceCensored > metrics.sampleCount ||
            metrics.gamutOrDisplayClamp > metrics.sampleCount ||
            metrics.sourceDistinctAdjacentPairs > metrics.adjacentPairs ||
            metrics.peakCollapsedDistinctAdjacentPairs >
                metrics.sourceDistinctAdjacentPairs ||
            metrics.brightPeakCollapsedDistinctAdjacentPairs >
                metrics.peakCollapsedDistinctAdjacentPairs ||
            reals.any { !it.isFinite() || it < 0.0 } ||
            metrics.peakCollapseFractionOfDistinctPairs > 1.0 ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeAppearanceHighlightDetailResult.Failed(
                "Fail-closed: Appearance Highlight Detail v0.1 contract mismatch.",
            )
        }

        return TruthNegativeAppearanceHighlightDetailResult.Success(metrics)
    }
}

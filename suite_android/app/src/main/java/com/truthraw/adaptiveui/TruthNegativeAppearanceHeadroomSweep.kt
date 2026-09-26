package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import org.json.JSONObject

private const val APPEARANCE_HEADROOM_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val APPEARANCE_HEADROOM_MAX_LOGICAL_BYTES = 64 * 1024 * 1024
private const val APPEARANCE_HEADROOM_MAX_EDGE = 192

object TruthNegativeAppearanceHeadroomSweepBridge {
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

data class TruthNegativeAppearanceHeadroomVariantMetrics(
    val id: String,
    val referenceWhiteNits: Double,
    val peakNits: Double,
    val mappedAtPeak: Long,
    val collapsedPairs: Long,
    val collapseFraction: Double,
    val collapseReductionVsBaseline: Double,
    val gradientRetention: Double,
    val belowKneeSamples: Long,
    val belowKneeChanged: Long,
    val gamutOrDisplayClamp: Long,
    val sourceCensored: Long,
)

data class TruthNegativeAppearanceHeadroomSweepMetrics(
    val width: Int,
    val height: Int,
    val fileBytes: Long,
    val sourceSha256: String,
    val scientificMasterSha256: String,
    val authorityFieldSha256: String,
    val truthNegativeStateSha256: String,
    val sweepSha256: String,
    val jsonSha256: String,
    val variants: List<TruthNegativeAppearanceHeadroomVariantMetrics>,
    val postWriteVerified: Boolean,
    val automaticWinnerSelected: Boolean,
)

sealed interface TruthNegativeAppearanceHeadroomSweepResult {
    data class Success(
        val metrics: TruthNegativeAppearanceHeadroomSweepMetrics,
    ) : TruthNegativeAppearanceHeadroomSweepResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeAppearanceHeadroomSweepResult
}

object TruthNegativeAppearanceHeadroomSweepExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeAppearanceHeadroomSweepResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                "Appearance Headroom Sweep v0.2 vereist de volledig admitted DNG-route.",
            )
        }

        val statusText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { src ->
                resolver.openFileDescriptor(destination, "rw")?.use { dst ->
                    TruthNegativeAppearanceHeadroomSweepBridge.exportAndVerify(
                        src.fd,
                        dst.fd,
                        APPEARANCE_HEADROOM_MAX_EDGE,
                        APPEARANCE_HEADROOM_MAX_SOURCE_BYTES,
                        APPEARANCE_HEADROOM_MAX_LOGICAL_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeAppearanceHeadroomSweepResult.Failed(
            "Appearance Headroom Sweep v0.2 kon niet worden geschreven.",
        )

        val json = runCatching { JSONObject(statusText) }.getOrNull()
            ?: return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                "Appearance Headroom Sweep v0.2 bridge gaf geen geldig statuspakket.",
            )

        val status = json.optInt("status", -999)
        if (status != 0) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                json.optString(
                    "message",
                    "Appearance Headroom Sweep v0.2 status $status",
                ),
            )
        }

        val variantArray = json.optJSONArray("variants")
            ?: return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                "Appearance Headroom Sweep v0.2 mist variantresultaten.",
            )
        val variants = ArrayList<TruthNegativeAppearanceHeadroomVariantMetrics>()
        for (i in 0 until variantArray.length()) {
            val v = variantArray.optJSONObject(i)
                ?: return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                    "Appearance Headroom Sweep v0.2 bevat een ongeldig variantrecord.",
                )
            variants += TruthNegativeAppearanceHeadroomVariantMetrics(
                id = v.optString("id"),
                referenceWhiteNits = v.optDouble("referenceWhiteNits"),
                peakNits = v.optDouble("peakNits"),
                mappedAtPeak = v.optLong("mappedAtPeak"),
                collapsedPairs = v.optLong("collapsedPairs"),
                collapseFraction = v.optDouble("collapseFraction"),
                collapseReductionVsBaseline =
                    v.optDouble("collapseReductionVsBaseline"),
                gradientRetention = v.optDouble("gradientRetention"),
                belowKneeSamples = v.optLong("belowKneeSamples"),
                belowKneeChanged = v.optLong("belowKneeChanged"),
                gamutOrDisplayClamp = v.optLong("gamutOrDisplayClamp"),
                sourceCensored = v.optLong("sourceCensored"),
            )
        }

        val metrics = TruthNegativeAppearanceHeadroomSweepMetrics(
            width = json.optInt("width"),
            height = json.optInt("height"),
            fileBytes = json.optLong("fileBytes"),
            sourceSha256 = json.optString("sourceSha256"),
            scientificMasterSha256 =
                json.optString("scientificMasterSha256"),
            authorityFieldSha256 =
                json.optString("authorityFieldSha256"),
            truthNegativeStateSha256 =
                json.optString("truthNegativeStateSha256"),
            sweepSha256 = json.optString("sweepSha256"),
            jsonSha256 = json.optString("jsonSha256"),
            variants = variants,
            postWriteVerified = json.optBoolean("postWriteVerified"),
            automaticWinnerSelected =
                json.optBoolean("automaticWinnerSelected"),
        )

        val digests = listOf(
            metrics.sourceSha256,
            metrics.scientificMasterSha256,
            metrics.authorityFieldSha256,
            metrics.truthNegativeStateSha256,
            metrics.sweepSha256,
            metrics.jsonSha256,
        )
        val variantIds = metrics.variants.map { it.id }
        val expectedIds = listOf(
            "baseline_100_100",
            "shoulder_90_100",
            "shoulder_80_100",
            "shoulder_70_100",
        )
        val invalidVariant = metrics.variants.any { v ->
            v.id.isBlank() ||
                !v.referenceWhiteNits.isFinite() ||
                !v.peakNits.isFinite() ||
                !v.collapseFraction.isFinite() ||
                !v.collapseReductionVsBaseline.isFinite() ||
                !v.gradientRetention.isFinite() ||
                v.referenceWhiteNits <= 0.0 ||
                v.peakNits != 100.0 ||
                v.referenceWhiteNits > v.peakNits ||
                v.mappedAtPeak < 0L ||
                v.collapsedPairs < 0L ||
                v.belowKneeSamples < 0L ||
                v.belowKneeChanged < 0L ||
                v.belowKneeChanged > v.belowKneeSamples ||
                v.gamutOrDisplayClamp < 0L ||
                v.sourceCensored < 0L ||
                v.collapseFraction < 0.0 ||
                v.collapseFraction > 1.0 ||
                v.gradientRetention < 0.0
        }

        if (!metrics.postWriteVerified ||
            metrics.automaticWinnerSelected ||
            metrics.width <= 0 ||
            metrics.height <= 0 ||
            metrics.fileBytes <= 0L ||
            metrics.variants.size != 4 ||
            variantIds != expectedIds ||
            invalidVariant ||
            digests.any {
                it.length != 64 ||
                    it.any { ch -> ch !in "0123456789abcdef" }
            }
        ) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeAppearanceHeadroomSweepResult.Failed(
                "Fail-closed: Appearance Headroom Sweep v0.2 contract mismatch.",
            )
        }

        return TruthNegativeAppearanceHeadroomSweepResult.Success(metrics)
    }
}

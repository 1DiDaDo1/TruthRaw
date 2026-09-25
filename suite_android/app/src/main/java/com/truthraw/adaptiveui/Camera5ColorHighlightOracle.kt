package com.truthraw.adaptiveui

import android.content.ContentResolver
import org.json.JSONArray
import org.json.JSONObject
import java.io.FileInputStream

private const val C5_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val C5_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object Camera5ColorHighlightNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun runOracle(
        sourceFd: Int,
        asShotR: Double,
        asShotG: Double,
        asShotB: Double,
        asShotKnown: Boolean,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): String
}

data class Camera5ColorHighlightReport(
    val firstFailureStage: String,
    val sampleCount: Long,
    val candidates: Long,
    val censoredFraction: Double,
    val metadataNeutralMismatch: Boolean,
    val metadataNeutralLogError: Double,
    val lowExposureGreenBias: Double,
    val exposureGreenDrift: Double,
    val appearanceDisplayDrift: Boolean,
    val sourceCensoringDominant: Boolean,
    val colorBindingBiasDetected: Boolean,
    val remosaicState: String,
    val asShotNeutralKnown: Boolean,
    val asShotNeutral: Triple<Double, Double, Double>,
    val empiricalNeutralKnown: Boolean,
    val empiricalNeutral: Triple<Double, Double, Double>,
    val whiteLevel: Double,
    val oracleSha256: String,
)

sealed interface Camera5ColorHighlightResult {
    data class Ready(
        val report: Camera5ColorHighlightReport,
    ) : Camera5ColorHighlightResult

    data class Failed(
        val reason: String,
    ) : Camera5ColorHighlightResult
}

object Camera5ColorHighlightOracleLoader {
    fun run(
        resolver: ContentResolver,
        job: RawJob,
    ): Camera5ColorHighlightResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return Camera5ColorHighlightResult.Failed(
                "Camera-5 Color/Highlight Oracle vereist een admitted DNG.",
            )
        }

        val neutral = readAsShotNeutral(resolver, job)
        val jsonText = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { pfd ->
                Camera5ColorHighlightNativeBridge.runOracle(
                    pfd.fd,
                    neutral?.first ?: 1.0,
                    neutral?.second ?: 1.0,
                    neutral?.third ?: 1.0,
                    neutral != null,
                    C5_MAX_SOURCE_BYTES,
                    C5_MAX_LOGICAL_BYTES,
                )
            }
        } catch (error: Throwable) {
            null
        } ?: return Camera5ColorHighlightResult.Failed(
            "Camera-5 oracle kon de bron niet analyseren.",
        )

        val json = runCatching { JSONObject(jsonText) }.getOrNull()
            ?: return Camera5ColorHighlightResult.Failed(
                "Camera-5 oracle gaf geen geldig resultaat.",
            )
        val status = json.optInt("status", -999)
        if (status != 0) {
            return Camera5ColorHighlightResult.Failed(
                json.optString(
                    "message",
                    "Camera-5 oracle native status " + status,
                ),
            )
        }

        fun triple(name: String): Triple<Double, Double, Double> {
            val a = json.optJSONArray(name) ?: JSONArray()
            return Triple(
                a.optDouble(0, 0.0),
                a.optDouble(1, 0.0),
                a.optDouble(2, 0.0),
            )
        }

        return Camera5ColorHighlightResult.Ready(
            Camera5ColorHighlightReport(
                firstFailureStage =
                    json.optString("firstFailureStage", "UNRESOLVED"),
                sampleCount = json.optLong("sampleCount"),
                candidates =
                    json.optLong("apparentWhiteHighlightCandidates"),
                censoredFraction =
                    json.optDouble("censoredChannelFraction"),
                metadataNeutralMismatch =
                    json.optBoolean("metadataNeutralMismatch"),
                metadataNeutralLogError =
                    json.optDouble("metadataNeutralLogError"),
                lowExposureGreenBias =
                    json.optDouble("lowExposureGreenBias"),
                exposureGreenDrift =
                    json.optDouble("exposureGreenDrift"),
                appearanceDisplayDrift =
                    json.optBoolean("appearanceDisplayDrift"),
                sourceCensoringDominant =
                    json.optBoolean("sourceCensoringDominant"),
                colorBindingBiasDetected =
                    json.optBoolean("colorBindingBiasDetected"),
                remosaicState =
                    json.optString("remosaicState", "UNKNOWN"),
                asShotNeutralKnown =
                    json.optBoolean("asShotNeutralKnown"),
                asShotNeutral = triple("asShotNeutral"),
                empiricalNeutralKnown =
                    json.optBoolean("empiricalNeutralKnown"),
                empiricalNeutral =
                    triple("empiricalCameraNeutral"),
                whiteLevel = json.optDouble("whiteLevel"),
                oracleSha256 = json.optString("oracleSha256"),
            ),
        )
    }

    private fun readAsShotNeutral(
        resolver: ContentResolver,
        job: RawJob,
    ): Triple<Double, Double, Double>? {
        return runCatching {
            val pfd = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return@runCatching null
            pfd.use { descriptor ->
                FileInputStream(descriptor.fileDescriptor).channel.use { channel ->
                    val parsed = DngContainerMetadataParser.parse(
                        channel,
                        channel.size(),
                    )
                    val ifds = parsed.getJSONArray("ifds")
                    for (i in 0 until ifds.length()) {
                        val entries =
                            ifds.getJSONObject(i).getJSONArray("entries")
                        for (j in 0 until entries.length()) {
                            val entry = entries.getJSONObject(j)
                            if (entry.optString("name") != "AsShotNeutral") {
                                continue
                            }
                            val values =
                                entry.optJSONArray("value") ?: continue
                            if (values.length() != 3) continue
                            val r = numeric(values.opt(0)) ?: continue
                            val g = numeric(values.opt(1)) ?: continue
                            val b = numeric(values.opt(2)) ?: continue
                            if (r > 0.0 && g > 0.0 && b > 0.0) {
                                return@runCatching Triple(r, g, b)
                            }
                        }
                    }
                    null
                }
            }
        }.getOrNull()
    }

    private fun numeric(value: Any?): Double? = when (value) {
        is Number -> value.toDouble()
        is JSONObject ->
            if (value.has("value") && !value.isNull("value")) {
                value.optDouble("value").takeIf { it.isFinite() }
            } else {
                null
            }
        else -> null
    }
}

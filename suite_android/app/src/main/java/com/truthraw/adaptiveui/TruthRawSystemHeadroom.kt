package com.truthraw.adaptiveui

import android.os.SystemClock
import org.json.JSONObject
import kotlin.math.max

object TruthRawSystemHeadroomNativeBridge {
    init { System.loadLibrary("truthraw_ui_preview_bridge") }
    external fun probeJson(): String
}

data class TruthRawSystemHeadroom(
    val apiSymbolsAvailable: Boolean,
    val cpuSupported: Boolean,
    val gpuSupported: Boolean,
    val cpuTemporarilyUnavailable: Boolean,
    val gpuTemporarilyUnavailable: Boolean,
    val cpuHeadroom: Float?,
    val gpuHeadroom: Float?,
    val cpuMinPollIntervalMs: Long,
    val gpuMinPollIntervalMs: Long,
    val changesScientificAuthority: Boolean,
) {
    val summary: String
        get() = buildString {
            append("ADPF resource headroom · API symbols=")
            append(apiSymbolsAvailable)
            append("\nCPU=")
            append(cpuHeadroom ?: if (cpuSupported) "temporarily unavailable" else "unsupported")
            append("% · GPU=")
            append(gpuHeadroom ?: if (gpuSupported) "temporarily unavailable" else "unsupported")
            append("%")
            append("\nmin poll CPU/GPU=")
            append(cpuMinPollIntervalMs)
            append("/")
            append(gpuMinPollIntervalMs)
            append(" ms · authority changed=")
            append(changesScientificAuthority)
        }
}

object TruthRawSystemHeadroomProbe {
    @Volatile
    private var cached: TruthRawSystemHeadroom? = null

    @Volatile
    private var cachedAtElapsedMs: Long = 0L

    @Synchronized
    fun sample(force: Boolean = false): Result<TruthRawSystemHeadroom> = runCatching {
        val now = SystemClock.elapsedRealtime()
        val previous = cached
        if (!force && previous != null) {
            val minimum = listOf(
                previous.cpuMinPollIntervalMs,
                previous.gpuMinPollIntervalMs,
            ).filter { it > 0L }.maxOrNull() ?: 1000L
            if (now - cachedAtElapsedMs < max(250L, minimum)) {
                return@runCatching previous
            }
        }

        val json = JSONObject(TruthRawSystemHeadroomNativeBridge.probeJson())
        require(json.getString("schema") == "TruthRawSystemHeadroom/0.1")
        val parsed = TruthRawSystemHeadroom(
            apiSymbolsAvailable = json.getBoolean("api_symbols_available"),
            cpuSupported = json.getBoolean("cpu_supported"),
            gpuSupported = json.getBoolean("gpu_supported"),
            cpuTemporarilyUnavailable =
                json.getBoolean("cpu_temporarily_unavailable"),
            gpuTemporarilyUnavailable =
                json.getBoolean("gpu_temporarily_unavailable"),
            cpuHeadroom = json.getDouble("cpu_headroom").toFloat()
                .takeIf { it in 0f..100f },
            gpuHeadroom = json.getDouble("gpu_headroom").toFloat()
                .takeIf { it in 0f..100f },
            cpuMinPollIntervalMs = json.getLong("cpu_min_poll_interval_ms"),
            gpuMinPollIntervalMs = json.getLong("gpu_min_poll_interval_ms"),
            changesScientificAuthority =
                json.getBoolean("changes_scientific_authority"),
        )
        require(!parsed.changesScientificAuthority)
        cached = parsed
        cachedAtElapsedMs = now
        parsed
    }
}

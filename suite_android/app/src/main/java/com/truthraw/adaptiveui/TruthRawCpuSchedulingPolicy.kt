package com.truthraw.adaptiveui

import android.app.ActivityManager
import android.content.Context
import android.os.Build
import android.os.PowerManager
import kotlin.math.ceil
import kotlin.math.max
import kotlin.math.min

enum class TruthRawCpuWorkload {
    EXACT_SCIENTIFIC,
    APPEARANCE,
    PRESENTATION,
    IO_HEAVY,
}

data class TruthRawCpuWorkerPlan(
    val onlineCores: Int,
    val recommendedWorkers: Int,
    val maxWorkersByMemory: Int,
    val thermalStatus: Int,
    val thermalHeadroom: Float?,
    val powerSaveMode: Boolean,
    val adpfAvailable: Boolean,
    val workload: TruthRawCpuWorkload,
    val reason: String,
)

object TruthRawCpuSchedulingPolicyV01 {
    /**
     * Conservative dynamic policy for sustained RAW work.
     *
     * This does not pin threads to cores. Android's scheduler remains free to
     * place work on big/mid/little cores. ADPF may later be attached to the
     * actual long-lived worker threads.
     */
    fun plan(
        context: Context,
        workload: TruthRawCpuWorkload,
        bytesPerWorkerEstimate: Long,
        reserveBytes: Long = 96L * 1024L * 1024L,
    ): TruthRawCpuWorkerPlan {
        val app = context.applicationContext
        val power = app.getSystemService(PowerManager::class.java)
        val activity = app.getSystemService(ActivityManager::class.java)

        val online = Runtime.getRuntime().availableProcessors().coerceAtLeast(1)
        val memoryClassBytes =
            (activity?.memoryClass?.toLong() ?: 256L) * 1024L * 1024L
        val usableBytes = (memoryClassBytes - reserveBytes).coerceAtLeast(
            32L * 1024L * 1024L,
        )
        val perWorker = bytesPerWorkerEstimate.coerceAtLeast(1L)
        val memoryWorkers =
            (usableBytes / perWorker).coerceIn(1L, online.toLong()).toInt()

        val powerSave = power?.isPowerSaveMode == true
        val thermalStatus = power?.currentThermalStatus ?: PowerManager.THERMAL_STATUS_NONE
        val headroom = runCatching {
            power?.getThermalHeadroom(10)?.takeIf { !it.isNaN() }
        }.getOrNull()

        // Keep one logical CPU free on multicore phones for Android/UI/I/O.
        var cpuTarget = if (online >= 4) online - 1 else online

        // Long exact/scientific work tends to be sustained rather than bursty.
        // Appearance/presentation can use a slightly more aggressive ceiling.
        cpuTarget = when (workload) {
            TruthRawCpuWorkload.EXACT_SCIENTIFIC -> min(cpuTarget, 8)
            TruthRawCpuWorkload.APPEARANCE,
            TruthRawCpuWorkload.PRESENTATION -> min(online, 8)
            TruthRawCpuWorkload.IO_HEAVY -> min(cpuTarget, 4)
        }

        if (powerSave) {
            cpuTarget = max(1, ceil(cpuTarget * 0.60).toInt())
        }

        cpuTarget = when {
            thermalStatus >= PowerManager.THERMAL_STATUS_SEVERE ->
                max(1, ceil(cpuTarget * 0.40).toInt())
            thermalStatus >= PowerManager.THERMAL_STATUS_MODERATE ->
                max(1, ceil(cpuTarget * 0.65).toInt())
            thermalStatus >= PowerManager.THERMAL_STATUS_LIGHT ->
                max(1, ceil(cpuTarget * 0.82).toInt())
            else -> cpuTarget
        }

        // Headroom is advisory and may be temporarily unavailable (NaN).
        // Values approaching 1 indicate less room before severe throttling.
        if (headroom != null) {
            cpuTarget = when {
                headroom >= 0.90f -> max(1, ceil(cpuTarget * 0.50).toInt())
                headroom >= 0.75f -> max(1, ceil(cpuTarget * 0.72).toInt())
                else -> cpuTarget
            }
        }

        val workers = min(cpuTarget.coerceAtLeast(1), memoryWorkers)
        val adpf = Build.VERSION.SDK_INT >= Build.VERSION_CODES.S

        return TruthRawCpuWorkerPlan(
            onlineCores = online,
            recommendedWorkers = workers,
            maxWorkersByMemory = memoryWorkers,
            thermalStatus = thermalStatus,
            thermalHeadroom = headroom,
            powerSaveMode = powerSave,
            adpfAvailable = adpf,
            workload = workload,
            reason = buildString {
                append("dynamic workers=")
                append(workers)
                append("/")
                append(online)
                append(" · memory cap=")
                append(memoryWorkers)
                append(" · thermal=")
                append(thermalStatus)
                append(" · headroom=")
                append(headroom ?: "unknown")
                append(" · powerSave=")
                append(powerSave)
                append(" · ADPF=")
                append(adpf)
                append(" · no manual core affinity")
            },
        )
    }
}

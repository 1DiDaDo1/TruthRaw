package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Context
import android.os.Debug
import android.os.PowerManager
import java.security.MessageDigest
import java.util.concurrent.atomic.AtomicBoolean

private const val MULTIWORKER_MAGIC = 0x54525732 // TRW2
private const val MULTIWORKER_HEADER_INTS = 32
private const val MULTIWORKER_MAX_PREVIEW_EDGE = 384
private const val MULTIWORKER_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val MULTIWORKER_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
private const val MULTIWORKER_SAMPLE_INTERVAL_MS = 50L

/**
 * Research-only JNI surface. Normal UI/scientific preview paths do not call this object.
 * workers=1 deliberately uses the validated streaming-v0.1 reference; workers=2/4 use
 * the v0.2 bounded multi-worker candidate.
 */
object NativeMultiWorkerEmpiricalBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun probePreview(
        fd: Int,
        workers: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray
}

data class MultiWorkerNativeMetrics(
    val requestedWorkers: Int,
    val effectiveWorkers: Int,
    val previewWidth: Int,
    val previewHeight: Int,
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceResidentUpperBoundBytes: Int,
    val logicalResidentUpperBoundBytes: Int,
    val rawPayloadBytesRead: Int,
    val metadataBytesRead: Int,
    val tileReadCalls: Int,
    val tilesProcessedPass1: Int,
    val tilesProcessedPass2: Int,
    val queueDepth: Int,
    val maxReadyPackets: Int,
    val fullRawMaterialized: Boolean,
    val fullFileMaterialized: Boolean,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val clippedCount: Int,
    val stage2Over1Count: Int,
    val halfGainSamplesObserved: Int,
    val orderedCommit: Boolean,
    val usedForwardMatrix: Boolean,
    val cameraCalibrationApplied: Boolean,
    val colorAuthorityCode: Int,
    val nativePipelineWallMs: Double,
    val nativeProcessCpuMs: Double,
    val sourceBoundAppearanceReleaseAllowed: Boolean,
    val scientificClaimAllowed: Boolean,
)

data class MultiWorkerRuntimeMetrics(
    val pssBeforeKb: Int,
    val pssPeakKb: Int,
    val pssAfterKb: Int,
    val thermalStart: Int,
    val thermalPeak: Int,
    val thermalEnd: Int,
)

data class MultiWorkerTrial(
    val workers: Int,
    val statusCode: Int,
    val metrics: MultiWorkerNativeMetrics?,
    val runtime: MultiWorkerRuntimeMetrics,
    val previewSha256: String?,
    val exactPreviewMatchesWorker1: Boolean?,
    val transportError: String? = null,
)

data class MultiWorkerComparison(
    val trials: List<MultiWorkerTrial>,
    val allSuccessfulCandidatesMatchWorker1Exactly: Boolean,
)

object MultiWorkerEmpiricalRunner {
    /**
     * Default physical-device gate. Keep the sequence short for the first admission run;
     * repeated/reordered trials can be supplied later for performance/thermal statistics.
     */
    fun runComparison(
        context: Context,
        resolver: ContentResolver,
        job: RawJob,
        workerSequence: IntArray = intArrayOf(1, 2, 4),
    ): MultiWorkerComparison {
        require(workerSequence.isNotEmpty())
        require(workerSequence.all { it == 1 || it == 2 || it == 4 })
        require(workerSequence.first() == 1) { "workerSequence must start with the one-worker reference" }

        var referencePixels: IntArray? = null
        val trials = ArrayList<MultiWorkerTrial>(workerSequence.size)

        for (workers in workerSequence) {
            val sampler = MultiWorkerRuntimeSampler(context)
            sampler.start()
            val decoded = try {
                val descriptor = resolver.openFileDescriptor(job.source.uri, "r")
                    ?: throw IllegalStateException("Documentprovider gaf geen file descriptor.")
                val packet = descriptor.use { pfd ->
                    NativeMultiWorkerEmpiricalBridge.probePreview(
                        pfd.fd,
                        workers,
                        MULTIWORKER_MAX_PREVIEW_EDGE,
                        MULTIWORKER_MAX_SOURCE_RESIDENT_BYTES,
                        MULTIWORKER_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
                decodePacket(packet)
            } catch (error: Throwable) {
                DecodedMultiWorkerPacket(
                    statusCode = Int.MIN_VALUE,
                    metrics = null,
                    pixels = null,
                    error = error.message ?: error.javaClass.simpleName,
                )
            }
            val runtime = sampler.stop()

            val exactMatch = when {
                decoded.statusCode != 0 || decoded.pixels == null -> null
                referencePixels == null -> true
                else -> referencePixels!!.contentEquals(decoded.pixels)
            }
            if (workers == 1 && decoded.statusCode == 0 && decoded.pixels != null && referencePixels == null) {
                referencePixels = decoded.pixels
            }

            trials += MultiWorkerTrial(
                workers = workers,
                statusCode = decoded.statusCode,
                metrics = decoded.metrics,
                runtime = runtime,
                previewSha256 = decoded.pixels?.let(::sha256Pixels),
                exactPreviewMatchesWorker1 = exactMatch,
                transportError = decoded.error,
            )
        }

        val successfulCandidates = trials.filter { it.workers > 1 && it.statusCode == 0 }
        val exact = referencePixels != null && successfulCandidates.isNotEmpty() &&
            successfulCandidates.all { it.exactPreviewMatchesWorker1 == true }
        return MultiWorkerComparison(trials, exact)
    }

    private data class DecodedMultiWorkerPacket(
        val statusCode: Int,
        val metrics: MultiWorkerNativeMetrics?,
        val pixels: IntArray?,
        val error: String?,
    )

    private fun decodePacket(packet: IntArray): DecodedMultiWorkerPacket {
        if (packet.size < MULTIWORKER_HEADER_INTS || packet[0] != MULTIWORKER_MAGIC) {
            return DecodedMultiWorkerPacket(Int.MIN_VALUE, null, null, "Ongeldig TRW2 native pakket.")
        }
        val status = packet[1]
        if (status != 0) return DecodedMultiWorkerPacket(status, null, null, null)

        val width = packet[4]
        val height = packet[5]
        if (width <= 0 || height <= 0 || width > MULTIWORKER_MAX_PREVIEW_EDGE || height > MULTIWORKER_MAX_PREVIEW_EDGE) {
            return DecodedMultiWorkerPacket(Int.MIN_VALUE, null, null, "TRW2 preview-afmetingen buiten contract.")
        }
        val pixelCount = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return DecodedMultiWorkerPacket(Int.MIN_VALUE, null, null, "TRW2 preview-afmetingen overflowden.")
        }
        if (packet.size != MULTIWORKER_HEADER_INTS + pixelCount) {
            return DecodedMultiWorkerPacket(Int.MIN_VALUE, null, null, "TRW2 payloadlengte buiten contract.")
        }

        val metrics = MultiWorkerNativeMetrics(
            requestedWorkers = packet[2],
            effectiveWorkers = packet[3],
            previewWidth = width,
            previewHeight = height,
            sourceWidth = packet[6],
            sourceHeight = packet[7],
            sourceResidentUpperBoundBytes = packet[8],
            logicalResidentUpperBoundBytes = packet[9],
            rawPayloadBytesRead = packet[10],
            metadataBytesRead = packet[11],
            tileReadCalls = packet[12],
            tilesProcessedPass1 = packet[13],
            tilesProcessedPass2 = packet[14],
            queueDepth = packet[15],
            maxReadyPackets = packet[16],
            fullRawMaterialized = packet[17] != 0,
            fullFileMaterialized = packet[18] != 0,
            physicalFrameCount = packet[19],
            independentEvidenceCount = packet[20],
            clippedCount = packet[21],
            stage2Over1Count = packet[22],
            halfGainSamplesObserved = packet[23],
            orderedCommit = packet[24] != 0,
            usedForwardMatrix = packet[25] != 0,
            cameraCalibrationApplied = packet[26] != 0,
            colorAuthorityCode = packet[27],
            nativePipelineWallMs = packet[28] / 1000.0,
            nativeProcessCpuMs = packet[29] / 1000.0,
            sourceBoundAppearanceReleaseAllowed = packet[30] != 0,
            scientificClaimAllowed = packet[31] != 0,
        )

        val invalid = metrics.requestedWorkers !in setOf(1, 2, 4) ||
            metrics.effectiveWorkers <= 0 ||
            metrics.physicalFrameCount != 1 ||
            metrics.independentEvidenceCount != 1 ||
            metrics.fullRawMaterialized ||
            metrics.fullFileMaterialized ||
            !metrics.orderedCommit ||
            !metrics.sourceBoundAppearanceReleaseAllowed ||
            metrics.scientificClaimAllowed ||
            metrics.logicalResidentUpperBoundBytes <= 0 ||
            metrics.logicalResidentUpperBoundBytes > MULTIWORKER_MAX_LOGICAL_RESIDENT_BYTES ||
            metrics.tilesProcessedPass1 <= 0 ||
            metrics.tilesProcessedPass2 <= 0
        if (invalid) {
            return DecodedMultiWorkerPacket(Int.MIN_VALUE, null, null, "TRW2 fail-closed invariant violation.")
        }

        return DecodedMultiWorkerPacket(
            statusCode = 0,
            metrics = metrics,
            pixels = packet.copyOfRange(MULTIWORKER_HEADER_INTS, packet.size),
            error = null,
        )
    }

    private fun sha256Pixels(pixels: IntArray): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = ByteArray(4)
        for (pixel in pixels) {
            bytes[0] = (pixel ushr 24).toByte()
            bytes[1] = (pixel ushr 16).toByte()
            bytes[2] = (pixel ushr 8).toByte()
            bytes[3] = pixel.toByte()
            digest.update(bytes)
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
}

private class MultiWorkerRuntimeSampler(context: Context) {
    private val powerManager = context.getSystemService(PowerManager::class.java)
    private val running = AtomicBoolean(false)
    private var thread: Thread? = null
    private var pssBeforeKb = 0
    @Volatile private var pssPeakKb = 0
    private var thermalStart = PowerManager.THERMAL_STATUS_NONE
    @Volatile private var thermalPeak = PowerManager.THERMAL_STATUS_NONE

    fun start() {
        pssBeforeKb = currentPssKb()
        pssPeakKb = pssBeforeKb
        thermalStart = powerManager.currentThermalStatus
        thermalPeak = thermalStart
        running.set(true)
        thread = Thread({
            while (running.get()) {
                pssPeakKb = maxOf(pssPeakKb, currentPssKb())
                thermalPeak = maxOf(thermalPeak, powerManager.currentThermalStatus)
                try {
                    Thread.sleep(MULTIWORKER_SAMPLE_INTERVAL_MS)
                } catch (_: InterruptedException) {
                    break
                }
            }
        }, "truthraw-multiworker-runtime-sampler").apply {
            isDaemon = true
            start()
        }
    }

    fun stop(): MultiWorkerRuntimeMetrics {
        running.set(false)
        thread?.interrupt()
        try {
            thread?.join(250)
        } catch (_: InterruptedException) {
            Thread.currentThread().interrupt()
        }
        val pssAfter = currentPssKb()
        val thermalEnd = powerManager.currentThermalStatus
        pssPeakKb = maxOf(pssPeakKb, pssAfter)
        thermalPeak = maxOf(thermalPeak, thermalEnd)
        return MultiWorkerRuntimeMetrics(
            pssBeforeKb = pssBeforeKb,
            pssPeakKb = pssPeakKb,
            pssAfterKb = pssAfter,
            thermalStart = thermalStart,
            thermalPeak = thermalPeak,
            thermalEnd = thermalEnd,
        )
    }

    private fun currentPssKb(): Int {
        val info = Debug.MemoryInfo()
        Debug.getMemoryInfo(info)
        return info.totalPss
    }
}

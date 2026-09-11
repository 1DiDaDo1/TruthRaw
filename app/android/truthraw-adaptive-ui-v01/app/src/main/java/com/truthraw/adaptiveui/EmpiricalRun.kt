package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Context
import android.os.Build
import android.os.Debug
import android.os.PowerManager
import android.os.SystemClock
import android.view.Choreographer
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.ceil

private const val EMPIRICAL_MAGIC = 0x54524531
private const val EMPIRICAL_PACKET_INTS = 32
private const val VALIDATED_SCIENTIFIC_ROUTE_SHA = "42b49ba16a6c5a0d2d6dbc407330acde3e161a46"
private const val EMPIRICAL_SCHEMA = "TRUTHRAW_ANDROID_HONOR_EMPIRICAL_V0_1"

object NativeEmpiricalBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    /** Read-only audit probe. It never feeds values into reconstruction, color, TruthRange, or release policy. */
    external fun probeSourceColor(fd: Int): IntArray
}

data class SourceColorProbe(
    val statusCode: Int,
    val sourceSha256: String?,
    val sourceByteLength: Long?,
    val metadataBytesRead: Int,
    val ifdEntriesVisited: Int,
    val parserWorkspacePeakBytes: Int,
    val delegatedSingleIlluminantV01: Boolean,
    val dualIlluminantUsed: Boolean,
    val thirdCalibrationSeen: Boolean,
    val usedForwardMatrix: Boolean,
    val usedSingleForwardMatrixAcrossTemperatures: Boolean,
    val cameraCalibrationPresent: Boolean,
    val cameraCalibrationSignatureMatched: Boolean,
    val cameraCalibrationApplied: Boolean,
    val calibrationIlluminant1: Int,
    val calibrationIlluminant2: Int,
    val resolvedWhiteTemperatureK: Double?,
    val interpolationWeightLow: Double?,
    val neutralSolveIterations: Int,
    val colorAuthorityCode: Int,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val hashWorkspacePeakBytes: Int,
    val probeWallMs: Double,
    val transportError: String? = null,
) {
    val metadataForm: String
        get() = when {
            delegatedSingleIlluminantV01 -> "SINGLE_ILLUMINANT_V0_1_DELEGATED"
            dualIlluminantUsed -> "DUAL_ILLUMINANT_V0_2"
            thirdCalibrationSeen -> "TRIPLE_OR_GREATER_UNSUPPORTED"
            statusCode != 0 -> "PRODUCER_OR_BINDING_FAILED"
            else -> "UNRESOLVED"
        }

    fun stableFingerprint(): String = listOf(
        statusCode,
        sourceSha256,
        sourceByteLength,
        delegatedSingleIlluminantV01,
        dualIlluminantUsed,
        thirdCalibrationSeen,
        usedForwardMatrix,
        usedSingleForwardMatrixAcrossTemperatures,
        cameraCalibrationPresent,
        cameraCalibrationSignatureMatched,
        cameraCalibrationApplied,
        calibrationIlluminant1,
        calibrationIlluminant2,
        resolvedWhiteTemperatureK,
        interpolationWeightLow,
        neutralSolveIterations,
        colorAuthorityCode,
        physicalFrameCount,
        independentEvidenceCount,
    ).joinToString("|")
}

data class RuntimeObservations(
    val pipelineWallMs: Double,
    val workerCpuMs: Double,
    val pssBeforeKb: Int,
    val pssPeakKb: Int,
    val pssAfterKb: Int,
    val thermalStart: Int,
    val thermalPeak: Int,
    val thermalEnd: Int,
)

data class UiFramePacingMetrics(
    val intervalCount: Int,
    val p50Ms: Double?,
    val p95Ms: Double?,
    val maxMs: Double?,
)

data class EmpiricalRunAudit(
    val generatedAtUtc: String,
    val validatedScientificRouteSha: String,
    val installedApkSha256: String?,
    val preProbe: SourceColorProbe,
    val postProbe: SourceColorProbe,
    val sourceStableAcrossHarness: Boolean?,
    val colorProbeStableAcrossHarness: Boolean?,
    val runtime: RuntimeObservations,
    val framePacing: UiFramePacingMetrics? = null,
)

data class EmpiricalPreviewResult(
    val state: TilePreviewUiState,
    val audit: EmpiricalRunAudit,
)

object EmpiricalPreviewRunner {
    fun run(context: Context, resolver: ContentResolver, job: RawJob): EmpiricalPreviewResult {
        val preProbe = probe(resolver, job)
        val sampler = RuntimeSampler(context)
        sampler.start()
        val cpuStart = Debug.threadCpuTimeNanos()
        val wallStart = SystemClock.elapsedRealtimeNanos()
        var state = TilePreviewLoader.load(resolver, job)
        val wallEnd = SystemClock.elapsedRealtimeNanos()
        val cpuEnd = Debug.threadCpuTimeNanos()
        val runtimeSample = sampler.stop()
        val postProbe = probe(resolver, job)

        val sourceStable = when {
            preProbe.sourceSha256 == null || postProbe.sourceSha256 == null -> null
            else -> preProbe.sourceSha256 == postProbe.sourceSha256 &&
                preProbe.sourceByteLength == postProbe.sourceByteLength
        }
        val colorStable = when {
            sourceStable != true -> null
            else -> preProbe.stableFingerprint() == postProbe.stableFingerprint()
        }

        if (state is TilePreviewUiState.Ready && sourceStable == false) {
            state.bitmap.recycle()
            state = TilePreviewUiState.Failed(
                job.id,
                "Empirische fail-closed grens: bron-SHA/byte-lengte veranderde tussen pre- en post-probe.",
            )
        } else if (state is TilePreviewUiState.Ready && colorStable == false) {
            state.bitmap.recycle()
            state = TilePreviewUiState.Failed(
                job.id,
                "Empirische fail-closed grens: kleurmetadata-audit veranderde rond dezelfde finalized route.",
            )
        }

        val runtime = RuntimeObservations(
            pipelineWallMs = nanosToMs(wallEnd - wallStart),
            workerCpuMs = nanosToMs(cpuEnd - cpuStart),
            pssBeforeKb = runtimeSample.pssBeforeKb,
            pssPeakKb = runtimeSample.pssPeakKb,
            pssAfterKb = runtimeSample.pssAfterKb,
            thermalStart = runtimeSample.thermalStart,
            thermalPeak = runtimeSample.thermalPeak,
            thermalEnd = runtimeSample.thermalEnd,
        )

        val audit = EmpiricalRunAudit(
            generatedAtUtc = Instant.now().toString(),
            validatedScientificRouteSha = VALIDATED_SCIENTIFIC_ROUTE_SHA,
            installedApkSha256 = installedApkSha256(context),
            preProbe = preProbe,
            postProbe = postProbe,
            sourceStableAcrossHarness = sourceStable,
            colorProbeStableAcrossHarness = colorStable,
            runtime = runtime,
        )
        return EmpiricalPreviewResult(state, audit)
    }

    private fun probe(resolver: ContentResolver, job: RawJob): SourceColorProbe {
        val started = SystemClock.elapsedRealtimeNanos()
        return try {
            val descriptor = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return transportProbe(
                    "Documentprovider gaf geen file descriptor voor empirical probe.",
                    nanosToMs(SystemClock.elapsedRealtimeNanos() - started),
                )
            val packet = descriptor.use { pfd -> NativeEmpiricalBridge.probeSourceColor(pfd.fd) }
            decodeProbe(packet, nanosToMs(SystemClock.elapsedRealtimeNanos() - started))
        } catch (error: Throwable) {
            transportProbe(
                error.message ?: error.javaClass.simpleName,
                nanosToMs(SystemClock.elapsedRealtimeNanos() - started),
            )
        }
    }

    private fun decodeProbe(packet: IntArray, wallMs: Double): SourceColorProbe {
        if (packet.size != EMPIRICAL_PACKET_INTS || packet[0] != EMPIRICAL_MAGIC) {
            return transportProbe("Ongeldig empirical native auditpakket.", wallMs)
        }
        val sha = if ((2..9).all { packet[it] == 0 }) null else buildString(64) {
            for (index in 2..9) append(packet[index].toUInt().toString(16).padStart(8, '0'))
        }
        val byteLength = if (sha == null) null else {
            ((packet[10].toLong() and 0xffffffffL) shl 32) or
                (packet[11].toLong() and 0xffffffffL)
        }
        val temperatureK = packet[25].takeIf { it != 0 }?.div(1000.0)
        val interpolationWeight = packet[26].takeIf { it != 0 }?.div(1_000_000_000.0)
        return SourceColorProbe(
            statusCode = packet[1],
            sourceSha256 = sha,
            sourceByteLength = byteLength,
            metadataBytesRead = packet[12],
            ifdEntriesVisited = packet[13],
            parserWorkspacePeakBytes = packet[14],
            delegatedSingleIlluminantV01 = packet[15] != 0,
            dualIlluminantUsed = packet[16] != 0,
            thirdCalibrationSeen = packet[17] != 0,
            usedForwardMatrix = packet[18] != 0,
            usedSingleForwardMatrixAcrossTemperatures = packet[19] != 0,
            cameraCalibrationPresent = packet[20] != 0,
            cameraCalibrationSignatureMatched = packet[21] != 0,
            cameraCalibrationApplied = packet[22] != 0,
            calibrationIlluminant1 = packet[23],
            calibrationIlluminant2 = packet[24],
            resolvedWhiteTemperatureK = temperatureK,
            interpolationWeightLow = interpolationWeight,
            neutralSolveIterations = packet[27],
            colorAuthorityCode = packet[28],
            physicalFrameCount = packet[29],
            independentEvidenceCount = packet[30],
            hashWorkspacePeakBytes = packet[31],
            probeWallMs = wallMs,
        )
    }

    private fun transportProbe(message: String, wallMs: Double) = SourceColorProbe(
        statusCode = Int.MIN_VALUE,
        sourceSha256 = null,
        sourceByteLength = null,
        metadataBytesRead = 0,
        ifdEntriesVisited = 0,
        parserWorkspacePeakBytes = 0,
        delegatedSingleIlluminantV01 = false,
        dualIlluminantUsed = false,
        thirdCalibrationSeen = false,
        usedForwardMatrix = false,
        usedSingleForwardMatrixAcrossTemperatures = false,
        cameraCalibrationPresent = false,
        cameraCalibrationSignatureMatched = false,
        cameraCalibrationApplied = false,
        calibrationIlluminant1 = 0,
        calibrationIlluminant2 = 0,
        resolvedWhiteTemperatureK = null,
        interpolationWeightLow = null,
        neutralSolveIterations = 0,
        colorAuthorityCode = 0,
        physicalFrameCount = 0,
        independentEvidenceCount = 0,
        hashWorkspacePeakBytes = 0,
        probeWallMs = wallMs,
        transportError = message,
    )
}

class UiFramePacingSampler {
    private val intervalsNanos = mutableListOf<Long>()
    private var running = false
    private var lastFrameNanos = 0L
    private val choreographer by lazy { Choreographer.getInstance() }
    private val callback = object : Choreographer.FrameCallback {
        override fun doFrame(frameTimeNanos: Long) {
            if (!running) return
            if (lastFrameNanos != 0L && frameTimeNanos > lastFrameNanos) {
                intervalsNanos += frameTimeNanos - lastFrameNanos
            }
            lastFrameNanos = frameTimeNanos
            choreographer.postFrameCallback(this)
        }
    }

    fun start() {
        check(!running)
        intervalsNanos.clear()
        lastFrameNanos = 0L
        running = true
        choreographer.postFrameCallback(callback)
    }

    fun stop(): UiFramePacingMetrics {
        running = false
        choreographer.removeFrameCallback(callback)
        if (intervalsNanos.isEmpty()) return UiFramePacingMetrics(0, null, null, null)
        val sorted = intervalsNanos.sorted()
        fun percentile(fraction: Double): Double {
            val index = (ceil(sorted.size * fraction).toInt() - 1).coerceIn(0, sorted.lastIndex)
            return nanosToMs(sorted[index])
        }
        return UiFramePacingMetrics(
            intervalCount = sorted.size,
            p50Ms = percentile(0.50),
            p95Ms = percentile(0.95),
            maxMs = nanosToMs(sorted.last()),
        )
    }
}

private data class RuntimeSample(
    val pssBeforeKb: Int,
    val pssPeakKb: Int,
    val pssAfterKb: Int,
    val thermalStart: Int,
    val thermalPeak: Int,
    val thermalEnd: Int,
)

private class RuntimeSampler(context: Context) {
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
                    Thread.sleep(50)
                } catch (_: InterruptedException) {
                    break
                }
            }
        }, "truthraw-empirical-runtime-sampler").apply {
            isDaemon = true
            start()
        }
    }

    fun stop(): RuntimeSample {
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
        return RuntimeSample(
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

object EmpiricalReportEncoder {
    fun toJson(context: Context, job: RawJob, state: TilePreviewUiState, audit: EmpiricalRunAudit): String {
        val packageInfo = context.packageManager.getPackageInfo(context.packageName, 0)
        val root = JSONObject()
            .put("schema", EMPIRICAL_SCHEMA)
            .put("generated_at_utc", audit.generatedAtUtc)
            .put("measurement_only", true)
            .put("changes_scientific_route", false)
            .put("validated_scientific_route_sha", audit.validatedScientificRouteSha)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("brand", Build.BRAND)
                .put("model", Build.MODEL)
                .put("device", Build.DEVICE)
                .put("android_sdk", Build.VERSION.SDK_INT)
                .put("android_release", Build.VERSION.RELEASE))
            .put("app", JSONObject()
                .put("package", context.packageName)
                .put("version_name", packageInfo.versionName ?: JSONObject.NULL)
                .put("version_code", packageInfo.longVersionCode)
                .put("installed_apk_sha256", audit.installedApkSha256 ?: JSONObject.NULL))
            .put("source", JSONObject()
                .put("display_name", job.source.displayName)
                .put("declared_size_bytes", job.source.declaredSizeBytes ?: JSONObject.NULL)
                .put("stable_across_empirical_wrapper", audit.sourceStableAcrossHarness ?: JSONObject.NULL)
                .put("color_probe_stable_across_empirical_wrapper", audit.colorProbeStableAcrossHarness ?: JSONObject.NULL)
                .put("pre_probe", probeJson(audit.preProbe))
                .put("post_probe", probeJson(audit.postProbe)))
            .put("runtime", JSONObject()
                .put("pipeline_wall_ms", audit.runtime.pipelineWallMs)
                .put("worker_cpu_ms", audit.runtime.workerCpuMs)
                .put("pss_before_kb", audit.runtime.pssBeforeKb)
                .put("pss_peak_kb", audit.runtime.pssPeakKb)
                .put("pss_after_kb", audit.runtime.pssAfterKb)
                .put("thermal_start", thermalJson(audit.runtime.thermalStart))
                .put("thermal_peak", thermalJson(audit.runtime.thermalPeak))
                .put("thermal_end", thermalJson(audit.runtime.thermalEnd)))

        val pacing = audit.framePacing
        root.put("ui_frame_pacing", if (pacing == null) JSONObject.NULL else JSONObject()
            .put("interval_count", pacing.intervalCount)
            .put("p50_ms", pacing.p50Ms ?: JSONObject.NULL)
            .put("p95_ms", pacing.p95Ms ?: JSONObject.NULL)
            .put("max_ms", pacing.maxMs ?: JSONObject.NULL))

        when (state) {
            is TilePreviewUiState.Ready -> root.put("preview", readyJson(state))
            is TilePreviewUiState.Failed -> root.put("preview", JSONObject()
                .put("outcome", "FAILED")
                .put("reason", state.reason))
            is TilePreviewUiState.Loading -> root.put("preview", JSONObject().put("outcome", "LOADING"))
            TilePreviewUiState.Idle -> root.put("preview", JSONObject().put("outcome", "IDLE"))
        }
        root.put("claim_boundary", JSONObject()
            .put("physical_frame_count_expected", 1)
            .put("independent_evidence_count_expected", 1)
            .put("source_metadata_color_is_independent_physical_calibration", false)
            .put("full_physical_color_claim_allowed_from_this_harness", false)
            .put("counterfactual_is_evidence", false))
        return root.toString(2)
    }

    private fun probeJson(probe: SourceColorProbe) = JSONObject()
        .put("status_code", probe.statusCode)
        .put("transport_error", probe.transportError ?: JSONObject.NULL)
        .put("probe_wall_ms", probe.probeWallMs)
        .put("source_sha256", probe.sourceSha256 ?: JSONObject.NULL)
        .put("source_byte_length", probe.sourceByteLength ?: JSONObject.NULL)
        .put("metadata_form", probe.metadataForm)
        .put("metadata_bytes_read", probe.metadataBytesRead)
        .put("ifd_entries_visited", probe.ifdEntriesVisited)
        .put("parser_workspace_peak_bytes", probe.parserWorkspacePeakBytes)
        .put("hash_workspace_peak_bytes", probe.hashWorkspacePeakBytes)
        .put("delegated_single_illuminant_v0_1", probe.delegatedSingleIlluminantV01)
        .put("dual_illuminant_used", probe.dualIlluminantUsed)
        .put("third_calibration_seen", probe.thirdCalibrationSeen)
        .put("used_forward_matrix", probe.usedForwardMatrix)
        .put("used_single_forward_matrix_across_temperatures", probe.usedSingleForwardMatrixAcrossTemperatures)
        .put("camera_calibration_present", probe.cameraCalibrationPresent)
        .put("camera_calibration_signature_matched", probe.cameraCalibrationSignatureMatched)
        .put("camera_calibration_applied", probe.cameraCalibrationApplied)
        .put("calibration_illuminant_1", probe.calibrationIlluminant1)
        .put("calibration_illuminant_2", probe.calibrationIlluminant2)
        .put("resolved_white_temperature_k", probe.resolvedWhiteTemperatureK ?: JSONObject.NULL)
        .put("interpolation_weight_low", probe.interpolationWeightLow ?: JSONObject.NULL)
        .put("neutral_solve_iterations", probe.neutralSolveIterations)
        .put("color_authority_code", probe.colorAuthorityCode)
        .put("physical_frame_count", probe.physicalFrameCount)
        .put("independent_evidence_count", probe.independentEvidenceCount)

    private fun readyJson(state: TilePreviewUiState.Ready): JSONObject {
        val m = state.metrics
        return JSONObject()
            .put("outcome", "READY")
            .put("preview_authority", m.previewAuthority.name)
            .put("source_width", m.sourceWidth)
            .put("source_height", m.sourceHeight)
            .put("source_resident_upper_bound_bytes", m.sourceResidentUpperBoundBytes)
            .put("logical_resident_upper_bound_bytes", m.logicalResidentUpperBoundBytes)
            .put("raw_payload_bytes_read", m.rawPayloadBytesRead)
            .put("metadata_bytes_read", m.metadataBytesRead)
            .put("tile_read_calls", m.tileReadCalls)
            .put("full_raw_materialized", m.fullRawMaterialized)
            .put("has_gain_field", m.hasGainField)
            .put("orientation", m.orientation)
            .put("source_bound_appearance_release_allowed", m.sourceBoundAppearanceReleaseAllowed)
            .put("scientific_preview_release_allowed", m.scientificPreviewReleaseAllowed)
            .put("scientific_claim_allowed", m.scientificClaimAllowed)
            .put("physical_frame_count", m.physicalFrameCount)
            .put("independent_evidence_count", m.independentEvidenceCount)
            .put("tiles_processed_pass_1", m.tilesProcessedPass1)
            .put("tiles_processed_pass_2", m.tilesProcessedPass2)
            .put("used_forward_matrix", m.usedForwardMatrix)
            .put("camera_calibration_applied", m.cameraCalibrationApplied)
    }

    private fun thermalJson(status: Int) = JSONObject()
        .put("code", status)
        .put("name", thermalName(status))
}

private fun installedApkSha256(context: Context): String? = try {
    sha256File(File(context.applicationInfo.sourceDir))
} catch (_: Throwable) {
    null
}

private fun sha256File(file: File): String {
    val digest = MessageDigest.getInstance("SHA-256")
    FileInputStream(file).use { input ->
        val buffer = ByteArray(64 * 1024)
        while (true) {
            val count = input.read(buffer)
            if (count < 0) break
            if (count > 0) digest.update(buffer, 0, count)
        }
    }
    return digest.digest().joinToString("") { "%02x".format(it.toInt() and 0xff) }
}

private fun thermalName(status: Int): String = when (status) {
    PowerManager.THERMAL_STATUS_NONE -> "NONE"
    PowerManager.THERMAL_STATUS_LIGHT -> "LIGHT"
    PowerManager.THERMAL_STATUS_MODERATE -> "MODERATE"
    PowerManager.THERMAL_STATUS_SEVERE -> "SEVERE"
    PowerManager.THERMAL_STATUS_CRITICAL -> "CRITICAL"
    PowerManager.THERMAL_STATUS_EMERGENCY -> "EMERGENCY"
    PowerManager.THERMAL_STATUS_SHUTDOWN -> "SHUTDOWN"
    else -> "UNKNOWN"
}

private fun nanosToMs(nanos: Long): Double = nanos / 1_000_000.0

package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val RAW_EXPORT_MAGIC = 0x54525831
private const val RAW_EXPORT_HEADER_INTS = 19
private const val EXPORT_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val EXPORT_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

enum class RawProjectionKind(
    val nativeCode: Int,
    val mimeType: String,
    val fileSuffix: String,
    val buttonLabel: String,
) {
    RECONSTRUCTED_CFA_RAWSENSOR(
        1,
        "application/octet-stream",
        "truthraw_reconstructed_cfa_compat_v0_2.rawsensor",
        "Save reconstructed CFA .rawsensor (compatibility)",
    ),
    RECONSTRUCTED_CFA_DNG(
        2,
        "image/x-adobe-dng",
        "truthraw_reconstructed_cfa_compat_v0_2.dng",
        "Save reconstructed CFA DNG (compatibility)",
    ),
    LINEAR_DNG(
        3,
        "image/x-adobe-dng",
        "truthraw_rgb_linearraw_v0_2.dng",
        "Save 16-bit TruthRaw RGB Linear DNG (compatibility)",
    ),
    TRUTHRAW_PURE_FLOAT32_DNG(
        4,
        "image/x-adobe-dng",
        "truthraw_pure_float32_v0_1.dng",
        "Save TRUTHRAW PURE · Float32 Scientific DNG",
    ),
}

data class RawProjectionMetrics(
    val kind: RawProjectionKind,
    val width: Int,
    val height: Int,
    val samplesPerPixel: Int,
    val stripsWritten: Int,
    val outputBytes: Int,
    val projectedSamples: Int,
    val clippedLowSamples: Int,
    val clippedHighSamples: Int,
    val logicalWorkspaceBytes: Int,
    val scientificGaugeScanPasses: Int,
    val colorClaimScopeCode: Int,
    val cameraCalibrationApplied: Boolean,
)

data class RawProjectionExportResult(
    val metrics: RawProjectionMetrics?,
    val message: String,
    val success: Boolean,
)

object NativeRawProjectionBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedProjection(
        sourceFd: Int,
        outputFd: Int,
        projectionKind: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray
}

object RawProjectionExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        outputUri: Uri,
        kind: RawProjectionKind,
    ): RawProjectionExportResult {
        val source = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return failure("Source could not be reopened: ${error.message ?: error.javaClass.simpleName}")
        } ?: return failure("The document provider did not return a source file descriptor.")

        val output = try {
            resolver.openFileDescriptor(outputUri, "rw")
        } catch (error: Exception) {
            source.close()
            return failure("Output file could not be opened: ${error.message ?: error.javaClass.simpleName}")
        } ?: run {
            source.close()
            return failure("The document provider did not return an output file descriptor.")
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    NativeRawProjectionBridge.exportFinalizedProjection(
                        src.fd,
                        dst.fd,
                        kind.nativeCode,
                        EXPORT_MAX_SOURCE_RESIDENT_BYTES,
                        EXPORT_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            return failure("Native projection export failed: ${error.message ?: error.javaClass.simpleName}")
        }

        if (packet.size != RAW_EXPORT_HEADER_INTS || packet[0] != RAW_EXPORT_MAGIC) {
            return failure("Invalid native projection export packet.")
        }
        if (packet[1] != 0) return failure(nativeStatusDescription(packet[1]))
        if (packet[2] != kind.nativeCode) return failure("Fail-closed: native projection kind differs from the selected export.")

        val metrics = RawProjectionMetrics(
            kind = kind,
            width = packet[3],
            height = packet[4],
            samplesPerPixel = packet[5],
            stripsWritten = packet[6],
            outputBytes = packet[7],
            projectedSamples = packet[8],
            clippedLowSamples = packet[9],
            clippedHighSamples = packet[10],
            logicalWorkspaceBytes = packet[11],
            scientificGaugeScanPasses = packet[16],
            colorClaimScopeCode = packet[17],
            cameraCalibrationApplied = packet[18] != 0,
        )

        val expectedSamples = when (kind) {
            RawProjectionKind.RECONSTRUCTED_CFA_RAWSENSOR,
            RawProjectionKind.RECONSTRUCTED_CFA_DNG -> 1
            RawProjectionKind.LINEAR_DNG,
            RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG -> 3
        }
        val contractViolation =
            metrics.width <= 0 || metrics.height <= 0 ||
                metrics.samplesPerPixel != expectedSamples ||
                metrics.stripsWritten <= 0 || metrics.outputBytes <= 0 ||
                metrics.logicalWorkspaceBytes <= 0 ||
                metrics.logicalWorkspaceBytes > EXPORT_MAX_LOGICAL_RESIDENT_BYTES ||
                packet[12] != 0 || // fullScientificMasterMaterialized
                packet[13] != 0 || // sourcePixelsClaimedMeasured
                packet[14] != 1 || packet[15] != 1 ||
                metrics.scientificGaugeScanPasses != 2 ||
                metrics.colorClaimScopeCode !in 1..2
        if (contractViolation) {
            return failure("Fail-closed: export violated the projection, evidence, scan, or memory contract.")
        }

        val format = when (kind) {
            RawProjectionKind.RECONSTRUCTED_CFA_RAWSENSOR ->
                "reconstructed CFA .rawsensor compatibility projection"
            RawProjectionKind.RECONSTRUCTED_CFA_DNG ->
                "reconstructed CFA DNG compatibility projection"
            RawProjectionKind.LINEAR_DNG ->
                "16-bit TruthRaw camera-native RGB LinearRaw DNG compatibility projection"
            RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG ->
                "TRUTHRAW PURE float32 XYZ-D50 LinearRaw DNG projection"
        }
        val rangeSummary = if (kind == RawProjectionKind.TRUTHRAW_PURE_FLOAT32_DNG) {
            "negative/>1 components retained=${metrics.clippedLowSamples}/${metrics.clippedHighSamples}"
        } else {
            "clipping low/high=${metrics.clippedLowSamples}/${metrics.clippedHighSamples}"
        }
        return RawProjectionExportResult(
            metrics = metrics,
            success = true,
            message = "$format saved · ${metrics.width}×${metrics.height} · ${formatBytes(metrics.outputBytes.toLong())} · " +
                "$rangeSummary · projection-only, no new evidence.",
        )
    }

    private fun failure(message: String) = RawProjectionExportResult(null, message, false)

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun nativeStatusDescription(status: Int): String = when (status) {
        -1 -> "Invalid projection export parameters."
        -2 -> "Fail-closed: pre-master authority state was not canonical."
        -3 -> "Fail-closed: Technical Backplane/admission was not bound to the exact same source."
        -4 -> "Fail-closed: projection attempted to violate an evidence/master invariant."

        in 2001..2099 -> "Source binding/finalization failed (status $status)."
        in 2101..2199 -> "DNG color producer v0.2 failed (status $status)."
        in 3001..3099 -> "TileNative DNG source failed (status $status)."
        in 6001..6099 -> when (status) {
            6001 -> "Projection writer: invalid argument."
            6002 -> "Projection writer: unsupported projection kind."
            6003 -> "Projection writer: source-bound color matrix cannot be written safely to DNG tags."
            6004 -> "Projection writer: Stage-2 source read failed."
            6005 -> "Projection writer: camera-native reconstruction failed."
            6006 -> "Projection writer: non-finite Scientific Master sample; fail-closed."
            6007 -> "Projection writer: logical memory budget exceeded."
            6008 -> "Projection writer: Android output is not seekable/truncatable."
            6009 -> "Projection writer: Android output write failed."
            6010 -> "Projection writer: classic TIFF/DNG offset range exceeded."
            else -> "Projection writer failed (status $status)."
        }
        in 7001..7099 -> "Technical Backplane phase 2 rejected export finalization (status $status)."
        in 8001..8099 -> "Scientific Master/TruthRange streaming rejected export finalization (status $status)."
        in 9001..9099 -> when (status) {
            9001 -> "Float32 Scientific DNG writer: invalid argument."
            9002 -> "Float32 Scientific DNG writer: invalid color transform."
            9003 -> "Float32 Scientific DNG writer: TIFF/DNG size overflow."
            9004 -> "Float32 Scientific DNG writer: Scientific Master tile source failed."
            9005 -> "Float32 Scientific DNG writer: master digest failed."
            9006 -> "Float32 Scientific DNG writer: replayed master identity did not match the admitted Scientific Master."
            9007 -> "Float32 Scientific DNG writer: transactional output sink failed."
            else -> "Float32 Scientific DNG writer failed (status $status)."
        }
        else -> "Unknown projection export status $status."
    }
}

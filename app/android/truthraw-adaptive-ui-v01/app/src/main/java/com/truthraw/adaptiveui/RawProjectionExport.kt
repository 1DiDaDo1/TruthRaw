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
        "truthraw_reconstructed_cfa_v0_1.rawsensor",
        "Reconstructed CFA .rawsensor opslaan",
    ),
    RECONSTRUCTED_CFA_DNG(
        2,
        "image/x-adobe-dng",
        "truthraw_reconstructed_cfa_v0_1.dng",
        "Reconstructed CFA DNG opslaan",
    ),
    LINEAR_DNG(
        3,
        "image/x-adobe-dng",
        "truthraw_linear_v0_1.dng",
        "Linear DNG opslaan",
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
            return failure("Bron kon niet opnieuw worden geopend: ${error.message ?: error.javaClass.simpleName}")
        } ?: return failure("Documentprovider gaf geen bron-file-descriptor.")

        val output = try {
            resolver.openFileDescriptor(outputUri, "rw")
        } catch (error: Exception) {
            source.close()
            return failure("Doelbestand kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}")
        } ?: run {
            source.close()
            return failure("Documentprovider gaf geen output-file-descriptor.")
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
            return failure("Native projection-export faalde: ${error.message ?: error.javaClass.simpleName}")
        }

        if (packet.size != RAW_EXPORT_HEADER_INTS || packet[0] != RAW_EXPORT_MAGIC) {
            return failure("Ongeldig native projection-exportpakket.")
        }
        if (packet[1] != 0) return failure(nativeStatusDescription(packet[1]))
        if (packet[2] != kind.nativeCode) return failure("Fail-closed: native projection-kind wijkt af van de gekozen export.")

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

        val expectedSamples = if (kind == RawProjectionKind.LINEAR_DNG) 3 else 1
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
            return failure("Fail-closed: export schond projection-, evidence-, scan- of memorycontract.")
        }

        val format = when (kind) {
            RawProjectionKind.RECONSTRUCTED_CFA_RAWSENSOR -> "headerless little-endian uint16 reconstructed CFA"
            RawProjectionKind.RECONSTRUCTED_CFA_DNG -> "uncompressed 16-bit reconstructed CFA DNG"
            RawProjectionKind.LINEAR_DNG -> "uncompressed 16-bit LinearRaw DNG"
        }
        return RawProjectionExportResult(
            metrics = metrics,
            success = true,
            message = "$format opgeslagen · ${metrics.width}×${metrics.height} · ${formatBytes(metrics.outputBytes.toLong())} · " +
                "clipping low/high=${metrics.clippedLowSamples}/${metrics.clippedHighSamples} · projection-only, geen nieuwe evidence.",
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
        -1 -> "Ongeldige projection-exportparameters."
        -2 -> "Fail-closed: pre-master authority-state was niet canoniek."
        -3 -> "Fail-closed: Technical Backplane/admission was niet aan exact dezelfde bron gebonden."
        -4 -> "Fail-closed: projection probeerde evidence/master-invariant te schenden."

        in 2001..2099 -> "Source binding/finalization faalde (status $status)."
        in 2101..2199 -> "DNG color producer v0.2 faalde (status $status)."
        in 3001..3099 -> "TileNative DNG-bron faalde (status $status)."
        in 6001..6099 -> when (status) {
            6001 -> "Projection writer: ongeldig argument."
            6002 -> "Projection writer: niet ondersteunde projection-kind."
            6003 -> "Projection writer: brongebonden kleurmatrix kan niet veilig naar DNG-tags worden geschreven."
            6004 -> "Projection writer: Stage-2 bronread faalde."
            6005 -> "Projection writer: camera-native reconstructie faalde."
            6006 -> "Projection writer: niet-finite Scientific-Master sample; fail-closed."
            6007 -> "Projection writer: logical memorybudget overschreden."
            6008 -> "Projection writer: Android-doelbestand is niet seekable/truncatable."
            6009 -> "Projection writer: schrijven naar Android-doelbestand faalde."
            6010 -> "Projection writer: classic TIFF/DNG offsetbereik overschreden."
            else -> "Projection writer faalde (status $status)."
        }
        in 7001..7099 -> "Technical Backplane phase 2 weigerde exportfinalisatie (status $status)."
        in 8001..8099 -> "Scientific Master/TruthRange streaming weigerde exportfinalisatie (status $status)."
        else -> "Onbekende projection-exportstatus $status."
    }
}

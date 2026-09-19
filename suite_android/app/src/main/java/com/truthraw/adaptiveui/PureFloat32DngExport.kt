package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val PURE_FLOAT_MAGIC = 0x54525046L
private const val PURE_FLOAT_PACKET_LONGS = 18
private const val PURE_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val PURE_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object PureFloat32DngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportPureFloat32Dng(
        sourceFd: Int,
        outputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class PureFloat32DngMetrics(
    val width: Long,
    val height: Long,
    val samplesPerPixel: Long,
    val bitsPerSample: Long,
    val outputBytes: Long,
    val projectedPixels: Long,
    val negativeComponentCount: Long,
    val overOneComponentCount: Long,
    val tilesWritten: Long,
    val logicalResidentUpperBoundBytes: Long,
    val scientificMasterIdentityVerified: Boolean,
    val appearanceApplied: Boolean,
    val counterfactualObservationCreated: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val colorClaimScopeCode: Long,
)

sealed interface PureFloat32DngExportResult {
    data class Success(val metrics: PureFloat32DngMetrics) : PureFloat32DngExportResult
    data class Failed(val reason: String) : PureFloat32DngExportResult
}

object PureFloat32DngExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): PureFloat32DngExportResult {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return PureFloat32DngExportResult.Failed(
                "TRUTHRAW PURE Float32 is momenteel alleen toegelaten voor de volledig admitted DNG-route.",
            )
        }

        return try {
            val source = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return PureFloat32DngExportResult.Failed(
                    "Bronprovider gaf geen leesbare file descriptor.",
                )
            val output = resolver.openFileDescriptor(destination, "rw")
                ?: run {
                    source.close()
                    return PureFloat32DngExportResult.Failed(
                        "Bestemmingsprovider gaf geen schrijfbare file descriptor.",
                    )
                }

            val packet = source.use { src ->
                output.use { dst ->
                    PureFloat32DngNativeBridge.exportPureFloat32Dng(
                        src.fd,
                        dst.fd,
                        PURE_MAX_SOURCE_RESIDENT_BYTES,
                        PURE_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }

            val decoded = decode(packet)
            if (decoded is PureFloat32DngExportResult.Failed) {
                runCatching { resolver.delete(destination, null, null) }
            }
            decoded
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            PureFloat32DngExportResult.Failed(
                "TRUTHRAW PURE Float32-export faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }
    }

    private fun decode(packet: LongArray): PureFloat32DngExportResult {
        if (packet.size != PURE_FLOAT_PACKET_LONGS || packet[0] != PURE_FLOAT_MAGIC) {
            return PureFloat32DngExportResult.Failed(
                "Ongeldig native TRUTHRAW PURE Float32-resultaat.",
            )
        }
        val status = packet[1]
        if (status != 0L) {
            return PureFloat32DngExportResult.Failed(statusDescription(status))
        }

        val metrics = PureFloat32DngMetrics(
            width = packet[2],
            height = packet[3],
            samplesPerPixel = packet[4],
            bitsPerSample = packet[5],
            outputBytes = packet[6],
            projectedPixels = packet[7],
            negativeComponentCount = packet[8],
            overOneComponentCount = packet[9],
            tilesWritten = packet[10],
            logicalResidentUpperBoundBytes = packet[11],
            scientificMasterIdentityVerified = packet[12] != 0L,
            appearanceApplied = packet[13] != 0L,
            counterfactualObservationCreated = packet[14] != 0L,
            physicalFrameCount = packet[15],
            independentEvidenceCount = packet[16],
            colorClaimScopeCode = packet[17],
        )

        val violation =
            metrics.width <= 0L ||
                metrics.height <= 0L ||
                metrics.samplesPerPixel != 3L ||
                metrics.bitsPerSample != 32L ||
                metrics.outputBytes <= 0L ||
                metrics.projectedPixels != metrics.width * metrics.height ||
                metrics.tilesWritten <= 0L ||
                metrics.logicalResidentUpperBoundBytes <= 0L ||
                metrics.logicalResidentUpperBoundBytes > PURE_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
                !metrics.scientificMasterIdentityVerified ||
                metrics.appearanceApplied ||
                metrics.counterfactualObservationCreated ||
                metrics.physicalFrameCount != 1L ||
                metrics.independentEvidenceCount != 1L ||
                metrics.colorClaimScopeCode !in 1L..2L

        if (violation) {
            return PureFloat32DngExportResult.Failed(
                "Fail-closed: PURE Float32 DNG schond master-, representation- of evidencecontract.",
            )
        }

        return PureFloat32DngExportResult.Success(metrics)
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "PURE Float32: ongeldige exportparameters."
        -2L -> "PURE Float32: pre-master authority-state was niet canoniek."
        -3L -> "PURE Float32: Phase-2/Scientific-Master/source binding kwam niet exact overeen."
        -4L -> "PURE Float32: projection probeerde master/appearance/evidence-invariant te schenden."

        in 2001L..2099L -> "PURE Float32: source binding faalde (status $status)."
        in 2101L..2199L -> "PURE Float32: DNG color binding faalde (status $status)."
        in 7001L..7099L -> "PURE Float32: generieke RAW/DNG adapter faalde (status $status)."
        in 8001L..8099L -> "PURE Float32: Scientific Master streaming faalde (status $status)."
        in 9001L..9099L -> "PURE Float32: Technical Backplane Phase-2 faalde (status $status)."
        in 10001L..10099L -> when (status) {
            10001L -> "Float32 Scientific DNG writer: ongeldig argument."
            10002L -> "Float32 Scientific DNG writer: ongeldige kleurtransformatie."
            10003L -> "Float32 Scientific DNG writer: TIFF/DNG-grootte overflow."
            10004L -> "Float32 Scientific DNG writer: Scientific-Master tile-read faalde."
            10005L -> "Float32 Scientific DNG writer: master-digest kon niet worden opgebouwd."
            10006L -> "Float32 Scientific DNG writer: replayed Master hash mismatch."
            10007L -> "Float32 Scientific DNG writer: transactionele output-sink faalde."
            10008L -> "Float32 Scientific DNG writer: Zero-Line/scene-scale/Technical-Backplane binding mismatch."
            else -> "Float32 Scientific DNG writer faalde (status $status)."
        }
        else -> "Onbekende TRUTHRAW PURE Float32-status $status."
    }
}

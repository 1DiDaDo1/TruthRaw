package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val DNG_EXPORT_MAGIC = 0x54524447L
private const val DNG_EXPORT_PACKET_LONGS = 16
private const val DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

enum class DngProjectionRole(val nativeCode: Int, val suffix: String, val displayName: String) {
    LINEAR_SCIENTIFIC_MASTER(
        0,
        "truthraw_linear_fixed_d50",
        "Linear Scientific Master DNG (fixed-D50 compatibility)",
    ),
    MEASURED_PRESERVING_CFA(
        1,
        "truthraw_reconstructed_cfa",
        "Reconstructed CFA DNG (derived)",
    ),
}

data class DngProjectionMetrics(
    val role: DngProjectionRole,
    val bytesWritten: Long,
    val tilesWritten: Long,
    val scientificMasterMatched: Boolean,
    val fullFrameMaterialized: Boolean,
    val projectionIsEvidence: Boolean,
    val colorAuthorityPromoted: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val logicalResidentUpperBound: Long,
    val sourceTileReadCalls: Long,
    val rawPayloadBytesRead: Long,
    val scientificGaugeScanPasses: Long,
    val colorAuthorityCode: Long,
)

sealed interface DngProjectionExportResult {
    data class Success(val metrics: DngProjectionMetrics) : DngProjectionExportResult
    data class Failed(val reason: String) : DngProjectionExportResult
}

object NativeDngProjectionBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedProjection(
        sourceFd: Int,
        destinationFd: Int,
        projectionRole: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

object DngProjectionExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        role: DngProjectionRole,
    ): DngProjectionExportResult {
        val source = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return DngProjectionExportResult.Failed(
                "Bron-DNG kon niet opnieuw worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return DngProjectionExportResult.Failed("Bron-DNG gaf geen file descriptor.")

        val output = try {
            resolver.openFileDescriptor(destination, "w")
        } catch (error: Exception) {
            source.close()
            return DngProjectionExportResult.Failed(
                "Doel-DNG kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: run {
            source.close()
            return DngProjectionExportResult.Failed("Doel-DNG gaf geen file descriptor.")
        }

        val packet = try {
            source.use { sourcePfd ->
                output.use { outputPfd ->
                    NativeDngProjectionBridge.exportFinalizedProjection(
                        sourcePfd.fd,
                        outputPfd.fd,
                        role.nativeCode,
                        DNG_MAX_SOURCE_RESIDENT_BYTES,
                        DNG_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            return DngProjectionExportResult.Failed(
                "Native DNG-projectie faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != DNG_EXPORT_PACKET_LONGS || packet[0] != DNG_EXPORT_MAGIC) {
            return DngProjectionExportResult.Failed("Ongeldig native DNG-exportpakket.")
        }
        val status = packet[1]
        if (status != 0L) {
            return DngProjectionExportResult.Failed(nativeDngStatusDescription(status))
        }
        if (packet[2] != role.nativeCode.toLong()) {
            return DngProjectionExportResult.Failed("DNG-projectierol wijzigde over de JNI-grens.")
        }

        val metrics = DngProjectionMetrics(
            role = role,
            bytesWritten = packet[3],
            tilesWritten = packet[4],
            scientificMasterMatched = packet[5] != 0L,
            fullFrameMaterialized = packet[6] != 0L,
            projectionIsEvidence = packet[7] != 0L,
            colorAuthorityPromoted = packet[8] != 0L,
            physicalFrameCount = packet[9],
            independentEvidenceCount = packet[10],
            logicalResidentUpperBound = packet[11],
            sourceTileReadCalls = packet[12],
            rawPayloadBytesRead = packet[13],
            scientificGaugeScanPasses = packet[14],
            colorAuthorityCode = packet[15],
        )

        val invalid =
            metrics.bytesWritten <= 0L ||
                metrics.tilesWritten <= 0L ||
                !metrics.scientificMasterMatched ||
                metrics.fullFrameMaterialized ||
                metrics.projectionIsEvidence ||
                metrics.colorAuthorityPromoted ||
                metrics.physicalFrameCount != 1L ||
                metrics.independentEvidenceCount != 1L ||
                metrics.logicalResidentUpperBound <= 0L ||
                metrics.logicalResidentUpperBound > DNG_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
                metrics.scientificGaugeScanPasses != 2L
        if (invalid) {
            return DngProjectionExportResult.Failed(
                "Fail-closed: DNG-projectie schond Master-, evidence-, resource- of authoritycontract.",
            )
        }
        return DngProjectionExportResult.Success(metrics)
    }

    private fun nativeDngStatusDescription(status: Long): String = when (status) {
        -1L -> "DNG-export: ongeldige file descriptor, projectierol of memorygrens."
        -2L -> "DNG-export v0.2: resolved source-white provenance ontbreekt; single-illuminant export blijft voorlopig fail-closed."
        -3L -> "DNG-export: pre-master source/color authority-state was niet canoniek."
        -4L -> "DNG-export: Scientific Master frame/evidence/readopt-invariant werd geschonden."
        -5L -> "DNG-export: canonical phase-2 admission wijkt af van de sealed source/color lineage."
        -6L -> "DNG-export: eindresultaat schond Master-, evidence-, materialisatie-, fixed-D50- of memorycontract."

        in 2001L..2010L -> "DNG-export source binding faalde (status $status)."
        in 2101L..2116L -> "DNG-export color producer v0.2 faalde (status $status)."
        in 3001L..3013L -> "DNG-export TileNative bron faalde (status $status)."
        6001L -> "DNG-export Scientific Master: ongeldig argument."
        6002L -> "DNG-export Scientific Master: source/tile-read faalde."
        6003L -> "DNG-export Scientific Master: reconstructie faalde."
        6004L -> "DNG-export Scientific Master: digest faalde."
        6005L -> "DNG-export Scientific Master: TruthRange self-gauge faalde."
        6006L -> "DNG-export Scientific Master: bounded memorybudget overschreden."
        in 6101L..6108L -> "DNG-export Technical Backplane phase-2 faalde (status $status)."
        7001L -> "DNG-writer: ongeldig argument."
        7002L -> "DNG-writer: kleur/source authority niet toegelaten."
        7003L -> "DNG-writer: compatibility kleurmatrix ongeldig."
        7004L -> "DNG-writer: resolved source white of fixed-D50 encoding ongeldig."
        7005L -> "DNG-writer: classic TIFF/DNG offset/size overflow."
        7006L -> "DNG-writer: Stage-2 bronread faalde."
        7007L -> "DNG-writer: camera-native reconstructie faalde."
        7008L -> "DNG-writer: Scientific Master digest faalde."
        7009L -> "DNG-writer: documentprovider weigerde sequentiële output."
        7010L -> "DNG-writer: gegenereerde pixels matchen niet exact met de finalized Scientific Master; output geweigerd."
        else -> "Onbekende native DNG-exportstatus $status."
    }
}

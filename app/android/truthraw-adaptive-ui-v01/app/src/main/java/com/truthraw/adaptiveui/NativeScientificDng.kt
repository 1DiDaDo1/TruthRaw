package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val DNG_EXPORT_MAGIC = 0x54524431
private const val DNG_EXPORT_HEADER_INTS = 18
private const val MAX_SOURCE_RESIDENT_BYTES_DNG = 8 * 1024 * 1024
private const val MAX_LOGICAL_RESIDENT_BYTES_DNG = 64 * 1024 * 1024

object NativeScientificDngBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportScientificDng(
        sourceFd: Int,
        outputFd: Int,
        roleCode: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray
}

enum class ScientificDngRole(val code: Int, val suffix: String, val title: String) {
    LINEAR_RAW_COMPATIBILITY(
        1,
        "truthraw_linear_raw_projection.dng",
        "Linear DNG",
    ),
    RECONSTRUCTED_CFA_COMPATIBILITY(
        2,
        "truthraw_reconstructed_cfa_projection.dng",
        "Reconstructed CFA DNG",
    );

    companion object {
        fun fromCode(code: Int): ScientificDngRole? = entries.firstOrNull { it.code == code }
    }
}

data class ScientificDngMetrics(
    val role: ScientificDngRole,
    val width: Int,
    val height: Int,
    val tileCount: Int,
    val bytesWritten: Int,
    val logicalWorkspaceUpperBoundBytes: Int,
    val scientificMasterIdentityMatched: Boolean,
    val finalizedLineageValidated: Boolean,
    val fullScientificMasterMaterialized: Boolean,
    val sourceMetadataBoundColor: Boolean,
    val independentPhysicalColor: Boolean,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val sourceTileReadCalls: Int,
    val rawPayloadBytesRead: Int,
    val metadataBytesRead: Int,
)

sealed interface ScientificDngExportResult {
    data class Success(val metrics: ScientificDngMetrics) : ScientificDngExportResult
    data class Failed(val reason: String) : ScientificDngExportResult
}

object ScientificDngExporter {
    fun export(
        resolver: ContentResolver,
        sourceUri: Uri,
        outputUri: Uri,
        role: ScientificDngRole,
    ): ScientificDngExportResult {
        val source = try {
            resolver.openFileDescriptor(sourceUri, "r")
        } catch (error: Exception) {
            return ScientificDngExportResult.Failed(
                "Bron kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return ScientificDngExportResult.Failed("Bronprovider gaf geen file descriptor.")

        val output = try {
            resolver.openFileDescriptor(outputUri, "rwt")
        } catch (error: Exception) {
            source.close()
            return ScientificDngExportResult.Failed(
                "Doelbestand kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: run {
            source.close()
            return ScientificDngExportResult.Failed("Doelprovider gaf geen file descriptor.")
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    NativeScientificDngBridge.exportScientificDng(
                        src.fd,
                        dst.fd,
                        role.code,
                        MAX_SOURCE_RESIDENT_BYTES_DNG,
                        MAX_LOGICAL_RESIDENT_BYTES_DNG,
                    )
                }
            }
        } catch (error: Throwable) {
            return ScientificDngExportResult.Failed(
                "Native DNG-export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != DNG_EXPORT_HEADER_INTS || packet[0] != DNG_EXPORT_MAGIC) {
            return ScientificDngExportResult.Failed("Ongeldig native DNG-exportpakket.")
        }
        if (packet[1] != 0) {
            return ScientificDngExportResult.Failed(nativeDngStatusDescription(packet[1]))
        }

        val reportedRole = ScientificDngRole.fromCode(packet[2])
            ?: return ScientificDngExportResult.Failed("Native DNG-export gaf onbekende projectierol terug.")
        if (reportedRole != role) {
            return ScientificDngExportResult.Failed("Native DNG-export wisselde onverwacht van projectierol.")
        }

        val metrics = ScientificDngMetrics(
            role = role,
            width = packet[3],
            height = packet[4],
            tileCount = packet[5],
            bytesWritten = packet[6],
            logicalWorkspaceUpperBoundBytes = packet[7],
            scientificMasterIdentityMatched = packet[8] != 0,
            finalizedLineageValidated = packet[9] != 0,
            fullScientificMasterMaterialized = packet[10] != 0,
            sourceMetadataBoundColor = packet[11] != 0,
            independentPhysicalColor = packet[12] != 0,
            physicalFrameCount = packet[13],
            independentEvidenceCount = packet[14],
            sourceTileReadCalls = packet[15],
            rawPayloadBytesRead = packet[16],
            metadataBytesRead = packet[17],
        )

        val invariantViolation =
            metrics.width <= 0 || metrics.height <= 0 || metrics.tileCount <= 0 || metrics.bytesWritten <= 0 ||
                !metrics.scientificMasterIdentityMatched || !metrics.finalizedLineageValidated ||
                metrics.fullScientificMasterMaterialized ||
                metrics.physicalFrameCount != 1 || metrics.independentEvidenceCount != 1 ||
                (!metrics.sourceMetadataBoundColor && !metrics.independentPhysicalColor) ||
                (metrics.sourceMetadataBoundColor && metrics.independentPhysicalColor)
        if (invariantViolation) {
            return ScientificDngExportResult.Failed(
                "Fail-closed: DNG-export schond master-, lineage-, evidence- of authoritycontract.",
            )
        }
        return ScientificDngExportResult.Success(metrics)
    }

    private fun nativeDngStatusDescription(status: Int): String = when (status) {
        -1 -> "Ongeldige DNG-exportparameters."
        -2 -> "Fail-closed: DNG-export schond post-write resource/evidencecontract."

        in 2001..2010 -> "Source-binding faalde tijdens DNG-export (status $status)."
        in 2101..2116 -> "DNG-kleurprofiel kon niet source-bound worden opgelost (status $status)."
        in 3001..3013 -> "TileNative DNG-bron kon niet worden geopend (status $status)."
        in 6001..6006 -> "Scientific Master replay/finalization faalde (status $status)."
        in 7001..7008 -> "Technical Backplane Phase 2 weigerde de exportlineage (status $status)."
        8001 -> "DNG writer: ongeldig argument."
        8002 -> "DNG writer: finalized/source-bound authority ontbreekt of wijkt af."
        8003 -> "DNG writer: afgeleide D50-kleurtransformatie is ongeldig."
        8004 -> "DNG writer: bronread faalde."
        8005 -> "DNG writer: camera-native reconstructie faalde."
        8006 -> "DNG writer: Scientific Master-digest kon niet worden opgebouwd."
        8007 -> "DNG writer: replayed Scientific Master wijkt af van Phase-2 master; export geblokkeerd."
        8008 -> "DNG writer: schrijven/flushen naar documentprovider faalde."
        8009 -> "DNG writer: logisch memorybudget overschreden."
        else -> "Onbekende native DNG-exportstatus $status."
    }
}

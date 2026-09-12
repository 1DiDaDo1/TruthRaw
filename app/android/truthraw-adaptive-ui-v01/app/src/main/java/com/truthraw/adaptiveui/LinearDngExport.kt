package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val LINEAR_DNG_MAGIC = 0x544c4431L
private const val LINEAR_DNG_PACKET_LONGS = 14
private const val LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object NativeLinearDngBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedLinearDng(
        sourceFd: Int,
        outputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class LinearDngExportMetrics(
    val width: Int,
    val height: Int,
    val outputBytes: Long,
    val tilesWritten: Long,
    val samplesWritten: Long,
    val clippedBelowZero: Long,
    val clippedAboveOne: Long,
    val logicalWorkspacePeakBytes: Long,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val scientificClaimAllowed: Boolean,
    val authorityCode: Int,
)

sealed interface LinearDngExportOutcome {
    data class Success(val metrics: LinearDngExportMetrics) : LinearDngExportOutcome
    data class Failed(val reason: String) : LinearDngExportOutcome
}

object LinearDngExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): LinearDngExportOutcome {
        val sourceDescriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return LinearDngExportOutcome.Failed(
                "Bron-DNG kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return LinearDngExportOutcome.Failed("Bron-DNG gaf geen file descriptor.")

        val outputDescriptor = try {
            resolver.openFileDescriptor(destination, "rw")
        } catch (error: Exception) {
            sourceDescriptor.close()
            return LinearDngExportOutcome.Failed(
                "Doel-DNG kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: run {
            sourceDescriptor.close()
            return LinearDngExportOutcome.Failed("Doel-DNG gaf geen file descriptor.")
        }

        val packet = try {
            sourceDescriptor.use { sourcePfd ->
                outputDescriptor.use { outputPfd ->
                    NativeLinearDngBridge.exportFinalizedLinearDng(
                        sourcePfd.fd,
                        outputPfd.fd,
                        LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES,
                        LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            return LinearDngExportOutcome.Failed(
                "Native Linear DNG-export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != LINEAR_DNG_PACKET_LONGS || packet[0] != LINEAR_DNG_MAGIC) {
            return LinearDngExportOutcome.Failed("Ongeldig Linear DNG-exportpakket.")
        }
        val status = packet[1]
        if (status != 0L) {
            return LinearDngExportOutcome.Failed(statusDescription(status))
        }

        val width = packet[2].toInt()
        val height = packet[3].toInt()
        val physicalFrames = packet[10].toInt()
        val evidenceRoots = packet[11].toInt()
        val scientificClaim = packet[12] != 0L
        val authorityCode = packet[13].toInt()
        if (width <= 0 || height <= 0 || packet[4] <= 0L || packet[5] <= 0L ||
            physicalFrames != 1 || evidenceRoots != 1 || authorityCode !in 1..2 ||
            scientificClaim != (authorityCode == 2)) {
            return LinearDngExportOutcome.Failed(
                "Fail-closed: Linear DNG-export schond dimensie-, evidence- of authoritycontract.",
            )
        }

        return LinearDngExportOutcome.Success(
            LinearDngExportMetrics(
                width = width,
                height = height,
                outputBytes = packet[4],
                tilesWritten = packet[5],
                samplesWritten = packet[6],
                clippedBelowZero = packet[7],
                clippedAboveOne = packet[8],
                logicalWorkspacePeakBytes = packet[9],
                physicalFrameCount = physicalFrames,
                independentEvidenceCount = evidenceRoots,
                scientificClaimAllowed = scientificClaim,
                authorityCode = authorityCode,
            ),
        )
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "Linear DNG-export: ongeldige file descriptor of memorygrens."
        -2L -> "Linear DNG-export: pre-master authority-state was niet canoniek."
        -3L -> "Linear DNG-export: finalized release/provenance werd geweigerd."
        -4L -> "Linear DNG-export: full-frame/evidence/projectie-invariant werd geweigerd."

        6101L -> "Linear DNG-projectie: ongeldig argument."
        6102L -> "Linear DNG-projectie: kleur-binding heeft geen toegestane authority."
        6103L -> "Linear DNG-projectie: camera→XYZ-kleur-binding wijkt af van de finalized bron."
        6104L -> "Linear DNG-projectie: logisch memorybudget overschreden."
        6105L -> "Linear DNG-projectie: doelbestand kon niet seek/write/flush uitvoeren."
        6106L -> "Linear DNG-projectie: classic-TIFF 4-GiB/offsetgrens overschreden."
        6107L -> "Linear DNG-projectie: Stage-2 bronread faalde."
        6108L -> "Linear DNG-projectie: camera-native reconstructie faalde."
        6109L -> "Linear DNG-projectie: niet-finiete reconstructed sample; fail-closed."

        in 2000L..2099L -> "Linear DNG-export: source-binding gate faalde (status $status)."
        in 2100L..2199L -> "Linear DNG-export: DNG color producer v0.2 faalde (status $status)."
        in 3000L..3099L -> "Linear DNG-export: TileNative DNG-bron faalde (status $status)."
        in 5000L..5099L -> "Linear DNG-export: finalized Scientific Preview gate faalde (status $status)."
        else -> "Onbekende native Linear DNG-exportstatus $status."
    }
}

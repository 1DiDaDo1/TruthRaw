package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val DNG_PACKET_MAGIC = 0x54524431L
private const val MAX_SOURCE_RESIDENT_BYTES_DNG = 8 * 1024 * 1024
private const val MAX_LOGICAL_RESIDENT_BYTES_DNG = 64 * 1024 * 1024

object NativeDngProjectionBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedLinearDng(
        sourceFd: Int,
        destinationFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class LinearDngExportResult(
    val bytesWritten: Long,
    val width: Int,
    val height: Int,
    val scientificSamples: Long,
    val clampedLowSamples: Long,
    val clampedHighSamples: Long,
    val projectionScale: Float,
    val bitDepth: Int,
)

object LinearDngProjectionExporter {
    fun export(
        resolver: ContentResolver,
        source: Uri,
        destination: Uri,
    ): Result<LinearDngExportResult> = runCatching {
        val sourcePfd = resolver.openFileDescriptor(source, "r")
            ?: error("Documentprovider gaf geen bron-file-descriptor.")
        val destinationPfd = resolver.openFileDescriptor(destination, "w")
            ?: run {
                sourcePfd.close()
                error("Documentprovider gaf geen doel-file-descriptor.")
            }

        val packet = sourcePfd.use { src ->
            destinationPfd.use { dst ->
                NativeDngProjectionBridge.exportFinalizedLinearDng(
                    src.fd,
                    dst.fd,
                    MAX_SOURCE_RESIDENT_BYTES_DNG,
                    MAX_LOGICAL_RESIDENT_BYTES_DNG,
                )
            }
        }

        require(packet.size >= 10) { "Native DNG-export gaf een te kort pakket." }
        require(packet[0] == DNG_PACKET_MAGIC) { "Native DNG-export gaf een onbekend pakket." }
        val status = packet[1].toInt()
        if (status != 0) error(statusDescription(status))

        val scaleBits = packet[8].toInt()
        LinearDngExportResult(
            bytesWritten = packet[4],
            width = packet[2].toInt(),
            height = packet[3].toInt(),
            scientificSamples = packet[5],
            clampedLowSamples = packet[6],
            clampedHighSamples = packet[7],
            projectionScale = Float.fromBits(scaleBits),
            bitDepth = packet[9].toInt(),
        )
    }

    private fun statusDescription(status: Int): String = when (status) {
        1 -> "Ongeldige DNG-exportparameters."
        2 -> "Bronseal/SHA-256 kon niet worden opgebouwd."
        3 -> "Brongebonden DNG-kleurmetadata kon niet worden gevalideerd."
        4 -> "Scientific Preview-bron kon niet canoniek worden voorbereid."
        5 -> "TileNative DNG-bron kon niet worden geopend."
        6 -> "Finalized Scientific Preview-gate werd niet gehaald; DNG-export is fail-closed geblokkeerd."
        7 -> "Bronbytes veranderden tijdens de DNG-export; fail-closed geblokkeerd."
        8 -> "Linear DNG v0.1 ondersteunt voor metadata-copy alleen classic little-endian TIFF/DNG."
        9 -> "Vereiste DNG-kleurmetadata kon niet veilig naar de projection worden gekopieerd."
        10 -> "Scientific Master kon niet exact opnieuw worden gereconstrueerd voor export."
        11 -> "Scientific-Master-digest van de exportreplay wijkt af van de finalized digest."
        12 -> "Scientific Master heeft geen geldige bounded projection-range."
        13 -> "Schrijven naar de gekozen Android-documentlocatie faalde."
        14 -> "Linear DNG-container zou classic-TIFF grenzen overschrijden."
        else -> "Onbekende Linear DNG-exportstatus $status."
    }
}

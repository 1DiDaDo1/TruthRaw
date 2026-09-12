package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.provider.DocumentsContract
import java.io.File

private const val SCIENTIFIC_DNG_PACKET_MAGIC = 0x54524644L
private const val SCIENTIFIC_DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val SCIENTIFIC_DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object NativeScientificDngBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedScientificMasterLinearDng(
        sourceFd: Int,
        tempOutputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class ScientificDngExportResult(
    val bytesWritten: Long,
    val width: Int,
    val height: Int,
    val projectedPixels: Long,
    val negativeComponents: Long,
    val overOneComponents: Long,
    val tilesWritten: Long,
    val scientificMasterIdentityVerified: Boolean,
    val artifactCommitted: Boolean,
    val representationOnly: Boolean,
)

object ScientificDngProjectionExporter {
    fun export(
        resolver: ContentResolver,
        cacheDir: File,
        source: Uri,
        destination: Uri,
    ): Result<ScientificDngExportResult> = runCatching {
        val tempFile = File.createTempFile("truthraw_scientific_master_", ".dng.tmp", cacheDir)
        try {
            val packet = resolver.openFileDescriptor(source, "r")?.use { src ->
                ParcelFileDescriptor.open(
                    tempFile,
                    ParcelFileDescriptor.MODE_READ_WRITE or
                        ParcelFileDescriptor.MODE_CREATE or
                        ParcelFileDescriptor.MODE_TRUNCATE,
                ).use { tmp ->
                    NativeScientificDngBridge.exportFinalizedScientificMasterLinearDng(
                        src.fd,
                        tmp.fd,
                        SCIENTIFIC_DNG_MAX_SOURCE_RESIDENT_BYTES,
                        SCIENTIFIC_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            } ?: error("Documentprovider gaf geen bron-file-descriptor.")

            require(packet.size >= 12) { "Native Scientific DNG-export gaf een te kort pakket." }
            require(packet[0] == SCIENTIFIC_DNG_PACKET_MAGIC) {
                "Native Scientific DNG-export gaf een onbekend pakket."
            }
            val status = packet[1].toInt()
            if (status != 0) error(statusDescription(status))

            val result = ScientificDngExportResult(
                bytesWritten = packet[4],
                width = packet[2].toInt(),
                height = packet[3].toInt(),
                projectedPixels = packet[5],
                negativeComponents = packet[6],
                overOneComponents = packet[7],
                tilesWritten = packet[8],
                scientificMasterIdentityVerified = packet[9] == 1L,
                artifactCommitted = packet[10] == 1L,
                representationOnly = packet[11] == 1L,
            )
            require(result.scientificMasterIdentityVerified && result.artifactCommitted && result.representationOnly) {
                "Native projection kwam terug zonder volledige identity/commit/representation invariants."
            }
            require(tempFile.length() == result.bytesWritten) {
                "App-private staged DNG-lengte wijkt af van de native writer-resultaatlengte."
            }

            try {
                resolver.openOutputStream(destination, "w")?.use { out ->
                    tempFile.inputStream().buffered(1024 * 1024).use { input ->
                        out.buffered(1024 * 1024).use { buffered -> input.copyTo(buffered, 1024 * 1024) }
                    }
                } ?: error("Documentprovider gaf geen doel-outputstream.")
            } catch (t: Throwable) {
                try {
                    DocumentsContract.deleteDocument(resolver, destination)
                } catch (_: Throwable) {
                    // Best-effort cleanup. A provider may not support delete through DocumentsContract.
                }
                throw t
            }

            result
        } finally {
            tempFile.delete()
        }
    }

    private fun statusDescription(status: Int): String = when (status) {
        1 -> "Ongeldige Scientific DNG-exportparameters."
        2 -> "Bronseal/SHA-256 kon niet worden opgebouwd."
        3 -> "Brongebonden DNG-kleurmetadata kon niet worden gevalideerd."
        4 -> "Scientific Preview-bron kon niet canoniek worden voorbereid."
        5 -> "Bronbytes veranderden tijdens de export; fail-closed geblokkeerd."
        6 -> "TileNative DNG-bron kon niet worden geopend."
        7 -> "Finalized Scientific Preview-gate werd niet gehaald; export geblokkeerd."
        8 -> "Scientific Master → float32 LinearRaw DNG projection faalde."
        9 -> "Projection authority-invariants werden geschonden; staged bestand is geaborteerd."
        else -> "Onbekende Scientific DNG-exportstatus $status."
    }
}

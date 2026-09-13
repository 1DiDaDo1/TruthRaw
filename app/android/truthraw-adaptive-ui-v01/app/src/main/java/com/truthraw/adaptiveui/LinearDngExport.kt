package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Context
import android.graphics.Bitmap
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.FileOutputStream

private const val LINEAR_DNG_MAGIC = 0x5452444cL
private const val LINEAR_DNG_PACKET_LONGS = 17
private const val LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object LinearDngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedLinearDng(
        sourceFd: Int,
        destinationFd: Int,
        previewJpegFd: Int,
        previewWidth: Int,
        previewHeight: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class LinearDngExportMetrics(
    val width: Long,
    val height: Long,
    val outputBytes: Long,
    val pixelPayloadBytes: Long,
    val tilesWritten: Long,
    val samplesClippedLow: Long,
    val samplesClippedHigh: Long,
    val logicalResidentUpperBoundBytes: Long,
    val fullScientificMasterMaterialized: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val previewWidth: Long,
    val previewHeight: Long,
    val previewJpegBytes: Long,
    val embeddedFinalizedPreview: Boolean,
)

sealed interface LinearDngExportResult {
    data class Success(val metrics: LinearDngExportMetrics) : LinearDngExportResult
    data class Failed(val reason: String) : LinearDngExportResult
}

object LinearDngExporter {
    fun export(
        context: Context,
        resolver: ContentResolver,
        job: RawJob,
        finalizedPreview: Bitmap,
        destination: Uri,
    ): LinearDngExportResult {
        if (finalizedPreview.isRecycled) {
            return LinearDngExportResult.Failed("Finalized previewbitmap was al vrijgegeven vóór DNG-export.")
        }
        if (finalizedPreview.width <= 0 || finalizedPreview.height <= 0) {
            return LinearDngExportResult.Failed("Finalized preview had ongeldige afmetingen.")
        }

        var previewFile: File? = null
        return try {
            previewFile = File.createTempFile("truthraw-finalized-preview-", ".jpg", context.cacheDir)
            FileOutputStream(previewFile).use { stream ->
                PortablePreviewEncoder.encodeJpeg(finalizedPreview, stream)
            }
            if (!previewFile.isFile || previewFile.length() <= 0L) {
                return LinearDngExportResult.Failed("Finalized preview-JPEG kon niet worden opgebouwd.")
            }

            val source = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return LinearDngExportResult.Failed("Bronprovider gaf geen leesbare file descriptor.")
            val output = resolver.openFileDescriptor(destination, "rw")
                ?: run {
                    source.close()
                    return LinearDngExportResult.Failed("Bestemmingsprovider gaf geen schrijfbare file descriptor.")
                }
            val preview = ParcelFileDescriptor.open(previewFile, ParcelFileDescriptor.MODE_READ_ONLY)

            val packet = source.use { sourcePfd ->
                output.use { outputPfd ->
                    preview.use { previewPfd ->
                        LinearDngNativeBridge.exportFinalizedLinearDng(
                            sourcePfd.fd,
                            outputPfd.fd,
                            previewPfd.fd,
                            finalizedPreview.width,
                            finalizedPreview.height,
                            LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES,
                            LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            }
            val decoded = decode(packet)
            if (decoded is LinearDngExportResult.Failed) {
                runCatching { resolver.delete(destination, null, null) }
            }
            decoded
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            LinearDngExportResult.Failed(
                "Linear DNG-export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        } finally {
            previewFile?.let { runCatching { it.delete() } }
        }
    }

    private fun decode(packet: LongArray): LinearDngExportResult {
        if (packet.size != LINEAR_DNG_PACKET_LONGS || packet[0] != LINEAR_DNG_MAGIC) {
            return LinearDngExportResult.Failed("Ongeldig native Linear DNG-resultaat.")
        }
        val status = packet[1]
        if (status != 0L) {
            return LinearDngExportResult.Failed(statusDescription(status))
        }

        val metrics = LinearDngExportMetrics(
            width = packet[2],
            height = packet[3],
            outputBytes = packet[4],
            pixelPayloadBytes = packet[5],
            tilesWritten = packet[6],
            samplesClippedLow = packet[7],
            samplesClippedHigh = packet[8],
            logicalResidentUpperBoundBytes = packet[9],
            fullScientificMasterMaterialized = packet[10] != 0L,
            physicalFrameCount = packet[11],
            independentEvidenceCount = packet[12],
            previewWidth = packet[13],
            previewHeight = packet[14],
            previewJpegBytes = packet[15],
            embeddedFinalizedPreview = packet[16] != 0L,
        )
        if (metrics.width <= 0L || metrics.height <= 0L || metrics.outputBytes <= 0L ||
            metrics.pixelPayloadBytes <= 0L || metrics.tilesWritten <= 0L ||
            metrics.logicalResidentUpperBoundBytes <= 0L ||
            metrics.logicalResidentUpperBoundBytes > LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
            metrics.fullScientificMasterMaterialized ||
            metrics.samplesClippedHigh != 0L ||
            metrics.physicalFrameCount != 1L || metrics.independentEvidenceCount != 1L ||
            metrics.previewWidth <= 0L || metrics.previewHeight <= 0L || metrics.previewJpegBytes <= 0L ||
            !metrics.embeddedFinalizedPreview) {
            return LinearDngExportResult.Failed(
                "Fail-closed: Linear DNG schond RAW-, preview-, memory- of evidencecontract.",
            )
        }
        return LinearDngExportResult.Success(metrics)
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "Ongeldige Linear DNG-exportparameters."
        -2L -> "Fail-closed: pre-master authority-state was niet canoniek."
        -3L -> "Fail-closed: finalized admission/evidence was niet geldig voor export."
        -4L -> "Fail-closed: geschreven projectie schond het Linear DNG-contract."
        -5L -> "Fail-closed: finalized Backplane/admission en exportbron hebben niet exact dezelfde bronidentiteit."

        2001L -> "Source binding: ongeldig argument."
        2002L -> "Source binding: bron kon niet volledig worden gelezen voor SHA-256."
        2003L -> "Source binding: bronseal is niet canoniek."
        2004L -> "Source binding: bronbytes verschillen van de sealed SHA-256 identiteit."
        2005L -> "Source binding: kleurbinding heeft geen toegestane authority."
        2006L -> "Source binding: kleurbinding hoort bij andere bronbytes."
        2007L -> "Source binding: camera→XYZ(D50)-matrix is ongeldig."
        2008L -> "Source binding: frame/evidence-invariant geweigerd."
        2009L -> "Source binding: Backplane geweigerd."
        2010L -> "Source binding: Backplane-bronhash wijkt af."

        2101L -> "DNG color producer v0.2: ongeldig argument."
        2102L -> "DNG color producer v0.2: sealed bronhash mismatch."
        2103L -> "DNG color producer v0.2: bron kon niet worden gelezen."
        2104L -> "DNG color producer v0.2: ongeldige TIFF/DNG-container."
        2105L -> "DNG color producer v0.2: BigTIFF wordt niet ondersteund."
        2106L -> "DNG color producer v0.2: ongeldige IFD0."
        2107L -> "DNG color producer v0.2: ColorMatrix1 ontbreekt."
        2108L -> "DNG color producer v0.2: AsShotNeutral ontbreekt."
        2109L -> "DNG color producer v0.2: dual-illuminant calibratieset is onvolledig."
        2110L -> "DNG color producer v0.2: triple-illuminant calibratie wordt nog niet ondersteund."
        2111L -> "DNG color producer v0.2: CalibrationIlluminant is niet ondersteund."
        2112L -> "DNG color producer v0.2: ongeldig tagtype."
        2113L -> "DNG color producer v0.2: ongeldige tagcardinaliteit."
        2114L -> "DNG color producer v0.2: ongeldige metadatawaarde."
        2115L -> "DNG color producer v0.2: singuliere kleurmatrix."
        2116L -> "DNG color producer v0.2: AsShotNeutral-oplossing convergeerde niet."

        in 3001L..3999L -> "Native DNG-source faalde met code $status."
        in 5001L..5999L -> "Finalized Scientific Preview-gate faalde met code $status."

        6101L -> "Linear DNG v0.2: ongeldige exportparameters."
        6102L -> "Linear DNG v0.2: broncamera-identiteit ontbreekt of is ongeldig."
        6103L -> "Linear DNG v0.2: onderliggende bounded RGB-projectie faalde."
        6104L -> "Linear DNG v0.2: gereconstrueerde scene overschrijdt het gevalideerde 2× finite headroom-venster; export is geweigerd in plaats van geclipt."
        6105L -> "Linear DNG v0.2: camera-identiteit/BaselineExposure kon niet veilig in de DNG-container worden hersteld."

        6201L -> "Linear DNG preview v0.3: ongeldige preview/exportparameters."
        6202L -> "Linear DNG preview v0.3: JPEG-preview overschrijdt het bounded previewcontract."
        6203L -> "Linear DNG preview v0.3: JPEG-preview kon niet volledig worden gelezen."
        6204L -> "Linear DNG preview v0.3: preview is geen geldige baseline 8-bit JPEG voor de opgegeven finalized sRGB-afmetingen."
        6205L -> "Linear DNG preview v0.3: onderliggende v0.2 RGB LinearRaw-projectie faalde."
        6206L -> "Linear DNG preview v0.3: DNG-containerpatch kon niet veilig worden uitgevoerd."
        6207L -> "Linear DNG preview v0.3: embedded preview kon niet veilig worden afgerond."

        // Historical v0.1 status vocabulary is retained for forensic/debug readability.
        6001L -> "Linear DNG writer v0.1: ongeldig argument."
        6002L -> "Linear DNG writer v0.1: finalized release ontbreekt."
        6003L -> "Linear DNG writer v0.1: frame/evidence/provenance-invariant geweigerd."
        6004L -> "Linear DNG writer v0.1: bronkleurmetadata kon niet veilig worden overgenomen."
        6005L -> "Linear DNG writer v0.1: bronread faalde."
        6006L -> "Linear DNG writer v0.1: broncontainer wordt niet ondersteund."
        6007L -> "Linear DNG writer v0.1: camera-native reconstructie faalde."
        6008L -> "Linear DNG writer v0.1: niet-finiete Scientific-Master sample werd geweigerd."
        6009L -> "Linear DNG writer v0.1: memorybudget overschreden."
        6010L -> "Linear DNG writer v0.1: output is te groot voor classic TIFF/DNG."
        6011L -> "Linear DNG writer v0.1: Android-bestemming ondersteunt de vereiste writes niet."
        else -> "Linear DNG-export faalde met native status $status."
    }
}

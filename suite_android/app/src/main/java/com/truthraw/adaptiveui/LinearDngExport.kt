package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File

private const val LINEAR_DNG_MAGIC = 0x5452444cL
private const val LINEAR_DNG_PACKET_LONGS = 19
private const val LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object LinearDngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedLinearDng(
        sourceFd: Int,
        destinationFd: Int,
        outputPreviewFd: Int,
        outputPreviewMaxEdge: Int,
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
    val unifiedOutputPreviewAvailable: Boolean,
    val unifiedOutputPreviewWidth: Int,
    val unifiedOutputPreviewHeight: Int,
    val unifiedOutputPreviewSourceSpaceCode: Int,
    val unifiedOutputPreviewSampledPixels: Long,
    val exactBoundedU16PreviewSourceUsed: Boolean,
)

sealed interface LinearDngExportResult {
    data class Success(
        val metrics: LinearDngExportMetrics,
        val unifiedOutputPreview: UnifiedOutputPreviewResult.Ready? = null,
    ) : LinearDngExportResult
    data class Failed(val reason: String) : LinearDngExportResult
}

object LinearDngExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        unifiedOutputPreviewFile: File? = null,
        unifiedOutputPreviewMaxEdge: Int = 384,
    ): LinearDngExportResult {
        return try {
            val source = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return LinearDngExportResult.Failed("Bronprovider gaf geen leesbare file descriptor.")
            val output = resolver.openFileDescriptor(destination, "rw")
                ?: run {
                    source.close()
                    return LinearDngExportResult.Failed("Bestemmingsprovider gaf geen schrijfbare file descriptor.")
                }

            val exactPreview = if (unifiedOutputPreviewFile != null) {
                if (unifiedOutputPreviewMaxEdge !in 1..1024) {
                    source.close()
                    output.close()
                    return LinearDngExportResult.Failed(
                        "Linear DNG Unified Output Preview maxEdge is buiten contract.",
                    )
                }
                unifiedOutputPreviewFile.parentFile?.mkdirs()
                unifiedOutputPreviewFile.delete()
                runCatching {
                    ParcelFileDescriptor.open(
                        unifiedOutputPreviewFile,
                        ParcelFileDescriptor.MODE_CREATE or
                            ParcelFileDescriptor.MODE_READ_WRITE or
                            ParcelFileDescriptor.MODE_TRUNCATE,
                    )
                }.getOrNull()
                    ?: run {
                        source.close()
                        output.close()
                        return LinearDngExportResult.Failed(
                            "Linear DNG Unified Output Preview staging kon niet worden geopend.",
                        )
                    }
            } else {
                null
            }

            val packet = source.use { sourcePfd ->
                output.use { outputPfd ->
                    if (exactPreview != null) {
                        exactPreview.use { previewPfd ->
                            LinearDngNativeBridge.exportFinalizedLinearDng(
                                sourcePfd.fd,
                                outputPfd.fd,
                                previewPfd.fd,
                                unifiedOutputPreviewMaxEdge,
                                LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES,
                                LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                            )
                        }
                    } else {
                        LinearDngNativeBridge.exportFinalizedLinearDng(
                            sourcePfd.fd,
                            outputPfd.fd,
                            -1,
                            0,
                            LINEAR_DNG_MAX_SOURCE_RESIDENT_BYTES,
                            LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            }
            val decoded = decode(packet, unifiedOutputPreviewFile != null)
            if (decoded is LinearDngExportResult.Failed) {
                unifiedOutputPreviewFile?.delete()
                runCatching { resolver.delete(destination, null, null) }
                decoded
            } else {
                val success = decoded as LinearDngExportResult.Success
                val previewResult = if (unifiedOutputPreviewFile != null) {
                    when (
                        val loaded = UnifiedOutputPreviewLoader.load(
                            unifiedOutputPreviewFile,
                            "Bounded Linear DNG",
                        )
                    ) {
                        is UnifiedOutputPreviewResult.Failed -> {
                            unifiedOutputPreviewFile.delete()
                            runCatching { resolver.delete(destination, null, null) }
                            return LinearDngExportResult.Failed(loaded.reason)
                        }
                        is UnifiedOutputPreviewResult.Ready -> {
                            val m = loaded.metrics
                            if (
                                m.width != success.metrics.unifiedOutputPreviewWidth ||
                                m.height != success.metrics.unifiedOutputPreviewHeight ||
                                m.sourceSpaceCode !=
                                    success.metrics.unifiedOutputPreviewSourceSpaceCode ||
                                m.sampledPrimaryPixels !=
                                    success.metrics.unifiedOutputPreviewSampledPixels
                            ) {
                                loaded.bitmap.recycle()
                                unifiedOutputPreviewFile.delete()
                                runCatching { resolver.delete(destination, null, null) }
                                return LinearDngExportResult.Failed(
                                    "Linear DNG UOP1 sidecar/native packet binding mismatch.",
                                )
                            }
                            loaded
                        }
                    }
                } else {
                    null
                }
                unifiedOutputPreviewFile?.delete()
                LinearDngExportResult.Success(
                    success.metrics,
                    unifiedOutputPreview = previewResult,
                )
            }
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            LinearDngExportResult.Failed(
                "Linear DNG-export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }
    }

    private fun decode(
        packet: LongArray,
        unifiedOutputPreviewExpected: Boolean,
    ): LinearDngExportResult {
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
            unifiedOutputPreviewAvailable = packet[13] != 0L,
            unifiedOutputPreviewWidth = packet[14].toInt(),
            unifiedOutputPreviewHeight = packet[15].toInt(),
            unifiedOutputPreviewSourceSpaceCode = packet[16].toInt(),
            unifiedOutputPreviewSampledPixels = packet[17],
            exactBoundedU16PreviewSourceUsed = packet[18] != 0L,
        )
        if (metrics.width <= 0L || metrics.height <= 0L || metrics.outputBytes <= 0L ||
            metrics.pixelPayloadBytes <= 0L || metrics.tilesWritten <= 0L ||
            metrics.logicalResidentUpperBoundBytes <= 0L ||
            metrics.logicalResidentUpperBoundBytes > LINEAR_DNG_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
            metrics.fullScientificMasterMaterialized ||
            metrics.physicalFrameCount != 1L ||
            metrics.independentEvidenceCount != 1L ||
            metrics.unifiedOutputPreviewAvailable != unifiedOutputPreviewExpected ||
            (unifiedOutputPreviewExpected &&
                (metrics.unifiedOutputPreviewWidth <= 0 ||
                    metrics.unifiedOutputPreviewHeight <= 0 ||
                    metrics.unifiedOutputPreviewWidth > 1024 ||
                    metrics.unifiedOutputPreviewHeight > 1024 ||
                    metrics.unifiedOutputPreviewSourceSpaceCode != 1 ||
                    metrics.unifiedOutputPreviewSampledPixels !=
                        metrics.unifiedOutputPreviewWidth.toLong() *
                            metrics.unifiedOutputPreviewHeight.toLong() ||
                    !metrics.exactBoundedU16PreviewSourceUsed)) ||
            (!unifiedOutputPreviewExpected &&
                (metrics.unifiedOutputPreviewWidth != 0 ||
                    metrics.unifiedOutputPreviewHeight != 0 ||
                    metrics.unifiedOutputPreviewSourceSpaceCode != 0 ||
                    metrics.unifiedOutputPreviewSampledPixels != 0L ||
                    metrics.exactBoundedU16PreviewSourceUsed))) {
            return LinearDngExportResult.Failed(
                "Fail-closed: Linear DNG schond output-, memory- of evidencecontract.",
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
        -6L -> "Linear DNG Unified Output Preview kon niet exact uit de bounded U16-primary worden opgebouwd."

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

        in 3001L..3999L -> "Legacy directe DNG-source faalde met code $status."
        in 5001L..5999L -> "Finalized Scientific Preview-gate faalde met code $status."
        7001L -> "RAW-adapter: ongeldig argument."
        7002L -> "RAW-adapter: sealed bronlengte kwam niet overeen."
        7003L -> "RAW-adapter: decoder-adapter ontbreekt."
        7004L -> "RAW-adapter: dubbele adapterregistratie."
        7005L -> "RAW-adapter: ongeldige container."
        7006L -> "RAW-adapter: containerfeature nog niet ondersteund."
        7007L -> "RAW-adapter: decode faalde."
        7008L -> "RAW-adapter: memorybudget overschreden."
        6001L -> "Linear DNG writer: ongeldig argument."
        6002L -> "Linear DNG writer: finalized release ontbreekt."
        6003L -> "Linear DNG writer: frame/evidence/provenance-invariant geweigerd."
        6004L -> "Linear DNG writer: bronkleurmetadata kon niet veilig worden overgenomen."
        6005L -> "Linear DNG writer: bronread faalde."
        6006L -> "Linear DNG writer: broncontainer wordt niet ondersteund."
        6007L -> "Linear DNG writer: camera-native reconstructie faalde."
        6008L -> "Linear DNG writer: niet-finiete Scientific-Master sample werd geweigerd."
        6009L -> "Linear DNG writer: memorybudget overschreden."
        6010L -> "Linear DNG writer: output is te groot voor classic TIFF/DNG v0.1."
        6011L -> "Linear DNG writer: Android-bestemming ondersteunt de vereiste writes niet."
        else -> "Linear DNG-export faalde met native status $status."
    }
}

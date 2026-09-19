package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.FileInputStream

private const val PURE_DNG_MAGIC = 0x54525046L
private const val PURE_DNG_PACKET_LONGS = 19
private const val PURE_DNG_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val PURE_DNG_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object PureDngNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportPureFloat32Dng(
        sourceFd: Int,
        privateTempFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class PureDngExportMetrics(
    val width: Long,
    val height: Long,
    val outputBytes: Long,
    val projectedPixels: Long,
    val negativeComponentCount: Long,
    val overOneComponentCount: Long,
    val tilesWritten: Long,
    val logicalWorkspacePeakBytes: Long,
    val logicalResidentUpperBoundBytes: Long,
    val scientificMasterIdentityVerified: Boolean,
    val artifactCommitted: Boolean,
    val representationOnly: Boolean,
    val scientificMasterModified: Boolean,
    val appearanceApplied: Boolean,
    val counterfactualObservationCreated: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
)

sealed interface PureDngExportResult {
    data class Success(val metrics: PureDngExportMetrics) : PureDngExportResult
    data class Failed(val reason: String) : PureDngExportResult
}

object PureDngExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        privateCacheDir: File,
    ): PureDngExportResult {
        val temp = File.createTempFile("truthraw_pure_", ".dng.part", privateCacheDir)
        try {
            val source = resolver.openFileDescriptor(job.source.uri, "r")
                ?: return PureDngExportResult.Failed("Bronprovider gaf geen leesbare file descriptor.")
            val tempPfd = ParcelFileDescriptor.open(
                temp,
                ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
            val packet = source.use { sourcePfd ->
                tempPfd.use { privateOutput ->
                    PureDngNativeBridge.exportPureFloat32Dng(
                        sourcePfd.fd,
                        privateOutput.fd,
                        PURE_DNG_MAX_SOURCE_RESIDENT_BYTES,
                        PURE_DNG_MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }

            val decoded = decode(packet, temp.length())
            if (decoded !is PureDngExportResult.Success) {
                runCatching { resolver.delete(destination, null, null) }
                return decoded
            }

            val output = resolver.openOutputStream(destination, "w")
                ?: run {
                    runCatching { resolver.delete(destination, null, null) }
                    return PureDngExportResult.Failed(
                        "Bestemmingsprovider gaf geen schrijfbare outputstream.",
                    )
                }

            try {
                output.use { out ->
                    FileInputStream(temp).use { input -> input.copyTo(out) }
                }
            } catch (error: Throwable) {
                runCatching { resolver.delete(destination, null, null) }
                return PureDngExportResult.Failed(
                    "PURE DNG kon na de master-gate niet transactioneel worden vrijgegeven: " +
                        (error.message ?: error.javaClass.simpleName),
                )
            }

            return decoded
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            return PureDngExportResult.Failed(
                "TRUTHRAW PURE-export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        } finally {
            runCatching { temp.delete() }
        }
    }

    private fun decode(packet: LongArray, tempBytes: Long): PureDngExportResult {
        if (packet.size != PURE_DNG_PACKET_LONGS || packet[0] != PURE_DNG_MAGIC) {
            return PureDngExportResult.Failed("Ongeldig native TRUTHRAW PURE-resultaat.")
        }
        val status = packet[1]
        if (status != 0L) {
            return PureDngExportResult.Failed(statusDescription(status))
        }

        val metrics = PureDngExportMetrics(
            width = packet[2],
            height = packet[3],
            outputBytes = packet[4],
            projectedPixels = packet[5],
            negativeComponentCount = packet[6],
            overOneComponentCount = packet[7],
            tilesWritten = packet[8],
            logicalWorkspacePeakBytes = packet[9],
            logicalResidentUpperBoundBytes = packet[10],
            scientificMasterIdentityVerified = packet[11] != 0L,
            artifactCommitted = packet[12] != 0L,
            representationOnly = packet[13] != 0L,
            scientificMasterModified = packet[14] != 0L,
            appearanceApplied = packet[15] != 0L,
            counterfactualObservationCreated = packet[16] != 0L,
            physicalFrameCount = packet[17],
            independentEvidenceCount = packet[18],
        )

        val expectedPixels = runCatching { Math.multiplyExact(metrics.width, metrics.height) }.getOrNull()
        if (metrics.width <= 0L || metrics.height <= 0L ||
            metrics.outputBytes <= 0L || tempBytes != metrics.outputBytes ||
            expectedPixels == null || metrics.projectedPixels != expectedPixels ||
            metrics.tilesWritten <= 0L ||
            metrics.logicalResidentUpperBoundBytes <= 0L ||
            metrics.logicalResidentUpperBoundBytes > PURE_DNG_MAX_LOGICAL_RESIDENT_BYTES.toLong() ||
            !metrics.scientificMasterIdentityVerified || !metrics.artifactCommitted ||
            !metrics.representationOnly || metrics.scientificMasterModified ||
            metrics.appearanceApplied || metrics.counterfactualObservationCreated ||
            metrics.physicalFrameCount != 1L || metrics.independentEvidenceCount != 1L) {
            return PureDngExportResult.Failed(
                "Fail-closed: TRUTHRAW PURE schond master-, transaction-, memory- of evidencecontract.",
            )
        }
        return PureDngExportResult.Success(metrics)
    }

    private fun statusDescription(status: Long): String = when (status) {
        -1L -> "Ongeldige TRUTHRAW PURE-exportparameters."
        -2L -> "Fail-closed: pre-master authority-state was niet canoniek."
        -3L -> "Fail-closed: finalized master/backplane/source-identiteit was niet geldig."
        -4L -> "Fail-closed: PURE-resultaat wijzigde master/appearance/evidence of miste de exacte digest-gate."

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

        in 2101L..2199L -> "DNG kleur-binding faalde met code $status."
        in 3001L..3999L -> "Native DNG-source faalde met code $status."
        in 5001L..5999L -> "Finalized Scientific Master/Backplane-gate faalde met code $status."

        7001L -> "PURE writer: ongeldig argument."
        7002L -> "PURE writer: camera→XYZ(D50)-transform is ongeldig."
        7003L -> "PURE writer: output/werkruimte overflow."
        7004L -> "PURE writer: Scientific-Master tile replay faalde."
        7005L -> "PURE writer: Scientific-Master digest kon niet worden opgebouwd."
        7006L -> "PURE writer: gereplayde Scientific Master hash wijkt af van de toegelaten master."
        7007L -> "PURE writer: private transactionele sink faalde."
        else -> "TRUTHRAW PURE-export faalde met native status $status."
    }
}

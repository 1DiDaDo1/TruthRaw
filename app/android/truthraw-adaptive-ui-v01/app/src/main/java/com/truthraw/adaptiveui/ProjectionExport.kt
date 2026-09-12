package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val EXPORT_MAGIC = 0x54524558L
private const val EXPORT_PACKET_LONGS = 16
private const val EXPORT_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val EXPORT_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

enum class ProjectionExportKind(
    val nativeCode: Int,
    val buttonLabel: String,
    val mimeType: String,
    val suffix: String,
) {
    LINEAR_DNG_16(
        1,
        "Linear DNG opslaan",
        "image/x-adobe-dng",
        "_truthraw_linear_reconstructed.dng",
    ),
    CFA_DNG_16(
        2,
        "CFA DNG (rawsensor-projectie) opslaan",
        "image/x-adobe-dng",
        "_truthraw_cfa_rawsensor_projection.dng",
    ),
    SCIENTIFIC_MASTER_F32(
        3,
        "Scientific Master .trmaster opslaan",
        "application/octet-stream",
        "_truthraw_scientific_master.trmaster",
    ),
}

data class ProjectionExportMetrics(
    val kind: ProjectionExportKind,
    val bytesWritten: Long,
    val tilesProcessed: Long,
    val logicalWorkspacePeakBytes: Long,
    val logicalResidentUpperBoundBytes: Long,
    val negativeSamplesClamped: Long,
    val overOneSamplesClamped: Long,
    val scientificMasterDigestVerified: Boolean,
    val fullScientificMasterMaterialized: Boolean,
    val compatibilityProjection: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val claimScopeCode: Long,
    val colorAuthorityCode: Long,
)

object NativeProjectionExportBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportScientificProjection(
        inputFd: Int,
        outputFd: Int,
        projectionKind: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

object ProjectionExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        kind: ProjectionExportKind,
    ): Result<ProjectionExportMetrics> = runCatching {
        val input = resolver.openFileDescriptor(job.source.uri, "r")
            ?: error("Documentprovider gaf geen input file descriptor.")
        val output = resolver.openFileDescriptor(destination, "rw")
            ?: error("Documentprovider gaf geen seekbare output file descriptor.")

        val packet = input.use { inputPfd ->
            output.use { outputPfd ->
                NativeProjectionExportBridge.exportScientificProjection(
                    inputPfd.fd,
                    outputPfd.fd,
                    kind.nativeCode,
                    EXPORT_MAX_SOURCE_RESIDENT_BYTES,
                    EXPORT_MAX_LOGICAL_RESIDENT_BYTES,
                )
            }
        }

        require(packet.size == EXPORT_PACKET_LONGS && packet[0] == EXPORT_MAGIC) {
            "Ongeldig native projection-exportpakket."
        }
        val status = packet[1]
        require(status == 0L) { nativeExportStatusDescription(status) }
        require(packet[2] == kind.nativeCode.toLong()) { "Native export-kind mismatch." }

        val metrics = ProjectionExportMetrics(
            kind = kind,
            bytesWritten = packet[3],
            tilesProcessed = packet[4],
            logicalWorkspacePeakBytes = packet[5],
            logicalResidentUpperBoundBytes = packet[6],
            negativeSamplesClamped = packet[7],
            overOneSamplesClamped = packet[8],
            scientificMasterDigestVerified = packet[9] != 0L,
            fullScientificMasterMaterialized = packet[10] != 0L,
            compatibilityProjection = packet[11] != 0L,
            physicalFrameCount = packet[12],
            independentEvidenceCount = packet[13],
            claimScopeCode = packet[14],
            colorAuthorityCode = packet[15],
        )

        val expectedCompatibility = kind != ProjectionExportKind.SCIENTIFIC_MASTER_F32
        require(metrics.bytesWritten > 0L && metrics.tilesProcessed > 0L) {
            "Projection-export leverde geen volledige uitvoer."
        }
        require(metrics.scientificMasterDigestVerified) {
            "Fail-closed: exportreconstructie matchte Scientific Master niet."
        }
        require(!metrics.fullScientificMasterMaterialized) {
            "Fail-closed: export materialiseerde verboden full-frame Scientific Master state."
        }
        require(metrics.compatibilityProjection == expectedCompatibility) {
            "Projection-role mismatch."
        }
        require(metrics.physicalFrameCount == 1L && metrics.independentEvidenceCount == 1L) {
            "Fail-closed: export veranderde frame/evidence-contract."
        }
        require(metrics.claimScopeCode == 1L || metrics.claimScopeCode == 2L) {
            "Fail-closed: export heeft geen finalized source/master admission."
        }
        metrics
    }

    private fun nativeExportStatusDescription(status: Long): String = when (status) {
        -1L -> "Ongeldige projection-exportparameters."
        -2L -> "Doelprovider levert geen truncate/seekbare descriptor; export is fail-closed geblokkeerd."
        -3L -> "Projection-export schond full-frame/evidence/digest-contract."
        6001L -> "Projection export: ongeldig argument."
        6002L -> "Projection export: finalized authority werd geweigerd."
        6003L -> "Projection export: logisch memorybudget overschreden."
        6004L -> "Projection export: bron/tile-read faalde."
        6005L -> "Projection export: reconstruction faalde."
        6006L -> "Projection export: Scientific Master digest faalde."
        6007L -> "Projection export: opnieuw berekende Scientific Master wijkt af; doelbestand is leeggemaakt."
        6008L -> "Projection export: projectietype wordt niet ondersteund."
        6009L -> "Projection export: schrijven naar doelbestand faalde."
        6010L -> "Projection export: DNG/.trmaster metadata kon niet veilig worden opgebouwd."
        else -> "Projection-export faalde met native status $status."
    }
}

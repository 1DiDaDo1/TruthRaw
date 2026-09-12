package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri

private const val DNG_PROJECTION_MAGIC = 0x54524450L
private const val DNG_PROJECTION_PACKET_LONGS = 21
private const val MAX_SOURCE_RESIDENT_BYTES_DNG = 8 * 1024 * 1024
private const val MAX_LOGICAL_RESIDENT_BYTES_DNG = 64 * 1024 * 1024

object NativeDngProjectionBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFinalizedProjection(
        sourceFd: Int,
        outputFd: Int,
        kindCode: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

enum class DngProjectionKind(val code: Int) {
    STAGE2_CFA(0),
    LINEAR_RAW(1),
}

data class DngProjectionMetrics(
    val kind: DngProjectionKind,
    val bytesWritten: Long,
    val stripsWritten: Long,
    val canonicalTilesProcessed: Long,
    val logicalWorkspacePeakBytes: Long,
    val logicalResidentUpperBoundBytes: Long,
    val sourceVerifiedBefore: Boolean,
    val sourceVerifiedAfter: Boolean,
    val scientificMasterHashMatched: Boolean,
    val fullFrameMaterialized: Boolean,
    val createsEvidence: Boolean,
    val physicalFrameCount: Long,
    val independentEvidenceCount: Long,
    val stage2GaugeScanPasses: Long,
    val rawPayloadBytesRead: Long,
    val metadataBytesRead: Long,
    val tileReadCalls: Long,
    val colorClaimScopeCode: Long,
    val colorAuthorityCode: Long,
)

sealed interface DngProjectionResult {
    data class Success(val metrics: DngProjectionMetrics) : DngProjectionResult
    data class Failure(val reason: String) : DngProjectionResult
}

object DngProjectionExporter {
    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
        kind: DngProjectionKind,
    ): DngProjectionResult {
        val sourceDescriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return DngProjectionResult.Failure(
                "DNG-export: bron kon niet als file descriptor worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return DngProjectionResult.Failure("DNG-export: documentprovider gaf geen bron-file-descriptor.")

        val outputDescriptor = try {
            resolver.openFileDescriptor(destination, "rw")
        } catch (error: Exception) {
            sourceDescriptor.close()
            return DngProjectionResult.Failure(
                "DNG-export: doel kon niet seekable worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: run {
            sourceDescriptor.close()
            return DngProjectionResult.Failure("DNG-export: documentprovider gaf geen doel-file-descriptor.")
        }

        val packet = try {
            sourceDescriptor.use { sourcePfd ->
                outputDescriptor.use { outputPfd ->
                    NativeDngProjectionBridge.exportFinalizedProjection(
                        sourcePfd.fd,
                        outputPfd.fd,
                        kind.code,
                        MAX_SOURCE_RESIDENT_BYTES_DNG,
                        MAX_LOGICAL_RESIDENT_BYTES_DNG,
                    )
                }
            }
        } catch (error: Throwable) {
            return DngProjectionResult.Failure(
                "Native DNG-projectie faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size < 2 || packet[0] != DNG_PROJECTION_MAGIC) {
            return DngProjectionResult.Failure("DNG-export: ongeldig native projectiepakket.")
        }
        val status = packet[1]
        if (status != 0L) {
            return DngProjectionResult.Failure(nativeDngStatusDescription(status))
        }
        if (packet.size != DNG_PROJECTION_PACKET_LONGS) {
            return DngProjectionResult.Failure("DNG-export: succesvol pakket had een ongeldige lengte.")
        }

        val packetKind = when (packet[2].toInt()) {
            DngProjectionKind.STAGE2_CFA.code -> DngProjectionKind.STAGE2_CFA
            DngProjectionKind.LINEAR_RAW.code -> DngProjectionKind.LINEAR_RAW
            else -> return DngProjectionResult.Failure("DNG-export: native projectietype is onbekend.")
        }
        if (packetKind != kind) {
            return DngProjectionResult.Failure("DNG-export: native projectietype wijkt af van de gekozen export.")
        }

        val metrics = DngProjectionMetrics(
            kind = packetKind,
            bytesWritten = packet[3],
            stripsWritten = packet[4],
            canonicalTilesProcessed = packet[5],
            logicalWorkspacePeakBytes = packet[6],
            logicalResidentUpperBoundBytes = packet[7],
            sourceVerifiedBefore = packet[8] != 0L,
            sourceVerifiedAfter = packet[9] != 0L,
            scientificMasterHashMatched = packet[10] != 0L,
            fullFrameMaterialized = packet[11] != 0L,
            createsEvidence = packet[12] != 0L,
            physicalFrameCount = packet[13],
            independentEvidenceCount = packet[14],
            stage2GaugeScanPasses = packet[15],
            rawPayloadBytesRead = packet[16],
            metadataBytesRead = packet[17],
            tileReadCalls = packet[18],
            colorClaimScopeCode = packet[19],
            colorAuthorityCode = packet[20],
        )

        val sourceBoundColor =
            metrics.colorClaimScopeCode == 1L &&
                (metrics.colorAuthorityCode == 2L || metrics.colorAuthorityCode == 3L)
        val independentlyCalibratedColor =
            metrics.colorClaimScopeCode == 2L && metrics.colorAuthorityCode == 4L
        val authorityValid = sourceBoundColor || independentlyCalibratedColor
        val invariantViolation =
            metrics.bytesWritten <= 0L ||
                metrics.stripsWritten <= 0L ||
                metrics.canonicalTilesProcessed <= 0L ||
                metrics.logicalWorkspacePeakBytes <= 0L ||
                metrics.logicalResidentUpperBoundBytes <= 0L ||
                metrics.logicalResidentUpperBoundBytes > MAX_LOGICAL_RESIDENT_BYTES_DNG.toLong() ||
                !metrics.sourceVerifiedBefore ||
                !metrics.sourceVerifiedAfter ||
                !metrics.scientificMasterHashMatched ||
                metrics.fullFrameMaterialized ||
                metrics.createsEvidence ||
                metrics.physicalFrameCount != 1L ||
                metrics.independentEvidenceCount != 1L ||
                metrics.stage2GaugeScanPasses != 2L ||
                !authorityValid

        if (invariantViolation) {
            return DngProjectionResult.Failure(
                "Fail-closed: DNG-projectie schond source/master/evidence/memory/authority-contract.",
            )
        }
        return DngProjectionResult.Success(metrics)
    }

    private fun nativeDngStatusDescription(status: Long): String = when (status) {
        -1L -> "DNG-export: ongeldige native parameters."
        -2L -> "DNG-export: doelbestand is niet truncate/seekbaar; provider wordt fail-closed geweigerd."
        -3L -> "DNG-export: bronbestand is leeg of niet seekbaar."

        2001L -> "DNG-export source binding: ongeldig argument."
        2002L -> "DNG-export source binding: bron kon niet volledig worden gelezen voor SHA-256."
        2003L -> "DNG-export source binding: bronseal is niet canoniek."
        2004L -> "DNG-export source binding: bronbytes verschillen van de sealed SHA-256-identiteit."
        2005L -> "DNG-export source binding: kleurbinding heeft geen toegestane authority."
        2006L -> "DNG-export source binding: kleurbinding hoort bij andere bronbytes."
        2007L -> "DNG-export source binding: camera→XYZ(D50)-matrix is ongeldig."
        2008L -> "DNG-export source binding: frame/evidence-invariant geweigerd."
        2009L -> "DNG-export source binding: Backplane geweigerd."
        2010L -> "DNG-export source binding: Backplane-bronhash wijkt af."

        2101L -> "DNG color v0.2: ongeldig argument."
        2102L -> "DNG color v0.2: sealed bronhash mismatch."
        2103L -> "DNG color v0.2: bron kon niet worden gelezen."
        2104L -> "DNG color v0.2: ongeldige TIFF/DNG-container."
        2105L -> "DNG color v0.2: BigTIFF wordt niet ondersteund."
        2106L -> "DNG color v0.2: ongeldige IFD0."
        2107L -> "DNG color v0.2: ColorMatrix1 ontbreekt."
        2108L -> "DNG color v0.2: AsShotNeutral ontbreekt."
        2109L -> "DNG color v0.2: dual-illuminant calibratieset is onvolledig."
        2110L -> "DNG color v0.2: triple-illuminant calibratie is nog geblokkeerd."
        2111L -> "DNG color v0.2: CalibrationIlluminant vereist nog niet ondersteunde metadata."
        2112L -> "DNG color v0.2: ongeldig tagtype."
        2113L -> "DNG color v0.2: ongeldige tag-cardinaliteit."
        2114L -> "DNG color v0.2: ongeldige matrix/neutral/calibratiewaarde."
        2115L -> "DNG color v0.2: singuliere kleurmatrix."
        2116L -> "DNG color v0.2: AsShotNeutral→xy solve convergeerde niet."

        in 3001L..3013L -> "DNG-export TileNative-bron faalde met status $status."
        in 5501L..5507L -> "DNG-export Scientific Master v0.2 faalde met status $status."
        in 5601L..5609L -> "DNG-export Technical Backplane phase 2 faalde met status $status."
        6001L -> "DNG-projectie: ongeldig argument."
        6002L -> "DNG-projectie: finalized admission ontbreekt of is inconsistent."
        6003L -> "DNG-projectie: bron/kleur-identiteit wijkt af."
        6004L -> "DNG-projectie: sealed source SHA wijkt af."
        6005L -> "DNG-projectie: bron kon tijdens export niet worden gelezen."
        6006L -> "DNG-projectie: bron-TIFF valt buiten de v0.1 writer-subset."
        6007L -> "DNG-projectie: vereiste brongebonden kleurmetadata ontbreekt."
        6008L -> "DNG-projectie: camera-native reconstructie faalde."
        6009L -> "DNG-projectie: Scientific-Master-digest kon niet worden berekend."
        6010L -> "DNG-projectie: gereconstrueerde export wijkt af van finalized Scientific Master; doel is gewist."
        6011L -> "DNG-projectie: logical memorybudget overschreden."
        6012L -> "DNG-projectie: classic-TIFF offset/size grens overschreden."
        6013L -> "DNG-projectie: documentprovider kon output niet volledig schrijven."
        else -> "Onbekende native DNG-exportstatus $status."
    }
}

package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val FINALIZED_PREVIEW_MAGIC = 0x54524631
private const val FINALIZED_HEADER_INTS = 24
private const val MAX_PREVIEW_EDGE = 384
private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object NativeTilePreviewBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    // Legacy diagnostic CFA proxy. Retained only as a separate troubleshooting path.
    external fun buildCfaPreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
    ): IntArray

    // Pre-master fallback/diagnostic path. It must never be presented as a finalized Scientific Preview.
    external fun buildSourceBoundColorPreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray

    // Default UI route: exact source seal -> source-bound color -> Scientific Master/self-gauge
    // -> Technical Backplane phase 2 -> finalized Scientific Preview -> bounded sRGB pixels.
    external fun buildFinalizedScientificColorPreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray

    // Advanced derivative route: same sealed source + Scientific Master lineage,
    // but downstream appearance controls are allowed. Never writes back into PURE.
    external fun buildAdvancedDerivativePreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
        flags: Int,
        sourceRouteCode: Int,
    ): IntArray
}

enum class PreviewAuthority {
    FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW,
    FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW,
}

data class TilePreviewMetrics(
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceResidentUpperBoundBytes: Int,
    val rawPayloadBytesRead: Int,
    val metadataBytesRead: Int,
    val tileReadCalls: Int,
    val fullRawMaterialized: Boolean,
    val hasGainField: Boolean,
    val orientation: Int,
    val sourceBoundAppearanceReleaseAllowed: Boolean,
    val scientificPreviewReleaseAllowed: Boolean,
    val scientificClaimAllowed: Boolean,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val logicalResidentUpperBoundBytes: Int,
    val tilesProcessedPass1: Int,
    val tilesProcessedPass2: Int,
    val usedForwardMatrix: Boolean,
    val cameraCalibrationApplied: Boolean,
    val previewAuthority: PreviewAuthority,
    val advancedDerivative: Boolean = false,
    val advancedFlags: Int = 0,
    val advancedRestoredPixels: Int = 0,
    val advancedCensoredPreviewPixels: Int = 0,
    val advancedHdrGainPixels: Int = 0,
    val advancedLightAdjustedPixels: Int = 0,
    val advancedDetailEnabled: Boolean = false,
    val advancedRestorationEnabled: Boolean = false,
    val openWorldSceneBound: Boolean = false,
    val openWorldIlluminationAuthority: Int = -1,
    val openWorldOutputAuthority: Int = -1,
    val dynamicAuthorityCalibratedPreviewPixels: Int = 0,
    val dynamicAuthorityReconstructedPreviewPixels: Int = 0,
    val dynamicAuthorityCensoredPreviewPixels: Int = 0,
    val dynamicAuthorityUnknownRgbSamples: Int = 0,
    val restorationPresentationOnly: Boolean = false,
    val canonicalOpenSceneArtifactSha256: String? = null,
    val canonicalOpenSceneChannelAuthoritySha256: String? = null,
    val uncertaintyAdmissionCode: Int = -1,
    val uncertaintyAdmissionSha256: String? = null,
    val reconstructedAuthorityAllowedByAdmission: Boolean = false,
    val advancedDetailBackendId: String? = null,
    val advancedDetailNoiseSigmaAt2Pct: Float? = null,
    val advancedDetailBindingSha256: String? = null,
)

sealed interface TilePreviewUiState {
    data object Idle : TilePreviewUiState
    data class Loading(val jobId: String) : TilePreviewUiState
    data class Ready(
        val jobId: String,
        val bitmap: Bitmap,
        val metrics: TilePreviewMetrics,
    ) : TilePreviewUiState
    data class Failed(val jobId: String, val reason: String) : TilePreviewUiState
}

object TilePreviewLoader {
    fun load(resolver: ContentResolver, job: RawJob): TilePreviewUiState {
        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return TilePreviewUiState.Failed(
                job.id,
                "Documentprovider gaf geen leesbare file descriptor: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return TilePreviewUiState.Failed(job.id, "Documentprovider gaf geen file descriptor.")

        val packet = try {
            descriptor.use { pfd ->
                NativeTilePreviewBridge.buildFinalizedScientificColorPreview(
                    pfd.fd,
                    MAX_PREVIEW_EDGE,
                    MAX_SOURCE_RESIDENT_BYTES,
                    MAX_LOGICAL_RESIDENT_BYTES,
                )
            }
        } catch (error: Throwable) {
            return TilePreviewUiState.Failed(
                job.id,
                "Native finalized Scientific Preview faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size < FINALIZED_HEADER_INTS || packet[0] != FINALIZED_PREVIEW_MAGIC) {
            return TilePreviewUiState.Failed(job.id, "Ongeldig finalized Scientific Preview-pakket.")
        }
        val status = packet[1]
        if (status != 0) {
            return TilePreviewUiState.Failed(job.id, nativeStatusDescription(status))
        }

        val width = packet[2]
        val height = packet[3]
        if (width <= 0 || height <= 0 || width > MAX_PREVIEW_EDGE || height > MAX_PREVIEW_EDGE) {
            return TilePreviewUiState.Failed(job.id, "Native preview-afmetingen zijn buiten contract.")
        }
        val pixelCount = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return TilePreviewUiState.Failed(job.id, "Native preview-afmetingen overflowden.")
        }
        if (packet.size != FINALIZED_HEADER_INTS + pixelCount) {
            return TilePreviewUiState.Failed(job.id, "Finalized preview-payload heeft een ongeldige lengte.")
        }

        val authority = when (packet[23]) {
            1 -> PreviewAuthority.FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW
            2 -> PreviewAuthority.FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW
            else -> return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: finalized preview had geen bekende authority-code.",
            )
        }

        val bitmap = try {
            PortablePreviewEncoder.createSrgbBitmap(width, height, packet, FINALIZED_HEADER_INTS)
        } catch (error: Exception) {
            return TilePreviewUiState.Failed(job.id, "sRGB-preview kon niet worden opgebouwd: ${error.message}")
        }

        val metrics = TilePreviewMetrics(
            sourceWidth = packet[4],
            sourceHeight = packet[5],
            sourceResidentUpperBoundBytes = packet[6],
            rawPayloadBytesRead = packet[7],
            metadataBytesRead = packet[8],
            tileReadCalls = packet[9],
            fullRawMaterialized = packet[10] != 0,
            hasGainField = packet[11] != 0,
            orientation = packet[12],
            sourceBoundAppearanceReleaseAllowed = packet[13] != 0,
            scientificPreviewReleaseAllowed = packet[14] != 0,
            scientificClaimAllowed = packet[15] != 0,
            physicalFrameCount = packet[16],
            independentEvidenceCount = packet[17],
            logicalResidentUpperBoundBytes = packet[18],
            tilesProcessedPass1 = packet[19],
            tilesProcessedPass2 = packet[20],
            usedForwardMatrix = packet[21] != 0,
            cameraCalibrationApplied = packet[22] != 0,
            previewAuthority = authority,
        )

        val strongerClaimExpected =
            metrics.previewAuthority == PreviewAuthority.FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW
        val authorityViolation =
            !metrics.sourceBoundAppearanceReleaseAllowed ||
                !metrics.scientificPreviewReleaseAllowed ||
                metrics.scientificClaimAllowed != strongerClaimExpected ||
                metrics.physicalFrameCount != 1 ||
                metrics.independentEvidenceCount != 1
        val memoryViolation =
            metrics.fullRawMaterialized ||
                metrics.logicalResidentUpperBoundBytes <= 0 ||
                metrics.logicalResidentUpperBoundBytes > MAX_LOGICAL_RESIDENT_BYTES
        val executionViolation = metrics.tilesProcessedPass1 <= 0 || metrics.tilesProcessedPass2 <= 0

        if (authorityViolation || memoryViolation || executionViolation) {
            bitmap.recycle()
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: finalized preview schond authority-, evidence-, tile- of memorycontract.",
            )
        }

        return TilePreviewUiState.Ready(job.id, bitmap, metrics)
    }

    private fun nativeStatusDescription(status: Int): String = when (status) {
        -1 -> "Ongeldige finalized previewparameters."
        -2 -> "Fail-closed: pre-master authority-state was niet canoniek."
        -3 -> "Fail-closed: finalized route materialiseerde verboden full-frame state of leverde een onvolledig oppervlak."
        -4 -> "Fail-closed: finalized authority/frame/evidence/provenance-invariant werd geschonden."

        2001 -> "Source binding: ongeldig argument."
        2002 -> "Source binding: bron kon niet volledig worden gelezen voor SHA-256."
        2003 -> "Source binding: bronseal is niet canoniek."
        2004 -> "Source binding: bronbytes verschillen van de sealed SHA-256 identiteit."
        2005 -> "Source binding: kleurbinding heeft geen toegestane authority."
        2006 -> "Source binding: kleurbinding hoort bij andere bronbytes."
        2007 -> "Source binding: camera→XYZ(D50)-matrix is ongeldig."
        2008 -> "Source binding: frame/evidence-invariant geweigerd."
        2009 -> "Source binding: Backplane geweigerd."
        2010 -> "Source binding: Backplane-bronhash wijkt af."

        2101 -> "DNG color producer v0.2: ongeldig argument."
        2102 -> "DNG color producer v0.2: sealed bronhash mismatch."
        2103 -> "DNG color producer v0.2: bron kon niet worden gelezen."
        2104 -> "DNG color producer v0.2: ongeldige TIFF/DNG-container."
        2105 -> "DNG color producer v0.2: BigTIFF wordt niet ondersteund."
        2106 -> "DNG color producer v0.2: ongeldige IFD0."
        2107 -> "DNG color producer v0.2: ColorMatrix1 ontbreekt."
        2108 -> "DNG color producer v0.2: AsShotNeutral ontbreekt."
        2109 -> "DNG color producer v0.2: dual-illuminant calibratieset is onvolledig; fail-closed."
        2110 -> "DNG color producer v0.2: triple-illuminant calibratie wordt nog niet ondersteund; fail-closed."
        2111 -> "DNG color producer v0.2: CalibrationIlluminant vereist nog niet ondersteunde IlluminantData/temperatuurmapping."
        2112 -> "DNG color producer v0.2: ongeldig tagtype."
        2113 -> "DNG color producer v0.2: ongeldige tag-cardinaliteit."
        2114 -> "DNG color producer v0.2: ongeldige matrix/neutral/calibratiewaarde."
        2115 -> "DNG color producer v0.2: singuliere kleurmatrix."
        2116 -> "DNG color producer v0.2: AsShotNeutral→xy iteratie convergeerde niet binnen 30 stappen; fail-closed."

        3001 -> "TileNativeDngSource: I/O-fout of niet-seekbare documentprovider."
        3002 -> "TileNativeDngSource: ongeldige TIFF/DNG-container."
        3003 -> "TileNativeDngSource: BigTIFF wordt in v0.1 niet ondersteund."
        3004 -> "TileNativeDngSource: compressie wordt in v0.1 niet ondersteund."
        3005 -> "TileNativeDngSource: sample-opslag wordt in v0.1 niet ondersteund."
        3006 -> "TileNativeDngSource: geen ondersteunde CFA-IFD gevonden."
        3007 -> "TileNativeDngSource: RAW-topologie wordt niet ondersteund."
        3008 -> "TileNativeDngSource: meerdere CFA-IFD's vereisen expliciete binding."
        3009 -> "TileNativeDngSource: verplichte DNG-tag ontbreekt."
        3010 -> "TileNativeDngSource: ongeldige DNG-tag."
        3011 -> "TileNativeDngSource: ongeldige strip/tile-opslag."
        3012 -> "TileNativeDngSource: vereiste bron/kleur-binding ontbreekt."
        3013 -> "TileNativeDngSource: resident-memorybudget overschreden."

        4001 -> "Main House streaming: ongeldig argument."
        4002 -> "Main House streaming: tile-bron faalde."
        4003 -> "Main House streaming: bounded preview-sink faalde."
        4004 -> "Main House streaming: logisch memorybudget overschreden."
        4005 -> "Main House streaming: reconstructie/appearance backend faalde."
        4006 -> "Main House streaming: uitvoering wordt niet ondersteund."

        5001 -> "Finalized release: ongeldig argument."
        5002 -> "Finalized release: Technical Backplane werd geweigerd."
        5003 -> "Finalized release: bronidentiteit wijkt af."
        5004 -> "Finalized release: kleuridentiteit wijkt af."
        5005 -> "Finalized release: Scientific Master/TruthRange/phase-2 kon niet worden gefinaliseerd."
        5006 -> "Finalized release: wetenschappelijke identiteit wijkt af."
        5007 -> "Finalized release: bounded streaming faalde."
        5008 -> "Finalized release: provenance/resource-invariant werd geweigerd."
        5009 -> "Finalized release: preview-oppervlak bleef onvolledig."

        7001 -> "RAW-adapter: ongeldig argument."
        7002 -> "RAW-adapter: sealed bronlengte kwam niet overeen."
        7003 -> "RAW-adapter: decoder-adapter ontbreekt."
        7004 -> "RAW-adapter: dubbele adapterregistratie."
        7005 -> "RAW-adapter: ongeldige container."
        7006 -> "RAW-adapter: containerfeature nog niet ondersteund."
        7007 -> "RAW-adapter: decode faalde."
        7008 -> "RAW-adapter: memorybudget overschreden."

        else -> "Onbekende native finalized Scientific Preview-status $status."
    }
}

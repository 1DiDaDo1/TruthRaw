package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.content.Context
import android.graphics.Bitmap

data class TruthRawAdvancedOptions(
    val naturalLight: Boolean = true,
    val naturalHdr: Boolean = true,
    val detail: Boolean = false,
    val restoration: Boolean = true,
) {
    fun flags(): Int {
        var flags = 0
        if (naturalLight) flags = flags or FLAG_LIGHT
        if (naturalHdr) flags = flags or FLAG_HDR
        if (detail) flags = flags or FLAG_DETAIL
        if (restoration) flags = flags or FLAG_RESTORATION
        return flags
    }

    companion object {
        const val FLAG_LIGHT = 1 shl 0
        const val FLAG_HDR = 1 shl 1
        const val FLAG_DETAIL = 1 shl 2
        const val FLAG_RESTORATION = 1 shl 3
    }
}

object TruthRawAdvancedSettings {
    private const val PREFS = "truthraw_advanced"
    private const val KEY_LIGHT = "natural_light"
    private const val KEY_HDR = "natural_hdr"
    private const val KEY_DETAIL = "detail"
    private const val KEY_RESTORATION = "restoration"

    fun load(context: Context): TruthRawAdvancedOptions {
        val p = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        return TruthRawAdvancedOptions(
            naturalLight = p.getBoolean(KEY_LIGHT, true),
            naturalHdr = p.getBoolean(KEY_HDR, true),
            detail = p.getBoolean(KEY_DETAIL, false),
            restoration = p.getBoolean(KEY_RESTORATION, true),
        )
    }

    fun save(context: Context, options: TruthRawAdvancedOptions) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putBoolean(KEY_LIGHT, options.naturalLight)
            .putBoolean(KEY_HDR, options.naturalHdr)
            .putBoolean(KEY_DETAIL, options.detail)
            .putBoolean(KEY_RESTORATION, options.restoration)
            .apply()
    }
}

object AdvancedTilePreviewLoader {
    private const val MAGIC = 0x54524144
    private const val HEADER_INTS = 40
    private const val MAX_PREVIEW_EDGE = 384
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun load(
        context: Context,
        resolver: ContentResolver,
        job: RawJob,
    ): TilePreviewUiState {
        val options = TruthRawAdvancedSettings.load(context)
        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced: bron kon niet worden geopend: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return TilePreviewUiState.Failed(job.id, "Advanced: geen leesbare file descriptor.")

        val packet = try {
            descriptor.use { pfd ->
                NativeTilePreviewBridge.buildAdvancedDerivativePreview(
                    pfd.fd,
                    MAX_PREVIEW_EDGE,
                    MAX_SOURCE_RESIDENT_BYTES,
                    MAX_LOGICAL_RESIDENT_BYTES,
                    options.flags(),
                )
            }
        } catch (error: Throwable) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced native route faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size < HEADER_INTS || packet[0] != MAGIC) {
            return TilePreviewUiState.Failed(job.id, "Ongeldig TRUTHRAW ADVANCED-pakket.")
        }
        if (packet[1] != 0) {
            return TilePreviewUiState.Failed(job.id, advancedStatusDescription(packet[1]))
        }

        val width = packet[2]
        val height = packet[3]
        if (width <= 0 || height <= 0 || width > MAX_PREVIEW_EDGE || height > MAX_PREVIEW_EDGE) {
            return TilePreviewUiState.Failed(job.id, "Advanced preview-afmetingen buiten contract.")
        }
        val pixelCount = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return TilePreviewUiState.Failed(job.id, "Advanced preview-afmetingen overflowden.")
        }
        if (packet.size != HEADER_INTS + pixelCount) {
            return TilePreviewUiState.Failed(job.id, "Advanced preview-payload heeft ongeldige lengte.")
        }
        if (packet[6] != options.flags()) {
            return TilePreviewUiState.Failed(job.id, "Advanced opties veranderden tussen aanvraag en native uitvoering.")
        }
        if (packet[13] != 1 || packet[14] != 1) {
            return TilePreviewUiState.Failed(job.id, "Advanced frame/evidence invariant is niet 1/1.")
        }
        if (packet[15] <= 0 || packet[15] > MAX_LOGICAL_RESIDENT_BYTES) {
            return TilePreviewUiState.Failed(job.id, "Advanced logical resident budget werd overschreden.")
        }
        if (packet[31] != 2 ||
            packet[32] != 1 ||
            packet[33] != 2 ||
            packet[34] != 4 ||
            packet[39] != 1
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced Open-World/Dynamic-Authority contract ontbreekt of is niet fail-closed.",
            )
        }

        val authority = when (packet[20]) {
            1 -> PreviewAuthority.FINALIZED_SOURCE_BOUND_SCIENTIFIC_PREVIEW
            2 -> PreviewAuthority.FINALIZED_INDEPENDENTLY_CALIBRATED_SCIENTIFIC_PREVIEW
            else -> return TilePreviewUiState.Failed(job.id, "Advanced had geen toegelaten kleur-authority.")
        }

        val bitmap: Bitmap = try {
            PortablePreviewEncoder.createSrgbBitmap(width, height, packet, HEADER_INTS)
        } catch (error: Exception) {
            return TilePreviewUiState.Failed(job.id, "Advanced sRGB-afbeelding faalde: ${error.message}")
        }

        val metrics = TilePreviewMetrics(
            sourceWidth = packet[4],
            sourceHeight = packet[5],
            sourceResidentUpperBoundBytes = packet[7],
            rawPayloadBytesRead = packet[8],
            metadataBytesRead = packet[9],
            tileReadCalls = packet[10],
            fullRawMaterialized = false,
            hasGainField = packet[11] != 0,
            orientation = packet[12],
            sourceBoundAppearanceReleaseAllowed = packet[18] != 0,
            scientificPreviewReleaseAllowed = true,
            scientificClaimAllowed = packet[20] == 2,
            physicalFrameCount = packet[13],
            independentEvidenceCount = packet[14],
            logicalResidentUpperBoundBytes = packet[15],
            tilesProcessedPass1 = packet[16],
            tilesProcessedPass2 = packet[17],
            usedForwardMatrix = packet[21] != 0,
            cameraCalibrationApplied = packet[22] != 0,
            previewAuthority = authority,
            advancedDerivative = true,
            advancedFlags = packet[6],
            advancedRestoredPixels = packet[25],
            advancedCensoredPreviewPixels = packet[26],
            advancedHdrGainPixels = packet[27],
            advancedLightAdjustedPixels = packet[28],
            advancedDetailEnabled = packet[29] != 0,
            advancedRestorationEnabled = packet[30] != 0,
            openWorldSceneBound = packet[32] != 0,
            openWorldIlluminationAuthority = packet[33],
            openWorldOutputAuthority = packet[34],
            dynamicAuthorityCalibratedPreviewPixels = packet[35],
            dynamicAuthorityReconstructedPreviewPixels = packet[36],
            dynamicAuthorityCensoredPreviewPixels = packet[37],
            dynamicAuthorityUnknownRgbSamples = packet[38],
            restorationPresentationOnly = packet[39] != 0,
        )

        if (!metrics.sourceBoundAppearanceReleaseAllowed ||
            metrics.physicalFrameCount != 1 ||
            metrics.independentEvidenceCount != 1 ||
            metrics.tilesProcessedPass1 <= 0 ||
            metrics.tilesProcessedPass2 <= 0
        ) {
            bitmap.recycle()
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: Advanced derivative schond authority-, evidence- of tilecontract.",
            )
        }

        return TilePreviewUiState.Ready(job.id, bitmap, metrics)
    }

    private fun advancedStatusDescription(status: Int): String = when (status) {
        -1 -> "Advanced: ongeldige opties of previewparameters."
        -2 -> "Advanced: sealed-source pre-master authority-state geweigerd."
        -3 -> "Advanced: DNG-auditbron ontbreekt."
        -4 -> "Advanced: Scientific Master/Backplane lineage geweigerd."
        -5 -> "Advanced: appearance/provenance/resource-invariant geweigerd."
        -6 -> "Advanced: restoration-mask kon niet veilig worden toegepast."
        -7 -> "Advanced: verboden full-frame RAW/file materialisatie gedetecteerd."
        -8 -> "Advanced: previewoppervlak bleef onvolledig."
        -9 -> "Advanced: Open-World illumination-authority binding werd geweigerd."
        in 2001..2099 -> "Advanced source binding faalde ($status)."
        in 2101..2199 -> "Advanced DNG-kleurbinding faalde ($status)."
        in 4001..4099 -> "Advanced streaming faalde ($status)."
        in 7001..7099 -> "Advanced RAW-adapter faalde ($status)."
        in 8001..8099 -> "Advanced Scientific Master-binding faalde ($status)."
        in 9001..9099 -> "Advanced Technical Backplane phase-2 faalde ($status)."
        else -> "Onbekende TRUTHRAW ADVANCED-status $status."
    }
}

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
    private const val HEADER_INTS = 176
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
                    when (job.source.sourceRoute) {
                        SourceIngressRoute.IMPORTED_FILE -> 0
                        SourceIngressRoute.CAMERA_CAPTURE -> 1
                    },
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
        if (packet[31] != 3 ||
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

        val canonicalOpenSceneSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[40 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        if (canonicalOpenSceneSha256.all { it == '0' }) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced canonical Open Scene artifact identity ontbreekt.",
            )
        }

        val channelAuthoritySha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[48 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        if (channelAuthoritySha256.all { it == '0' }) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced Open Scene v0.78 channel-authority identity ontbreekt.",
            )
        }

        val uncertaintyAdmissionCode = packet[56]
        val reconstructedAllowedByAdmission = packet[57] != 0
        val uncertaintyAdmissionSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[58 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        if (uncertaintyAdmissionSha256.all { it == '0' }) {
            return TilePreviewUiState.Failed(
                job.id,
                "Advanced v0.79 uncertainty-admission identity ontbreekt.",
            )
        }
        val expectedBlockedCode = when (job.source.sourceRoute) {
            SourceIngressRoute.IMPORTED_FILE -> 0 // BLOCKED_NO_SOURCE_ATTESTATION
            SourceIngressRoute.CAMERA_CAPTURE -> 1 // BLOCKED_SOURCE_DOMAIN_MISMATCH
        }
        if (uncertaintyAdmissionCode != expectedBlockedCode ||
            reconstructedAllowedByAdmission ||
            packet[36] != 0
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: v0.79 uncertainty-admission of RECONSTRUCTED authority week af van de bronroute.",
            )
        }

        val detailBackendCode = packet[66]
        val detailNoiseSigmaAt2Pct = Float.fromBits(packet[67])
        val detailBindingSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[68 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        if (options.detail) {
            if (detailBackendCode != 1 ||
                !detailNoiseSigmaAt2Pct.isFinite() ||
                detailNoiseSigmaAt2Pct < 0f ||
                detailBindingSha256.all { it == '0' }
            ) {
                return TilePreviewUiState.Failed(
                    job.id,
                    "Fail-closed: v0.80 Adaptive Detail mist canonieke backend/sigma/binding.",
                )
            }
        } else {
            if (detailBackendCode != 0 ||
                packet[67] != 0 ||
                detailBindingSha256.any { it != '0' }
            ) {
                return TilePreviewUiState.Failed(
                    job.id,
                    "Fail-closed: v0.80 Detail-afleiding bestaat terwijl Detail uit staat.",
                )
            }
        }

        val outputAcutanceApplied = packet[76] != 0
        val outputAcutanceProfile = packet[77]
        val outputAcutanceNoiseSigmaAt2Pct = Float.fromBits(packet[78])
        val outputAcutanceResizeRatio = Float.fromBits(packet[79])
        val outputAcutanceStrength = Float.fromBits(packet[80])
        val outputAcutanceDeltaCap = Float.fromBits(packet[81])
        val outputAcutanceHdrRebased = packet[82] != 0
        val outputAcutanceChangedPixels = packet[83]
        val outputAcutanceHdrRebasedPixels = packet[84]
        val outputAcutanceMaxHdrTargetAbsError = Float.fromBits(packet[85])
        val outputAcutanceBindingSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[88 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        val expectedOutputProfile = if (options.detail) 2 else 0
        if (!outputAcutanceApplied ||
            outputAcutanceProfile != expectedOutputProfile ||
            !outputAcutanceNoiseSigmaAt2Pct.isFinite() ||
            outputAcutanceNoiseSigmaAt2Pct < 0f ||
            !outputAcutanceResizeRatio.isFinite() ||
            outputAcutanceResizeRatio < 1f ||
            !outputAcutanceStrength.isFinite() ||
            outputAcutanceStrength < 0.012f ||
            outputAcutanceStrength > 0.130001f ||
            !outputAcutanceDeltaCap.isFinite() ||
            outputAcutanceDeltaCap < 0.0045f ||
            outputAcutanceDeltaCap > 0.007001f ||
            outputAcutanceHdrRebased != options.naturalHdr ||
            outputAcutanceChangedPixels < 0 ||
            outputAcutanceHdrRebasedPixels < 0 ||
            !outputAcutanceMaxHdrTargetAbsError.isFinite() ||
            outputAcutanceMaxHdrTargetAbsError < 0f ||
            packet[86] != 1 ||
            packet[87] != 0 ||
            outputAcutanceBindingSha256.all { it == '0' } ||
            outputAcutanceHdrRebasedPixels != packet[27] ||
            (!options.naturalHdr && outputAcutanceHdrRebasedPixels != 0)
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: v0.81 Output Acutance/HDR-rebase contract week af.",
            )
        }

        val illuminationWhitePointAuthority = packet[96]
        val illuminationWhitePointKnown = packet[97] != 0
        val illuminationCctK = Float.fromBits(packet[98])
        val illuminationDuv1960 = Float.fromBits(packet[99])
        val illuminationWhiteX = Float.fromBits(packet[100])
        val illuminationWhiteY = Float.fromBits(packet[101])
        val illuminationSceneLightKind = packet[102]
        val illuminationSpectrumAuthority = packet[103]
        val illuminationDirectionAuthority = packet[104]
        val illuminationSpatialExtentAuthority = packet[105]
        val illuminationTemporalAuthority = packet[106]
        val illuminationDualCalibrationUsed = packet[107] != 0
        val illuminationCalibrationIlluminant1 = packet[108]
        val illuminationCalibrationIlluminant2 = packet[109]
        val illuminationStateSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[118 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }

        if (illuminationWhitePointAuthority !in 0..1 ||
            illuminationSceneLightKind != 0 ||
            illuminationSpectrumAuthority != 0 ||
            illuminationDirectionAuthority != 0 ||
            illuminationSpatialExtentAuthority != 0 ||
            illuminationTemporalAuthority != 0 ||
            packet[110] != 0 ||
            packet[111] != 0 ||
            packet[112] != 0 ||
            packet[113] != 0 ||
            packet[114] != 0 ||
            packet[115] != 0 ||
            packet[116] != 1 ||
            packet[117] != 1 ||
            packet[126] != 0 ||
            packet[127] != 0 ||
            illuminationStateSha256.all { it == '0' }
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: v0.82 illumination-authority/state contract week af.",
            )
        }

        if (illuminationWhitePointKnown) {
            if (illuminationWhitePointAuthority != 1 ||
                !illuminationCctK.isFinite() || illuminationCctK <= 0f ||
                !illuminationDuv1960.isFinite() ||
                !illuminationWhiteX.isFinite() || illuminationWhiteX <= 0f ||
                !illuminationWhiteY.isFinite() || illuminationWhiteY <= 0f ||
                illuminationWhiteX + illuminationWhiteY >= 1f
            ) {
                return TilePreviewUiState.Failed(
                    job.id,
                    "Fail-closed: v0.82 bekende white-point staat mist geldige brongebonden coördinaten.",
                )
            }
        } else {
            if (illuminationWhitePointAuthority != 0 ||
                packet[98] != 0 || packet[99] != 0 ||
                packet[100] != 0 || packet[101] != 0
            ) {
                return TilePreviewUiState.Failed(
                    job.id,
                    "Fail-closed: v0.82 onbekende white-point mag geen CCT/Duv/x/y payload dragen.",
                )
            }
        }

        val hdrScientificAuthority = packet[128]
        val hdrPresentationAuthority = packet[129]
        val hdrBlockedReason = packet[130]
        val hdrScientificGainAllowed = packet[131] != 0
        val hdrPresentationGainAllowed = packet[132] != 0
        val hdrAuthorityStateSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[148 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        val expectedHdrPresentationAuthority = if (options.naturalHdr) 1 else 0

        val outputChannelAuthorityAvailable = packet[160] != 0
        val outputChannelAuthorityMappingMode = packet[161]
        val outputAuthorityCalibratedChannels = packet[162]
        val outputAuthorityReconstructedChannels = packet[163]
        val outputAuthorityCensoredChannels = packet[164]
        val outputAuthorityUnknownChannels = packet[165]
        val outputAuthorityCensoredSupportPixels = packet[166]
        val outputAuthorityPixelCount = packet[167]
        val outputAuthorityArtifactSha256 = buildString(64) {
            for (word in 0 until 8) {
                val value = packet[168 + word]
                for (byte in 0 until 4) {
                    append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                }
            }
        }
        val outputAuthorityRecordCount =
            outputAuthorityCalibratedChannels +
                outputAuthorityReconstructedChannels +
                outputAuthorityCensoredChannels +
                outputAuthorityUnknownChannels
        if (!outputChannelAuthorityAvailable ||
            outputChannelAuthorityMappingMode !in 1..2 ||
            outputAuthorityPixelCount != width * height ||
            outputAuthorityRecordCount != outputAuthorityPixelCount * 3 ||
            outputAuthorityArtifactSha256.all { it == '0' } ||
            outputAuthorityReconstructedChannels != 0 ||
            outputAuthorityUnknownChannels <= 0
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: v0.84 per-output-channel authority map ontbreekt of promoveert authority.",
            )
        }

        if (hdrScientificAuthority != 0 ||
            hdrPresentationAuthority != expectedHdrPresentationAuthority ||
            hdrBlockedReason != 2 ||
            hdrScientificGainAllowed ||
            hdrPresentationGainAllowed != options.naturalHdr ||
            packet[133] != 0 ||
            packet[134] != 0 ||
            packet[135] != 0 ||
            packet[136] != 1 ||
            packet[137] != 1 ||
            packet[138] != packet[27] ||
            packet[139] != width * height ||
            packet[140] != 1 ||
            packet[141] != 1 ||
            packet[142] != 1 ||
            packet[143] != (if (reconstructedAllowedByAdmission) 1 else 0) ||
            packet[144] != 1 ||
            packet[145] != (if (illuminationWhitePointKnown) 1 else 0) ||
            packet[146] != 0 ||
            packet[147] != 0 ||
            packet[156] != 0 || packet[157] != 0 ||
            packet[158] != 0 || packet[159] != 0 ||
            hdrAuthorityStateSha256.all { it == '0' }
        ) {
            return TilePreviewUiState.Failed(
                job.id,
                "Fail-closed: v0.83 HDR authority contract week af.",
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
            canonicalOpenSceneArtifactSha256 = canonicalOpenSceneSha256,
            canonicalOpenSceneChannelAuthoritySha256 = channelAuthoritySha256,
            uncertaintyAdmissionCode = uncertaintyAdmissionCode,
            uncertaintyAdmissionSha256 = uncertaintyAdmissionSha256,
            reconstructedAuthorityAllowedByAdmission = reconstructedAllowedByAdmission,
            advancedDetailBackendId = if (options.detail) {
                "adaptive_detailed_crisp_multiband_hard_edge_guard_v47j"
            } else null,
            advancedDetailNoiseSigmaAt2Pct = if (options.detail) {
                detailNoiseSigmaAt2Pct
            } else null,
            advancedDetailBindingSha256 = if (options.detail) {
                detailBindingSha256
            } else null,
            outputAcutanceApplied = outputAcutanceApplied,
            outputAcutanceProfile = outputAcutanceProfile,
            outputAcutanceNoiseSigmaAt2Pct = outputAcutanceNoiseSigmaAt2Pct,
            outputAcutanceResizeRatio = outputAcutanceResizeRatio,
            outputAcutanceStrength = outputAcutanceStrength,
            outputAcutanceDeltaCap = outputAcutanceDeltaCap,
            outputAcutanceHdrRebased = outputAcutanceHdrRebased,
            outputAcutanceChangedPixels = outputAcutanceChangedPixels,
            outputAcutanceHdrRebasedPixels = outputAcutanceHdrRebasedPixels,
            outputAcutanceMaxHdrTargetAbsError = outputAcutanceMaxHdrTargetAbsError,
            outputAcutanceBindingSha256 = outputAcutanceBindingSha256,
            illuminationWhitePointAuthority = illuminationWhitePointAuthority,
            illuminationWhitePointKnown = illuminationWhitePointKnown,
            illuminationCctK = if (illuminationWhitePointKnown) illuminationCctK else null,
            illuminationDuv1960 = if (illuminationWhitePointKnown) illuminationDuv1960 else null,
            illuminationWhiteX = if (illuminationWhitePointKnown) illuminationWhiteX else null,
            illuminationWhiteY = if (illuminationWhitePointKnown) illuminationWhiteY else null,
            illuminationSceneLightKind = illuminationSceneLightKind,
            illuminationSpectrumAuthority = illuminationSpectrumAuthority,
            illuminationDirectionAuthority = illuminationDirectionAuthority,
            illuminationSpatialExtentAuthority = illuminationSpatialExtentAuthority,
            illuminationTemporalAuthority = illuminationTemporalAuthority,
            illuminationDualCalibrationUsed = illuminationDualCalibrationUsed,
            illuminationCalibrationIlluminant1 = illuminationCalibrationIlluminant1,
            illuminationCalibrationIlluminant2 = illuminationCalibrationIlluminant2,
            illuminationStateSha256 = illuminationStateSha256,
            hdrScientificAuthority = hdrScientificAuthority,
            hdrPresentationAuthority = hdrPresentationAuthority,
            hdrBlockedReason = hdrBlockedReason,
            hdrScientificGainAllowed = hdrScientificGainAllowed,
            hdrPresentationGainAllowed = hdrPresentationGainAllowed,
            hdrRequiresPerOutputChannelAuthority = packet[136] != 0,
            hdrRequiresAdmittedUncertaintyForReconstructed = packet[137] != 0,
            hdrAuthorityStateSha256 = hdrAuthorityStateSha256,
            outputChannelAuthorityAvailable = outputChannelAuthorityAvailable,
            outputChannelAuthorityMappingMode = outputChannelAuthorityMappingMode,
            outputAuthorityCalibratedChannels = outputAuthorityCalibratedChannels,
            outputAuthorityReconstructedChannels = outputAuthorityReconstructedChannels,
            outputAuthorityCensoredChannels = outputAuthorityCensoredChannels,
            outputAuthorityUnknownChannels = outputAuthorityUnknownChannels,
            outputAuthorityCensoredSupportPixels = outputAuthorityCensoredSupportPixels,
            outputAuthorityArtifactSha256 = outputAuthorityArtifactSha256,
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
        -10 -> "Advanced: canonical Open Scene v0.70 kon niet exact uit de bron worden opgebouwd."
        -11 -> "Advanced: Open Scene v0.78 channel-authority sidecar faalde fail-closed."
        -12 -> "Advanced: v0.79 gaf onverwacht RECONSTRUCTED authority vrij zonder toegelaten trace/runtime p95-pad."
        -13 -> "Advanced: v0.80 Adaptive Detail provenance/authority-grens werd geschonden."
        -14 -> "Advanced: v0.81 Output Acutance/HDR-rebase faalde fail-closed."
        -16 -> "Advanced: v0.84 per-output-channel authority-map faalde fail-closed."
        in 2001..2099 -> "Advanced source binding faalde ($status)."
        in 2101..2199 -> "Advanced DNG-kleurbinding faalde ($status)."
        in 4001..4099 -> "Advanced streaming faalde ($status)."
        in 7001..7099 -> "Advanced RAW-adapter faalde ($status)."
        in 8001..8099 -> "Advanced Scientific Master-binding faalde ($status)."
        in 9001..9099 -> "Advanced Technical Backplane phase-2 faalde ($status)."
        else -> "Onbekende TRUTHRAW ADVANCED-status $status."
    }
}

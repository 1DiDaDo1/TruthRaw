package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val TN_CONTINUOUS_MAGIC = 0x35434e54
private const val TN_CONTINUOUS_HEADER_INTS = 120
private const val TN_CONTINUOUS_MAX_EDGE = 192
private const val TN_CONTINUOUS_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
private const val TN_CONTINUOUS_MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

object TruthNegativeContinuousNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun buildProContinuousPreview(
        sourceFd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray
}

data class TruthNegativeContinuousPreviewMetrics(
    val width: Int,
    val height: Int,
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceOrientation: Int,
    val authorityFieldRecords: Int,
    val calibratedEstimateRecords: Int,
    val reconstructedRecords: Int,
    val censoredRecords: Int,
    val unknownRecords: Int,
    val targetPixels: Int,
    val resolvedReconstructedChannels: Int,
    val resolvedCensoredChannels: Int,
    val resolvedUnknownChannels: Int,
    val p95KnownChannels: Int,
    val boundKnownChannels: Int,
    val sourceFootprintLinks: Int,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val createsNewEvidence: Boolean,
    val scientificWritebackAllowed: Boolean,
    val appearanceApplied: Boolean,
    val displayEncoded: Boolean,
    val sourceSceneMutated: Boolean,
    val masterFieldBitMismatches: Int,
    val masterFieldBitMatches: Int,
    val usedForwardMatrix: Boolean,
    val cameraCalibrationApplied: Boolean,
    val displayClampPixels: Int,
    val sourceTileLoads: Int,
    val stateSha256: String,
    val authorityFieldSha256: String,
    val drawNegativeStateSha256: String,
    val n2AuditExecuted: Boolean,
    val n2NoiseProfileAvailable: Boolean,
    val n2Sampled: Int,
    val n2Eligible: Int,
    val n2CorrectedCandidates: Int,
    val n2Preserved: Int,
    val n2CensoredProtected: Int,
    val n2CensorBoundaryProtected: Int,
    val n2StructureProtected: Int,
    val n2UnknownNoiseProtected: Int,
    val n2NoNeighborhoodProtected: Int,
    val n2ResidualOutlierProtected: Int,
    val n2BorderProtected: Int,
    val n2RemovedResidualEnergyFraction: Double,
    val n2MaxAbsCorrectionStage2: Double,
    val n2SamplingPeriod: Int,
    val n2CandidateSha256: String,
    val n2AuditSha256: String,
    val n2CfaPhaseSamples: List<Int>,
    val n2SourceValuesModified: Boolean,
    val n2TruthNegativeModified: Boolean,
    val n2CreatesNewEvidence: Boolean,
    val n2ScientificWritebackAllowed: Boolean,
    val n2CandidateAppliedToAppearance: Boolean,
    val n2AuditOnly: Boolean,
    val n2MeasuredCfaDomain: Boolean,
    val n2AppearanceCandidateAvailable: Boolean,
    val n2AppearanceOnly: Boolean,
    val n2AppearanceCandidateRendered: Boolean,
    val n2AppearanceCreatesNewEvidence: Boolean,
    val n2AppearanceScientificWritebackAllowed: Boolean,
    val n2AppearanceSourceSceneMutated: Boolean,
    val n2AppearanceGridWidth: Int,
    val n2AppearanceGridHeight: Int,
    val n2AppearanceChangedPixels: Int,
    val n2AppearanceAdjustedChannels: Int,
    val n2AppearanceDisplayClampPixels: Int,
    val n2AppearanceGridSha256: String,
)

sealed interface TruthNegativeContinuousPreviewResult {
    data class Ready(
        val bitmap: Bitmap,
        val n2CandidateBitmap: Bitmap,
        val metrics: TruthNegativeContinuousPreviewMetrics,
    ) : TruthNegativeContinuousPreviewResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeContinuousPreviewResult
}

object TruthNegativeContinuousPreviewLoader {
    fun load(
        resolver: ContentResolver,
        job: RawJob,
    ): TruthNegativeContinuousPreviewResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative v0.1 vereist de volledig admitted DNG-route.",
            )
        }

        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "Bron kon niet voor D.RAWnegative worden geopend: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        } ?: return TruthNegativeContinuousPreviewResult.Failed(
            "Documentprovider gaf geen file descriptor voor D.RAWnegative.",
        )

        val packet = try {
            descriptor.use { pfd ->
                TruthNegativeContinuousNativeBridge.buildProContinuousPreview(
                    pfd.fd,
                    TN_CONTINUOUS_MAX_EDGE,
                    TN_CONTINUOUS_MAX_SOURCE_RESIDENT_BYTES,
                    TN_CONTINUOUS_MAX_LOGICAL_RESIDENT_BYTES,
                )
            }
        } catch (error: Throwable) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "Native D.RAWnegative bridge faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        if (packet.size < TN_CONTINUOUS_HEADER_INTS ||
            packet[0] != TN_CONTINUOUS_MAGIC
        ) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "Ongeldig D.RAWnegative v0.1 pakket.",
            )
        }
        if (packet[1] != 0) {
            return TruthNegativeContinuousPreviewResult.Failed(
                nativeStatus(packet[1]),
            )
        }

        val width = packet[2]
        val height = packet[3]
        if (width <= 0 || height <= 0 ||
            width > TN_CONTINUOUS_MAX_EDGE ||
            height > TN_CONTINUOUS_MAX_EDGE
        ) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative preview-afmetingen zijn buiten contract.",
            )
        }

        val pixels = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative preview-afmetingen overflowden.",
            )
        }
        val expectedPacketSize = try {
            Math.addExact(
                TN_CONTINUOUS_HEADER_INTS,
                Math.multiplyExact(pixels, 2),
            )
        } catch (_: ArithmeticException) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative A/B-payloadgrootte overflowde.",
            )
        }
        if (packet.size != expectedPacketSize) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative A/B-preview-payload heeft een ongeldige lengte.",
            )
        }

        val bitmap = try {
            Bitmap.createBitmap(
                packet.copyOfRange(
                    TN_CONTINUOUS_HEADER_INTS,
                    TN_CONTINUOUS_HEADER_INTS + pixels,
                ),
                width,
                height,
                Bitmap.Config.ARGB_8888,
            )
        } catch (error: Exception) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "D.RAWnegative bitmap kon niet worden opgebouwd: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        val n2CandidateBitmap = try {
            Bitmap.createBitmap(
                packet.copyOfRange(
                    TN_CONTINUOUS_HEADER_INTS + pixels,
                    TN_CONTINUOUS_HEADER_INTS + pixels + pixels,
                ),
                width,
                height,
                Bitmap.Config.ARGB_8888,
            )
        } catch (error: Exception) {
            bitmap.recycle()
            return TruthNegativeContinuousPreviewResult.Failed(
                "N2 appearance-only candidatebitmap kon niet worden opgebouwd: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        val metrics = TruthNegativeContinuousPreviewMetrics(
            width = width,
            height = height,
            sourceWidth = packet[4],
            sourceHeight = packet[5],
            sourceOrientation = packet[6],
            authorityFieldRecords = packet[7],
            calibratedEstimateRecords = packet[8],
            reconstructedRecords = packet[9],
            censoredRecords = packet[10],
            unknownRecords = packet[11],
            targetPixels = packet[12],
            resolvedReconstructedChannels = packet[13],
            resolvedCensoredChannels = packet[14],
            resolvedUnknownChannels = packet[15],
            p95KnownChannels = packet[16],
            boundKnownChannels = packet[17],
            sourceFootprintLinks = packet[18],
            physicalFrameCount = packet[19],
            independentEvidenceCount = packet[20],
            createsNewEvidence = packet[21] != 0,
            scientificWritebackAllowed = packet[22] != 0,
            appearanceApplied = packet[23] != 0,
            displayEncoded = packet[24] != 0,
            sourceSceneMutated = packet[25] != 0,
            masterFieldBitMismatches = packet[26],
            masterFieldBitMatches = packet[27],
            usedForwardMatrix = packet[28] != 0,
            cameraCalibrationApplied = packet[29] != 0,
            displayClampPixels = packet[30],
            sourceTileLoads = packet[31],
            stateSha256 = digestWords(packet, 32),
            authorityFieldSha256 = digestWords(packet, 40),
            drawNegativeStateSha256 = digestWords(packet, 112),
            n2AuditExecuted = packet[48] != 0,
            n2NoiseProfileAvailable = packet[49] != 0,
            n2Sampled = packet[50],
            n2Eligible = packet[51],
            n2CorrectedCandidates = packet[52],
            n2Preserved = packet[53],
            n2CensoredProtected = packet[54],
            n2CensorBoundaryProtected = packet[55],
            n2StructureProtected = packet[56],
            n2UnknownNoiseProtected = packet[57],
            n2NoNeighborhoodProtected = packet[58],
            n2ResidualOutlierProtected = packet[59],
            n2BorderProtected = packet[60],
            n2RemovedResidualEnergyFraction = packet[61].toDouble() / 1_000_000.0,
            n2MaxAbsCorrectionStage2 = packet[62].toDouble() / 1_000_000_000.0,
            n2SamplingPeriod = packet[63],
            n2CandidateSha256 = digestWords(packet, 64),
            n2AuditSha256 = digestWords(packet, 72),
            n2CfaPhaseSamples = listOf(packet[80], packet[81], packet[82], packet[83]),
            n2SourceValuesModified = packet[84] != 0,
            n2TruthNegativeModified = packet[85] != 0,
            n2CreatesNewEvidence = packet[86] != 0,
            n2ScientificWritebackAllowed = packet[87] != 0,
            n2CandidateAppliedToAppearance = packet[88] != 0,
            n2AuditOnly = packet[89] != 0,
            n2MeasuredCfaDomain = packet[90] != 0,
            n2AppearanceCandidateAvailable = packet[92] != 0,
            n2AppearanceOnly = packet[93] != 0,
            n2AppearanceCandidateRendered = packet[94] != 0,
            n2AppearanceCreatesNewEvidence = packet[95] != 0,
            n2AppearanceScientificWritebackAllowed = packet[96] != 0,
            n2AppearanceSourceSceneMutated = packet[97] != 0,
            n2AppearanceGridWidth = packet[98],
            n2AppearanceGridHeight = packet[99],
            n2AppearanceChangedPixels = packet[100],
            n2AppearanceAdjustedChannels = packet[101],
            n2AppearanceDisplayClampPixels = packet[102],
            n2AppearanceGridSha256 = digestWords(packet, 104),
        )

        val contractViolation =
            metrics.physicalFrameCount != 1 ||
                metrics.independentEvidenceCount != 1 ||
                metrics.createsNewEvidence ||
                metrics.scientificWritebackAllowed ||
                !metrics.appearanceApplied ||
                !metrics.displayEncoded ||
                metrics.sourceSceneMutated ||
                metrics.masterFieldBitMismatches != 0 ||
                metrics.targetPixels != pixels ||
                metrics.authorityFieldRecords <= 0 ||
                metrics.stateSha256.all { it == '0' } ||
                metrics.authorityFieldSha256.all { it == '0' } ||
                metrics.drawNegativeStateSha256.all { it == '0' } ||
                !metrics.n2AuditExecuted ||
                metrics.n2Sampled <= 0 ||
                metrics.n2CandidateSha256.all { it == '0' } ||
                metrics.n2AuditSha256.all { it == '0' } ||
                metrics.n2SourceValuesModified ||
                metrics.n2TruthNegativeModified ||
                metrics.n2CreatesNewEvidence ||
                metrics.n2ScientificWritebackAllowed ||
                metrics.n2CandidateAppliedToAppearance ||
                !metrics.n2AuditOnly ||
                !metrics.n2MeasuredCfaDomain ||
                !metrics.n2AppearanceCandidateAvailable ||
                !metrics.n2AppearanceOnly ||
                !metrics.n2AppearanceCandidateRendered ||
                metrics.n2AppearanceCreatesNewEvidence ||
                metrics.n2AppearanceScientificWritebackAllowed ||
                metrics.n2AppearanceSourceSceneMutated ||
                metrics.n2AppearanceGridWidth != width ||
                metrics.n2AppearanceGridHeight != height ||
                metrics.n2AppearanceChangedPixels < 0 ||
                metrics.n2AppearanceChangedPixels > pixels ||
                metrics.n2AppearanceAdjustedChannels < 0 ||
                metrics.n2AppearanceAdjustedChannels > pixels * 3 ||
                metrics.n2AppearanceGridSha256.all { it == '0' }

        if (contractViolation) {
            bitmap.recycle()
            n2CandidateBitmap.recycle()
            return TruthNegativeContinuousPreviewResult.Failed(
                "Fail-closed: D.RAWnegative schond scene-, authority-, evidence- of writebackcontract.",
            )
        }

        return TruthNegativeContinuousPreviewResult.Ready(
            bitmap,
            n2CandidateBitmap,
            metrics,
        )
    }

    fun toUnifiedPreview(
        ready: TruthNegativeContinuousPreviewResult.Ready,
        userQuarterTurns: Int,
    ): UnifiedOutputPreviewResult.Ready {
        val sourceTurns = when (ready.metrics.sourceOrientation) {
            1 -> 0
            6 -> 1
            3 -> 2
            8 -> 3
            else -> 0
        }
        val displayTurns =
            ((sourceTurns + userQuarterTurns) % 4 + 4) % 4
        return UnifiedOutputPreviewResult.Ready(
            bitmap = ready.bitmap,
            metrics = UnifiedOutputPreviewMetrics(
                width = ready.metrics.width,
                height = ready.metrics.height,
                sourceWidth = ready.metrics.sourceWidth,
                sourceHeight = ready.metrics.sourceHeight,
                sourceSpaceCode = 1,
                displayQuarterTurns = displayTurns,
                sampledPrimaryPixels = ready.metrics.targetPixels.toLong(),
                primaryTileSourceUsedDirectly = false,
                appearanceAddedByPreview = true,
                scientificWritebackAllowed = false,
            ),
            outputLabel = "PRO · D.RAWnegative v0.1 · Appearance View",
        )
    }

    private fun digestWords(packet: IntArray, start: Int): String {
        val hex = "0123456789abcdef"
        val out = StringBuilder(64)
        repeat(8) { wordIndex ->
            val word = packet[start + wordIndex]
            repeat(4) { byteIndex ->
                val value = (word ushr (8 * byteIndex)) and 0xff
                out.append(hex[value ushr 4])
                out.append(hex[value and 0x0f])
            }
        }
        return out.toString()
    }

    private fun nativeStatus(status: Int): String = when (status) {
        -1 -> "D.RAWnegative: ongeldige bridge-parameters."
        -2 -> "D.RAWnegative: pre-master authority/evidence-contract geweigerd."
        -3 -> "D.RAWnegative: Technical Backplane/Scientific Master-lineage geweigerd."
        -4 -> "D.RAWnegative: ongeldige brongeometrie."
        -5 -> "D.RAWnegative: Open Scene authority-field kon niet canoniek worden gebonden."
        -6 -> "D.RAWnegative: raster-onafhankelijke state kon niet worden gefinaliseerd."
        -7 -> "D.RAWnegative: Scientific Master/Open Scene bit-identity binding faalde."
        -8 -> "D.RAWnegative: previewdoelgeometrie faalde."
        -9 -> "D.RAWnegative: area-footprint resolver kon niet starten."
        -10 -> "D.RAWnegative: previewraster is te groot."
        -11 -> "D.RAWnegative: target query/authority resolve faalde."
        -12 -> "D.RAWnegative: Appearance/Display resolve faalde."
        -13 -> "D.RAWnegative: Scientific Master/Open Scene query-binding was niet exact."
        -14 -> "D.RAWnegative: camera-plane bijdrage kon niet authority-preserving aan Deep Scene worden gebonden."
        -15 -> "D.RAWnegative: Deep Scene scientific resolve faalde."
        -16 -> "D.RAWnegative: Deep Scene veranderde radiometrische authority/uncertainty."
        -17 -> "D.RAWnegative: N2 CFA audit-only side-car faalde fail-closed."
        -18 -> "D.RAWnegative: N2 appearance-only A/B candidate faalde fail-closed."
        -19 -> "D.RAWnegative: observation/gauge/state-binding faalde fail-closed."
        in 2000..2099 -> "D.RAWnegative source-binding faalde (status $status)."
        in 2100..2199 -> "D.RAWnegative color-binding faalde (status $status)."
        in 7000..7099 -> "D.RAWnegative RAW-adapter faalde (status $status)."
        in 8000..8099 -> "D.RAWnegative Scientific Master-binding faalde (status $status)."
        in 9000..9099 -> "D.RAWnegative Backplane phase-2 faalde (status $status)."
        else -> "D.RAWnegative native status $status."
    }
}

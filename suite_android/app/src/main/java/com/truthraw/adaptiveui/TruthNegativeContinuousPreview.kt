package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val TN_CONTINUOUS_MAGIC = 0x35434e54
private const val TN_CONTINUOUS_HEADER_INTS = 92
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
)

sealed interface TruthNegativeContinuousPreviewResult {
    data class Ready(
        val bitmap: Bitmap,
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
                "TruthNegative Continuous v0.5 vereist de volledig admitted DNG-route.",
            )
        }

        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "Bron kon niet voor TruthNegative Continuous worden geopend: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        } ?: return TruthNegativeContinuousPreviewResult.Failed(
            "Documentprovider gaf geen file descriptor voor TruthNegative Continuous.",
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
                "Native TruthNegative Continuous bridge faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        if (packet.size < TN_CONTINUOUS_HEADER_INTS ||
            packet[0] != TN_CONTINUOUS_MAGIC
        ) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "Ongeldig TruthNegative Continuous v0.5 pakket.",
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
                "TruthNegative Continuous preview-afmetingen zijn buiten contract.",
            )
        }

        val pixels = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "TruthNegative Continuous preview-afmetingen overflowden.",
            )
        }
        if (packet.size != TN_CONTINUOUS_HEADER_INTS + pixels) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "TruthNegative Continuous preview-payload heeft een ongeldige lengte.",
            )
        }

        val bitmap = try {
            Bitmap.createBitmap(
                packet.copyOfRange(TN_CONTINUOUS_HEADER_INTS, packet.size),
                width,
                height,
                Bitmap.Config.ARGB_8888,
            )
        } catch (error: Exception) {
            return TruthNegativeContinuousPreviewResult.Failed(
                "TruthNegative Continuous bitmap kon niet worden opgebouwd: " +
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
                metrics.authorityFieldSha256.all { it == '0' }

        if (contractViolation) {
            bitmap.recycle()
            return TruthNegativeContinuousPreviewResult.Failed(
                "Fail-closed: TruthNegative Continuous schond scene-, authority-, evidence- of writebackcontract.",
            )
        }

        return TruthNegativeContinuousPreviewResult.Ready(bitmap, metrics)
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
            outputLabel = "PRO · TruthNegative Continuous v0.5 · Appearance View",
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
        -1 -> "TruthNegative Continuous: ongeldige bridge-parameters."
        -2 -> "TruthNegative Continuous: pre-master authority/evidence-contract geweigerd."
        -3 -> "TruthNegative Continuous: Technical Backplane/Scientific Master-lineage geweigerd."
        -4 -> "TruthNegative Continuous: ongeldige brongeometrie."
        -5 -> "TruthNegative Continuous: Open Scene authority-field kon niet canoniek worden gebonden."
        -6 -> "TruthNegative Continuous: raster-onafhankelijke state kon niet worden gefinaliseerd."
        -7 -> "TruthNegative Continuous: Scientific Master/Open Scene bit-identity binding faalde."
        -8 -> "TruthNegative Continuous: previewdoelgeometrie faalde."
        -9 -> "TruthNegative Continuous: area-footprint resolver kon niet starten."
        -10 -> "TruthNegative Continuous: previewraster is te groot."
        -11 -> "TruthNegative Continuous: target query/authority resolve faalde."
        -12 -> "TruthNegative Continuous: Appearance/Display resolve faalde."
        -13 -> "TruthNegative Continuous: Scientific Master/Open Scene query-binding was niet exact."
        -14 -> "TruthNegative Continuous: camera-plane bijdrage kon niet authority-preserving aan Deep Scene worden gebonden."
        -15 -> "TruthNegative Continuous: Deep Scene scientific resolve faalde."
        -16 -> "TruthNegative Continuous: Deep Scene veranderde radiometrische authority/uncertainty."
        in 2000..2099 -> "TruthNegative Continuous source-binding faalde (status $status)."
        in 2100..2199 -> "TruthNegative Continuous color-binding faalde (status $status)."
        in 7000..7099 -> "TruthNegative Continuous RAW-adapter faalde (status $status)."
        in 8000..8099 -> "TruthNegative Continuous Scientific Master-binding faalde (status $status)."
        in 9000..9099 -> "TruthNegative Continuous Backplane phase-2 faalde (status $status)."
        else -> "TruthNegative Continuous native status $status."
    }
}

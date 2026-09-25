package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val N2_CROP_AB_MAGIC = 0x31424132
private const val N2_CROP_AB_COUNT = 3
private const val N2_CROP_AB_META_INTS = 32
private const val N2_CROP_AB_HEADER_INTS = 48 + N2_CROP_AB_COUNT * N2_CROP_AB_META_INTS
private const val N2_CROP_AB_MAX_SOURCE_BYTES = 8 * 1024 * 1024
private const val N2_CROP_AB_MAX_LOGICAL_BYTES = 64 * 1024 * 1024

object TruthNegativeN2CropAbNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun buildDiagnosticCrops(
        sourceFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): IntArray
}

enum class TruthNegativeN2CropKind {
    QUIET_CANDIDATE,
    STRUCTURE,
    CENSOR,
}

data class TruthNegativeN2CropMetrics(
    val kind: TruthNegativeN2CropKind,
    val sourceX: Int,
    val sourceY: Int,
    val width: Int,
    val height: Int,
    val sampled: Int,
    val corrected: Int,
    val preserved: Int,
    val structureProtected: Int,
    val censoredProtected: Int,
    val censorBoundaryProtected: Int,
    val residualOutlierProtected: Int,
    val noNeighborhoodProtected: Int,
    val changedPixels: Int,
    val adjustedChannels: Int,
    val meanAbsEncodedDelta: Double,
    val maxAbsEncodedDelta: Double,
    val removedResidualEnergyFraction: Double,
    val maxAbsCorrectionStage2: Double,
    val displayClampA: Int,
    val displayClampB: Int,
    val baselineRgbMismatches: Int,
    val candidateStage2Sites: Int,
    val fullColourCandidate: Boolean,
    val candidateIdentitySha256: String,
)

data class TruthNegativeN2CropPanel(
    val aBitmap: Bitmap,
    val bBitmap: Bitmap,
    val deltaBitmap: Bitmap,
    val metrics: TruthNegativeN2CropMetrics,
)

data class TruthNegativeN2CropAbReport(
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceOrientation: Int,
    val deltaGain: Int,
    val fullColourCandidate: Boolean,
    val truthNegativeStateSha256: String,
    val authorityFieldSha256: String,
    val coarseAuditSha256: String,
    val coarseSpatialSha256: String,
    val crops: List<TruthNegativeN2CropPanel>,
)

sealed interface TruthNegativeN2CropAbResult {
    data class Ready(
        val report: TruthNegativeN2CropAbReport,
    ) : TruthNegativeN2CropAbResult

    data class Failed(
        val reason: String,
    ) : TruthNegativeN2CropAbResult
}

object TruthNegativeN2CropAbLoader {
    fun load(
        resolver: ContentResolver,
        job: RawJob,
    ): TruthNegativeN2CropAbResult {
        if (!job.source.format.nativeProcessingReady ||
            job.source.format.id != "DNG"
        ) {
            return TruthNegativeN2CropAbResult.Failed(
                "N2 1:1 cropdiagnose vereist de admitted DNG-route.",
            )
        }

        val packet = try {
            resolver.openFileDescriptor(job.source.uri, "r")?.use { pfd ->
                TruthNegativeN2CropAbNativeBridge.buildDiagnosticCrops(
                    pfd.fd,
                    N2_CROP_AB_MAX_SOURCE_BYTES,
                    N2_CROP_AB_MAX_LOGICAL_BYTES,
                )
            }
        } catch (error: Throwable) {
            null
        } ?: return TruthNegativeN2CropAbResult.Failed(
            "N2 1:1 cropdiagnose kon de bron niet openen of native uitvoeren.",
        )

        if (packet.size < N2_CROP_AB_HEADER_INTS ||
            packet[0] != N2_CROP_AB_MAGIC
        ) {
            return TruthNegativeN2CropAbResult.Failed(
                "Ongeldig N2 1:1 cropdiagnosepakket.",
            )
        }
        if (packet[1] != 0) {
            return TruthNegativeN2CropAbResult.Failed(
                nativeStatus(packet[1]),
            )
        }

        val cropCount = packet[2]
        val edge = packet[3]
        val sourceWidth = packet[4]
        val sourceHeight = packet[5]
        val orientation = packet[6]
        val physicalFrames = packet[7]
        val independentEvidence = packet[8]
        val createsNewEvidence = packet[9] != 0
        val scientificWriteback = packet[10] != 0
        val sourceSceneMutated = packet[11] != 0
        val deltaGain = packet[12]
        val fullColourCandidate = packet[13] != 0

        if (cropCount != N2_CROP_AB_COUNT ||
            edge <= 0 ||
            sourceWidth <= 0 ||
            sourceHeight <= 0 ||
            physicalFrames != 1 ||
            independentEvidence != 1 ||
            createsNewEvidence ||
            scientificWriteback ||
            sourceSceneMutated ||
            deltaGain <= 0 ||
            !fullColourCandidate
        ) {
            return TruthNegativeN2CropAbResult.Failed(
                "Fail-closed: N2 1:1 cropdiagnose schond evidence/writebackcontract.",
            )
        }

        val cropPixels = try {
            Math.multiplyExact(edge, edge)
        } catch (_: ArithmeticException) {
            return TruthNegativeN2CropAbResult.Failed(
                "N2 1:1 cropgeometrie overflowde.",
            )
        }
        val expected = try {
            Math.addExact(
                N2_CROP_AB_HEADER_INTS,
                Math.multiplyExact(cropPixels, cropCount * 3),
            )
        } catch (_: ArithmeticException) {
            return TruthNegativeN2CropAbResult.Failed(
                "N2 1:1 cropdiagnose-payload overflowde.",
            )
        }
        if (packet.size != expected) {
            return TruthNegativeN2CropAbResult.Failed(
                "N2 1:1 cropdiagnose-payload heeft een ongeldige lengte.",
            )
        }

        val panels = mutableListOf<TruthNegativeN2CropPanel>()
        try {
            repeat(cropCount) { index ->
                val m = 48 + index * N2_CROP_AB_META_INTS
                val kind = when (packet[m]) {
                    1 -> TruthNegativeN2CropKind.QUIET_CANDIDATE
                    2 -> TruthNegativeN2CropKind.STRUCTURE
                    3 -> TruthNegativeN2CropKind.CENSOR
                    else -> throw IllegalStateException("unknown crop kind")
                }
                val x = packet[m + 1]
                val y = packet[m + 2]
                val width = packet[m + 3]
                val height = packet[m + 4]
                val sampled = packet[m + 5]
                val corrected = packet[m + 6]
                val preserved = packet[m + 7]
                if (width != edge ||
                    height != edge ||
                    x < 0 ||
                    y < 0 ||
                    x + width > sourceWidth ||
                    y + height > sourceHeight ||
                    sampled != cropPixels ||
                    corrected < 0 ||
                    preserved < 0 ||
                    corrected + preserved != sampled
                ) {
                    throw IllegalStateException("crop contract mismatch")
                }

                val baselineRgbMismatches = packet[m + 21]
                val candidateIdentitySha256 = digestWords(packet, m + 22)
                val candidateStage2Sites = packet[m + 30]
                val cropFullColourCandidate = packet[m + 31] != 0
                if (baselineRgbMismatches != 0 ||
                    candidateStage2Sites < 0 ||
                    candidateStage2Sites > sourceWidth * sourceHeight ||
                    !cropFullColourCandidate ||
                    candidateIdentitySha256.length != 64 ||
                    candidateIdentitySha256.all { it == '0' }
                ) {
                    throw IllegalStateException(
                        "full-colour candidate provenance mismatch",
                    )
                }

                val base = N2_CROP_AB_HEADER_INTS + index * 3 * cropPixels
                val a = Bitmap.createBitmap(
                    packet.copyOfRange(base, base + cropPixels),
                    edge,
                    edge,
                    Bitmap.Config.ARGB_8888,
                )
                val b = Bitmap.createBitmap(
                    packet.copyOfRange(
                        base + cropPixels,
                        base + 2 * cropPixels,
                    ),
                    edge,
                    edge,
                    Bitmap.Config.ARGB_8888,
                )
                val delta = Bitmap.createBitmap(
                    packet.copyOfRange(
                        base + 2 * cropPixels,
                        base + 3 * cropPixels,
                    ),
                    edge,
                    edge,
                    Bitmap.Config.ARGB_8888,
                )

                panels += TruthNegativeN2CropPanel(
                    aBitmap = a,
                    bBitmap = b,
                    deltaBitmap = delta,
                    metrics = TruthNegativeN2CropMetrics(
                        kind = kind,
                        sourceX = x,
                        sourceY = y,
                        width = width,
                        height = height,
                        sampled = sampled,
                        corrected = corrected,
                        preserved = preserved,
                        structureProtected = packet[m + 8],
                        censoredProtected = packet[m + 9],
                        censorBoundaryProtected = packet[m + 10],
                        residualOutlierProtected = packet[m + 11],
                        noNeighborhoodProtected = packet[m + 12],
                        changedPixels = packet[m + 13],
                        adjustedChannels = packet[m + 14],
                        meanAbsEncodedDelta =
                            packet[m + 15].toDouble() / 1_000_000_000.0,
                        maxAbsEncodedDelta =
                            packet[m + 16].toDouble() / 1_000_000_000.0,
                        removedResidualEnergyFraction =
                            packet[m + 17].toDouble() / 1_000_000.0,
                        maxAbsCorrectionStage2 =
                            packet[m + 18].toDouble() / 1_000_000_000.0,
                        displayClampA = packet[m + 19],
                        displayClampB = packet[m + 20],
                        baselineRgbMismatches = baselineRgbMismatches,
                        candidateIdentitySha256 = candidateIdentitySha256,
                        candidateStage2Sites = candidateStage2Sites,
                        fullColourCandidate = cropFullColourCandidate,
                    ),
                )
            }
        } catch (error: Throwable) {
            panels.forEach {
                it.aBitmap.recycle()
                it.bBitmap.recycle()
                it.deltaBitmap.recycle()
            }
            return TruthNegativeN2CropAbResult.Failed(
                "N2 1:1 cropdiagnose kon de A/B/Δ-rasterset niet veilig decoderen: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        val state = digestWords(packet, 14)
        val authority = digestWords(packet, 22)
        val audit = digestWords(packet, 30)
        val spatial = digestWords(packet, 38)
        if (listOf(state, authority, audit, spatial).any {
                it.length != 64 || it.all { ch -> ch == '0' }
            }
        ) {
            panels.forEach {
                it.aBitmap.recycle()
                it.bBitmap.recycle()
                it.deltaBitmap.recycle()
            }
            return TruthNegativeN2CropAbResult.Failed(
                "Fail-closed: N2 1:1 cropdiagnose identity-digest ontbreekt.",
            )
        }

        return TruthNegativeN2CropAbResult.Ready(
            TruthNegativeN2CropAbReport(
                sourceWidth = sourceWidth,
                sourceHeight = sourceHeight,
                sourceOrientation = orientation,
                deltaGain = deltaGain,
                fullColourCandidate = fullColourCandidate,
                truthNegativeStateSha256 = state,
                authorityFieldSha256 = authority,
                coarseAuditSha256 = audit,
                coarseSpatialSha256 = spatial,
                crops = panels,
            ),
        )
    }

    fun recycle(result: TruthNegativeN2CropAbResult.Ready?) {
        result?.report?.crops?.forEach {
            it.aBitmap.recycle()
            it.bBitmap.recycle()
            it.deltaBitmap.recycle()
        }
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
        -1 -> "N2 1:1 cropdiagnose: ongeldige bridge-parameters."
        -2 -> "N2 1:1 cropdiagnose: brongeometrie te klein."
        -3 -> "N2 1:1 cropdiagnose: Scientific Master/Open Scene binding faalde."
        -4 -> "N2 1:1 cropdiagnose: globale tile-selectie faalde."
        -5 -> "N2 1:1 cropdiagnose: full-resolution TruthNegative resolver faalde."
        -6 -> "N2 1:1 cropdiagnose: outputgeometrie te groot."
        -7 -> "N2 1:1 cropdiagnose: full-lattice cropaudit faalde."
        -8 -> "N2 1:1 cropdiagnose: exact Scientific Master bronpixel kon niet worden gelezen."
        -9 -> "N2 1:1 cropdiagnose: gereserveerde legacy Deep Scene status."
        -10 -> "N2 1:1 cropdiagnose: A appearance resolve faalde."
        -11 -> "N2 1:1 cropdiagnose: kandidaatcorrectie ongeldig."
        -12 -> "N2 1:1 cropdiagnose: B appearance resolve faalde."
        -13 -> "N2 1:1 cropdiagnose: Δ-meting ongeldig."
        -14 -> "N2 1:1 cropdiagnose: post-run lineage/source verificatie faalde."
        -15 -> "N2 1:1 cropdiagnose: kandidaat Stage-2 reconstructietile kon niet worden opgebouwd."
        -16 -> "N2 1:1 cropdiagnose: full-lattice audit van de reconstructiehalo faalde."
        -17 -> "N2 1:1 cropdiagnose: full-colour measured-preserving kandidaat-reconstructie faalde."
        -18 -> "N2 1:1 cropdiagnose: baseline-reconstructie was niet bit-identiek aan de exacte Scientific Master bronpixel."
        in 2000..9999 -> "N2 1:1 cropdiagnose: upstream pipeline status $status."
        else -> "N2 1:1 cropdiagnose: native status $status."
    }
}

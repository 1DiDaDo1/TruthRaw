package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import java.io.BufferedInputStream

object TruthNegativeNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportTruthNegative(
        sourceFd: Int,
        outputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class TruthNegativeExportMetrics(
    val width: Int,
    val height: Int,
    val outputBytes: Long,
    val cellCount: Long,
    val calibratedEstimateSamples: Long,
    val reconstructedSamples: Long,
    val censoredSamples: Long,
    val unknownSamples: Long,
    val scientificMasterReplayVerified: Boolean,
    val exactMasterSampleBitsWritten: Boolean,
    val createsNewEvidence: Boolean,
    val createsSecondScientificWorld: Boolean,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val payloadBytes: Long,
    val authorityBytes: Long,
    val postWriteVerified: Boolean,
)

sealed interface TruthNegativeExportResult {
    data class Success(val metrics: TruthNegativeExportMetrics) : TruthNegativeExportResult
    data class Failed(val reason: String) : TruthNegativeExportResult
}

object TruthNegativeExporter {
    private const val MAGIC = 0x54524e47L
    private const val PACKET_LONGS = 22
    private const val HEADER_BYTES = 4096
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): TruthNegativeExportResult {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return TruthNegativeExportResult.Failed(
                "TRUTHNEGATIVE is momenteel alleen toegelaten voor de volledig admitted DNG-route.",
            )
        }

        val source = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (_: Exception) {
            null
        } ?: return TruthNegativeExportResult.Failed("TruthNegative: bron-FD kon niet worden geopend.")

        val output = try {
            resolver.openFileDescriptor(destination, "rw")
        } catch (_: Exception) {
            null
        } ?: run {
            source.close()
            return TruthNegativeExportResult.Failed("TruthNegative: doel-FD kon niet worden geopend.")
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    TruthNegativeNativeBridge.exportTruthNegative(
                        src.fd,
                        dst.fd,
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeExportResult.Failed(
                "TruthNegative native export faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != PACKET_LONGS || packet[0] != MAGIC) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeExportResult.Failed("TruthNegative gaf een ongeldig native result-pakket.")
        }
        if (packet[1] != 0L) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeExportResult.Failed(
                "TruthNegative fail-closed status ${packet[1]}.",
            )
        }

        val metrics = TruthNegativeExportMetrics(
            width = packet[2].toInt(),
            height = packet[3].toInt(),
            outputBytes = packet[6],
            cellCount = packet[7],
            calibratedEstimateSamples = packet[8],
            reconstructedSamples = packet[9],
            censoredSamples = packet[10],
            unknownSamples = packet[11],
            scientificMasterReplayVerified = packet[14] != 0L,
            exactMasterSampleBitsWritten = packet[15] != 0L,
            createsNewEvidence = packet[16] != 0L,
            createsSecondScientificWorld = packet[17] != 0L,
            physicalFrameCount = packet[18].toInt(),
            independentEvidenceCount = packet[19].toInt(),
            payloadBytes = packet[20],
            authorityBytes = packet[21],
            postWriteVerified = false,
        )

        val invariantFailure =
            metrics.width <= 0 ||
                metrics.height <= 0 ||
                !metrics.scientificMasterReplayVerified ||
                !metrics.exactMasterSampleBitsWritten ||
                metrics.createsNewEvidence ||
                metrics.createsSecondScientificWorld ||
                metrics.physicalFrameCount != 1 ||
                metrics.independentEvidenceCount != 1 ||
                metrics.reconstructedSamples != 0L ||
                metrics.calibratedEstimateSamples + metrics.censoredSamples !=
                    metrics.width.toLong() * metrics.height.toLong() ||
                metrics.unknownSamples !=
                    2L * metrics.width.toLong() * metrics.height.toLong()

        if (invariantFailure) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeExportResult.Failed(
                "TruthNegative authority/master invariant faalde na native export.",
            )
        }

        val verify = verifySavedTruthNegative(resolver, destination, metrics.outputBytes)
        if (!verify.first) {
            runCatching { resolver.delete(destination, null, null) }
            return TruthNegativeExportResult.Failed(
                "TruthNegative post-write verify faalde: ${verify.second}",
            )
        }

        return TruthNegativeExportResult.Success(
            metrics.copy(postWriteVerified = true),
        )
    }

    private fun verifySavedTruthNegative(
        resolver: ContentResolver,
        destination: Uri,
        expectedBytes: Long,
    ): Pair<Boolean, String> {
        val header = try {
            resolver.openInputStream(destination)?.use { raw ->
                val input = BufferedInputStream(raw)
                val bytes = ByteArray(HEADER_BYTES)
                var offset = 0
                while (offset < bytes.size) {
                    val n = input.read(bytes, offset, bytes.size - offset)
                    if (n <= 0) break
                    offset += n
                }
                if (offset != HEADER_BYTES) return false to "header is korter dan 4096 bytes"
                bytes.toString(Charsets.US_ASCII)
            }
        } catch (_: Exception) {
            null
        } ?: return false to "doel kon niet worden teruggelezen"

        val required = listOf(
            "magic=TRUTHNEGATIVE_V0_2_TN2",
            "container_version=2",
            "role=TRUTHNEGATIVE_SOURCE_RESOLUTION_SCIENTIFIC_NEGATIVE",
            "pixel_role=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB",
            "sample_encoding=IEEE754_BINARY32_LE",
            "layout=CANONICAL_64X64_CELL_SEQUENCE",
            "dynamic_authority_schema=TRUTHRAW_DYNAMIC_AUTHORITY_GENERIC_FAIL_CLOSED_V0_66",
            "generic_missing_channel_policy=UNKNOWN_UNTIL_SOURCE_BOUND_UNCERTAINTY_IS_ADMITTED",
            "creates_new_evidence=0",
            "creates_second_scientific_world=0",
            "scientific_master_modified=0",
            "appearance_applied=0",
            "counterfactual_observation_created=0",
            "physical_frame_count=1",
            "independent_evidence_count=1",
            "END_HEADER",
        )
        val missing = required.firstOrNull { !header.contains(it) }
        if (missing != null) return false to "marker ontbreekt: $missing"

        val master = header.lineSequence()
            .firstOrNull { it.startsWith("scientific_master_sha256=") }
            ?.substringAfter('=')
            .orEmpty()
        if (master.length != 64 || master.any { it !in "0123456789abcdef" }) {
            return false to "ongeldige Scientific Master SHA-256"
        }

        val source = header.lineSequence()
            .firstOrNull { it.startsWith("source_sha256=") }
            ?.substringAfter('=')
            .orEmpty()
        if (source.length != 64 || source.any { it !in "0123456789abcdef" }) {
            return false to "ongeldige source SHA-256"
        }

        val backplane = header.lineSequence()
            .firstOrNull { it.startsWith("technical_backplane_serialized_hex=") }
            ?.substringAfter('=')
            .orEmpty()
        if (backplane.length != 360 || backplane.any { it !in "0123456789abcdef" }) {
            return false to "Technical Backplane is niet exact 180 bytes"
        }

        val size = try {
            resolver.openFileDescriptor(destination, "r")?.use { it.statSize }
        } catch (_: Exception) {
            -1L
        } ?: -1L
        if (size >= 0L && size != expectedBytes) {
            return false to "bestandsgrootte $size != verwacht $expectedBytes"
        }

        return true to "TN-2 header, lineage en bestandsgrootte geverifieerd"
    }
}

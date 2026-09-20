package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import java.io.BufferedInputStream

object FullResRestorationNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportFullResRestoration(
        sourceFd: Int,
        outputFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class FullResRestorationMetrics(
    val width: Int,
    val height: Int,
    val outputBytes: Long,
    val tileCount: Long,
    val preservedPixels: Long,
    val censoredPixels: Long,
    val restoredPixels: Long,
    val unresolvedPixels: Long,
    val changedComponents: Long,
    val payloadBytes: Long,
    val roleBytes: Long,
    val fullResolution: Boolean,
    val retreatable: Boolean,
    val provenanceBound: Boolean,
    val masterReplayVerified: Boolean,
    val masterModified: Boolean,
    val scientificWritebackAllowed: Boolean,
    val createsNewEvidence: Boolean,
    val createsSecondScientificWorld: Boolean,
    val physicalFrameCount: Int,
    val independentEvidenceCount: Int,
    val postWriteVerified: Boolean,
)

sealed interface FullResRestorationExportResult {
    data class Success(val metrics: FullResRestorationMetrics) : FullResRestorationExportResult
    data class Failed(val reason: String) : FullResRestorationExportResult
}

object FullResRestorationExporter {
    private const val MAGIC = 0x54525253L
    private const val PACKET_LONGS = 24
    private const val HEADER_BYTES = 8192
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun export(
        resolver: ContentResolver,
        job: RawJob,
        destination: Uri,
    ): FullResRestorationExportResult {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration is momenteel alleen toegelaten voor de volledig admitted DNG-route.",
            )
        }

        val source = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (_: Exception) {
            null
        } ?: return FullResRestorationExportResult.Failed(
            "Full-resolution Restoration: bron-FD kon niet worden geopend.",
        )

        val output = try {
            resolver.openFileDescriptor(destination, "rw")
        } catch (_: Exception) {
            null
        } ?: run {
            source.close()
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration: doel-FD kon niet worden geopend.",
            )
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    FullResRestorationNativeBridge.exportFullResRestoration(
                        src.fd,
                        dst.fd,
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            runCatching { resolver.delete(destination, null, null) }
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration native export faalde: " +
                    "${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != PACKET_LONGS || packet[0] != MAGIC) {
            runCatching { resolver.delete(destination, null, null) }
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration gaf een ongeldig native result-pakket.",
            )
        }
        if (packet[1] != 0L) {
            runCatching { resolver.delete(destination, null, null) }
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration fail-closed status ${packet[1]}.",
            )
        }

        val metrics = FullResRestorationMetrics(
            width = packet[2].toInt(),
            height = packet[3].toInt(),
            outputBytes = packet[4],
            tileCount = packet[5],
            preservedPixels = packet[6],
            censoredPixels = packet[7],
            restoredPixels = packet[8],
            unresolvedPixels = packet[9],
            changedComponents = packet[10],
            payloadBytes = packet[11],
            roleBytes = packet[12],
            fullResolution = packet[13] != 0L,
            retreatable = packet[14] != 0L,
            provenanceBound = packet[15] != 0L,
            masterReplayVerified = packet[16] != 0L,
            masterModified = packet[17] != 0L,
            scientificWritebackAllowed = packet[18] != 0L,
            createsNewEvidence = packet[19] != 0L,
            createsSecondScientificWorld = packet[20] != 0L,
            physicalFrameCount = packet[21].toInt(),
            independentEvidenceCount = packet[22].toInt(),
            postWriteVerified = false,
        )

        val totalPixels = metrics.width.toLong() * metrics.height.toLong()
        val invariantFailure =
            metrics.width <= 0 ||
                metrics.height <= 0 ||
                metrics.outputBytes <= HEADER_BYTES ||
                !metrics.fullResolution ||
                !metrics.retreatable ||
                !metrics.provenanceBound ||
                !metrics.masterReplayVerified ||
                metrics.masterModified ||
                metrics.scientificWritebackAllowed ||
                metrics.createsNewEvidence ||
                metrics.createsSecondScientificWorld ||
                metrics.physicalFrameCount != 1 ||
                metrics.independentEvidenceCount != 1 ||
                metrics.preservedPixels + metrics.censoredPixels != totalPixels ||
                metrics.restoredPixels + metrics.unresolvedPixels != metrics.censoredPixels ||
                metrics.roleBytes != totalPixels ||
                packet[23] != 1L

        if (invariantFailure) {
            runCatching { resolver.delete(destination, null, null) }
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration authority/master invariant faalde.",
            )
        }

        val verify = verifySavedRestoration(resolver, destination, metrics.outputBytes)
        if (!verify.first) {
            runCatching { resolver.delete(destination, null, null) }
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration post-write verify faalde: ${verify.second}",
            )
        }

        return FullResRestorationExportResult.Success(
            metrics.copy(postWriteVerified = true),
        )
    }

    private fun verifySavedRestoration(
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
                if (offset != HEADER_BYTES) return false to "header is korter dan 8192 bytes"
                bytes.toString(Charsets.US_ASCII)
            }
        } catch (_: Exception) {
            null
        } ?: return false to "doel kon niet worden teruggelezen"

        val required = listOf(
            "magic=TRUTHRAW_FULLRES_RESTORATION_V0_67",
            "role=FULL_RESOLUTION_RETREATABLE_RESTORATION_DERIVATIVE",
            "pixel_domain=CAMERA_NATIVE_SCIENTIFIC_MASTER_RGB",
            "sample_encoding=IEEE754_BINARY32_LE",
            "condition_trigger=SOURCE_CFA_SAMPLE_AT_OR_ABOVE_WHITELEVEL",
            "support_excludes_censored_source_sites=1",
            "role_0=PRESERVE_SCIENTIFIC_MASTER",
            "role_1=AESTHETIC_REINTEGRATION_ONLY",
            "role_2=UNRESOLVED_LOSS",
            "full_resolution=1",
            "retreatable=1",
            "provenance_bound=1",
            "scientific_master_replay_verified=1",
            "scientific_master_modified=0",
            "scientific_writeback_allowed=0",
            "creates_new_evidence=0",
            "creates_second_scientific_world=0",
            "physical_frame_count=1",
            "independent_evidence_count=1",
            "END_HEADER",
        )
        val missing = required.firstOrNull { !header.contains(it) }
        if (missing != null) return false to "marker ontbreekt: $missing"

        fun shaField(name: String): String = header.lineSequence()
            .firstOrNull { it.startsWith("$name=") }
            ?.substringAfter('=')
            .orEmpty()

        for (field in listOf(
            "source_sha256",
            "scientific_master_sha256",
            "restoration_derivative_rgb_sha256",
            "zero_line_sha256",
            "scene_scale_sha256",
        )) {
            val value = shaField(field)
            if (value.length != 64 || value.any { it !in "0123456789abcdef" }) {
                return false to "ongeldige $field"
            }
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

        return true to "full-resolution restoration header, lineage en grootte geverifieerd"
    }
}

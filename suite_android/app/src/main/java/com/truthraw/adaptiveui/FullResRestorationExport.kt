package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.system.Os
import java.io.BufferedInputStream
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

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

data class FullResRestorationSource(
    val uri: Uri,
    val formatId: String,
    val nativeProcessingReady: Boolean,
) {
    companion object {
        fun from(job: RawJob): FullResRestorationSource = FullResRestorationSource(
            uri = job.source.uri,
            formatId = job.source.format.id,
            nativeProcessingReady = job.source.format.nativeProcessingReady,
        )
    }
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
    val stagingVerified: Boolean,
    val postWriteVerified: Boolean,
    val containerSha256: String?,
)

sealed interface FullResRestorationExportResult {
    data class Success(val metrics: FullResRestorationMetrics) : FullResRestorationExportResult
    data class Failed(val reason: String) : FullResRestorationExportResult
}

/**
 * v0.68 transaction wrapper around the unchanged v0.67 scientific restoration container.
 *
 * Important commit discipline:
 * 1. Native reconstruction writes only to an app-private staging file.
 * 2. The staging file is fully verified and hashed.
 * 3. The SAF destination receives a zero header first, then the complete body.
 * 4. The valid 8192-byte header is committed LAST and fsync'd.
 * 5. The exact destination is reopened, contract-verified, size-verified and
 *    whole-file SHA-256 compared with staging.
 *
 * Therefore an interrupted copy can never look like a valid v0.67 artifact:
 * its header remains zero/uncommitted until the body is complete.
 */
object FullResRestorationExporter {
    private const val MAGIC = 0x54525253L
    private const val PACKET_LONGS = 24
    const val HEADER_BYTES = 8192
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
    private const val COPY_BUFFER_BYTES = 1024 * 1024

    fun exportToStaging(
        resolver: ContentResolver,
        source: FullResRestorationSource,
        stagingFile: File,
    ): FullResRestorationExportResult {
        if (!source.nativeProcessingReady || source.formatId != "DNG") {
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration is momenteel alleen toegelaten voor de volledig admitted DNG-route.",
            )
        }

        stagingFile.parentFile?.mkdirs()
        if (stagingFile.exists() && !stagingFile.delete()) {
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration staging kon niet schoon worden gestart.",
            )
        }

        val sourcePfd = openSourcePfd(resolver, source.uri)
            ?: return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration: bron-FD kon niet worden geopend.",
            )

        val outputPfd = try {
            ParcelFileDescriptor.open(
                stagingFile,
                ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
        } catch (_: Exception) {
            null
        } ?: run {
            sourcePfd.close()
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration: private staging-FD kon niet worden geopend.",
            )
        }

        val packet = try {
            sourcePfd.use { src ->
                outputPfd.use { dst ->
                    FullResRestorationNativeBridge.exportFullResRestoration(
                        src.fd,
                        dst.fd,
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            stagingFile.delete()
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration native staging faalde: " +
                    "${error.message ?: error.javaClass.simpleName}",
            )
        }

        val parsed = parseAndValidatePacket(packet)
        if (parsed is FullResRestorationExportResult.Failed) {
            stagingFile.delete()
            return parsed
        }
        val metrics = (parsed as FullResRestorationExportResult.Success).metrics

        val verify = verifyFile(stagingFile, metrics.outputBytes)
        if (!verify.first) {
            stagingFile.delete()
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration staging-verify faalde: ${verify.second}",
            )
        }

        val sha256 = sha256(stagingFile)
            ?: run {
                stagingFile.delete()
                return FullResRestorationExportResult.Failed(
                    "Full-resolution Restoration staging SHA-256 kon niet worden berekend.",
                )
            }

        return FullResRestorationExportResult.Success(
            metrics.copy(
                stagingVerified = true,
                postWriteVerified = false,
                containerSha256 = sha256,
            ),
        )
    }

    fun commitStagingToDestination(
        resolver: ContentResolver,
        stagingFile: File,
        destination: Uri,
        stagedMetrics: FullResRestorationMetrics,
    ): FullResRestorationExportResult {
        if (!stagedMetrics.stagingVerified ||
            stagedMetrics.containerSha256.isNullOrBlank() ||
            stagingFile.length() != stagedMetrics.outputBytes
        ) {
            return FullResRestorationExportResult.Failed(
                "Transactional commit geweigerd: staging is niet volledig geverifieerd.",
            )
        }

        val header = try {
            FileInputStream(stagingFile).use { input ->
                val bytes = ByteArray(HEADER_BYTES)
                var offset = 0
                while (offset < bytes.size) {
                    val n = input.read(bytes, offset, bytes.size - offset)
                    if (n <= 0) break
                    offset += n
                }
                if (offset != HEADER_BYTES) null else bytes
            }
        } catch (_: Exception) {
            null
        } ?: return FullResRestorationExportResult.Failed(
            "Transactional commit geweigerd: staging-header is niet volledig.",
        )

        val destinationPfd = try {
            resolver.openFileDescriptor(destination, "rw")
        } catch (_: Exception) {
            null
        } ?: return FullResRestorationExportResult.Failed(
            "Transactional commit: doel-FD kon niet worden geopend.",
        )

        val copied = try {
            destinationPfd.use { pfd ->
                val output = FileOutputStream(pfd.fileDescriptor)
                val channel = output.channel
                channel.truncate(0L)
                channel.position(0L)

                // Invalid/uncommitted artifact marker: header stays zero until body is complete.
                val zeroHeader = ByteArray(HEADER_BYTES)
                output.write(zeroHeader)
                output.flush()
                Os.fsync(pfd.fileDescriptor)

                FileInputStream(stagingFile).use { input ->
                    var skipped = 0L
                    while (skipped < HEADER_BYTES.toLong()) {
                        val n = input.skip(HEADER_BYTES.toLong() - skipped)
                        if (n <= 0L) break
                        skipped += n
                    }
                    if (skipped != HEADER_BYTES.toLong()) {
                        throw IllegalStateException("staging body offset kon niet worden bereikt")
                    }

                    val buffer = ByteArray(COPY_BUFFER_BYTES)
                    while (true) {
                        val n = input.read(buffer)
                        if (n <= 0) break
                        output.write(buffer, 0, n)
                    }
                }

                output.flush()
                channel.truncate(stagedMetrics.outputBytes)
                Os.fsync(pfd.fileDescriptor)

                if (channel.size() != stagedMetrics.outputBytes) {
                    throw IllegalStateException(
                        "doelgrootte ${channel.size()} != verwacht ${stagedMetrics.outputBytes}",
                    )
                }

                // Atomic-validity boundary for TruthRaw semantics: commit header LAST.
                channel.position(0L)
                output.write(header)
                output.flush()
                Os.fsync(pfd.fileDescriptor)
                true
            }
        } catch (_: Throwable) {
            false
        }

        if (!copied) {
            cleanupDestination(resolver, destination)
            return FullResRestorationExportResult.Failed(
                "Transactional commit faalde; onvolledige doeluitvoer is verwijderd of ongeldig gemaakt.",
            )
        }

        val verify = verifySavedRestoration(
            resolver,
            destination,
            stagedMetrics.outputBytes,
            stagedMetrics.containerSha256,
        )
        if (!verify.first) {
            cleanupDestination(resolver, destination)
            return FullResRestorationExportResult.Failed(
                "Transactional post-write verify faalde: ${verify.second}",
            )
        }

        return FullResRestorationExportResult.Success(
            stagedMetrics.copy(postWriteVerified = true),
        )
    }

    fun cleanupDestination(resolver: ContentResolver, destination: Uri) {
        val deleted = runCatching {
            resolver.delete(destination, null, null)
        }.getOrDefault(0) > 0
        if (deleted) return

        // Some document providers do not allow delete through ContentResolver.
        // In that case leave a zero-byte, definitely-invalid placeholder.
        runCatching {
            resolver.openFileDescriptor(destination, "rw")?.use { pfd ->
                FileOutputStream(pfd.fileDescriptor).channel.use { channel ->
                    channel.truncate(0L)
                    channel.force(true)
                }
            }
        }
    }

    private fun parseAndValidatePacket(packet: LongArray): FullResRestorationExportResult {
        if (packet.size != PACKET_LONGS || packet[0] != MAGIC) {
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration gaf een ongeldig native result-pakket.",
            )
        }
        if (packet[1] != 0L) {
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
            stagingVerified = false,
            postWriteVerified = false,
            containerSha256 = null,
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
            return FullResRestorationExportResult.Failed(
                "Full-resolution Restoration authority/master invariant faalde.",
            )
        }

        return FullResRestorationExportResult.Success(metrics)
    }

    private fun openSourcePfd(
        resolver: ContentResolver,
        uri: Uri,
    ): ParcelFileDescriptor? = try {
        if (uri.scheme == ContentResolver.SCHEME_FILE) {
            val path = uri.path ?: return null
            ParcelFileDescriptor.open(File(path), ParcelFileDescriptor.MODE_READ_ONLY)
        } else {
            resolver.openFileDescriptor(uri, "r")
        }
    } catch (_: Exception) {
        null
    }

    private fun verifyFile(file: File, expectedBytes: Long): Pair<Boolean, String> {
        if (!file.isFile) return false to "stagingbestand ontbreekt"
        if (file.length() != expectedBytes) {
            return false to "staginggrootte ${file.length()} != verwacht $expectedBytes"
        }
        val header = try {
            FileInputStream(file).use { input ->
                val bytes = ByteArray(HEADER_BYTES)
                var offset = 0
                while (offset < bytes.size) {
                    val n = input.read(bytes, offset, bytes.size - offset)
                    if (n <= 0) break
                    offset += n
                }
                if (offset != HEADER_BYTES) null else bytes.toString(Charsets.US_ASCII)
            }
        } catch (_: Exception) {
            null
        } ?: return false to "staging-header kon niet volledig worden gelezen"

        return verifyHeader(header)
    }

    private fun verifySavedRestoration(
        resolver: ContentResolver,
        destination: Uri,
        expectedBytes: Long,
        expectedSha256: String,
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

        val headerVerify = verifyHeader(header)
        if (!headerVerify.first) return headerVerify

        val size = try {
            resolver.openFileDescriptor(destination, "r")?.use { it.statSize }
        } catch (_: Exception) {
            -1L
        } ?: -1L
        if (size >= 0L && size != expectedBytes) {
            return false to "bestandsgrootte $size != verwacht $expectedBytes"
        }

        val actualSha256 = sha256(resolver, destination)
            ?: return false to "doel SHA-256 kon niet worden berekend"
        if (actualSha256 != expectedSha256) {
            return false to "doel SHA-256 wijkt af van geverifieerde staging"
        }

        return true to "header, lineage, grootte en whole-file SHA-256 geverifieerd"
    }

    private fun verifyHeader(header: String): Pair<Boolean, String> {
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

        return true to "restoration contract geldig"
    }

    private fun sha256(file: File): String? = try {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(COPY_BUFFER_BYTES)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                digest.update(buffer, 0, n)
            }
        }
        digest.digest().joinToString("") { "%02x".format(it) }
    } catch (_: Exception) {
        null
    }

    private fun sha256(resolver: ContentResolver, uri: Uri): String? = try {
        val digest = MessageDigest.getInstance("SHA-256")
        resolver.openInputStream(uri)?.use { input ->
            val buffer = ByteArray(COPY_BUFFER_BYTES)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                digest.update(buffer, 0, n)
            }
        } ?: return null
        digest.digest().joinToString("") { "%02x".format(it) }
    } catch (_: Exception) {
        null
    }
}

package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

object RestorationProjectionNativeBridge {
    init { System.loadLibrary("truthraw_ui_preview_bridge") }

    external fun projectRestoration(
        sourceFd: Int,
        trrFd: Int,
        outputFd: Int,
        format: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray

    external fun buildUnifiedOutputPreview(
        sourceFd: Int,
        trrFd: Int,
        previewFd: Int,
        maxEdge: Int,
        applyStoredOrientation: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

enum class RestorationProjectionFormat(
    val nativeId: Int,
    val extension: String,
    val mimeType: String,
    val label: String,
) {
    DNG(1, "dng", "image/x-adobe-dng", "DNG"),
    TIFF(2, "tiff", "image/tiff", "TIFF"),
    EXR(3, "exr", "application/octet-stream", "EXR"),
}

data class RestorationProjectionMetrics(
    val format: RestorationProjectionFormat,
    val width: Int,
    val height: Int,
    val outputBytes: Long,
    val pixels: Long,
    val negativeComponents: Long,
    val overOneComponents: Long,
    val role0Pixels: Long,
    val role1Pixels: Long,
    val role2Pixels: Long,
    val derivativeIdentityVerified: Boolean,
    val lineageVerified: Boolean,
    val fullResolution: Boolean,
    val outputSha256: String?,
    val postWriteVerified: Boolean,
)

sealed interface RestorationProjectionResult {
    data class Success(val metrics: RestorationProjectionMetrics) : RestorationProjectionResult
    data class Failed(val reason: String) : RestorationProjectionResult
}

object RestorationUnifiedOutputPreviewBuilder {
    private const val MAGIC = 0x5452504aL
    private const val PACKET_LONGS = 20
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun build(
        resolver: ContentResolver,
        sourceUri: Uri,
        trrUri: Uri,
        staging: File,
        outputLabel: String,
        applyStoredOrientation: Boolean = true,
        maxEdge: Int = 384,
    ): UnifiedOutputPreviewResult {
        if (maxEdge !in 1..1024) {
            return UnifiedOutputPreviewResult.Failed(
                "Restoration Unified Output Preview maxEdge is buiten contract.",
            )
        }
        staging.parentFile?.mkdirs()
        staging.delete()

        val source = openRead(resolver, sourceUri)
            ?: return UnifiedOutputPreviewResult.Failed(
                "Restoration preview: bron-DNG kon niet worden geopend.",
            )
        val trr = openRead(resolver, trrUri)
            ?: run {
                source.close()
                return UnifiedOutputPreviewResult.Failed(
                    "Restoration preview: .trr kon niet worden geopend.",
                )
            }
        val preview = runCatching {
            ParcelFileDescriptor.open(
                staging,
                ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
        }.getOrNull()
            ?: run {
                source.close()
                trr.close()
                return UnifiedOutputPreviewResult.Failed(
                    "Restoration preview: UOP1 staging kon niet worden geopend.",
                )
            }

        val packet = try {
            source.use { sourceFd ->
                trr.use { trrFd ->
                    preview.use { previewFd ->
                        RestorationProjectionNativeBridge.buildUnifiedOutputPreview(
                            sourceFd.fd,
                            trrFd.fd,
                            previewFd.fd,
                            maxEdge,
                            if (applyStoredOrientation) 1 else 0,
                            MAX_SOURCE_RESIDENT_BYTES,
                            MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            }
        } catch (error: Throwable) {
            staging.delete()
            return UnifiedOutputPreviewResult.Failed(
                "Restoration Unified Output Preview faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }

        if (
            packet.size != PACKET_LONGS ||
            packet[0] != MAGIC ||
            packet[1] != 0L ||
            packet[2] <= 0L ||
            packet[3] <= 0L ||
            packet[4] <= 0L ||
            packet[5] <= 0L ||
            packet[6] != 1L ||
            packet[7] != packet[2] * packet[3] ||
            packet[10] != 1L ||
            packet[11] != 0L ||
            packet[12] != 0L ||
            packet[13] != 1L ||
            packet[14] != 1L ||
            packet[15] != 1L
        ) {
            val status = packet.getOrNull(1)?.toString() ?: "pakketfout"
            staging.delete()
            return UnifiedOutputPreviewResult.Failed(
                "Restoration Unified Output Preview native contract faalde (status=" +
                    status + ").",
            )
        }

        val loaded = UnifiedOutputPreviewLoader.load(staging, outputLabel)
        staging.delete()
        if (loaded is UnifiedOutputPreviewResult.Ready) {
            val m = loaded.metrics
            if (
                m.width != packet[2].toInt() ||
                m.height != packet[3].toInt() ||
                m.sourceWidth != packet[4].toInt() ||
                m.sourceHeight != packet[5].toInt() ||
                m.sourceSpaceCode != packet[6].toInt() ||
                m.sampledPrimaryPixels != packet[7]
            ) {
                loaded.bitmap.recycle()
                return UnifiedOutputPreviewResult.Failed(
                    "Restoration UOP1 sidecar/native packet binding mismatch.",
                )
            }
        }
        return loaded
    }

    private fun openRead(
        resolver: ContentResolver,
        uri: Uri,
    ): ParcelFileDescriptor? = try {
        if (uri.scheme == ContentResolver.SCHEME_FILE) {
            val path = uri.path ?: return null
            ParcelFileDescriptor.open(File(path), ParcelFileDescriptor.MODE_READ_ONLY)
        } else {
            resolver.openFileDescriptor(uri, "r")
        }
    } catch (_: Throwable) {
        null
    }
}

object RestorationProjectionExporter {
    private const val MAGIC = 0x5452504aL
    private const val PACKET_LONGS = 20
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
    private const val BUFFER = 1024 * 1024

    fun exportToStaging(
        resolver: ContentResolver,
        sourceUri: Uri,
        trrUri: Uri,
        format: RestorationProjectionFormat,
        staging: File,
    ): RestorationProjectionResult {
        staging.parentFile?.mkdirs()
        if (staging.exists() && !staging.delete()) {
            return RestorationProjectionResult.Failed("Projection staging kon niet schoon starten.")
        }

        val source = openRead(resolver, sourceUri)
            ?: return RestorationProjectionResult.Failed("Bron-DNG kon niet worden geopend.")
        val trr = openRead(resolver, trrUri)
            ?: run {
                source.close()
                return RestorationProjectionResult.Failed("Volledige .trr kon niet worden geopend.")
            }
        val output = try {
            ParcelFileDescriptor.open(
                staging,
                ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
        } catch (_: Exception) {
            null
        } ?: run {
            source.close(); trr.close()
            return RestorationProjectionResult.Failed("Projection staging-FD kon niet worden geopend.")
        }

        val packet = try {
            source.use { s ->
                trr.use { t ->
                    output.use { o ->
                        RestorationProjectionNativeBridge.projectRestoration(
                            s.fd, t.fd, o.fd, format.nativeId,
                            MAX_SOURCE_RESIDENT_BYTES, MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            }
        } catch (error: Throwable) {
            staging.delete()
            return RestorationProjectionResult.Failed(
                "Native ${format.label}-projectie faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != PACKET_LONGS || packet[0] != MAGIC || packet[1] != 0L) {
            staging.delete()
            val status = packet.getOrNull(1)
            return RestorationProjectionResult.Failed(
                "Native ${format.label}-projectie fail-closed status ${status ?: "pakketfout"}.",
            )
        }

        val metrics = RestorationProjectionMetrics(
            format = format,
            width = packet[3].toInt(),
            height = packet[4].toInt(),
            outputBytes = packet[5],
            pixels = packet[6],
            negativeComponents = packet[7],
            overOneComponents = packet[8],
            role0Pixels = packet[9],
            role1Pixels = packet[10],
            role2Pixels = packet[11],
            derivativeIdentityVerified = packet[12] != 0L,
            lineageVerified = packet[13] != 0L,
            fullResolution = packet[14] != 0L,
            outputSha256 = null,
            postWriteVerified = false,
        )
        val total = metrics.width.toLong() * metrics.height.toLong()
        if (metrics.width <= 0 || metrics.height <= 0 ||
            metrics.outputBytes <= 0L || staging.length() != metrics.outputBytes ||
            metrics.pixels != total ||
            metrics.role0Pixels + metrics.role1Pixels + metrics.role2Pixels != total ||
            !metrics.derivativeIdentityVerified || !metrics.lineageVerified ||
            !metrics.fullResolution ||
            packet[15] != 1L ||
            packet[16] != 1L ||
            packet[17] != 1L ||
            packet[18] != 1L ||
            packet[19] != 1L
        ) {
            staging.delete()
            return RestorationProjectionResult.Failed(
                "${format.label}-projectie invariant faalde na native export.",
            )
        }

        val verify = verifyStaging(staging, format)
        if (!verify.first) {
            staging.delete()
            return RestorationProjectionResult.Failed(
                "${format.label}-staging verify faalde: ${verify.second}",
            )
        }
        val sha = sha256(staging)
            ?: run {
                staging.delete()
                return RestorationProjectionResult.Failed("Projection SHA-256 kon niet worden berekend.")
            }
        return RestorationProjectionResult.Success(metrics.copy(outputSha256 = sha))
    }

    fun commit(
        resolver: ContentResolver,
        staging: File,
        destination: Uri,
        metrics: RestorationProjectionMetrics,
    ): RestorationProjectionResult {
        val expectedSha = metrics.outputSha256
            ?: return RestorationProjectionResult.Failed("Projection staging hash ontbreekt.")
        val pfd = try { resolver.openFileDescriptor(destination, "rw") } catch (_: Exception) { null }
            ?: return RestorationProjectionResult.Failed("Projection doel-FD kon niet worden geopend.")

        val ok = try {
            pfd.use { outPfd ->
                FileOutputStream(outPfd.fileDescriptor).use { out ->
                    out.channel.truncate(0L)
                    FileInputStream(staging).use { input ->
                        val buffer = ByteArray(BUFFER)
                        while (true) {
                            val n = input.read(buffer)
                            if (n <= 0) break
                            out.write(buffer, 0, n)
                        }
                    }
                    out.flush()
                    out.channel.force(true)
                }
            }
            true
        } catch (_: Throwable) {
            false
        }
        if (!ok) {
            cleanup(resolver, destination)
            return RestorationProjectionResult.Failed("Projection commit faalde; doel is verwijderd/geleegd.")
        }

        val size = try { resolver.openFileDescriptor(destination, "r")?.use { it.statSize } } catch (_: Exception) { -1L } ?: -1L
        val targetSha = sha256(resolver, destination)
        if ((size >= 0L && size != metrics.outputBytes) || targetSha != expectedSha) {
            cleanup(resolver, destination)
            return RestorationProjectionResult.Failed(
                "Projection post-write verify faalde: staging en doel zijn niet byte-identiek.",
            )
        }

        return RestorationProjectionResult.Success(metrics.copy(postWriteVerified = true))
    }

    fun cleanup(resolver: ContentResolver, uri: Uri) {
        val deleted = runCatching { resolver.delete(uri, null, null) }.getOrDefault(0) > 0
        if (deleted) return
        runCatching {
            resolver.openFileDescriptor(uri, "rw")?.use { pfd ->
                FileOutputStream(pfd.fileDescriptor).channel.use { it.truncate(0L); it.force(true) }
            }
        }
    }

    private fun openRead(resolver: ContentResolver, uri: Uri): ParcelFileDescriptor? = try {
        resolver.openFileDescriptor(uri, "r")
    } catch (_: Exception) {
        null
    }

    private fun verifyStaging(file: File, format: RestorationProjectionFormat): Pair<Boolean, String> {
        if (!file.isFile || file.length() < 32L) return false to "bestand ontbreekt of is te klein"
        val prefix = FileInputStream(file).use { input ->
            val bytes = ByteArray(minOf(131072L, file.length()).toInt())
            val n = input.read(bytes)
            if (n <= 0) ByteArray(0) else bytes.copyOf(n)
        }
        return when (format) {
            RestorationProjectionFormat.DNG -> {
                val tiff = prefix.size >= 4 &&
                    prefix[0] == 'I'.code.toByte() && prefix[1] == 'I'.code.toByte() &&
                    prefix[2] == 42.toByte() && prefix[3] == 0.toByte()
                val text = prefix.toString(Charsets.ISO_8859_1)
                if (!tiff ||
                    !text.contains("TRUTHRAW_RESTORATION_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION_V0_69") ||
                    !text.contains("restoration_role_mask_sha256=") ||
                    !text.contains("restoration_role_mask_embedded=1") ||
                    !text.contains("TRUTHRAW_ROLE_MASK_BINARY_V1") ||
                    !text.contains("open_scene_state_sha256=")
                ) {
                    false to "DNG/TIFF header, role-mask of canonical Open Scene binding ontbreekt"
                } else true to "DNG header + derivative/role/Open-Scene binding geldig"
            }
            RestorationProjectionFormat.TIFF -> {
                val tiff = prefix.size >= 4 &&
                    prefix[0] == 'I'.code.toByte() && prefix[1] == 'I'.code.toByte() &&
                    prefix[2] == 42.toByte() && prefix[3] == 0.toByte()
                val text = prefix.toString(Charsets.ISO_8859_1)
                val roleTag = hasClassicTiffTag(prefix, 65000)
                if (!tiff ||
                    !text.contains("TruthRaw Restoration Projection v0.69") ||
                    !text.contains("restoration_role_mask_sha256=") ||
                    !text.contains("open_scene_artifact_sha256=") ||
                    !roleTag
                ) {
                    false to "TIFF header/provenance/embedded role-mask/Open-Scene binding ontbreekt"
                } else true to "TIFF header + embedded role-mask + Open-Scene binding geldig"
            }
            RestorationProjectionFormat.EXR -> {
                val exr = prefix.size >= 4 &&
                    (prefix[0].toInt() and 0xff) == 0x76 &&
                    (prefix[1].toInt() and 0xff) == 0x2f &&
                    (prefix[2].toInt() and 0xff) == 0x31 &&
                    (prefix[3].toInt() and 0xff) == 0x01
                val text = prefix.toString(Charsets.ISO_8859_1)
                if (!exr || !text.contains("truthrawProvenance") ||
                    !text.contains("TruthRaw Restoration Projection v0.69") ||
                    !text.contains("restoration_role_mask_sha256=") ||
                    !text.contains("open_scene_artifact_sha256=") ||
                    !text.contains("TR_ROLE")
                ) {
                    false to "OpenEXR magic/provenance/TR_ROLE/Open-Scene binding ontbreekt"
                } else true to "OpenEXR header + TR_ROLE role-mask + Open-Scene binding geldig"
            }
        }
    }

    private fun hasClassicTiffTag(prefix: ByteArray, wantedTag: Int): Boolean {
        if (prefix.size < 8 ||
            prefix[0] != 'I'.code.toByte() ||
            prefix[1] != 'I'.code.toByte()
        ) return false
        fun u16(offset: Int): Int =
            (prefix[offset].toInt() and 0xff) or
                ((prefix[offset + 1].toInt() and 0xff) shl 8)
        fun u32(offset: Int): Int =
            (prefix[offset].toInt() and 0xff) or
                ((prefix[offset + 1].toInt() and 0xff) shl 8) or
                ((prefix[offset + 2].toInt() and 0xff) shl 16) or
                ((prefix[offset + 3].toInt() and 0xff) shl 24)

        val ifdOffset = u32(4)
        if (ifdOffset < 0 || ifdOffset + 2 > prefix.size) return false
        val count = u16(ifdOffset)
        var pos = ifdOffset + 2
        repeat(count) {
            if (pos + 12 > prefix.size) return false
            if (u16(pos) == wantedTag) return true
            pos += 12
        }
        return false
    }

    private fun sha256(file: File): String? = try {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val b = ByteArray(BUFFER)
            while (true) {
                val n = input.read(b)
                if (n <= 0) break
                md.update(b, 0, n)
            }
        }
        md.digest().joinToString("") { "%02x".format(it) }
    } catch (_: Exception) { null }

    private fun sha256(resolver: ContentResolver, uri: Uri): String? {
        return try {
            val md = MessageDigest.getInstance("SHA-256")
            val input = resolver.openInputStream(uri) ?: return null
            input.use {
                val b = ByteArray(BUFFER)
                while (true) {
                    val n = it.read(b)
                    if (n <= 0) break
                    md.update(b, 0, n)
                }
            }
            md.digest().joinToString("") { "%02x".format(it) }
        } catch (_: Exception) { null }
    }
}

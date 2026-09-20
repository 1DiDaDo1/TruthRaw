package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.BitmapFactory
import android.graphics.ImageFormat
import android.graphics.Rect
import android.graphics.YuvImage
import android.net.Uri
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest

object PhotoExportNativeBridge {
    init { System.loadLibrary("truthraw_ui_preview_bridge") }

    external fun renderFullResNv21(
        sourceFd: Int,
        outputFd: Int,
        flags: Int,
        sourceRouteCode: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): LongArray
}

data class FullResJpegMetrics(
    val width: Int,
    val height: Int,
    val sourceWidth: Int,
    val sourceHeight: Int,
    val orientation: Int,
    val nv21Bytes: Long,
    val jpegBytes: Long,
    val advancedFlags: Int,
    val detailApplied: Boolean,
    val lightAdjustedPixels: Long,
    val hdrPositiveGainSamples: Long,
    val scientificMasterBound: Boolean,
    val backplaneBound: Boolean,
    val sourceReverified: Boolean,
    val fullResolution: Boolean,
    val hdrBakedIntoFront: Boolean,
    val restorationBakedIntoFront: Boolean,
    val jpegSha256: String,
)

sealed interface FullResJpegResult {
    data class Success(val metrics: FullResJpegMetrics, val file: File) : FullResJpegResult
    data class Failed(val reason: String) : FullResJpegResult
}

object FullResJpegExporter {
    private const val MAGIC = 0x54524a50L
    private const val PACKET_LONGS = 20
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
    private const val JPEG_QUALITY = 96
    private const val COPY_BUFFER = 1024 * 1024

    fun renderToPrivateJpeg(
        resolver: ContentResolver,
        job: RawJob,
        flags: Int,
        workingDir: File,
    ): FullResJpegResult {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return FullResJpegResult.Failed("Full-resolution JPG is alleen beschikbaar voor de admitted DNG-route.")
        }
        workingDir.mkdirs()
        val nv21File = File(workingDir, "photo_fullres.nv21.part")
        val jpegFile = File(workingDir, "photo_fullres.jpg.part")
        nv21File.delete()
        jpegFile.delete()

        val source = openRead(resolver, job.source.uri)
            ?: return FullResJpegResult.Failed("JPG: bron-FD kon niet worden geopend.")
        val output = try {
            ParcelFileDescriptor.open(
                nv21File,
                ParcelFileDescriptor.MODE_CREATE or
                    ParcelFileDescriptor.MODE_READ_WRITE or
                    ParcelFileDescriptor.MODE_TRUNCATE,
            )
        } catch (_: Throwable) {
            null
        } ?: run {
            source.close()
            return FullResJpegResult.Failed("JPG: NV21-staging kon niet worden geopend.")
        }

        val packet = try {
            source.use { src ->
                output.use { dst ->
                    PhotoExportNativeBridge.renderFullResNv21(
                        src.fd,
                        dst.fd,
                        flags,
                        when (job.source.sourceRoute) {
                            SourceIngressRoute.IMPORTED_FILE -> 0
                            SourceIngressRoute.CAMERA_CAPTURE -> 1
                        },
                        MAX_SOURCE_RESIDENT_BYTES,
                        MAX_LOGICAL_RESIDENT_BYTES,
                    )
                }
            }
        } catch (error: Throwable) {
            nv21File.delete()
            return FullResJpegResult.Failed(
                "JPG native full-resolution render faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size != PACKET_LONGS || packet[0] != MAGIC || packet[1] != 0L) {
            nv21File.delete()
            return FullResJpegResult.Failed(
                "JPG full-resolution render fail-closed status ${packet.getOrNull(1) ?: "pakketfout"}.",
            )
        }

        val width = packet[2].toInt()
        val height = packet[3].toInt()
        val sourceWidth = packet[4].toInt()
        val sourceHeight = packet[5].toInt()
        val orientation = packet[6].toInt()
        val nv21Bytes = packet[7]
        if (width <= 0 || height <= 0 || sourceWidth <= 0 || sourceHeight <= 0 ||
            nv21Bytes != width.toLong() * height.toLong() * 3L / 2L ||
            nv21File.length() != nv21Bytes ||
            packet[12] != 1L || packet[13] != 1L || packet[14] != 1L ||
            packet[15] != 1L || packet[18] != 1L || packet[19] != 1L
        ) {
            nv21File.delete()
            return FullResJpegResult.Failed("JPG full-resolution lineage/raster invariant faalde.")
        }

        val nv21 = try {
            if (nv21Bytes > Int.MAX_VALUE.toLong()) throw IllegalStateException("NV21 groter dan Java byte-array limiet.")
            FileInputStream(nv21File).use { input ->
                val bytes = ByteArray(nv21Bytes.toInt())
                var offset = 0
                while (offset < bytes.size) {
                    val n = input.read(bytes, offset, bytes.size - offset)
                    if (n <= 0) break
                    offset += n
                }
                if (offset != bytes.size) throw IllegalStateException("NV21 staging is onvolledig.")
                bytes
            }
        } catch (error: Throwable) {
            nv21File.delete()
            return FullResJpegResult.Failed("JPG NV21 staging kon niet worden geladen: ${error.message ?: error.javaClass.simpleName}")
        }

        val encoded = try {
            FileOutputStream(jpegFile).use { out ->
                YuvImage(nv21, ImageFormat.NV21, width, height, null)
                    .compressToJpeg(Rect(0, 0, width, height), JPEG_QUALITY, out)
            }
        } catch (_: Throwable) {
            false
        } finally {
            nv21File.delete()
        }
        if (!encoded || !jpegFile.isFile || jpegFile.length() <= 4L) {
            jpegFile.delete()
            return FullResJpegResult.Failed("Android JPEG-encoder gaf geen geldig full-resolution bestand.")
        }

        val options = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeFile(jpegFile.absolutePath, options)
        if (options.outWidth != width || options.outHeight != height) {
            jpegFile.delete()
            return FullResJpegResult.Failed(
                "JPG post-write resolutie ${options.outWidth}×${options.outHeight} != $width×$height.",
            )
        }
        val jpegSha = sha256(jpegFile)
            ?: run {
                jpegFile.delete()
                return FullResJpegResult.Failed("JPG SHA-256 kon niet worden berekend.")
            }

        return FullResJpegResult.Success(
            FullResJpegMetrics(
                width = width,
                height = height,
                sourceWidth = sourceWidth,
                sourceHeight = sourceHeight,
                orientation = orientation,
                nv21Bytes = nv21Bytes,
                jpegBytes = jpegFile.length(),
                advancedFlags = packet[8].toInt(),
                detailApplied = packet[9] != 0L,
                lightAdjustedPixels = packet[10],
                hdrPositiveGainSamples = packet[11],
                scientificMasterBound = packet[12] != 0L,
                backplaneBound = packet[13] != 0L,
                sourceReverified = packet[14] != 0L,
                fullResolution = packet[15] != 0L,
                hdrBakedIntoFront = packet[16] != 0L,
                restorationBakedIntoFront = packet[17] != 0L,
                jpegSha256 = jpegSha,
            ),
            jpegFile,
        )
    }

    fun commit(
        resolver: ContentResolver,
        privateJpeg: File,
        destination: Uri,
        expectedSha256: String,
    ): Boolean {
        val out = try { resolver.openFileDescriptor(destination, "rw") } catch (_: Throwable) { null }
            ?: return false
        val copied = try {
            out.use { pfd ->
                FileOutputStream(pfd.fileDescriptor).use { output ->
                    output.channel.truncate(0L)
                    FileInputStream(privateJpeg).use { input ->
                        val buffer = ByteArray(COPY_BUFFER)
                        while (true) {
                            val n = input.read(buffer)
                            if (n <= 0) break
                            output.write(buffer, 0, n)
                        }
                    }
                    output.flush()
                    output.channel.force(true)
                }
            }
            true
        } catch (_: Throwable) {
            false
        }
        if (!copied) return false
        return sha256(resolver, destination) == expectedSha256
    }

    private fun openRead(resolver: ContentResolver, uri: Uri): ParcelFileDescriptor? {
        return try {
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

    fun sha256(file: File): String? = try {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(COPY_BUFFER)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                md.update(buffer, 0, n)
            }
        }
        md.digest().joinToString("") { "%02x".format(it) }
    } catch (_: Throwable) { null }

    private fun sha256(resolver: ContentResolver, uri: Uri): String? {
        return try {
            val md = MessageDigest.getInstance("SHA-256")
            val stream = resolver.openInputStream(uri) ?: return null
            stream.use { input ->
                val buffer = ByteArray(COPY_BUFFER)
                while (true) {
                    val n = input.read(buffer)
                    if (n <= 0) break
                    md.update(buffer, 0, n)
                }
            }
            md.digest().joinToString("") { "%02x".format(it) }
        } catch (_: Throwable) {
            null
        }
    }
}

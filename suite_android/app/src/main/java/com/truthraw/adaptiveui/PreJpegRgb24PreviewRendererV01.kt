package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap
import android.os.ParcelFileDescriptor
import java.io.File
import java.io.RandomAccessFile

/**
 * Presentation-only screen renderer backed by the exact full-resolution photo
 * output renderer before JPEG encoding/chroma reduction.
 *
 * This is deliberately a sibling consumer of the admitted output cable:
 * - it never reads pixels from a UI preview or JPEG;
 * - it never creates evidence or writes Scientific Master;
 * - it never mutates the sealed source;
 * - the full-resolution RGB24 stage is private/ephemeral and only sampled for
 *   the screen bitmap.
 */
internal object PreJpegRgb24PreviewRendererV01 {
    const val CONTRACT_VERSION = "PreJpegRgb24PreviewRenderer/0.1"

    private const val MAGIC = 0x54524a50L
    private const val PACKET_LONGS = 48
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
    private const val DEFAULT_MAX_DIMENSION = 2048

    sealed interface Result {
        data class Ready(
            val bitmap: Bitmap,
            val sourceWidth: Int,
            val sourceHeight: Int,
            val outputWidth: Int,
            val outputHeight: Int,
            val sampleSize: Int,
            val advancedFlags: Int,
            val presentationHeadroomMode: Int,
        ) : Result

        data class Failed(val reason: String) : Result
    }

    fun render(
        resolver: ContentResolver,
        job: RawJob,
        flags: Int,
        presentationHeadroomMode: Int,
        userQuarterTurns: Int = 0,
        maxDimension: Int = DEFAULT_MAX_DIMENSION,
    ): Result {
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return Result.Failed("Exact output-preview vereist de admitted DNG-route.")
        }
        if (!PresentationHeadroomModeV01.isKnown(presentationHeadroomMode)) {
            return Result.Failed("Exact output-preview: onbekende presentation-headroom modus.")
        }
        if (userQuarterTurns !in 0..3) {
            return Result.Failed("Exact output-preview: ongeldige downstream oriëntatie.")
        }
        if (maxDimension <= 0) {
            return Result.Failed("Exact output-preview: ongeldige display-afmeting.")
        }

        val safeJobId = job.id.replace(Regex("[^A-Za-z0-9._-]"), "_").take(64)
        val workingDir = File.createTempFile("draw_preview_${safeJobId}_", ".dir").let { marker ->
            marker.delete()
            if (!marker.mkdirs()) {
                return Result.Failed("Exact output-preview: private stagingmap kon niet worden gemaakt.")
            }
            marker
        }
        val nativeStageFile = File(workingDir, "fullres.nv21_rgb24.part")

        try {
            val source = try {
                resolver.openFileDescriptor(job.source.uri, "r")
            } catch (_: Throwable) {
                null
            } ?: return Result.Failed("Exact output-preview: bron-FD kon niet worden geopend.")

            val output = try {
                ParcelFileDescriptor.open(
                    nativeStageFile,
                    ParcelFileDescriptor.MODE_CREATE or
                        ParcelFileDescriptor.MODE_READ_WRITE or
                        ParcelFileDescriptor.MODE_TRUNCATE,
                )
            } catch (_: Throwable) {
                null
            } ?: run {
                source.close()
                return Result.Failed("Exact output-preview: native staging kon niet worden geopend.")
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
                            userQuarterTurns,
                            presentationHeadroomMode,
                            MAX_SOURCE_RESIDENT_BYTES,
                            MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            } catch (error: Throwable) {
                return Result.Failed(
                    "Exact output-preview native render faalde: ${error.message ?: error.javaClass.simpleName}",
                )
            }

            if (packet.size != PACKET_LONGS || packet[0] != MAGIC || packet[1] != 0L) {
                return Result.Failed(
                    "Exact output-preview fail-closed status ${packet.getOrNull(1) ?: "pakketfout"}.",
                )
            }

            val width = packet[2].toInt()
            val height = packet[3].toInt()
            val sourceWidth = packet[4].toInt()
            val sourceHeight = packet[5].toInt()
            val nv21Bytes = packet[7]
            val presentationRgbBytes = packet[47]
            val packetUserQuarterTurns = packet[20].toInt()
            val effectiveOrientation = packet[21].toInt()
            val outputAuthorityArtifactSha256 = buildString(64) {
                for (word in 0 until 8) {
                    val value = packet[39 + word].toInt()
                    for (byte in 0 until 4) {
                        append(((value ushr (byte * 8)) and 0xff).toString(16).padStart(2, '0'))
                    }
                }
            }
            val expectedRgbBytes = width.toLong() * height.toLong() * 3L
            val expectedStageBytes = nv21Bytes + presentationRgbBytes

            if (
                width <= 0 || height <= 0 || sourceWidth <= 0 || sourceHeight <= 0 ||
                width > Int.MAX_VALUE / 3 ||
                nv21Bytes != width.toLong() * height.toLong() * 3L / 2L ||
                presentationRgbBytes != expectedRgbBytes ||
                expectedStageBytes <= nv21Bytes ||
                nativeStageFile.length() != expectedStageBytes ||
                packet[8].toInt() != flags ||
                packet[12] != 1L || packet[13] != 1L || packet[14] != 1L ||
                packet[15] != 1L || packet[18] != 1L || packet[19] != 1L ||
                packetUserQuarterTurns != userQuarterTurns ||
                effectiveOrientation !in setOf(1, 3, 6, 8) ||
                packet[22] != 1L || packet[23] != 1L ||
                packet[25] != 0L || packet[26] != 1L ||
                packet[28] != 0L || packet[30] != 2L || packet[31] != 1L ||
                packet[32] != 1L || packet[34] != 0L || packet[36] <= 0L ||
                packet[38] != sourceWidth.toLong() * sourceHeight.toLong() ||
                packet[33] + packet[34] + packet[35] + packet[36] != packet[38] * 3L ||
                outputAuthorityArtifactSha256.all { it == '0' }
            ) {
                return Result.Failed("Exact output-preview lineage/raster invariant faalde.")
            }

            val sampleSize = sampleSizeFor(width, height, maxDimension)
                ?: return Result.Failed("Exact output-preview display-sample overflow.")
            val outWidth = (width + sampleSize - 1) / sampleSize
            val outHeight = (height + sampleSize - 1) / sampleSize
            val pixelCount = outWidth.toLong() * outHeight.toLong()
            if (pixelCount <= 0L || pixelCount > Int.MAX_VALUE.toLong()) {
                return Result.Failed("Exact output-preview display-raster is te groot.")
            }

            val pixels = try {
                IntArray(pixelCount.toInt())
            } catch (_: OutOfMemoryError) {
                return Result.Failed("Exact output-preview: onvoldoende geheugen voor display-sample.")
            }
            val row = try {
                ByteArray(width * 3)
            } catch (_: OutOfMemoryError) {
                return Result.Failed("Exact output-preview: onvoldoende geheugen voor RGB24-rij.")
            }

            try {
                RandomAccessFile(nativeStageFile, "r").use { input ->
                    var oy = 0
                    var sy = 0
                    while (sy < height) {
                        val rowOffset =
                            nv21Bytes + sy.toLong() * width.toLong() * 3L
                        input.seek(rowOffset)
                        input.readFully(row)
                        val dstBase = oy * outWidth
                        var ox = 0
                        var sx = 0
                        while (sx < width) {
                            val p = sx * 3
                            val r = row[p].toInt() and 0xff
                            val g = row[p + 1].toInt() and 0xff
                            val b = row[p + 2].toInt() and 0xff
                            pixels[dstBase + ox] =
                                (0xff shl 24) or (r shl 16) or (g shl 8) or b
                            ox += 1
                            sx += sampleSize
                        }
                        oy += 1
                        sy += sampleSize
                    }
                }
            } catch (error: Throwable) {
                return Result.Failed(
                    "Exact output-preview RGB24-sampling faalde: ${error.message ?: error.javaClass.simpleName}",
                )
            }

            val bitmap = try {
                Bitmap.createBitmap(pixels, outWidth, outHeight, Bitmap.Config.ARGB_8888)
            } catch (_: OutOfMemoryError) {
                return Result.Failed("Exact output-preview bitmap faalde: onvoldoende geheugen.")
            } catch (error: Throwable) {
                return Result.Failed(
                    "Exact output-preview bitmap faalde: ${error.message ?: error.javaClass.simpleName}",
                )
            }

            return Result.Ready(
                bitmap = bitmap,
                sourceWidth = sourceWidth,
                sourceHeight = sourceHeight,
                outputWidth = width,
                outputHeight = height,
                sampleSize = sampleSize,
                advancedFlags = packet[8].toInt(),
                presentationHeadroomMode = presentationHeadroomMode,
            )
        } finally {
            nativeStageFile.delete()
            workingDir.delete()
        }
    }

    private fun sampleSizeFor(width: Int, height: Int, maxDimension: Int): Int? {
        var sampleSize = 1
        while (
            width / sampleSize > maxDimension ||
            height / sampleSize > maxDimension
        ) {
            if (sampleSize > Int.MAX_VALUE / 2) return null
            sampleSize *= 2
        }
        return sampleSize
    }
}

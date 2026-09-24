package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import java.io.File
import java.io.FileInputStream

private const val UOP1_MAGIC = 0x31504f55
private const val UOP1_HEADER_BYTES = 40
private const val UOP1_MAX_EDGE = 1024

data class UnifiedOutputPreviewMetrics(
    val width: Int,
    val height: Int,
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceSpaceCode: Int,
    val sampledPrimaryPixels: Long,
    val primaryTileSourceUsedDirectly: Boolean = true,
    val appearanceAddedByPreview: Boolean = false,
    val scientificWritebackAllowed: Boolean = false,
)

sealed interface UnifiedOutputPreviewResult {
    data class Ready(
        val bitmap: Bitmap,
        val metrics: UnifiedOutputPreviewMetrics,
        val outputLabel: String,
    ) : UnifiedOutputPreviewResult

    data class Failed(val reason: String) : UnifiedOutputPreviewResult
}

object UnifiedOutputPreviewLoader {
    fun loadSavedJpeg(
        resolver: ContentResolver,
        uri: Uri,
        outputLabel: String,
        maxEdge: Int = 384,
    ): UnifiedOutputPreviewResult {
        if (maxEdge !in 1..1024) {
            return UnifiedOutputPreviewResult.Failed(
                "JPEG uitkomst-preview maxEdge is buiten contract.",
            )
        }
        return try {
            val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            resolver.openInputStream(uri)?.use { input ->
                BitmapFactory.decodeStream(input, null, bounds)
            } ?: return UnifiedOutputPreviewResult.Failed(
                "Opgeslagen JPEG kon niet worden geopend voor preview.",
            )
            if (bounds.outWidth <= 0 || bounds.outHeight <= 0) {
                return UnifiedOutputPreviewResult.Failed(
                    "Opgeslagen JPEG heeft geen geldige afmetingen.",
                )
            }

            var sample = 1
            while (
                bounds.outWidth / (sample * 2) >= maxEdge ||
                bounds.outHeight / (sample * 2) >= maxEdge
            ) {
                sample *= 2
            }
            val options = BitmapFactory.Options().apply {
                inJustDecodeBounds = false
                inSampleSize = sample
                inPreferredConfig = Bitmap.Config.ARGB_8888
            }
            val decoded = resolver.openInputStream(uri)?.use { input ->
                BitmapFactory.decodeStream(input, null, options)
            } ?: return UnifiedOutputPreviewResult.Failed(
                "Opgeslagen JPEG kon niet worden gedecodeerd.",
            )

            val scale = minOf(
                1f,
                maxEdge.toFloat() /
                    maxOf(decoded.width, decoded.height).toFloat(),
            )
            val targetWidth =
                maxOf(1, kotlin.math.round(decoded.width * scale).toInt())
            val targetHeight =
                maxOf(1, kotlin.math.round(decoded.height * scale).toInt())
            val preview = if (
                targetWidth == decoded.width &&
                targetHeight == decoded.height
            ) {
                decoded
            } else {
                Bitmap.createScaledBitmap(
                    decoded,
                    targetWidth,
                    targetHeight,
                    true,
                ).also { decoded.recycle() }
            }

            UnifiedOutputPreviewResult.Ready(
                bitmap = preview,
                metrics = UnifiedOutputPreviewMetrics(
                    width = preview.width,
                    height = preview.height,
                    sourceWidth = bounds.outWidth,
                    sourceHeight = bounds.outHeight,
                    sourceSpaceCode = 4,
                    sampledPrimaryPixels =
                        preview.width.toLong() * preview.height.toLong(),
                    primaryTileSourceUsedDirectly = true,
                    appearanceAddedByPreview = false,
                    scientificWritebackAllowed = false,
                ),
                outputLabel = outputLabel,
            )
        } catch (error: Throwable) {
            UnifiedOutputPreviewResult.Failed(
                "Opgeslagen JPEG uitkomst-preview faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }
    }

    fun load(file: File, outputLabel: String): UnifiedOutputPreviewResult {
        if (!file.isFile || file.length() < UOP1_HEADER_BYTES.toLong()) {
            return UnifiedOutputPreviewResult.Failed(
                "Unified Output Preview ontbreekt of is te klein.",
            )
        }

        return try {
            FileInputStream(file).use { input ->
                val header = ByteArray(UOP1_HEADER_BYTES)
                var offset = 0
                while (offset < header.size) {
                    val n = input.read(header, offset, header.size - offset)
                    if (n <= 0) break
                    offset += n
                }
                if (offset != header.size) {
                    return UnifiedOutputPreviewResult.Failed(
                        "Unified Output Preview header is onvolledig.",
                    )
                }

                fun u32(at: Int): Int =
                    (header[at].toInt() and 0xff) or
                        ((header[at + 1].toInt() and 0xff) shl 8) or
                        ((header[at + 2].toInt() and 0xff) shl 16) or
                        ((header[at + 3].toInt() and 0xff) shl 24)

                fun u64(at: Int): Long {
                    var value = 0L
                    for (i in 0 until 8) {
                        value = value or
                            ((header[at + i].toLong() and 0xffL) shl (8 * i))
                    }
                    return value
                }

                val magic = u32(0)
                val version = u32(4)
                val width = u32(8)
                val height = u32(12)
                val sourceWidth = u32(16)
                val sourceHeight = u32(20)
                val sourceSpace = u32(24)
                val flags = u32(28)
                val sampled = u64(32)

                if (
                    magic != UOP1_MAGIC ||
                    version != 1 ||
                    width !in 1..UOP1_MAX_EDGE ||
                    height !in 1..UOP1_MAX_EDGE ||
                    sourceWidth <= 0 ||
                    sourceHeight <= 0 ||
                    sourceSpace !in 1..3 ||
                    flags != 0 ||
                    sampled != width.toLong() * height.toLong()
                ) {
                    return UnifiedOutputPreviewResult.Failed(
                        "Unified Output Preview contract faalde.",
                    )
                }

                val pixelCount = width.toLong() * height.toLong()
                val expectedBytes =
                    UOP1_HEADER_BYTES.toLong() + pixelCount * 4L
                if (file.length() != expectedBytes) {
                    return UnifiedOutputPreviewResult.Failed(
                        "Unified Output Preview bytegrootte klopt niet.",
                    )
                }

                val argb = IntArray(pixelCount.toInt())
                val pixel = ByteArray(4)
                for (i in argb.indices) {
                    var read = 0
                    while (read < 4) {
                        val n = input.read(pixel, read, 4 - read)
                        if (n <= 0) break
                        read += n
                    }
                    if (read != 4) {
                        return UnifiedOutputPreviewResult.Failed(
                            "Unified Output Preview pixelpayload is onvolledig.",
                        )
                    }
                    argb[i] =
                        (pixel[0].toInt() and 0xff) or
                            ((pixel[1].toInt() and 0xff) shl 8) or
                            ((pixel[2].toInt() and 0xff) shl 16) or
                            ((pixel[3].toInt() and 0xff) shl 24)
                }

                val bitmap = Bitmap.createBitmap(
                    argb,
                    width,
                    height,
                    Bitmap.Config.ARGB_8888,
                )

                UnifiedOutputPreviewResult.Ready(
                    bitmap = bitmap,
                    metrics = UnifiedOutputPreviewMetrics(
                        width = width,
                        height = height,
                        sourceWidth = sourceWidth,
                        sourceHeight = sourceHeight,
                        sourceSpaceCode = sourceSpace,
                        sampledPrimaryPixels = sampled,
                    ),
                    outputLabel = outputLabel,
                )
            }
        } catch (error: Throwable) {
            UnifiedOutputPreviewResult.Failed(
                "Unified Output Preview kon niet worden geladen: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }
    }
}

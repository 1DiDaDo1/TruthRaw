package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import java.io.File
import java.io.RandomAccessFile

/**
 * Presentation-only decoder for the bridge-owned full-resolution Free Raster
 * artifact.
 *
 * The backing artifact remains full resolution. This loader may sample only the
 * display bitmap needed by the current Workspace viewport; that sampling never
 * changes the underlying output raster, source authority or Scientific Master.
 */
internal object UnifiedOutputFreeRasterDisplayLoaderV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterDisplayLoader/0.2"
    private const val DEFAULT_MAX_DIMENSION = 2048

    sealed interface Result {
        data class Ready(
            val bitmap: Bitmap,
            val sourceWidth: Int,
            val sourceHeight: Int,
            val sampleSize: Int,
            val decoderId: String,
        ) : Result

        data class Failed(
            val reason: String,
        ) : Result
    }

    fun load(
        snapshot: UnifiedOutputFreeRasterSnapshotV01,
        maxDimension: Int = DEFAULT_MAX_DIMENSION,
    ): Result {
        if (snapshot.request.purpose !=
            DrawUnifiedOutputRasterRequestV01.Purpose.FREE_RASTER_VIEW
        ) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: snapshot is geen FREE_RASTER_VIEW output.",
            )
        }
        if (snapshot.viewportAuthority != "PRESENTATION_ONLY") {
            return Result.Failed(
                "Free Raster decode geblokkeerd: viewport-authority mismatch.",
            )
        }
        if (snapshot.artifactAuthority != DrawPhotoOutputCableV01.OUTPUT_AUTHORITY) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: artifact-authority mismatch.",
            )
        }
        if (maxDimension <= 0) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: ongeldige display-afmeting.",
            )
        }

        val file = File(snapshot.artifactPath)
        if (!file.isFile || file.length() != snapshot.artifactBytes) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: private full-resolution artifact ontbreekt.",
            )
        }

        return when {
            snapshot.mediaType == FullResPresentationRasterV01.MEDIA_TYPE &&
                snapshot.pixelFormat == FullResPresentationRasterV01.PIXEL_FORMAT ->
                loadRgb24(snapshot, file, maxDimension)
            snapshot.mediaType == "image/jpeg" ->
                loadEncodedBitmap(snapshot, file, maxDimension)
            else -> Result.Failed(
                "Free Raster decode geblokkeerd: onbekend presentation-rasterformaat.",
            )
        }
    }

    private fun loadRgb24(
        snapshot: UnifiedOutputFreeRasterSnapshotV01,
        file: File,
        maxDimension: Int,
    ): Result {
        val width = snapshot.rasterWidth
        val height = snapshot.rasterHeight
        if (width <= 0 || height <= 0) {
            return Result.Failed("Free Raster RGB24 decode: ongeldige rastergeometrie.")
        }
        val expectedBytes = width.toLong() * height.toLong() * 3L
        if (expectedBytes <= 0L || file.length() != expectedBytes) {
            return Result.Failed(
                "Free Raster RGB24 decode geblokkeerd: bytegeometrie wijkt af van de binding.",
            )
        }

        val sampleSize = sampleSizeFor(width, height, maxDimension)
            ?: return Result.Failed("Free Raster RGB24 decode: display-sample overflow.")
        val outWidth = (width + sampleSize - 1) / sampleSize
        val outHeight = (height + sampleSize - 1) / sampleSize
        val pixelCount = outWidth.toLong() * outHeight.toLong()
        if (pixelCount <= 0L || pixelCount > Int.MAX_VALUE.toLong()) {
            return Result.Failed("Free Raster RGB24 decode: display-raster is te groot.")
        }

        val pixels = try {
            IntArray(pixelCount.toInt())
        } catch (_: OutOfMemoryError) {
            return Result.Failed("Free Raster RGB24 decode faalde: onvoldoende geheugen.")
        }
        val row = ByteArray(width * 3)
        try {
            RandomAccessFile(file, "r").use { input ->
                var oy = 0
                var sy = 0
                while (sy < height) {
                    input.seek(sy.toLong() * width.toLong() * 3L)
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
                "Free Raster RGB24 decode faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        val bitmap = try {
            Bitmap.createBitmap(pixels, outWidth, outHeight, Bitmap.Config.ARGB_8888)
        } catch (_: OutOfMemoryError) {
            return Result.Failed("Free Raster RGB24 bitmap faalde: onvoldoende geheugen.")
        } catch (error: Throwable) {
            return Result.Failed(
                "Free Raster RGB24 bitmap faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }
        return Result.Ready(
            bitmap = bitmap,
            sourceWidth = width,
            sourceHeight = height,
            sampleSize = sampleSize,
            decoderId = "truthraw.rgb24.sampled.v1",
        )
    }

    private fun loadEncodedBitmap(
        snapshot: UnifiedOutputFreeRasterSnapshotV01,
        file: File,
        maxDimension: Int,
    ): Result {
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        try {
            BitmapFactory.decodeFile(file.absolutePath, bounds)
        } catch (error: Exception) {
            return Result.Failed(
                "Free Raster bounds-decode faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (bounds.outWidth <= 0 || bounds.outHeight <= 0) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: artifact heeft geen leesbare rasterafmetingen.",
            )
        }
        if (bounds.outWidth != snapshot.rasterWidth ||
            bounds.outHeight != snapshot.rasterHeight
        ) {
            return Result.Failed(
                "Free Raster decode geblokkeerd: artifactgeometrie wijkt af van de output-binding.",
            )
        }

        val sampleSize = sampleSizeFor(bounds.outWidth, bounds.outHeight, maxDimension)
            ?: return Result.Failed("Free Raster decode geblokkeerd: display-sample overflow.")
        val options = BitmapFactory.Options().apply {
            inSampleSize = sampleSize
            inPreferredConfig = Bitmap.Config.ARGB_8888
        }

        val bitmap = try {
            BitmapFactory.decodeFile(file.absolutePath, options)
        } catch (_: OutOfMemoryError) {
            return Result.Failed(
                "Free Raster display-decode faalde: onvoldoende geheugen.",
            )
        } catch (error: Exception) {
            return Result.Failed(
                "Free Raster display-decode faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return Result.Failed(
            "Free Raster display-decode leverde geen bitmap.",
        )

        return Result.Ready(
            bitmap = bitmap,
            sourceWidth = bounds.outWidth,
            sourceHeight = bounds.outHeight,
            sampleSize = sampleSize,
            decoderId = "android.bitmap_factory.private_file.v1",
        )
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

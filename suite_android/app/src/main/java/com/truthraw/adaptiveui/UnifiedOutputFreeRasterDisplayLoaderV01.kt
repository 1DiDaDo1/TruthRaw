package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import java.io.File

/**
 * Presentation-only decoder for the bridge-owned full-resolution Free Raster
 * artifact.
 *
 * The backing artifact remains full resolution. This loader may sample only the
 * display bitmap needed by the current Workspace viewport; that sampling never
 * changes the underlying output raster, source authority or Scientific Master.
 * A later tiled/region viewport may replace this adapter without changing the
 * outer Free Raster contract.
 */
internal object UnifiedOutputFreeRasterDisplayLoaderV01 {
    const val CONTRACT_VERSION = "UnifiedOutputFreeRasterDisplayLoader/0.1"
    private const val DEFAULT_MAX_DIMENSION = 2048

    sealed interface Result {
        data class Ready(
            val bitmap: Bitmap,
            val sourceWidth: Int,
            val sourceHeight: Int,
            val sampleSize: Int,
            val decoderId: String = "android.bitmap_factory.private_file.v1",
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

        var sampleSize = 1
        while (
            bounds.outWidth / sampleSize > maxDimension ||
            bounds.outHeight / sampleSize > maxDimension
        ) {
            if (sampleSize > Int.MAX_VALUE / 2) {
                return Result.Failed(
                    "Free Raster decode geblokkeerd: display-sample overflow.",
                )
            }
            sampleSize *= 2
        }

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
        )
    }
}

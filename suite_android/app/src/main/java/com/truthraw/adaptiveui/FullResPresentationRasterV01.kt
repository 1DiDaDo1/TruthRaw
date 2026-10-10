package com.truthraw.adaptiveui

import java.io.File

/**
 * Process-local binding between one compatibility JPEG candidate and the exact
 * full-resolution RGB24 presentation raster produced by the same native render.
 *
 * This is presentation lineage only. It never upgrades authority, creates
 * MEASURED evidence or writes to Scientific Master.
 */
internal data class FullResPresentationRasterV01(
    val file: File,
    val width: Int,
    val height: Int,
    val bytes: Long,
    val sha256: String,
    val pixelFormat: String = PIXEL_FORMAT,
) {
    companion object {
        const val PIXEL_FORMAT = "RGB24_SRGB_8"
        const val MEDIA_TYPE = "application/x-truthraw-rgb24"
    }
}

internal object FullResPresentationRasterRegistryV01 {
    const val CONTRACT_VERSION = "FullResPresentationRasterRegistry/0.1"

    private data class Entry(
        val jpegPath: String,
        val raster: FullResPresentationRasterV01,
    )

    private val lock = Any()
    private var current: Entry? = null

    fun register(jpegFile: File, raster: FullResPresentationRasterV01): Boolean {
        val expectedBytes = raster.width.toLong() * raster.height.toLong() * 3L
        if (
            !jpegFile.isFile ||
            raster.width <= 0 ||
            raster.height <= 0 ||
            raster.bytes != expectedBytes ||
            !raster.file.isFile ||
            raster.file.length() != expectedBytes ||
            !raster.sha256.matches(Regex("[0-9a-f]{64}")) ||
            raster.pixelFormat != FullResPresentationRasterV01.PIXEL_FORMAT
        ) {
            return false
        }

        synchronized(lock) {
            val previous = current
            current = Entry(jpegFile.absolutePath, raster)
            if (previous != null && previous.raster.file.absolutePath != raster.file.absolutePath) {
                previous.raster.file.delete()
            }
        }
        return true
    }

    fun resolveForJpeg(
        jpegFile: File,
        expectedWidth: Int,
        expectedHeight: Int,
    ): FullResPresentationRasterV01? = synchronized(lock) {
        val entry = current ?: return@synchronized null
        val raster = entry.raster
        val expectedBytes = expectedWidth.toLong() * expectedHeight.toLong() * 3L
        if (
            entry.jpegPath != jpegFile.absolutePath ||
            raster.width != expectedWidth ||
            raster.height != expectedHeight ||
            raster.bytes != expectedBytes ||
            !raster.file.isFile ||
            raster.file.length() != expectedBytes
        ) {
            return@synchronized null
        }
        raster
    }

    fun releaseForJpeg(jpegFile: File, deleteRaster: Boolean) {
        synchronized(lock) {
            val entry = current ?: return
            if (entry.jpegPath != jpegFile.absolutePath) return
            current = null
            if (deleteRaster) entry.raster.file.delete()
        }
    }

    fun clear(deleteRaster: Boolean = true) {
        synchronized(lock) {
            val entry = current
            current = null
            if (deleteRaster) entry?.raster?.file?.delete()
        }
    }
}

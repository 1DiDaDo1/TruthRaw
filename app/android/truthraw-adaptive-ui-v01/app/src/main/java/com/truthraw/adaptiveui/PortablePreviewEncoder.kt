package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.ColorSpace
import java.io.IOException
import java.io.OutputStream
import java.lang.ref.WeakReference

object PortablePreviewEncoder {
    const val DEFAULT_JPEG_QUALITY = 92

    // Staged DNG-preview integration: keep only a weak reference to the most
    // recently materialized bounded sRGB preview. MainActivity already gates
    // DNG export on the active TilePreviewUiState.Ready instance. The exporter
    // takes its own small snapshot so UI lifecycle/recycle events cannot mutate
    // preview bytes while the native DNG writer is running.
    //
    // This is intentionally not a scientific cache and carries no authority of
    // its own. The authoritative admission remains Finalized Scientific Preview.
    private var latestSrgbPreview: WeakReference<Bitmap>? = null

    @Synchronized
    fun createSrgbBitmap(
        width: Int,
        height: Int,
        argb: IntArray,
        offset: Int = 0,
    ): Bitmap {
        require(width > 0 && height > 0) { "Preview dimensions must be positive." }
        val pixelCount = Math.multiplyExact(width, height)
        require(offset >= 0 && offset <= argb.size - pixelCount) { "ARGB payload is outside bounds." }

        val srgb = ColorSpace.get(ColorSpace.Named.SRGB)
        val bitmap = Bitmap.createBitmap(
            width,
            height,
            Bitmap.Config.ARGB_8888,
            false,
            srgb,
        )
        bitmap.setPixels(argb, offset, width, 0, 0, width, height)
        latestSrgbPreview = WeakReference(bitmap)
        return bitmap
    }

    @Synchronized
    fun snapshotLatestSrgbBitmap(): Bitmap? {
        val source = latestSrgbPreview?.get() ?: return null
        if (source.isRecycled || source.width <= 0 || source.height <= 0) return null
        if (source.config != Bitmap.Config.ARGB_8888) return null
        val sourceColorSpace = source.colorSpace
        if (sourceColorSpace == null || !sourceColorSpace.isSrgb) return null

        val copy = source.copy(Bitmap.Config.ARGB_8888, false) ?: return null
        val copyColorSpace = copy.colorSpace
        if (copyColorSpace == null || !copyColorSpace.isSrgb) {
            copy.recycle()
            return null
        }
        return copy
    }

    @Throws(IOException::class)
    fun encodeJpeg(
        bitmap: Bitmap,
        destination: OutputStream,
        quality: Int = DEFAULT_JPEG_QUALITY,
    ) {
        require(quality in 1..100) { "JPEG quality must be in 1..100." }
        require(bitmap.config == Bitmap.Config.ARGB_8888) { "Portable preview must originate from ARGB_8888." }
        val colorSpace = bitmap.colorSpace
        require(colorSpace != null && colorSpace.isSrgb) { "Portable preview must be rendered to sRGB before JPEG encoding." }

        if (!bitmap.compress(Bitmap.CompressFormat.JPEG, quality, destination)) {
            throw IOException("Android JPEG encoder returned false.")
        }
        destination.flush()
    }
}

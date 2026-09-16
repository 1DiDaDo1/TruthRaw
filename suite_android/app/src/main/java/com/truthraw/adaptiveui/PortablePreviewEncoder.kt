package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.ColorSpace
import java.io.IOException
import java.io.OutputStream

object PortablePreviewEncoder {
    const val DEFAULT_JPEG_QUALITY = 92

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
        return bitmap
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

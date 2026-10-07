package com.truthraw.adaptiveui

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.ByteArrayOutputStream
import java.io.File

class HighFidelityJpegContractV01Test {
    @Test
    fun acceptsExactQ100444Header() {
        withTempJpeg(jpegHeader(width = 4080, height = 3072)) { file ->
            val result = HighFidelityJpegContractV01.verify(file, 4080, 3072)
            assertTrue(result is HighFidelityJpegContractV01.Result.Ready)
            val ready = result as HighFidelityJpegContractV01.Result.Ready
            assertEquals(100, ready.requestedQuality)
            assertEquals("1x1,1x1,1x1", ready.sampling)
            assertTrue(ready.quantizationTablesAllOnes)
        }
    }

    @Test
    fun rejects420Sampling() {
        withTempJpeg(jpegHeader(width = 4080, height = 3072, ySampling = 0x22)) { file ->
            val result = HighFidelityJpegContractV01.verify(file, 4080, 3072)
            assertTrue(result is HighFidelityJpegContractV01.Result.Failed)
        }
    }

    @Test
    fun rejectsNonQ100Quantization() {
        withTempJpeg(jpegHeader(width = 4080, height = 3072, dqtValue = 2)) { file ->
            val result = HighFidelityJpegContractV01.verify(file, 4080, 3072)
            assertTrue(result is HighFidelityJpegContractV01.Result.Failed)
        }
    }

    @Test
    fun rejectsWrongGeometry() {
        withTempJpeg(jpegHeader(width = 4080, height = 3072)) { file ->
            val result = HighFidelityJpegContractV01.verify(file, 3072, 4080)
            assertTrue(result is HighFidelityJpegContractV01.Result.Failed)
        }
    }

    private fun withTempJpeg(bytes: ByteArray, block: (File) -> Unit) {
        val file = File.createTempFile("draw_hf_jpeg_", ".jpg")
        try {
            file.writeBytes(bytes)
            block(file)
        } finally {
            file.delete()
        }
    }

    private fun jpegHeader(
        width: Int,
        height: Int,
        ySampling: Int = 0x11,
        dqtValue: Int = 1,
    ): ByteArray {
        val out = ByteArrayOutputStream()
        fun b(value: Int) = out.write(value and 0xff)
        fun u16(value: Int) {
            b(value ushr 8)
            b(value)
        }

        b(0xff); b(0xd8)

        b(0xff); b(0xdb); u16(132)
        b(0x00); repeat(64) { b(dqtValue) }
        b(0x01); repeat(64) { b(dqtValue) }

        b(0xff); b(0xc0); u16(17)
        b(8); u16(height); u16(width); b(3)
        b(1); b(ySampling); b(0)
        b(2); b(0x11); b(1)
        b(3); b(0x11); b(1)

        b(0xff); b(0xda); u16(12)
        b(3)
        b(1); b(0x00)
        b(2); b(0x11)
        b(3); b(0x11)
        b(0); b(63); b(0)
        return out.toByteArray()
    }
}

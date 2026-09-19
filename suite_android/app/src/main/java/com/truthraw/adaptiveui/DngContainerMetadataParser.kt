package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel

/**
 * Minimal read-only classic-TIFF/DNG metadata parser.
 *
 * TIFF headers, IFDs and metadata values only. Image sample payload is never decoded.
 * Strip/tile offset and byte-count arrays are summarized but never followed into pixel data.
 */
object DngContainerMetadataParser {
    private const val MAX_IFD_ENTRIES = 4096
    private const val MAX_METADATA_VALUE_BYTES = 1_048_576
    private const val MAX_RECURSION = 4

    private val tagNames = mapOf(
        254 to "NewSubfileType", 256 to "ImageWidth", 257 to "ImageLength",
        258 to "BitsPerSample", 259 to "Compression", 262 to "PhotometricInterpretation",
        270 to "ImageDescription", 271 to "Make", 272 to "Model",
        273 to "StripOffsets", 274 to "Orientation", 277 to "SamplesPerPixel",
        278 to "RowsPerStrip", 279 to "StripByteCounts", 284 to "PlanarConfiguration",
        305 to "Software", 306 to "DateTime", 324 to "TileOffsets", 325 to "TileByteCounts",
        330 to "SubIFDs", 33421 to "CFARepeatPatternDim", 33422 to "CFAPattern",
        33434 to "ExposureTime", 33437 to "FNumber", 34855 to "ISOSpeedRatings",
        37386 to "FocalLength", 50706 to "DNGVersion", 50707 to "DNGBackwardVersion",
        50708 to "UniqueCameraModel", 50710 to "CFAPlaneColor", 50711 to "CFALayout",
        50713 to "BlackLevelRepeatDim", 50714 to "BlackLevel", 50717 to "WhiteLevel",
        50718 to "DefaultScale", 50719 to "DefaultCropOrigin", 50720 to "DefaultCropSize",
        50721 to "ColorMatrix1", 50722 to "ColorMatrix2", 50723 to "CameraCalibration1",
        50724 to "CameraCalibration2", 50728 to "AsShotNeutral", 50730 to "BaselineExposure",
        50778 to "CalibrationIlluminant1", 50779 to "CalibrationIlluminant2",
        50829 to "ActiveArea", 50964 to "ForwardMatrix1", 50965 to "ForwardMatrix2",
        51009 to "OpcodeList2", 51022 to "OpcodeList3", 51041 to "NoiseProfile",
    )

    private val typeNames = mapOf(
        1 to "BYTE", 2 to "ASCII", 3 to "SHORT", 4 to "LONG", 5 to "RATIONAL",
        6 to "SBYTE", 7 to "UNDEFINED", 8 to "SSHORT", 9 to "SLONG",
        10 to "SRATIONAL", 11 to "FLOAT", 12 to "DOUBLE",
    )

    private data class Entry(
        val tag: Int,
        val type: Int,
        val count: Long,
        val valueOffset: Long,
        val inlineBytes: ByteArray,
    )

    fun parse(channel: FileChannel, fileSize: Long): JSONObject {
        val header = readAt(channel, 0L, 8)
        require(header.size == 8) { "TIFF header te kort" }

        val order = when {
            header[0] == 'I'.code.toByte() && header[1] == 'I'.code.toByte() -> ByteOrder.LITTLE_ENDIAN
            header[0] == 'M'.code.toByte() && header[1] == 'M'.code.toByte() -> ByteOrder.BIG_ENDIAN
            else -> error("Geen TIFF byte-order marker")
        }

        val hb = ByteBuffer.wrap(header).order(order)
        val magic = u16(hb.getShort(2))
        require(magic == 42) { "Alleen classic TIFF/DNG magic 42 ondersteund; observed=$magic" }
        val firstIfd = u32(hb.getInt(4))

        val ifds = JSONArray()
        val visited = mutableSetOf<Long>()
        parseIfdRecursive(channel, fileSize, order, firstIfd, "IFD0", 0, visited, ifds)

        val rawCandidates = JSONArray()
        for (i in 0 until ifds.length()) {
            val ifd = ifds.getJSONObject(i)
            if (ifd.optLong("photometricInterpretation", -1L) == 32803L) {
                rawCandidates.put(
                    JSONObject()
                        .put("path", ifd.optString("path"))
                        .put("imageWidth", ifd.opt("imageWidth"))
                        .put("imageLength", ifd.opt("imageLength"))
                        .put("bitsPerSample", ifd.opt("bitsPerSample"))
                        .put("compression", ifd.opt("compression"))
                        .put("samplesPerPixel", ifd.opt("samplesPerPixel"))
                        .put("cfaRepeatPatternDim", ifd.opt("cfaRepeatPatternDim"))
                        .put("cfaPattern", ifd.opt("cfaPattern"))
                        .put("blackLevel", ifd.opt("blackLevel"))
                        .put("whiteLevel", ifd.opt("whiteLevel"))
                        .put("activeArea", ifd.opt("activeArea"))
                        .put("defaultCropSize", ifd.opt("defaultCropSize"))
                        .put("stripByteCounts", ifd.opt("stripByteCounts"))
                        .put("tileByteCounts", ifd.opt("tileByteCounts")),
                )
            }
        }

        return JSONObject()
            .put("parser", "truthraw.classic-tiff-dng-container-metadata.v0.54")
            .put("readOnly", true)
            .put("imageSamplesDecoded", false)
            .put("pixelPayloadBytesRead", false)
            .put("fileSizeBytes", fileSize)
            .put("byteOrder", if (order == ByteOrder.LITTLE_ENDIAN) "II_LITTLE_ENDIAN" else "MM_BIG_ENDIAN")
            .put("tiffMagic", magic)
            .put("firstIfdOffset", firstIfd)
            .put("ifds", ifds)
            .put("rawCfaIfdCandidates", rawCandidates)
    }

    private fun parseIfdRecursive(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        offset: Long,
        path: String,
        depth: Int,
        visited: MutableSet<Long>,
        out: JSONArray,
    ) {
        if (offset <= 0L || offset >= fileSize || depth > MAX_RECURSION || !visited.add(offset)) return

        val countBytes = readAt(channel, offset, 2)
        require(countBytes.size == 2) { "IFD count buiten bestand: $path@$offset" }
        val count = u16(ByteBuffer.wrap(countBytes).order(order).short)
        require(count <= MAX_IFD_ENTRIES) { "IFD entry count absurd: $count" }

        val blockBytes = 2 + count * 12 + 4
        val block = readAt(channel, offset, blockBytes)
        require(block.size == blockBytes) { "IFD block onvolledig: $path" }

        val entries = mutableListOf<Entry>()
        var p = 2
        repeat(count) {
            val eb = ByteBuffer.wrap(block, p, 12).slice().order(order)
            val tag = u16(eb.short)
            val type = u16(eb.short)
            val itemCount = u32(eb.int)
            val valueField = ByteArray(4)
            eb.get(valueField)
            val valueOffset = u32(ByteBuffer.wrap(valueField).order(order).int)
            entries += Entry(tag, type, itemCount, valueOffset, valueField)
            p += 12
        }
        val nextIfdOffset = u32(ByteBuffer.wrap(block, p, 4).order(order).int)

        val entryJson = JSONArray()
        val ifdJson = JSONObject()
            .put("path", path)
            .put("offset", offset)
            .put("entryCount", count)
            .put("nextIfdOffset", nextIfdOffset)
            .put("entries", entryJson)

        var subIfdOffsets: List<Long> = emptyList()

        for (entry in entries) {
            val decoded = decodeEntry(channel, fileSize, order, entry)
            val tagJson = JSONObject()
                .put("tag", entry.tag)
                .put("name", tagNames[entry.tag] ?: "TAG_${entry.tag}")
                .put("type", entry.type)
                .put("typeName", typeNames[entry.type] ?: "TYPE_${entry.type}")
                .put("count", entry.count)
                .put("valueByteCount", safeMul(entry.count, typeSize(entry.type).toLong()))
                .put("value", decoded)
            entryJson.put(tagJson)

            when (entry.tag) {
                256 -> ifdJson.put("imageWidth", decoded)
                257 -> ifdJson.put("imageLength", decoded)
                258 -> ifdJson.put("bitsPerSample", decoded)
                259 -> ifdJson.put("compression", decoded)
                262 -> ifdJson.put("photometricInterpretation", decoded)
                277 -> ifdJson.put("samplesPerPixel", decoded)
                273 -> ifdJson.put("stripOffsets", decoded)
                279 -> ifdJson.put("stripByteCounts", decoded)
                324 -> ifdJson.put("tileOffsets", decoded)
                325 -> ifdJson.put("tileByteCounts", decoded)
                33421 -> ifdJson.put("cfaRepeatPatternDim", decoded)
                33422 -> ifdJson.put("cfaPattern", decoded)
                50706 -> ifdJson.put("dngVersion", decoded)
                50707 -> ifdJson.put("dngBackwardVersion", decoded)
                50708 -> ifdJson.put("uniqueCameraModel", decoded)
                50713 -> ifdJson.put("blackLevelRepeatDim", decoded)
                50714 -> ifdJson.put("blackLevel", decoded)
                50717 -> ifdJson.put("whiteLevel", decoded)
                50719 -> ifdJson.put("defaultCropOrigin", decoded)
                50720 -> ifdJson.put("defaultCropSize", decoded)
                50829 -> ifdJson.put("activeArea", decoded)
                330 -> subIfdOffsets = decodeUnsignedLongValues(channel, fileSize, order, entry)
            }
        }

        out.put(ifdJson)

        subIfdOffsets.forEachIndexed { index, child ->
            parseIfdRecursive(channel, fileSize, order, child, "$path/SubIFD[$index]", depth + 1, visited, out)
        }
        if (nextIfdOffset > 0L) {
            parseIfdRecursive(channel, fileSize, order, nextIfdOffset, "$path/NextIFD", depth + 1, visited, out)
        }
    }

    private fun decodeEntry(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry,
    ): Any {
        val size = typeSize(entry.type)
        if (size <= 0) return JSONObject().put("unsupportedType", entry.type)

        val byteCount = safeMul(entry.count, size.toLong())
        if (byteCount < 0L) return JSONObject().put("overflow", true)

        val bytes = if (byteCount <= 4L) {
            entry.inlineBytes.copyOf(byteCount.toInt())
        } else {
            if (entry.valueOffset < 0L || entry.valueOffset + byteCount > fileSize) {
                return JSONObject().put("invalidOffset", entry.valueOffset).put("byteCount", byteCount)
            }
            if (byteCount > MAX_METADATA_VALUE_BYTES) {
                return JSONObject()
                    .put("notDecodedTooLarge", true)
                    .put("offset", entry.valueOffset)
                    .put("byteCount", byteCount)
            }
            readAt(channel, entry.valueOffset, byteCount.toInt())
        }

        if (entry.type == 2) {
            return bytes.toString(Charsets.US_ASCII).trimEnd('\u0000')
        }

        if (entry.type == 7 && entry.count > 64L) {
            return JSONObject()
                .put("opaqueMetadataBytes", bytes.size)
                .put("hexPrefix", bytes.take(16).joinToString("") { "%02x".format(it.toInt() and 0xff) })
        }

        val values = decodeValues(bytes, order, entry.type, entry.count.toInt())

        return if ((entry.tag == 273 || entry.tag == 279 || entry.tag == 324 || entry.tag == 325) && values.length() > 16) {
            summarizeNumericArray(values)
        } else if (values.length() == 1) {
            values.get(0)
        } else {
            values
        }
    }

    private fun decodeUnsignedLongValues(
        channel: FileChannel,
        fileSize: Long,
        order: ByteOrder,
        entry: Entry,
    ): List<Long> {
        if (entry.type != 4) return emptyList()
        val byteCount = safeMul(entry.count, 4L)
        if (byteCount <= 0 || byteCount > MAX_METADATA_VALUE_BYTES) return emptyList()
        val bytes = if (byteCount <= 4) entry.inlineBytes.copyOf(byteCount.toInt())
        else {
            if (entry.valueOffset + byteCount > fileSize) return emptyList()
            readAt(channel, entry.valueOffset, byteCount.toInt())
        }
        val b = ByteBuffer.wrap(bytes).order(order)
        return List(entry.count.toInt()) { u32(b.int) }
    }

    private fun decodeValues(bytes: ByteArray, order: ByteOrder, type: Int, count: Int): JSONArray {
        val out = JSONArray()
        val b = ByteBuffer.wrap(bytes).order(order)
        repeat(count) {
            when (type) {
                1 -> out.put(b.get().toInt() and 0xff)
                3 -> out.put(u16(b.short))
                4 -> out.put(u32(b.int))
                5 -> {
                    val num = u32(b.int)
                    val den = u32(b.int)
                    out.put(JSONObject().put("numerator", num).put("denominator", den)
                        .put("value", if (den != 0L) num.toDouble() / den.toDouble() else JSONObject.NULL))
                }
                6 -> out.put(b.get().toInt())
                8 -> out.put(b.short.toInt())
                9 -> out.put(b.int.toLong())
                10 -> {
                    val num = b.int.toLong()
                    val den = b.int.toLong()
                    out.put(JSONObject().put("numerator", num).put("denominator", den)
                        .put("value", if (den != 0L) num.toDouble() / den.toDouble() else JSONObject.NULL))
                }
                11 -> out.put(b.float.toDouble())
                12 -> out.put(b.double)
                7 -> out.put(b.get().toInt() and 0xff)
                else -> return out
            }
        }
        return out
    }

    private fun summarizeNumericArray(values: JSONArray): JSONObject {
        var min: Long? = null
        var max: Long? = null
        var sum = 0L
        var sumOverflow = false
        val first = JSONArray()
        val last = JSONArray()
        val n = values.length()

        for (i in 0 until n) {
            val v = (values.opt(i) as? Number)?.toLong() ?: continue
            min = min?.let { kotlin.math.min(it, v) } ?: v
            max = max?.let { kotlin.math.max(it, v) } ?: v
            if (!sumOverflow) {
                val next = sum + v
                if ((v > 0 && next < sum) || (v < 0 && next > sum)) sumOverflow = true else sum = next
            }
            if (i < 8) first.put(v)
            if (i >= n - 8) last.put(v)
        }

        return JSONObject()
            .put("count", n)
            .put("min", min ?: JSONObject.NULL)
            .put("max", max ?: JSONObject.NULL)
            .put("sum", if (sumOverflow) JSONObject.NULL else sum)
            .put("sumOverflow", sumOverflow)
            .put("first", first)
            .put("last", last)
    }

    private fun typeSize(type: Int): Int = when (type) {
        1, 2, 6, 7 -> 1
        3, 8 -> 2
        4, 9, 11 -> 4
        5, 10, 12 -> 8
        else -> -1
    }

    private fun safeMul(a: Long, b: Long): Long {
        if (a < 0 || b < 0) return -1
        if (a != 0L && b > Long.MAX_VALUE / a) return -1
        return a * b
    }

    private fun readAt(channel: FileChannel, offset: Long, length: Int): ByteArray {
        require(offset >= 0L)
        require(length >= 0)
        val buffer = ByteBuffer.allocate(length)
        channel.position(offset)
        while (buffer.hasRemaining()) {
            val n = channel.read(buffer)
            if (n < 0) break
        }
        return buffer.array().copyOf(buffer.position())
    }

    private fun u16(v: Short): Int = v.toInt() and 0xffff
    private fun u32(v: Int): Long = v.toLong() and 0xffffffffL
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.security.MessageDigest

/**
 * Read-only Stage 3.6 audit of an already sealed RAW_SENSOR file.
 *
 * It never reads from Image/HardwareBuffer and never writes the source file. All whole-raster,
 * row and band evidence is evaluated before any demosaic/scene interpretation. No zero-line,
 * TruthRange or Scientific Master semantics are used here.
 */
object RawSensorRasterAudit {
    private const val BAND_COUNT = 16

    fun audit(file: File, width: Int, height: Int, pixelBytes: Int = 2): JSONObject {
        val rowBytes = width.toLong() * pixelBytes.toLong()
        val expectedBytes = rowBytes * height.toLong()
        val rowsPerBand = if (height % BAND_COUNT == 0) height / BAND_COUNT else 0
        val report = JSONObject()
            .put("schema", "truthraw.rawsensor-raster-write-audit.v0.1")
            .put("readOnly", true)
            .put("sourceModified", false)
            .put("sourceFile", file.name)
            .put("sourceBytes", file.length())
            .put("width", width)
            .put("height", height)
            .put("pixelBytes", pixelBytes)
            .put("rowBytes", rowBytes)
            .put("expectedBytes", expectedBytes)
            .put("bandCount", BAND_COUNT)
            .put("rowsPerBand", rowsPerBand)
            .put("integerDomainAudit", true)
            .put("demosaicPerformed", false)
            .put("zeroLineUsed", false)
            .put("truthRangeUsed", false)
            .put("sceneLightClaimMade", false)

        if (pixelBytes != 2 || rowBytes > Int.MAX_VALUE || file.length() != expectedBytes || rowsPerBand <= 0) {
            return report
                .put("status", "BLOCKED_NONCANONICAL_SOURCE")
                .put("canonicalSizePass", file.length() == expectedBytes)
        }

        val row = ByteArray(rowBytes.toInt())
        val bandStates = Array(BAND_COUNT) { BandAccumulator(it, rowsPerBand) }
        val whole = ExactAccumulator()
        var firstNonZeroRow: Int? = null
        var lastNonZeroRow: Int? = null
        var allZeroRows = 0
        var nonZeroRows = 0
        var firstNonZeroByteOffset: Long? = null
        var lastNonZeroByteOffset: Long? = null
        var firstNonZeroSampleIndex: Long? = null
        var lastNonZeroSampleIndex: Long? = null
        var repeatedAdjacentRows = 0
        var previousRowHash: String? = null
        val uniqueRowHashes = HashSet<String>(height * 2)
        val periods = intArrayOf(1, 2, 4, 8, 16, 768, 3072, 6144)
        val periodHashes = periods.associateWith { arrayOfNulls<String>(it) }
        val periodMatches = periods.associateWith { 0 }.toMutableMap()

        FileInputStream(file).use { input ->
            for (y in 0 until height) {
                var off = 0
                while (off < row.size) {
                    val n = input.read(row, off, row.size - off)
                    if (n < 0) error("Short read at row=$y offset=$off")
                    off += n
                }

                val rowHash = MessageDigest.getInstance("SHA-256").digest(row).toHex()
                uniqueRowHashes += rowHash
                if (previousRowHash == rowHash) repeatedAdjacentRows++
                previousRowHash = rowHash
                periods.forEach { p ->
                    val slot = y % p
                    val cache = periodHashes.getValue(p)
                    if (y >= p && cache[slot] == rowHash) {
                        periodMatches[p] = periodMatches.getValue(p) + 1
                    }
                    cache[slot] = rowHash
                }

                val band = bandStates[y / rowsPerBand]
                band.updateHash(row)
                var rowNonZero = false
                var byteX = 0
                while (byteX < row.size) {
                    val lo = row[byteX].toInt() and 0xff
                    val hi = row[byteX + 1].toInt() and 0xff
                    val value = lo or (hi shl 8)
                    whole.add(value)
                    band.add(value)
                    if (value != 0) {
                        rowNonZero = true
                        val sampleIndex = y.toLong() * width.toLong() + (byteX / 2).toLong()
                        val byteOffset = sampleIndex * 2L
                        if (firstNonZeroRow == null) firstNonZeroRow = y
                        lastNonZeroRow = y
                        if (firstNonZeroSampleIndex == null) firstNonZeroSampleIndex = sampleIndex
                        lastNonZeroSampleIndex = sampleIndex
                        if (firstNonZeroByteOffset == null) firstNonZeroByteOffset = byteOffset
                        lastNonZeroByteOffset = byteOffset
                    }
                    byteX += 2
                }
                if (rowNonZero) nonZeroRows++ else allZeroRows++
            }
            if (input.read() != -1) error("Trailing bytes beyond canonical raster")
        }

        bandStates.forEach { it.finishHash() }
        val zeroBands = bandStates.filter { it.nonZeroCount == 0L }.map { it.index }
        val nonZeroBands = bandStates.filter { it.nonZeroCount > 0L }.map { it.index }
        val uniqueBandHashes = bandStates.map { it.hashHex ?: "" }.toSet().size
        val exact4080x3072Bytes = 4080L * 3072L * 2L
        val firstBandBytes = rowBytes * rowsPerBand.toLong()

        val periodJson = JSONObject()
        periods.forEach { p -> periodJson.put(p.toString(), periodMatches.getValue(p)) }

        val classification = when {
            nonZeroRows == 0 -> "ENTIRE_RASTER_ZERO"
            nonZeroRows == rowsPerBand && firstNonZeroRow == 0 && lastNonZeroRow == rowsPerBand - 1 ->
                "ONLY_FIRST_768_ROWS_NONZERO__EXACT_12P5MP_BYTE_PAYLOAD_SIGNATURE"
            zeroBands.isNotEmpty() -> "WHOLE_ZERO_BANDS_PRESENT"
            uniqueBandHashes < BAND_COUNT -> "EXACT_REPEATED_768_ROW_BANDS_PRESENT"
            uniqueRowHashes.size < height -> "EXACT_REPEATED_ROWS_PRESENT"
            else -> "FULL_RASTER_POPULATED_BY_CURRENT_ZERO_REPEAT_TESTS__INDEPENDENCE_UNPROVEN"
        }

        val bands = JSONArray()
        bandStates.forEach { bands.put(it.toJson()) }
        return report
            .put("status", "PASS_READ_ONLY_AUDIT_COMPLETED")
            .put("canonicalSizePass", true)
            .put("littleEndianU16InterpretationUsedForStatistics", true)
            .put("wholeRaster", whole.toJson())
            .put("rowPopulation", JSONObject()
                .put("totalRows", height)
                .put("nonZeroRows", nonZeroRows)
                .put("allZeroRows", allZeroRows)
                .put("firstNonZeroRow", firstNonZeroRow ?: JSONObject.NULL)
                .put("lastNonZeroRow", lastNonZeroRow ?: JSONObject.NULL)
                .put("uniqueRowSha256Count", uniqueRowHashes.size)
                .put("repeatedAdjacentRows", repeatedAdjacentRows)
                .put("exactPeriodRowHashMatches", periodJson))
            .put("samplePopulation", JSONObject()
                .put("firstNonZeroSampleIndex", firstNonZeroSampleIndex ?: JSONObject.NULL)
                .put("lastNonZeroSampleIndex", lastNonZeroSampleIndex ?: JSONObject.NULL)
                .put("firstNonZeroByteOffset", firstNonZeroByteOffset ?: JSONObject.NULL)
                .put("lastNonZeroByteOffset", lastNonZeroByteOffset ?: JSONObject.NULL))
            .put("bands", bands)
            .put("zeroBandIndices", JSONArray(zeroBands))
            .put("nonZeroBandIndices", JSONArray(nonZeroBands))
            .put("uniqueBandSha256Count", uniqueBandHashes)
            .put("geometrySignatures", JSONObject()
                .put("firstBandBytes", firstBandBytes)
                .put("raw4080x3072x16Bytes", exact4080x3072Bytes)
                .put("firstBandEquals4080x3072Raw16ByteCount", firstBandBytes == exact4080x3072Bytes)
                .put("rowsPerBandEquals768", rowsPerBand == 768)
                .put("declaredTo4080LinearRatioX", width.toDouble() / 4080.0)
                .put("declaredTo3072LinearRatioY", height.toDouble() / 3072.0))
            .put("classification", classification)
            .put("interpretationBoundary", "RASTER_POPULATION_AND_EXACT_REPETITION_ONLY__NO_OPTICAL_INDEPENDENCE_OR_SENSOR_CIRCUIT_CLAIM")
    }

    private class ExactAccumulator {
        var count = 0L
        var zeroCount = 0L
        var nonZeroCount = 0L
        var min = 65535
        var max = 0
        var sum = 0L
        var sumSquares = 0L

        fun add(v: Int) {
            count++
            if (v == 0) zeroCount++ else nonZeroCount++
            if (v < min) min = v
            if (v > max) max = v
            sum += v.toLong()
            sumSquares += v.toLong() * v.toLong()
        }

        fun toJson() = JSONObject()
            .put("sampleCount", count)
            .put("zeroCount", zeroCount)
            .put("nonZeroCount", nonZeroCount)
            .put("min", if (count == 0L) JSONObject.NULL else min)
            .put("max", if (count == 0L) JSONObject.NULL else max)
            .put("sumExactInt64", sum)
            .put("sumSquaresExactInt64", sumSquares)
    }

    private class BandAccumulator(val index: Int, private val rowsPerBand: Int) {
        private val exact = ExactAccumulator()
        private val digest = MessageDigest.getInstance("SHA-256")
        var hashHex: String? = null
            private set
        val nonZeroCount: Long get() = exact.nonZeroCount

        fun updateHash(bytes: ByteArray) = digest.update(bytes)
        fun add(v: Int) = exact.add(v)
        fun finishHash() { if (hashHex == null) hashHex = digest.digest().toHex() }

        fun toJson() = JSONObject()
            .put("bandIndex", index)
            .put("rowStart", index * rowsPerBand)
            .put("rowEndExclusive", (index + 1) * rowsPerBand)
            .put("sha256", hashHex ?: JSONObject.NULL)
            .put("sampleCount", exact.count)
            .put("zeroCount", exact.zeroCount)
            .put("nonZeroCount", exact.nonZeroCount)
            .put("min", if (exact.count == 0L) JSONObject.NULL else exact.min)
            .put("max", if (exact.count == 0L) JSONObject.NULL else exact.max)
            .put("sumExactInt64", exact.sum)
            .put("sumSquaresExactInt64", exact.sumSquares)
    }

    private fun ByteArray.toHex(): String = joinToString("") { "%02x".format(it.toInt() and 0xff) }
}

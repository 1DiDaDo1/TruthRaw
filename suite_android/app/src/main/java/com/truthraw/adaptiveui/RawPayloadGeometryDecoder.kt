package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.util.Size
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Stage 3.7: read-only interpretation probe for an already sealed RAW_SENSOR file.
 *
 * The source file is never opened for writing.  If Stage 3.6 proves that only a contiguous
 * prefix is populated, this decoder asks whether that prefix byte count exactly matches one of
 * Camera-5's advertised STANDARD RAW_SENSOR geometries.  A matching prefix may be copied byte for
 * byte into a smaller diagnostic .rawpayload file and rendered into an appearance-only grayscale
 * PNG.  Neither output replaces the sealed source and neither is promoted to sensor truth.
 */
object RawPayloadGeometryDecoder {
    private const val PIXEL_BYTES = 2

    fun decode(
        source: File,
        rasterAudit: JSONObject,
        declaredWidth: Int,
        declaredHeight: Int,
        advertisedStandardRawSizes: List<Size>,
        candidatePayloadFile: File,
        diagnosticPreviewFile: File,
    ): JSONObject {
        val samplePopulation = rasterAudit.optJSONObject("samplePopulation")
            ?: error("Stage 3.6 samplePopulation missing")
        val wholeRaster = rasterAudit.optJSONObject("wholeRaster")
            ?: error("Stage 3.6 wholeRaster missing")

        val firstNonZeroByteOffset = samplePopulation.optLong("firstNonZeroByteOffset", -1L)
        val lastNonZeroByteOffset = samplePopulation.optLong("lastNonZeroByteOffset", -1L)
        require(firstNonZeroByteOffset >= 0L && lastNonZeroByteOffset >= firstNonZeroByteOffset) {
            "No bounded non-zero payload prefix available"
        }
        val payloadBytes = lastNonZeroByteOffset + PIXEL_BYTES
        require(payloadBytes > 0L && payloadBytes <= source.length()) { "payloadBytes=$payloadBytes invalid" }
        require(payloadBytes % PIXEL_BYTES == 0L) { "payloadBytes not U16 aligned" }
        val payloadSamples = payloadBytes / PIXEL_BYTES
        val nonZeroCount = wholeRaster.optLong("nonZeroCount", -1L)
        val contiguousPrefixAllSamplesNonZero =
            firstNonZeroByteOffset == 0L && nonZeroCount >= 0L && nonZeroCount * PIXEL_BYTES == payloadBytes

        val advertised = JSONArray()
        advertisedStandardRawSizes.sortedBy { it.width.toLong() * it.height.toLong() }.forEach { size ->
            advertised.put(JSONObject()
                .put("width", size.width)
                .put("height", size.height)
                .put("bytesU16", size.width.toLong() * size.height.toLong() * PIXEL_BYTES)
                .put("payloadByteExactMatch", size.width.toLong() * size.height.toLong() * PIXEL_BYTES == payloadBytes))
        }
        val exactMatches = advertisedStandardRawSizes.filter {
            it.width.toLong() * it.height.toLong() * PIXEL_BYTES == payloadBytes
        }.distinctBy { "${it.width}x${it.height}" }

        val factorCandidates = JSONArray()
        val widths = linkedSetOf<Int>()
        widths += declaredWidth
        if (declaredWidth % 2 == 0) widths += declaredWidth / 2
        if (declaredWidth % 4 == 0) widths += declaredWidth / 4
        if (declaredWidth % 8 == 0) widths += declaredWidth / 8
        advertisedStandardRawSizes.forEach { widths += it.width }
        widths.filter { it > 0 && payloadSamples % it.toLong() == 0L }.sortedDescending().forEach { w ->
            val h = (payloadSamples / w.toLong()).toInt()
            factorCandidates.put(JSONObject()
                .put("width", w)
                .put("height", h)
                .put("aspectRatio", w.toDouble() / h.toDouble())
                .put("isDeclaredPrefixLayout", w == declaredWidth)
                .put("isAdvertisedStandardRaw", advertisedStandardRawSizes.any { it.width == w && it.height == h }))
        }

        val report = JSONObject()
            .put("schema", "truthraw.raw-payload-geometry-decoder.v0.1")
            .put("readOnlySource", true)
            .put("sourceModified", false)
            .put("sourceFile", source.name)
            .put("sourceBytes", source.length())
            .put("declaredWidth", declaredWidth)
            .put("declaredHeight", declaredHeight)
            .put("payloadFirstNonZeroByteOffset", firstNonZeroByteOffset)
            .put("payloadLastNonZeroByteOffset", lastNonZeroByteOffset)
            .put("payloadBytes", payloadBytes)
            .put("payloadSamplesU16", payloadSamples)
            .put("contiguousPrefixAllSamplesNonZero", contiguousPrefixAllSamplesNonZero)
            .put("advertisedStandardRawSizes", advertised)
            .put("factorCompatibleCandidateGeometries", factorCandidates)
            .put("advertisedStandardRawExactMatchCount", exactMatches.size)
            .put("sourceAuthorityUnchanged", true)
            .put("scientificMasterModified", false)
            .put("geometrySemanticPromotionAllowed", false)

        if (exactMatches.size != 1) {
            return report
                .put("status", if (exactMatches.isEmpty()) "NO_UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH" else "AMBIGUOUS_ADVERTISED_STANDARD_RAW_BYTE_MATCH")
                .put("classification", "PAYLOAD_GEOMETRY_NOT_UNIQUELY_RESOLVED")
        }

        val selected = exactMatches.single()
        val selectedBytes = selected.width.toLong() * selected.height.toLong() * PIXEL_BYTES
        require(selectedBytes == payloadBytes)

        val prefixCopy = copyExactPrefix(source, candidatePayloadFile, payloadBytes)
        val metrics = analyzeCandidate(source, selected.width, selected.height, payloadBytes)
        val preview = createDiagnosticPreview(
            source = source,
            width = selected.width,
            height = selected.height,
            minCode = metrics.getInt("minCode"),
            maxCode = metrics.getInt("maxCode"),
            output = diagnosticPreviewFile,
        )

        val firstBandHash = rasterAudit.optJSONArray("bands")
            ?.optJSONObject(0)?.optString("sha256", null)
        val prefixHashMatchesFirstBand = firstBandHash != null && firstBandHash.equals(prefixCopy.sha256, ignoreCase = true)

        return report
            .put("status", "UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED")
            .put("selectedCandidate", JSONObject()
                .put("width", selected.width)
                .put("height", selected.height)
                .put("bytesU16", selectedBytes)
                .put("selectionBasis", "EXACT_PAYLOAD_BYTE_COUNT_MATCH_TO_SINGLE_ADVERTISED_STANDARD_RAW_SENSOR_SIZE")
                .put("interpretationOnly", true))
            .put("candidatePayload", JSONObject()
                .put("file", candidatePayloadFile.name)
                .put("bytes", prefixCopy.bytes)
                .put("sha256", prefixCopy.sha256)
                .put("sourceByteStart", 0)
                .put("sourceByteEndExclusive", payloadBytes)
                .put("copyTransform", "NONE_EXACT_BYTE_PREFIX_COPY")
                .put("firstBandSha256", firstBandHash ?: JSONObject.NULL)
                .put("sha256MatchesStage36FirstBand", prefixHashMatchesFirstBand)
                .put("sourceReplacement", false))
            .put("candidateMetrics", metrics)
            .put("diagnosticPreview", preview)
            .put("classification", "EXACT_PREFIX_BYTES_MATCH_ONE_ADVERTISED_STANDARD_RAW_GEOMETRY__GEOMETRY_INTERPRETATION_CANDIDATE_NOT_SENSOR_PROOF")
            .put("interpretationBoundary", "BYTE_COUNT_PLUS_DEVICE_ADVERTISEMENT_PLUS_SPATIAL_DIAGNOSTICS_ONLY__NO_PROOF_OF_NATIVE_ADC_GEOMETRY_OR_OPTICAL_RESOLUTION")
    }

    private data class CopyResult(val bytes: Long, val sha256: String)

    private fun copyExactPrefix(source: File, output: File, bytes: Long): CopyResult {
        val md = MessageDigest.getInstance("SHA-256")
        var remaining = bytes
        var copied = 0L
        FileInputStream(source).use { input ->
            FileOutputStream(output, false).use { out ->
                val buffer = ByteArray(1024 * 1024)
                while (remaining > 0L) {
                    val want = min(buffer.size.toLong(), remaining).toInt()
                    val n = input.read(buffer, 0, want)
                    if (n < 0) error("Short source while copying payload prefix at $copied/$bytes")
                    out.write(buffer, 0, n)
                    md.update(buffer, 0, n)
                    copied += n
                    remaining -= n
                }
                out.fd.sync()
            }
        }
        require(copied == bytes && output.length() == bytes) { "prefix copy length mismatch" }
        return CopyResult(copied, md.digest().toHex())
    }

    private fun analyzeCandidate(source: File, width: Int, height: Int, payloadBytes: Long): JSONObject {
        require(width.toLong() * height.toLong() * PIXEL_BYTES == payloadBytes)
        val rowBytes = width * PIXEL_BYTES
        val row = ByteArray(rowBytes)
        val phaseCount = LongArray(4)
        val phaseSum = LongArray(4)
        val phaseSumSq = LongArray(4)
        val h1 = PairAccumulator()
        val h2 = PairAccumulator()
        val v1 = PairAccumulator()
        val v2 = PairAccumulator()
        var minCode = 65535
        var maxCode = 0
        var sum = 0L
        var sumSq = 0L
        var count = 0L
        var prev1: IntArray? = null
        var prev2: IntArray? = null

        FileInputStream(source).use { input ->
            for (y in 0 until height) {
                readFully(input, row)
                val current = IntArray(width)
                var b = 0
                for (x in 0 until width) {
                    val v = (row[b].toInt() and 0xff) or ((row[b + 1].toInt() and 0xff) shl 8)
                    current[x] = v
                    count++
                    sum += v.toLong()
                    sumSq += v.toLong() * v.toLong()
                    minCode = min(minCode, v)
                    maxCode = max(maxCode, v)
                    val phase = ((y and 1) shl 1) or (x and 1)
                    phaseCount[phase]++
                    phaseSum[phase] += v.toLong()
                    phaseSumSq[phase] += v.toLong() * v.toLong()
                    b += 2
                }
                // Sample pair statistics sparsely; source codes themselves remain untouched/exact.
                var x = 0
                while (x < width) {
                    if (x + 1 < width) h1.add(current[x], current[x + 1])
                    if (x + 2 < width) h2.add(current[x], current[x + 2])
                    prev1?.let { v1.add(it[x], current[x]) }
                    prev2?.let { v2.add(it[x], current[x]) }
                    x += 8
                }
                prev2 = prev1
                prev1 = current
            }
        }

        val phaseJson = JSONArray()
        for (p in 0..3) {
            val n = phaseCount[p]
            val mean = if (n > 0) phaseSum[p].toDouble() / n.toDouble() else Double.NaN
            val variance = if (n > 0) phaseSumSq[p].toDouble() / n.toDouble() - mean * mean else Double.NaN
            phaseJson.put(JSONObject()
                .put("phase", p)
                .put("yParity", p shr 1)
                .put("xParity", p and 1)
                .put("count", n)
                .put("sumExactInt64", phaseSum[p])
                .put("sumSquaresExactInt64", phaseSumSq[p])
                .put("meanF64", mean)
                .put("variancePopulationF64", max(0.0, variance)))
        }

        return JSONObject()
            .put("width", width)
            .put("height", height)
            .put("rowBytes", rowBytes)
            .put("sampleCount", count)
            .put("minCode", minCode)
            .put("maxCode", maxCode)
            .put("sumExactInt64", sum)
            .put("sumSquaresExactInt64", sumSq)
            .put("phase2x2", phaseJson)
            .put("sampledPairStatistics", JSONObject()
                .put("horizontalDx1", h1.toJson())
                .put("horizontalDx2", h2.toJson())
                .put("verticalDy1", v1.toJson())
                .put("verticalDy2", v2.toJson())
                .put("samplingXStep", 8))
            .put("floatUseBoundary", "F64_ONLY_FOR_DERIVED_CORRELATION_AND_MOMENTS__SOURCE_REMAINS_U16_EXACT")
    }

    private fun createDiagnosticPreview(
        source: File,
        width: Int,
        height: Int,
        minCode: Int,
        maxCode: Int,
        output: File,
    ): JSONObject {
        val block = 4
        val previewW = width / block
        val previewH = height / block
        require(previewW > 0 && previewH > 0 && width % block == 0 && height % block == 0)
        val pixels = IntArray(previewW * previewH)
        val rows = Array(block) { ByteArray(width * PIXEL_BYTES) }
        val range = max(1, maxCode - minCode)

        FileInputStream(source).use { input ->
            for (py in 0 until previewH) {
                for (ry in 0 until block) readFully(input, rows[ry])
                for (px in 0 until previewW) {
                    var blockSum = 0L
                    for (ry in 0 until block) {
                        val row = rows[ry]
                        for (rx in 0 until block) {
                            val x = px * block + rx
                            val bi = x * 2
                            val v = (row[bi].toInt() and 0xff) or ((row[bi + 1].toInt() and 0xff) shl 8)
                            blockSum += v.toLong()
                        }
                    }
                    val avg = blockSum.toDouble() / 16.0
                    val linear = ((avg - minCode.toDouble()) / range.toDouble()).coerceIn(0.0, 1.0)
                    val display = sqrt(linear)
                    val g = (display * 255.0 + 0.5).toInt().coerceIn(0, 255)
                    pixels[py * previewW + px] = (0xff shl 24) or (g shl 16) or (g shl 8) or g
                }
            }
        }
        val bitmap = Bitmap.createBitmap(pixels, previewW, previewH, Bitmap.Config.ARGB_8888)
        FileOutputStream(output, false).use { out ->
            check(bitmap.compress(Bitmap.CompressFormat.PNG, 100, out)) { "PNG compression failed" }
            out.fd.sync()
        }
        bitmap.recycle()
        val sha = sha256File(output)
        return JSONObject()
            .put("file", output.name)
            .put("bytes", output.length())
            .put("sha256", sha)
            .put("width", previewW)
            .put("height", previewH)
            .put("sourceGeometryWidth", width)
            .put("sourceGeometryHeight", height)
            .put("blockAverage", "4x4_U16_MEAN")
            .put("displayTransform", "MINMAX_NORMALIZE_THEN_SQRT_GAMMA")
            .put("appearanceOnly", true)
            .put("scientificEvidence", false)
    }

    private class PairAccumulator {
        private var n = 0L
        private var sx = 0L
        private var sy = 0L
        private var sxx = 0L
        private var syy = 0L
        private var sxy = 0L
        private var absDiff = 0L

        fun add(x: Int, y: Int) {
            n++
            sx += x.toLong()
            sy += y.toLong()
            sxx += x.toLong() * x.toLong()
            syy += y.toLong() * y.toLong()
            sxy += x.toLong() * y.toLong()
            absDiff += kotlin.math.abs(x - y).toLong()
        }

        fun toJson(): JSONObject {
            val numerator = n.toDouble() * sxy.toDouble() - sx.toDouble() * sy.toDouble()
            val dx = n.toDouble() * sxx.toDouble() - sx.toDouble() * sx.toDouble()
            val dy = n.toDouble() * syy.toDouble() - sy.toDouble() * sy.toDouble()
            val corr = if (n > 1 && dx > 0.0 && dy > 0.0) numerator / sqrt(dx * dy) else Double.NaN
            return JSONObject()
                .put("pairCount", n)
                .put("meanAbsDiffF64", if (n > 0) absDiff.toDouble() / n.toDouble() else Double.NaN)
                .put("pearsonCorrelationF64", corr)
        }
    }

    private fun readFully(input: FileInputStream, buffer: ByteArray) {
        var off = 0
        while (off < buffer.size) {
            val n = input.read(buffer, off, buffer.size - off)
            if (n < 0) error("Short read at $off/${buffer.size}")
            off += n
        }
    }

    private fun sha256File(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                md.update(buffer, 0, n)
            }
        }
        return md.digest().toHex()
    }

    private fun ByteArray.toHex(): String = joinToString("") { "%02x".format(it.toInt() and 0xff) }
}

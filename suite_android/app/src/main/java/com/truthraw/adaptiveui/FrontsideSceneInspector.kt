package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.ImageDecoder
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.nio.ByteBuffer
import java.nio.channels.FileChannel
import java.security.MessageDigest
import kotlin.math.atan2
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Read-only inspection of the visible "front side" of one selected source.
 *
 * This lives at Universal Intake beside source/container profiling. It is not a
 * replacement for source evidence: every result is APPEARANCE_DERIVED_ONLY and
 * may guide scene understanding/routing without scientific writeback.
 */
object FrontsideSceneInspector {
    private const val MAX_ANALYSIS_EDGE = 384
    private const val MAX_EMBEDDED_PREVIEW_BYTES = 32 * 1024 * 1024

    fun inspect(
        resolver: ContentResolver,
        uri: Uri,
        containerMetadata: JSONObject?,
        sourceSha256: String,
    ): JSONObject {
        val decoded = decodeFrontside(resolver, uri, containerMetadata)

        if (decoded == null) {
            return JSONObject()
                .put("schema", "D.RAW/FrontsideSceneInspection/0.2")
                .put("status", "FRONTSIDE_PREVIEW_UNAVAILABLE")
                .put("source_sha256", sourceSha256)
                .put("authority", "APPEARANCE_DERIVED_ONLY")
                .put("decoded_preview_used", false)
                .put("natural_feature_geometry_candidate", false)
                .put(
                    "semantic_scene_understanding",
                    JSONObject()
                        .put("level", "FRONTSIDE_UNAVAILABLE")
                        .put("future_classical_vision_extension_allowed", true),
                )
                .put("creates_sensor_evidence", false)
                .put("scientific_writeback_allowed", false)
        }

        val bitmap = scaleForAnalysis(decoded.bitmap)
        val width = bitmap.width
        val height = bitmap.height
        val n = width * height

        val luma = IntArray(n)
        val histogram = IntArray(32)
        var sum = 0.0
        var sum2 = 0.0

        var idx = 0
        for (y in 0 until height) {
            for (x in 0 until width) {
                val c = bitmap.getPixel(x, y)
                val yy = (
                    77 * Color.red(c) +
                        150 * Color.green(c) +
                        29 * Color.blue(c)
                    ) shr 8
                luma[idx++] = yy
                histogram[min(31, yy / 8)]++
                sum += yy
                sum2 += yy.toDouble() * yy.toDouble()
            }
        }

        val mean = if (n > 0) sum / n else 0.0
        val variance = if (n > 0) max(0.0, sum2 / n - mean * mean) else 0.0
        val stddev = sqrt(variance)

        var entropy = 0.0
        if (n > 0) {
            for (count in histogram) {
                if (count <= 0) continue
                val p = count.toDouble() / n.toDouble()
                entropy -= p * (ln(p) / ln(2.0))
            }
        }

        val orientationBins = LongArray(8)
        var edgeCount = 0L
        var gradSum = 0.0
        var minEdgeX = width
        var minEdgeY = height
        var maxEdgeX = -1
        var maxEdgeY = -1
        var edgeXWeighted = 0.0
        var edgeYWeighted = 0.0
        var edgeWeightTotal = 0.0
        val edgeThreshold = max(18.0, stddev * 0.55)

        if (width >= 3 && height >= 3) {
            for (y in 1 until height - 1) {
                for (x in 1 until width - 1) {
                    val center = y * width + x
                    val gx = (luma[center + 1] - luma[center - 1]).toDouble()
                    val gy = (luma[center + width] - luma[center - width]).toDouble()
                    val mag = sqrt(gx * gx + gy * gy)
                    gradSum += mag

                    if (mag >= edgeThreshold) {
                        edgeCount++
                        minEdgeX = min(minEdgeX, x)
                        minEdgeY = min(minEdgeY, y)
                        maxEdgeX = max(maxEdgeX, x)
                        maxEdgeY = max(maxEdgeY, y)
                        edgeXWeighted += x * mag
                        edgeYWeighted += y * mag
                        edgeWeightTotal += mag

                        var angle = Math.toDegrees(atan2(gy, gx))
                        while (angle < 0.0) angle += 180.0
                        while (angle >= 180.0) angle -= 180.0
                        var bin = (angle / 22.5).toInt()
                        if (bin !in 0..7) bin = 7
                        orientationBins[bin]++
                    }
                }
            }
        }

        val interiorPixels = max(1, (width - 2) * (height - 2))
        val edgeDensity = edgeCount.toDouble() / interiorPixels.toDouble()
        val meanGradient = gradSum / interiorPixels.toDouble()
        val dominantBin = orientationBins.indices.maxByOrNull { orientationBins[it] } ?: 0
        val dominantAngleDeg = dominantBin * 22.5 + 11.25

        val bbox = if (edgeCount > 0 && maxEdgeX >= minEdgeX && maxEdgeY >= minEdgeY) {
            JSONObject()
                .put("left", minEdgeX.toDouble() / width.toDouble())
                .put("top", minEdgeY.toDouble() / height.toDouble())
                .put("right", (maxEdgeX + 1).toDouble() / width.toDouble())
                .put("bottom", (maxEdgeY + 1).toDouble() / height.toDouble())
                .put(
                    "width_fraction",
                    (maxEdgeX - minEdgeX + 1).toDouble() / width.toDouble(),
                )
                .put(
                    "height_fraction",
                    (maxEdgeY - minEdgeY + 1).toDouble() / height.toDouble(),
                )
        } else {
            JSONObject.NULL
        }

        val centroid = if (edgeWeightTotal > 0.0) {
            JSONObject()
                .put("x", edgeXWeighted / edgeWeightTotal / width.toDouble())
                .put("y", edgeYWeighted / edgeWeightTotal / height.toDouble())
        } else {
            JSONObject.NULL
        }

        val signature = structuralSignature(bitmap)
        val enoughStructure =
            width >= 96 &&
                height >= 96 &&
                edgeDensity >= 0.008 &&
                entropy >= 2.0

        val proportions = JSONObject()
            .put("pixel_width", width)
            .put("pixel_height", height)
            .put(
                "aspect_ratio_width_over_height",
                width.toDouble() / max(1, height).toDouble(),
            )
            .put("structural_content_bbox_normalized", bbox)
            .put("structural_centroid_normalized", centroid)

        val edgeOrientationJson = JSONArray()
        for (i in orientationBins.indices) {
            edgeOrientationJson.put(
                JSONObject()
                    .put("center_degrees", i * 22.5 + 11.25)
                    .put("count", orientationBins[i]),
            )
        }

        return JSONObject()
            .put("schema", "D.RAW/FrontsideSceneInspection/0.2")
            .put("status", "FRONTSIDE_STRUCTURAL_INSPECTION_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("decode_method", decoded.method)
            .put("decoded_preview_used", true)
            .put("analysis_width", width)
            .put("analysis_height", height)
            .put("proportions", proportions)
            .put(
                "appearance_statistics",
                JSONObject()
                    .put("mean_luma_0_255", mean)
                    .put("luma_stddev", stddev)
                    .put("luma_entropy_bits_32_bin", entropy)
                    .put("mean_gradient", meanGradient)
                    .put("edge_density", edgeDensity),
            )
            .put("dominant_edge_orientation_degrees", dominantAngleDeg)
            .put("edge_orientation_histogram", edgeOrientationJson)
            .put("structural_feature_signature_sha256", signature)
            .put(
                "geometry_readiness",
                JSONObject()
                    .put("natural_feature_geometry_candidate", enoughStructure)
                    .put(
                        "reason",
                        if (enoughStructure) {
                            "VISIBLE_STRUCTURE_SUFFICIENT_FOR_LATER_PAIR_MATCHING"
                        } else {
                            "VISIBLE_STRUCTURE_TOO_WEAK_OR_PREVIEW_TOO_SMALL"
                        },
                    )
                    .put("indexed_target_required", false)
                    .put("absolute_metric_scale_proven", false),
            )
            .put(
                "semantic_scene_understanding",
                JSONObject()
                    .put("level", "STRUCTURAL_VISION_V0_2")
                    .put(
                        "description",
                        "Frontside proportions, luminance structure, edges and orientation are inspected at intake. Higher-level deterministic object/material/geometry/scene analysis may be added only through inspectable non-AI algorithms.",
                    )
                    .put("future_classical_vision_extension_allowed", true),
            )
            .put("creates_sensor_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private data class Decoded(
        val bitmap: Bitmap,
        val method: String,
    )

    private fun decodeFrontside(
        resolver: ContentResolver,
        uri: Uri,
        containerMetadata: JSONObject?,
    ): Decoded? {
        try {
            val source = ImageDecoder.createSource(resolver, uri)
            val bmp = ImageDecoder.decodeBitmap(source) { decoder, info, _ ->
                decoder.allocator = ImageDecoder.ALLOCATOR_SOFTWARE
                val size = info.size
                val scale = min(
                    1.0,
                    MAX_ANALYSIS_EDGE.toDouble() /
                        max(size.width, size.height).toDouble(),
                )
                val targetW = max(1, (size.width * scale).toInt())
                val targetH = max(1, (size.height * scale).toInt())
                decoder.setTargetSize(targetW, targetH)
            }
            return Decoded(bmp, "ANDROID_IMAGE_DECODER_URI")
        } catch (_: Throwable) {
        }

        val embedded = findEmbeddedJpeg(resolver, uri, containerMetadata)
        if (embedded != null) {
            val bmp = BitmapFactory.decodeByteArray(
                embedded.first,
                0,
                embedded.first.size,
            )
            if (bmp != null) {
                return Decoded(bmp, embedded.second)
            }
        }

        try {
            resolver.openInputStream(uri)?.use { input ->
                val bmp = BitmapFactory.decodeStream(input)
                if (bmp != null) return Decoded(bmp, "ANDROID_BITMAP_FACTORY_URI")
            }
        } catch (_: Throwable) {
        }

        return null
    }

    private fun findEmbeddedJpeg(
        resolver: ContentResolver,
        uri: Uri,
        metadata: JSONObject?,
    ): Pair<ByteArray, String>? {
        val ifds = metadata?.optJSONArray("ifds") ?: return null
        val pfd = resolver.openFileDescriptor(uri, "r") ?: return null
        val statSize = pfd.statSize
        ParcelFileDescriptor.AutoCloseInputStream(pfd).use { input ->
            val channel = input.channel
            val fileSize = if (statSize >= 0L) statSize else channel.size()
            if (fileSize <= 0L) return null

            for (i in 0 until ifds.length()) {
                val ifd = ifds.optJSONObject(i) ?: continue

                val jpegOffset = scalarLong(findEntryValue(ifd, 513))
                val jpegLength = scalarLong(findEntryValue(ifd, 514))
                if (
                    jpegOffset != null &&
                    jpegLength != null &&
                    jpegLength in 16..MAX_EMBEDDED_PREVIEW_BYTES.toLong()
                ) {
                    val bytes = readRange(channel, fileSize, jpegOffset, jpegLength.toInt())
                    if (looksLikeJpeg(bytes)) {
                        return bytes to "DNG_JPEG_INTERCHANGE_PREVIEW"
                    }
                }

                val compression = scalarLong(ifd.opt("compression"))
                if (compression == 6L || compression == 7L) {
                    val stripOffset = singleArrayOrScalar(ifd.opt("stripOffsets"))
                    val stripBytes = singleArrayOrScalar(ifd.opt("stripByteCounts"))
                    if (
                        stripOffset != null &&
                        stripBytes != null &&
                        stripBytes in 16..MAX_EMBEDDED_PREVIEW_BYTES.toLong()
                    ) {
                        val bytes = readRange(channel, fileSize, stripOffset, stripBytes.toInt())
                        if (looksLikeJpeg(bytes)) {
                            return bytes to "DNG_JPEG_STRIP_PREVIEW"
                        }
                    }

                    val tileOffset = singleArrayOrScalar(ifd.opt("tileOffsets"))
                    val tileBytes = singleArrayOrScalar(ifd.opt("tileByteCounts"))
                    if (
                        tileOffset != null &&
                        tileBytes != null &&
                        tileBytes in 16..MAX_EMBEDDED_PREVIEW_BYTES.toLong()
                    ) {
                        val bytes = readRange(channel, fileSize, tileOffset, tileBytes.toInt())
                        if (looksLikeJpeg(bytes)) {
                            return bytes to "DNG_JPEG_TILE_PREVIEW"
                        }
                    }
                }
            }
        }
        return null
    }

    private fun findEntryValue(ifd: JSONObject, tag: Int): Any? {
        val entries = ifd.optJSONArray("entries") ?: return null
        for (i in 0 until entries.length()) {
            val entry = entries.optJSONObject(i) ?: continue
            if (entry.optInt("tag", -1) == tag) {
                val value = entry.opt("value")
                if (value != null && value !== JSONObject.NULL) return value
            }
        }
        return null
    }

    private fun singleArrayOrScalar(value: Any?): Long? = when (value) {
        is Number -> value.toLong()
        is JSONArray -> if (value.length() == 1) {
            (value.opt(0) as? Number)?.toLong()
        } else {
            null
        }
        else -> null
    }

    private fun scalarLong(value: Any?): Long? = when (value) {
        is Number -> value.toLong()
        is JSONArray -> if (value.length() == 1) {
            (value.opt(0) as? Number)?.toLong()
        } else {
            null
        }
        else -> null
    }

    private fun readRange(
        channel: FileChannel,
        fileSize: Long,
        offset: Long,
        length: Int,
    ): ByteArray {
        require(offset >= 0L)
        require(length >= 0)
        require(offset + length <= fileSize)
        val buffer = ByteBuffer.allocate(length)
        channel.position(offset)
        while (buffer.hasRemaining()) {
            val n = channel.read(buffer)
            if (n < 0) break
        }
        return buffer.array().copyOf(buffer.position())
    }

    private fun looksLikeJpeg(bytes: ByteArray): Boolean =
        bytes.size >= 4 &&
            (bytes[0].toInt() and 0xff) == 0xff &&
            (bytes[1].toInt() and 0xff) == 0xd8

    private fun scaleForAnalysis(bitmap: Bitmap): Bitmap {
        val maxEdge = max(bitmap.width, bitmap.height)
        if (maxEdge <= MAX_ANALYSIS_EDGE) return bitmap

        val scale = MAX_ANALYSIS_EDGE.toDouble() / maxEdge.toDouble()
        val w = max(1, (bitmap.width * scale).toInt())
        val h = max(1, (bitmap.height * scale).toInt())
        return Bitmap.createScaledBitmap(bitmap, w, h, true)
    }

    private fun structuralSignature(bitmap: Bitmap): String {
        val small = Bitmap.createScaledBitmap(bitmap, 32, 32, true)
        val digest = MessageDigest.getInstance("SHA-256")
        for (y in 0 until 32) {
            for (x in 0 until 32) {
                val c = small.getPixel(x, y)
                val yy = (
                    77 * Color.red(c) +
                        150 * Color.green(c) +
                        29 * Color.blue(c)
                    ) shr 8
                digest.update(yy.toByte())
            }
        }
        return digest.digest().joinToString("") {
            "%02x".format(it.toInt() and 0xff)
        }
    }
}

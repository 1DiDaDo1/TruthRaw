package com.truthraw.adaptiveui

import android.graphics.Bitmap
import android.graphics.Color
import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sin
import kotlin.math.sqrt

/**
 * Deterministic, classical, appearance-derived local feature extraction.
 *
 * This module is intentionally independent from camera/lens/vendor identity.
 * Features are derived from the decoded frontside preview and therefore carry
 * APPEARANCE_DERIVED_ONLY authority. They may support later geometric
 * hypotheses but can never create sensor evidence or scientific writeback.
 */
object DeterministicLocalFeatureGeometryV01 {
    const val SCHEMA = "D.RAW/DeterministicLocalFeatureGeometry/0.1"

    private const val MAX_KEYPOINTS = 96
    private const val PATCH_RADIUS = 10
    private const val MIN_KEYPOINT_DISTANCE_PX = 8
    private const val HARRIS_K = 0.04
    private const val DESCRIPTOR_BITS = 128

    // Secondary appearance-only support for constrained controlled-rotation
    // diagnostics. The primary keypoint set and generic pair geometry remain
    // unchanged.
    private const val ROTATION_SUPPORT_MAX_KEYPOINTS = 192
    private const val ROTATION_SUPPORT_MIN_KEYPOINT_DISTANCE_PX = 5
    private const val ROTATION_SUPPORT_RELATIVE_RESPONSE_FLOOR = 0.0025

    private data class Candidate(
        val x: Int,
        val y: Int,
        val response: Double,
        val gx: Double,
        val gy: Double,
    )

    fun extract(
        bitmap: Bitmap,
        sourceSha256: String,
    ): JSONObject {
        val width = bitmap.width
        val height = bitmap.height

        if (
            width < PATCH_RADIUS * 2 + 8 ||
            height < PATCH_RADIUS * 2 + 8
        ) {
            return unavailable(
                sourceSha256,
                "PREVIEW_TOO_SMALL_FOR_LOCAL_FEATURE_GEOMETRY",
            )
        }

        val n = width * height
        val luma = IntArray(n)
        var idx = 0
        for (y in 0 until height) {
            for (x in 0 until width) {
                val c = bitmap.getPixel(x, y)
                luma[idx++] =
                    (
                        77 * Color.red(c) +
                            150 * Color.green(c) +
                            29 * Color.blue(c)
                        ) shr 8
            }
        }

        val gx = DoubleArray(n)
        val gy = DoubleArray(n)
        for (y in 1 until height - 1) {
            for (x in 1 until width - 1) {
                val i = y * width + x
                gx[i] = (luma[i + 1] - luma[i - 1]).toDouble()
                gy[i] = (luma[i + width] - luma[i - width]).toDouble()
            }
        }

        val candidates = ArrayList<Candidate>()
        var maxPositiveResponse = 0.0
        for (
            y in PATCH_RADIUS until
                height - PATCH_RADIUS
        ) {
            for (
                x in PATCH_RADIUS until
                    width - PATCH_RADIUS
            ) {
                var sxx = 0.0
                var syy = 0.0
                var sxy = 0.0
                for (dy in -2..2) {
                    for (dx in -2..2) {
                        val i = (y + dy) * width + (x + dx)
                        val ix = gx[i]
                        val iy = gy[i]
                        sxx += ix * ix
                        syy += iy * iy
                        sxy += ix * iy
                    }
                }
                val det = sxx * syy - sxy * sxy
                val trace = sxx + syy
                val response = det - HARRIS_K * trace * trace
                if (response > 0.0 && response.isFinite()) {
                    maxPositiveResponse =
                        max(maxPositiveResponse, response)
                    candidates += Candidate(
                        x = x,
                        y = y,
                        response = response,
                        gx = gx[y * width + x],
                        gy = gy[y * width + x],
                    )
                }
            }
        }

        if (
            candidates.isEmpty() ||
            maxPositiveResponse <= 0.0
        ) {
            return unavailable(
                sourceSha256,
                "NO_POSITIVE_CLASSICAL_CORNER_RESPONSE",
            )
        }

        val threshold = maxPositiveResponse * 0.01
        val sorted =
            candidates
                .asSequence()
                .filter { it.response >= threshold }
                .sortedWith(
                    compareByDescending<Candidate> { it.response }
                        .thenBy { it.y }
                        .thenBy { it.x },
                )
                .toList()

        val accepted = ArrayList<Candidate>()
        val minDistance2 =
            MIN_KEYPOINT_DISTANCE_PX *
                MIN_KEYPOINT_DISTANCE_PX

        for (candidate in sorted) {
            val separated =
                accepted.none { prior ->
                    val dx = candidate.x - prior.x
                    val dy = candidate.y - prior.y
                    dx * dx + dy * dy < minDistance2
                }
            if (!separated) continue
            accepted += candidate
            if (accepted.size >= MAX_KEYPOINTS) break
        }

        if (accepted.size < 8) {
            return unavailable(
                sourceSha256,
                "INSUFFICIENT_SPATIALLY_SEPARATED_LOCAL_FEATURES",
                accepted.size,
            )
        }

        val rotationSupportSorted =
            candidates
                .asSequence()
                .filter {
                    it.response >=
                        maxPositiveResponse *
                        ROTATION_SUPPORT_RELATIVE_RESPONSE_FLOOR
                }
                .sortedWith(
                    compareByDescending<Candidate> { it.response }
                        .thenBy { it.y }
                        .thenBy { it.x },
                )
                .toList()
        val rotationSupportAccepted =
            ArrayList<Candidate>()
        val rotationSupportMinDistance2 =
            ROTATION_SUPPORT_MIN_KEYPOINT_DISTANCE_PX *
                ROTATION_SUPPORT_MIN_KEYPOINT_DISTANCE_PX

        for (candidate in rotationSupportSorted) {
            val separated =
                rotationSupportAccepted.none { prior ->
                    val dx = candidate.x - prior.x
                    val dy = candidate.y - prior.y
                    dx * dx + dy * dy <
                        rotationSupportMinDistance2
                }
            if (!separated) continue
            rotationSupportAccepted += candidate
            if (
                rotationSupportAccepted.size >=
                ROTATION_SUPPORT_MAX_KEYPOINTS
            ) {
                break
            }
        }

        val keypoints = JSONArray()
        for ((index, kp) in accepted.withIndex()) {
            val orientation =
                atan2(kp.gy, kp.gx)
            val descriptor =
                descriptorHex(
                    luma = luma,
                    width = width,
                    height = height,
                    x = kp.x,
                    y = kp.y,
                    orientationRadians = orientation,
                )
            keypoints.put(
                JSONObject()
                    .put("index", index)
                    .put(
                        "x_normalized",
                        kp.x.toDouble() /
                            max(1, width - 1).toDouble(),
                    )
                    .put(
                        "y_normalized",
                        kp.y.toDouble() /
                            max(1, height - 1).toDouble(),
                    )
                    .put("x_analysis_px", kp.x)
                    .put("y_analysis_px", kp.y)
                    .put(
                        "response_relative_to_max",
                        kp.response / maxPositiveResponse,
                    )
                    .put(
                        "orientation_degrees",
                        Math.toDegrees(orientation),
                    )
                    .put(
                        "descriptor_hex_128bit",
                        descriptor,
                    ),
            )
        }

        val rotationSupportKeypoints =
            JSONArray()
        for (
            (index, kp) in
            rotationSupportAccepted.withIndex()
        ) {
            val orientation =
                atan2(kp.gy, kp.gx)
            val descriptor =
                descriptorHex(
                    luma = luma,
                    width = width,
                    height = height,
                    x = kp.x,
                    y = kp.y,
                    orientationRadians = orientation,
                )
            rotationSupportKeypoints.put(
                JSONObject()
                    .put("index", index)
                    .put(
                        "x_normalized",
                        kp.x.toDouble() /
                            max(1, width - 1).toDouble(),
                    )
                    .put(
                        "y_normalized",
                        kp.y.toDouble() /
                            max(1, height - 1).toDouble(),
                    )
                    .put("x_analysis_px", kp.x)
                    .put("y_analysis_px", kp.y)
                    .put(
                        "response_relative_to_max",
                        kp.response /
                            maxPositiveResponse,
                    )
                    .put(
                        "orientation_degrees",
                        Math.toDegrees(orientation),
                    )
                    .put(
                        "descriptor_hex_128bit",
                        descriptor,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE",
            )
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("analysis_width", width)
            .put("analysis_height", height)
            .put("keypoint_count", keypoints.length())
            .put("max_keypoints", MAX_KEYPOINTS)
            .put(
                "detector",
                JSONObject()
                    .put("family", "HARRIS_STRUCTURE_TENSOR")
                    .put("window", "5x5")
                    .put("k", HARRIS_K)
                    .put(
                        "relative_response_floor",
                        0.01,
                    )
                    .put(
                        "minimum_keypoint_distance_px",
                        MIN_KEYPOINT_DISTANCE_PX,
                    )
                    .put("selection_order", "RESPONSE_DESC_Y_X"),
            )
            .put(
                "descriptor",
                JSONObject()
                    .put(
                        "family",
                        "ORIENTATION_NORMALIZED_BINARY_INTENSITY_PAIRS",
                    )
                    .put("bits", DESCRIPTOR_BITS)
                    .put("patch_radius_px", PATCH_RADIUS)
                    .put(
                        "pair_pattern",
                        "FIXED_INTEGER_FORMULA_V0_1",
                    ),
            )
            .put("keypoints", keypoints)
            .put(
                "rotation_support_keypoint_count",
                rotationSupportKeypoints.length(),
            )
            .put(
                "rotation_support_keypoints",
                rotationSupportKeypoints,
            )
            .put(
                "rotation_support_detector",
                JSONObject()
                    .put(
                        "purpose",
                        "CONTROLLED_ROTATION_CONSTRAINED_GEOMETRY_DIAGNOSTIC_ONLY",
                    )
                    .put(
                        "family",
                        "HARRIS_STRUCTURE_TENSOR",
                    )
                    .put(
                        "relative_response_floor",
                        ROTATION_SUPPORT_RELATIVE_RESPONSE_FLOOR,
                    )
                    .put(
                        "minimum_keypoint_distance_px",
                        ROTATION_SUPPORT_MIN_KEYPOINT_DISTANCE_PX,
                    )
                    .put(
                        "max_keypoints",
                        ROTATION_SUPPORT_MAX_KEYPOINTS,
                    )
                    .put(
                        "primary_pair_geometry_replaced",
                        false,
                    )
                    .put(
                        "scientific_promotion_threshold",
                        false,
                    ),
            )
            .put(
                "rotation_support_authority",
                "APPEARANCE_DERIVED_DIAGNOSTIC_ONLY",
            )
            .put("camera_identity_used", false)
            .put("lens_identity_used", false)
            .put("vendor_mapping_used", false)
            .put("ai_ml_neural_generative_used", false)
            .put("is_world_registration_proof", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun descriptorHex(
        luma: IntArray,
        width: Int,
        height: Int,
        x: Int,
        y: Int,
        orientationRadians: Double,
    ): String {
        val bytes = ByteArray(DESCRIPTOR_BITS / 8)
        val c = cos(orientationRadians)
        val s = sin(orientationRadians)

        for (bit in 0 until DESCRIPTOR_BITS) {
            val a = patternOffset(bit, 0)
            val b = patternOffset(bit, 1)

            val ax =
                (x + c * a.first - s * a.second)
                    .roundToInt()
                    .coerceIn(0, width - 1)
            val ay =
                (y + s * a.first + c * a.second)
                    .roundToInt()
                    .coerceIn(0, height - 1)
            val bx =
                (x + c * b.first - s * b.second)
                    .roundToInt()
                    .coerceIn(0, width - 1)
            val by =
                (y + s * b.first + c * b.second)
                    .roundToInt()
                    .coerceIn(0, height - 1)

            val av = luma[ay * width + ax]
            val bv = luma[by * width + bx]
            if (av < bv) {
                val byteIndex = bit / 8
                val bitIndex = bit % 8
                bytes[byteIndex] =
                    (bytes[byteIndex].toInt() or
                        (1 shl bitIndex)).toByte()
            }
        }

        return bytes.joinToString("") {
            "%02x".format(it.toInt() and 0xff)
        }
    }

    private fun patternOffset(
        bit: Int,
        side: Int,
    ): Pair<Int, Int> {
        val seed =
            bit * 37 +
                side * 71 +
                19
        val span = PATCH_RADIUS * 2 - 1
        val ox =
            ((seed * 17 + 11) % span) -
                (PATCH_RADIUS - 1)
        val oy =
            ((seed * 29 + 7) % span) -
                (PATCH_RADIUS - 1)
        return ox to oy
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
        observedKeypoints: Int = 0,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("keypoint_count", observedKeypoints)
            .put("camera_identity_used", false)
            .put("lens_identity_used", false)
            .put("vendor_mapping_used", false)
            .put("ai_ml_neural_generative_used", false)
            .put("is_world_registration_proof", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

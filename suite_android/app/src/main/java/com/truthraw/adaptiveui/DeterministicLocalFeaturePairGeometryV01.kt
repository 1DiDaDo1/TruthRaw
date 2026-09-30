package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Deterministic pair geometry over frontside local features.
 *
 * All coordinates and descriptors are APPEARANCE_DERIVED_ONLY. A successful
 * fit is a geometry hypothesis, never proof that the two sources image the
 * same physical world structure.
 */
object DeterministicLocalFeaturePairGeometryV01 {
    const val SCHEMA = "D.RAW/DeterministicLocalFeaturePairGeometry/0.1"

    private const val MAX_DESCRIPTOR_DISTANCE = 52
    private const val RATIO_NUMERATOR = 4
    private const val RATIO_DENOMINATOR = 5
    private const val MIN_MATCHES_FOR_AFFINE = 6
    private const val MIN_RESIDUAL_THRESHOLD = 0.012
    private const val MAD_SCALE = 1.4826

    private data class Feature(
        val index: Int,
        val x: Double,
        val y: Double,
        val descriptor: ByteArray,
    )

    private data class Match(
        val left: Feature,
        val right: Feature,
        val distance: Int,
    )

    fun evaluate(
        leftFrontside: JSONObject,
        rightFrontside: JSONObject,
    ): JSONObject {
        val leftSha = leftFrontside.optString("source_sha256")
        val rightSha = rightFrontside.optString("source_sha256")

        if (
            leftSha.isBlank() ||
            rightSha.isBlank() ||
            leftSha == rightSha
        ) {
            return unavailable(
                leftSha,
                rightSha,
                "DISTINCT_SOURCE_ROOTS_REQUIRED",
            )
        }

        val left = readFeatures(leftFrontside)
        val right = readFeatures(rightFrontside)
        if (
            left.size < MIN_MATCHES_FOR_AFFINE ||
            right.size < MIN_MATCHES_FOR_AFFINE
        ) {
            return unavailable(
                leftSha,
                rightSha,
                "INSUFFICIENT_LOCAL_FEATURES",
            )
        }

        val forward = directedBestMatches(left, right)
        val reverse = directedBestMatches(right, left)

        val reverseMap =
            reverse.associateBy {
                it.left.index to it.right.index
            }

        val mutual =
            forward.filter { match ->
                reverseMap.containsKey(
                    match.right.index to match.left.index,
                )
            }.sortedWith(
                compareBy<Match> { it.distance }
                    .thenBy { it.left.index }
                    .thenBy { it.right.index },
            )

        if (mutual.size < MIN_MATCHES_FOR_AFFINE) {
            return unavailable(
                leftSha,
                rightSha,
                "INSUFFICIENT_MUTUAL_RATIO_FILTERED_MATCHES",
                mutual.size,
            )
        }

        val firstFit = fitAffine(mutual)
            ?: return unavailable(
                leftSha,
                rightSha,
                "AFFINE_NORMAL_EQUATIONS_SINGULAR",
                mutual.size,
            )

        val firstResiduals =
            mutual.map { residual(firstFit, it) }
        val residualMedian = median(firstResiduals)
        val residualMad =
            median(
                firstResiduals.map {
                    abs(it - residualMedian)
                },
            )
        val threshold =
            max(
                MIN_RESIDUAL_THRESHOLD,
                residualMedian +
                    3.0 * MAD_SCALE * residualMad,
            )

        val inliers =
            mutual.filter {
                residual(firstFit, it) <= threshold
            }

        if (inliers.size < MIN_MATCHES_FOR_AFFINE) {
            return unavailable(
                leftSha,
                rightSha,
                "ROBUST_AFFINE_INLIER_COUNT_TOO_LOW",
                inliers.size,
            )
        }

        val affine = fitAffine(inliers)
            ?: return unavailable(
                leftSha,
                rightSha,
                "ROBUST_AFFINE_REFIT_SINGULAR",
                inliers.size,
            )

        val residuals =
            inliers.map { residual(affine, it) }.sorted()
        val rms =
            sqrt(
                residuals.sumOf { it * it } /
                    max(1, residuals.size).toDouble(),
            )
        val med = median(residuals)
        val p95 = percentile(residuals, 0.95)
        val descriptorDistances =
            inliers.map { it.distance.toDouble() }.sorted()

        val a = affine[0]
        val b = affine[1]
        val c = affine[2]
        val d = affine[3]
        val e = affine[4]
        val f = affine[5]
        val det = a * e - b * d

        val matchJson = JSONArray()
        for (match in mutual) {
            val r = residual(affine, match)
            matchJson.put(
                JSONObject()
                    .put("left_index", match.left.index)
                    .put("right_index", match.right.index)
                    .put("descriptor_hamming", match.distance)
                    .put("left_x_normalized", match.left.x)
                    .put("left_y_normalized", match.left.y)
                    .put("right_x_normalized", match.right.x)
                    .put("right_y_normalized", match.right.y)
                    .put("residual_normalized", r)
                    .put("robust_inlier", r <= threshold),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "PAIR_GEOMETRY_CANDIDATE_AVAILABLE",
            )
            .put("left_source_sha256", leftSha)
            .put("right_source_sha256", rightSha)
            .put("authority", "APPEARANCE_DERIVED_GEOMETRY_CANDIDATE_ONLY")
            .put(
                "matching",
                JSONObject()
                    .put("descriptor_metric", "HAMMING_128")
                    .put("mutual_best_required", true)
                    .put("ratio_test", "BEST_LE_0_8_SECOND")
                    .put(
                        "maximum_descriptor_distance",
                        MAX_DESCRIPTOR_DISTANCE,
                    )
                    .put("mutual_match_count", mutual.size)
                    .put("robust_inlier_count", inliers.size)
                    .put(
                        "median_inlier_descriptor_hamming",
                        median(descriptorDistances),
                    ),
            )
            .put(
                "geometry_candidate",
                JSONObject()
                    .put(
                        "model_family",
                        "AFFINE_2D_APPEARANCE_HYPOTHESIS",
                    )
                    .put(
                        "left_to_right_matrix_2x3",
                        JSONArray()
                            .put(JSONArray().put(a).put(b).put(c))
                            .put(JSONArray().put(d).put(e).put(f)),
                    )
                    .put("linear_determinant", det)
                    .put(
                        "approx_area_scale_abs",
                        abs(det),
                    )
                    .put(
                        "approx_rotation_degrees_from_first_column",
                        Math.toDegrees(atan2(d, a)),
                    )
                    .put(
                        "robust_residual_threshold_normalized",
                        threshold,
                    )
                    .put("rms_residual_normalized", rms)
                    .put("median_residual_normalized", med)
                    .put("p95_residual_normalized", p95)
                    .put(
                        "left_inlier_bbox_normalized",
                        bbox(inliers.map { it.left }),
                    )
                    .put(
                        "right_inlier_bbox_normalized",
                        bbox(inliers.map { it.right }),
                    ),
            )
            .put("matches", matchJson)
            .put(
                "uncertainty",
                JSONObject()
                    .put(
                        "geometry_uncertainty_proxy",
                        "NORMALIZED_INLIER_RESIDUALS_ONLY",
                    )
                    .put("rms_normalized", rms)
                    .put("p95_normalized", p95)
                    .put(
                        "photometric_uncertainty_estimated",
                        false,
                    )
                    .put(
                        "absolute_world_scale_uncertainty",
                        "UNKNOWN",
                    ),
            )
            .put(
                "authority_boundary",
                JSONObject()
                    .put("same_world_structure_proven", false)
                    .put("same_physical_camera_proven", false)
                    .put("same_physical_lens_proven", false)
                    .put(
                        "camera_or_lens_identity_used",
                        false,
                    )
                    .put(
                        "may_support_future_registration_validation",
                        true,
                    )
                    .put("registration_promoted", false)
                    .put("calibration_promoted", false)
                    .put("correction_authorized", false),
            )
            .put("automatic_model_winner_used", false)
            .put("ai_ml_neural_generative_used", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("source_sample_values_modified", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun readFeatures(frontside: JSONObject): List<Feature> {
        val geometry =
            frontside.optJSONObject(
                "deterministic_local_feature_geometry_v0_1",
            ) ?: return emptyList()

        if (
            geometry.optString("status") !=
            "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE"
        ) {
            return emptyList()
        }

        val array = geometry.optJSONArray("keypoints")
            ?: return emptyList()

        val out = ArrayList<Feature>()
        for (i in 0 until array.length()) {
            val item = array.optJSONObject(i) ?: continue
            val descriptor =
                hexToBytes(
                    item.optString(
                        "descriptor_hex_128bit",
                    ),
                ) ?: continue
            out += Feature(
                index = item.optInt("index", i),
                x = item.optDouble("x_normalized", Double.NaN),
                y = item.optDouble("y_normalized", Double.NaN),
                descriptor = descriptor,
            )
        }
        return out.filter {
            it.x.isFinite() &&
                it.y.isFinite() &&
                it.x in 0.0..1.0 &&
                it.y in 0.0..1.0
        }
    }

    private fun directedBestMatches(
        left: List<Feature>,
        right: List<Feature>,
    ): List<Match> {
        val out = ArrayList<Match>()
        for (l in left) {
            var best: Feature? = null
            var bestDistance = Int.MAX_VALUE
            var secondDistance = Int.MAX_VALUE

            for (r in right) {
                val distance =
                    hamming(l.descriptor, r.descriptor)
                if (distance < bestDistance) {
                    secondDistance = bestDistance
                    bestDistance = distance
                    best = r
                } else if (distance < secondDistance) {
                    secondDistance = distance
                }
            }

            val candidate = best ?: continue
            if (bestDistance > MAX_DESCRIPTOR_DISTANCE) {
                continue
            }
            if (
                secondDistance != Int.MAX_VALUE &&
                bestDistance * RATIO_DENOMINATOR >
                secondDistance * RATIO_NUMERATOR
            ) {
                continue
            }
            out += Match(
                left = l,
                right = candidate,
                distance = bestDistance,
            )
        }
        return out
    }

    private fun fitAffine(matches: List<Match>): DoubleArray? {
        if (matches.size < 3) return null

        val normal = Array(6) { DoubleArray(6) }
        val rhs = DoubleArray(6)

        fun accumulate(row: DoubleArray, target: Double) {
            for (i in 0 until 6) {
                rhs[i] += row[i] * target
                for (j in 0 until 6) {
                    normal[i][j] += row[i] * row[j]
                }
            }
        }

        for (m in matches) {
            val x = m.left.x
            val y = m.left.y
            accumulate(
                doubleArrayOf(x, y, 1.0, 0.0, 0.0, 0.0),
                m.right.x,
            )
            accumulate(
                doubleArrayOf(0.0, 0.0, 0.0, x, y, 1.0),
                m.right.y,
            )
        }

        return solveLinearSystem(normal, rhs)
    }

    private fun solveLinearSystem(
        matrix: Array<DoubleArray>,
        rhs: DoubleArray,
    ): DoubleArray? {
        val n = rhs.size
        val a =
            Array(n) { row ->
                DoubleArray(n + 1) { col ->
                    if (col < n) {
                        matrix[row][col]
                    } else {
                        rhs[row]
                    }
                }
            }

        for (col in 0 until n) {
            var pivot = col
            for (row in col + 1 until n) {
                if (abs(a[row][col]) > abs(a[pivot][col])) {
                    pivot = row
                }
            }
            if (abs(a[pivot][col]) < 1.0e-12) {
                return null
            }
            if (pivot != col) {
                val tmp = a[pivot]
                a[pivot] = a[col]
                a[col] = tmp
            }

            val divisor = a[col][col]
            for (j in col until n + 1) {
                a[col][j] /= divisor
            }

            for (row in 0 until n) {
                if (row == col) continue
                val factor = a[row][col]
                if (factor == 0.0) continue
                for (j in col until n + 1) {
                    a[row][j] -= factor * a[col][j]
                }
            }
        }

        return DoubleArray(n) { a[it][n] }
    }

    private fun residual(
        affine: DoubleArray,
        match: Match,
    ): Double {
        val px =
            affine[0] * match.left.x +
                affine[1] * match.left.y +
                affine[2]
        val py =
            affine[3] * match.left.x +
                affine[4] * match.left.y +
                affine[5]
        val dx = px - match.right.x
        val dy = py - match.right.y
        return sqrt(dx * dx + dy * dy)
    }

    private fun bbox(features: List<Feature>): JSONObject {
        if (features.isEmpty()) {
            return JSONObject()
                .put("status", "UNKNOWN")
        }
        var minX = Double.POSITIVE_INFINITY
        var minY = Double.POSITIVE_INFINITY
        var maxX = Double.NEGATIVE_INFINITY
        var maxY = Double.NEGATIVE_INFINITY
        for (f in features) {
            minX = min(minX, f.x)
            minY = min(minY, f.y)
            maxX = max(maxX, f.x)
            maxY = max(maxY, f.y)
        }
        return JSONObject()
            .put("left", minX)
            .put("top", minY)
            .put("right", maxX)
            .put("bottom", maxY)
            .put(
                "area_fraction",
                max(0.0, maxX - minX) *
                    max(0.0, maxY - minY),
            )
    }

    private fun hamming(
        a: ByteArray,
        b: ByteArray,
    ): Int {
        val n = min(a.size, b.size)
        var distance = 0
        for (i in 0 until n) {
            distance +=
                Integer.bitCount(
                    (a[i].toInt() xor b[i].toInt()) and 0xff,
                )
        }
        distance += abs(a.size - b.size) * 8
        return distance
    }

    private fun hexToBytes(hex: String): ByteArray? {
        if (
            hex.length != 32 ||
            hex.any { !it.isDigit() && it.lowercaseChar() !in 'a'..'f' }
        ) {
            return null
        }
        return ByteArray(16) { index ->
            val start = index * 2
            hex.substring(start, start + 2)
                .toInt(16)
                .toByte()
        }
    }

    private fun median(values: List<Double>): Double {
        if (values.isEmpty()) return Double.NaN
        val sorted = values.sorted()
        val n = sorted.size
        return if (n % 2 == 1) {
            sorted[n / 2]
        } else {
            0.5 * (
                sorted[n / 2 - 1] +
                    sorted[n / 2]
                )
        }
    }

    private fun percentile(
        values: List<Double>,
        q: Double,
    ): Double {
        if (values.isEmpty()) return Double.NaN
        val sorted = values.sorted()
        val index =
            ((sorted.size - 1) * q.coerceIn(0.0, 1.0))
                .toInt()
        return sorted[index]
    }

    private fun unavailable(
        leftSha: String,
        rightSha: String,
        reason: String,
        matchCount: Int = 0,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put(
                "left_source_sha256",
                leftSha.ifBlank { JSONObject.NULL },
            )
            .put(
                "right_source_sha256",
                rightSha.ifBlank { JSONObject.NULL },
            )
            .put("candidate_match_count", matchCount)
            .put("authority", "APPEARANCE_DERIVED_ONLY")
            .put("same_world_structure_proven", false)
            .put("registration_promoted", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("ai_ml_neural_generative_used", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Deterministic target-blind geometry model bank over already admitted
 * appearance-derived robust pair matches.
 *
 * All model families are reported side-by-side. No winner is selected and no
 * model is promoted to physical world geometry.
 */
object DeterministicPairGeometryModelBankV01 {
    const val SCHEMA = "D.RAW/DeterministicPairGeometryModelBank/0.1"

    private data class Match(
        val x: Double,
        val y: Double,
        val u: Double,
        val v: Double,
    )

    fun evaluate(pairGeometry: JSONObject): JSONObject {
        if (
            pairGeometry.optString("status") !=
            "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
        ) {
            return unavailable("PAIR_GEOMETRY_CANDIDATE_NOT_AVAILABLE")
        }

        val matches = readRobustMatches(pairGeometry)
        if (matches.size < 3) {
            return unavailable("INSUFFICIENT_ROBUST_MATCHES")
        }

        val models = JSONArray()

        val translation = fitTranslation(matches)
        models.put(
            modelReport(
                family = "TRANSLATION_2D",
                parameterCount = 2,
                parameters = translation,
                matches = matches,
                apply = ::applyTranslation,
            ),
        )

        val similarity =
            if (matches.size >= 3) fitSimilarity(matches) else null
        models.put(
            modelReport(
                family = "SIMILARITY_2D",
                parameterCount = 4,
                parameters = similarity,
                matches = matches,
                apply = ::applySimilarity,
            ),
        )

        val affine =
            if (matches.size >= 3) fitAffine(matches) else null
        models.put(
            modelReport(
                family = "AFFINE_2D",
                parameterCount = 6,
                parameters = affine,
                matches = matches,
                apply = ::applyAffine,
            ),
        )

        val homography =
            if (matches.size >= 6) fitHomography(matches) else null
        models.put(
            modelReport(
                family = "HOMOGRAPHY_2D_PROJECTIVE",
                parameterCount = 8,
                parameters = homography,
                matches = matches,
                apply = ::applyHomography,
            ),
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "PAIR_GEOMETRY_MODEL_BANK_AVAILABLE",
            )
            .put(
                "left_source_sha256",
                pairGeometry.opt(
                    "left_source_sha256",
                ) ?: JSONObject.NULL,
            )
            .put(
                "right_source_sha256",
                pairGeometry.opt(
                    "right_source_sha256",
                ) ?: JSONObject.NULL,
            )
            .put(
                "input_authority",
                pairGeometry.optString(
                    "authority",
                    "APPEARANCE_DERIVED_ONLY",
                ),
            )
            .put(
                "input_robust_match_count",
                matches.size,
            )
            .put("models", models)
            .put(
                "selection_policy",
                JSONObject()
                    .put(
                        "automatic_model_winner_used",
                        false,
                    )
                    .put(
                        "lowest_residual_model_is_physical_truth",
                        false,
                    )
                    .put(
                        "complexity_penalty_applied",
                        false,
                    )
                    .put(
                        "all_valid_models_reported_side_by_side",
                        true,
                    ),
            )
            .put(
                "future_rotation_policy",
                JSONObject()
                    .put(
                        "pure_3d_rotation_model_implemented",
                        false,
                    )
                    .put(
                        "reason",
                        "REQUIRES_ADMITTED_INTRINSIC_OR_EQUIVALENT_RAY_GEOMETRY",
                    )
                    .put(
                        "focal_length_metadata_alone_is_sufficient_intrinsics",
                        false,
                    ),
            )
            .put("same_world_structure_proven", false)
            .put("world_registration_promoted", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun readRobustMatches(
        pair: JSONObject,
    ): List<Match> {
        val array = pair.optJSONArray("matches") ?: return emptyList()
        val out = ArrayList<Match>()
        for (i in 0 until array.length()) {
            val m = array.optJSONObject(i) ?: continue
            if (!m.optBoolean("robust_inlier", false)) continue
            val x =
                m.optDouble(
                    "left_x_normalized",
                    Double.NaN,
                )
            val y =
                m.optDouble(
                    "left_y_normalized",
                    Double.NaN,
                )
            val u =
                m.optDouble(
                    "right_x_normalized",
                    Double.NaN,
                )
            val v =
                m.optDouble(
                    "right_y_normalized",
                    Double.NaN,
                )
            if (
                x.isFinite() &&
                y.isFinite() &&
                u.isFinite() &&
                v.isFinite()
            ) {
                out += Match(x, y, u, v)
            }
        }
        return out
    }

    private fun fitTranslation(
        matches: List<Match>,
    ): DoubleArray {
        val tx =
            matches.sumOf { it.u - it.x } /
                matches.size.toDouble()
        val ty =
            matches.sumOf { it.v - it.y } /
                matches.size.toDouble()
        return doubleArrayOf(tx, ty)
    }

    private fun fitSimilarity(
        matches: List<Match>,
    ): DoubleArray? {
        val normal = Array(4) { DoubleArray(4) }
        val rhs = DoubleArray(4)

        fun add(row: DoubleArray, target: Double) {
            for (i in row.indices) {
                rhs[i] += row[i] * target
                for (j in row.indices) {
                    normal[i][j] += row[i] * row[j]
                }
            }
        }

        for (m in matches) {
            add(
                doubleArrayOf(
                    m.x,
                    -m.y,
                    1.0,
                    0.0,
                ),
                m.u,
            )
            add(
                doubleArrayOf(
                    m.y,
                    m.x,
                    0.0,
                    1.0,
                ),
                m.v,
            )
        }
        return solve(normal, rhs)
    }

    private fun fitAffine(
        matches: List<Match>,
    ): DoubleArray? {
        val normal = Array(6) { DoubleArray(6) }
        val rhs = DoubleArray(6)

        fun add(row: DoubleArray, target: Double) {
            for (i in row.indices) {
                rhs[i] += row[i] * target
                for (j in row.indices) {
                    normal[i][j] += row[i] * row[j]
                }
            }
        }

        for (m in matches) {
            add(
                doubleArrayOf(
                    m.x, m.y, 1.0,
                    0.0, 0.0, 0.0,
                ),
                m.u,
            )
            add(
                doubleArrayOf(
                    0.0, 0.0, 0.0,
                    m.x, m.y, 1.0,
                ),
                m.v,
            )
        }
        return solve(normal, rhs)
    }

    private fun fitHomography(
        matches: List<Match>,
    ): DoubleArray? {
        val normal = Array(8) { DoubleArray(8) }
        val rhs = DoubleArray(8)

        fun add(row: DoubleArray, target: Double) {
            for (i in row.indices) {
                rhs[i] += row[i] * target
                for (j in row.indices) {
                    normal[i][j] += row[i] * row[j]
                }
            }
        }

        for (m in matches) {
            add(
                doubleArrayOf(
                    m.x,
                    m.y,
                    1.0,
                    0.0,
                    0.0,
                    0.0,
                    -m.u * m.x,
                    -m.u * m.y,
                ),
                m.u,
            )
            add(
                doubleArrayOf(
                    0.0,
                    0.0,
                    0.0,
                    m.x,
                    m.y,
                    1.0,
                    -m.v * m.x,
                    -m.v * m.y,
                ),
                m.v,
            )
        }
        return solve(normal, rhs)
    }

    private fun modelReport(
        family: String,
        parameterCount: Int,
        parameters: DoubleArray?,
        matches: List<Match>,
        apply: (DoubleArray, Match) -> Pair<Double, Double>?,
    ): JSONObject {
        if (parameters == null) {
            return JSONObject()
                .put("model_family", family)
                .put("status", "NUMERICALLY_UNAVAILABLE")
                .put("parameter_count", parameterCount)
                .put(
                    "scientific_model_promoted",
                    false,
                )
        }

        val residuals = ArrayList<Double>()
        var invalidProjectionCount = 0
        for (m in matches) {
            val p = apply(parameters, m)
            if (p == null) {
                invalidProjectionCount++
                continue
            }
            val dx = p.first - m.u
            val dy = p.second - m.v
            residuals += sqrt(dx * dx + dy * dy)
        }

        val sorted = residuals.sorted()
        val rms =
            if (sorted.isNotEmpty()) {
                sqrt(
                    sorted.sumOf { it * it } /
                        sorted.size.toDouble(),
                )
            } else {
                Double.NaN
            }

        return JSONObject()
            .put("model_family", family)
            .put("status", "CANDIDATE_AVAILABLE")
            .put("parameter_count", parameterCount)
            .put(
                "parameters",
                JSONArray().also { array ->
                    for (value in parameters) {
                        array.put(value)
                    }
                },
            )
            .put(
                "valid_projection_count",
                sorted.size,
            )
            .put(
                "invalid_projection_count",
                invalidProjectionCount,
            )
            .put(
                "rms_residual_normalized",
                finiteOrNull(rms),
            )
            .put(
                "median_residual_normalized",
                finiteOrNull(median(sorted)),
            )
            .put(
                "p95_residual_normalized",
                finiteOrNull(percentile(sorted, 0.95)),
            )
            .put(
                "scientific_model_promoted",
                false,
            )
    }

    private fun applyTranslation(
        p: DoubleArray,
        m: Match,
    ): Pair<Double, Double> =
        (m.x + p[0]) to (m.y + p[1])

    private fun applySimilarity(
        p: DoubleArray,
        m: Match,
    ): Pair<Double, Double> =
        (
            p[0] * m.x -
                p[1] * m.y +
                p[2]
            ) to (
            p[1] * m.x +
                p[0] * m.y +
                p[3]
            )

    private fun applyAffine(
        p: DoubleArray,
        m: Match,
    ): Pair<Double, Double> =
        (
            p[0] * m.x +
                p[1] * m.y +
                p[2]
            ) to (
            p[3] * m.x +
                p[4] * m.y +
                p[5]
            )

    private fun applyHomography(
        p: DoubleArray,
        m: Match,
    ): Pair<Double, Double>? {
        val w =
            p[6] * m.x +
                p[7] * m.y +
                1.0
        if (!w.isFinite() || abs(w) < 1.0e-12) return null

        val u =
            (
                p[0] * m.x +
                    p[1] * m.y +
                    p[2]
                ) / w
        val v =
            (
                p[3] * m.x +
                    p[4] * m.y +
                    p[5]
                ) / w
        if (!u.isFinite() || !v.isFinite()) return null
        return u to v
    }

    private fun solve(
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
            if (
                !a[pivot][col].isFinite() ||
                abs(a[pivot][col]) < 1.0e-12
            ) {
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
            .takeIf { values ->
                values.all { it.isFinite() }
            }
    }

    private fun median(
        values: List<Double>,
    ): Double {
        if (values.isEmpty()) return Double.NaN
        val n = values.size
        return if (n % 2 == 1) {
            values[n / 2]
        } else {
            0.5 * (
                values[n / 2 - 1] +
                    values[n / 2]
                )
        }
    }

    private fun percentile(
        values: List<Double>,
        q: Double,
    ): Double {
        if (values.isEmpty()) return Double.NaN
        val index =
            ((values.size - 1) *
                q.coerceIn(0.0, 1.0))
                .toInt()
        return values[index]
    }

    private fun finiteOrNull(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("automatic_model_winner_used", false)
            .put("same_world_structure_proven", false)
            .put("world_registration_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

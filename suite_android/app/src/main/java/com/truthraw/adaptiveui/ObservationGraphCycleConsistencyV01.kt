package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Descriptive loop/cycle consistency audit for affine pair-geometry
 * hypotheses. It never chooses a registration winner or promotes a world
 * relation.
 */
object ObservationGraphCycleConsistencyV01 {
    const val SCHEMA = "D.RAW/ObservationGraphCycleConsistency/0.1"

    private data class Edge(
        val left: String,
        val right: String,
        val matrix: DoubleArray,
    )

    fun evaluate(graph: JSONObject): JSONObject {
        val nodes = graph.optJSONArray("nodes") ?: JSONArray()
        val roots = ArrayList<String>()
        for (i in 0 until nodes.length()) {
            val sha =
                nodes.optJSONObject(i)
                    ?.optString("source_sha256")
                    ?: continue
            if (sha.isNotBlank()) roots += sha
        }
        val sortedRoots = roots.distinct().sorted()
        val edges = readEdges(graph)

        val triangles = JSONArray()
        val rmsValues = ArrayList<Double>()

        for (i in 0 until sortedRoots.size) {
            for (j in i + 1 until sortedRoots.size) {
                for (k in j + 1 until sortedRoots.size) {
                    val a = sortedRoots[i]
                    val b = sortedRoots[j]
                    val c = sortedRoots[k]

                    val ab = transform(edges, a, b) ?: continue
                    val bc = transform(edges, b, c) ?: continue
                    val ac = transform(edges, a, c) ?: continue

                    val viaB = compose(bc, ab)
                    val errors =
                        canonicalPoints().map { point ->
                            val direct = apply(ac, point.first, point.second)
                            val indirect = apply(viaB, point.first, point.second)
                            val dx = direct.first - indirect.first
                            val dy = direct.second - indirect.second
                            sqrt(dx * dx + dy * dy)
                        }
                    val rms =
                        sqrt(
                            errors.sumOf { it * it } /
                                max(1, errors.size).toDouble(),
                        )
                    val maxError =
                        errors.maxOrNull() ?: Double.NaN
                    if (rms.isFinite()) rmsValues += rms

                    triangles.put(
                        JSONObject()
                            .put("source_a_sha256", a)
                            .put("source_b_sha256", b)
                            .put("source_c_sha256", c)
                            .put(
                                "comparison",
                                "DIRECT_A_TO_C_VS_A_TO_B_TO_C",
                            )
                            .put(
                                "canonical_point_count",
                                errors.size,
                            )
                            .put(
                                "closure_rms_normalized",
                                finiteOrNull(rms),
                            )
                            .put(
                                "closure_max_normalized",
                                finiteOrNull(maxError),
                            )
                            .put(
                                "closure_consistent_proven",
                                false,
                            ),
                    )
                }
            }
        }

        val sortedRms = rmsValues.sorted()
        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (triangles.length() > 0) {
                    "DESCRIPTIVE_CYCLE_CONSISTENCY_AVAILABLE"
                } else {
                    "NO_CLOSED_THREE_OBSERVATION_CYCLES_AVAILABLE"
                },
            )
            .put(
                "source_graph_identity_sha256",
                graph.opt(
                    "graph_identity_sha256",
                ) ?: JSONObject.NULL,
            )
            .put(
                "closed_triangle_count",
                triangles.length(),
            )
            .put(
                "median_cycle_closure_rms_normalized",
                finiteOrNull(median(sortedRms)),
            )
            .put(
                "max_cycle_closure_rms_normalized",
                finiteOrNull(
                    sortedRms.maxOrNull()
                        ?: Double.NaN,
                ),
            )
            .put("triangles", triangles)
            .put(
                "interpretation",
                JSONObject()
                    .put(
                        "automatic_consistency_threshold_used",
                        false,
                    )
                    .put(
                        "world_registration_proven",
                        false,
                    )
                    .put(
                        "world_registration_promoted",
                        false,
                    )
                    .put(
                        "cycle_consistency_is_same_world_proof",
                        false,
                    )
                    .put(
                        "low_closure_error_may_motivate_future_validation",
                        true,
                    ),
            )
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun readEdges(graph: JSONObject): List<Edge> {
        val out = ArrayList<Edge>()
        val array = graph.optJSONArray("edges") ?: return out
        for (i in 0 until array.length()) {
            val edge = array.optJSONObject(i) ?: continue
            val pair = edge.optJSONObject("pair_geometry") ?: continue
            if (
                pair.optString("status") !=
                "PAIR_GEOMETRY_CANDIDATE_AVAILABLE"
            ) {
                continue
            }
            val matrix =
                pair.optJSONObject("geometry_candidate")
                    ?.optJSONArray("left_to_right_matrix_2x3")
                    ?: continue
            val parsed = parseMatrix(matrix) ?: continue
            val left = edge.optString("left_source_sha256")
            val right = edge.optString("right_source_sha256")
            if (
                left.isBlank() ||
                right.isBlank() ||
                left == right
            ) {
                continue
            }
            out += Edge(left, right, parsed)
        }
        return out
    }

    private fun transform(
        edges: List<Edge>,
        from: String,
        to: String,
    ): DoubleArray? {
        for (edge in edges) {
            if (edge.left == from && edge.right == to) {
                return edge.matrix
            }
            if (edge.left == to && edge.right == from) {
                return invert(edge.matrix)
            }
        }
        return null
    }

    private fun parseMatrix(array: JSONArray): DoubleArray? {
        if (array.length() != 2) return null
        val r0 = array.optJSONArray(0) ?: return null
        val r1 = array.optJSONArray(1) ?: return null
        if (r0.length() != 3 || r1.length() != 3) return null
        val out =
            doubleArrayOf(
                r0.optDouble(0, Double.NaN),
                r0.optDouble(1, Double.NaN),
                r0.optDouble(2, Double.NaN),
                r1.optDouble(0, Double.NaN),
                r1.optDouble(1, Double.NaN),
                r1.optDouble(2, Double.NaN),
            )
        return out.takeIf { row -> row.all { it.isFinite() } }
    }

    private fun compose(
        after: DoubleArray,
        before: DoubleArray,
    ): DoubleArray =
        doubleArrayOf(
            after[0] * before[0] + after[1] * before[3],
            after[0] * before[1] + after[1] * before[4],
            after[0] * before[2] + after[1] * before[5] + after[2],
            after[3] * before[0] + after[4] * before[3],
            after[3] * before[1] + after[4] * before[4],
            after[3] * before[2] + after[4] * before[5] + after[5],
        )

    private fun invert(m: DoubleArray): DoubleArray? {
        val det = m[0] * m[4] - m[1] * m[3]
        if (!det.isFinite() || abs(det) < 1.0e-10) {
            return null
        }
        val invDet = 1.0 / det
        val a = m[4] * invDet
        val b = -m[1] * invDet
        val d = -m[3] * invDet
        val e = m[0] * invDet
        val c = -(a * m[2] + b * m[5])
        val f = -(d * m[2] + e * m[5])
        return doubleArrayOf(a, b, c, d, e, f)
    }

    private fun apply(
        m: DoubleArray,
        x: Double,
        y: Double,
    ): Pair<Double, Double> =
        Pair(
            m[0] * x + m[1] * y + m[2],
            m[3] * x + m[4] * y + m[5],
        )

    private fun canonicalPoints(): List<Pair<Double, Double>> =
        listOf(
            0.25 to 0.25,
            0.75 to 0.25,
            0.50 to 0.50,
            0.25 to 0.75,
            0.75 to 0.75,
        )

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

    private fun finiteOrNull(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL
}

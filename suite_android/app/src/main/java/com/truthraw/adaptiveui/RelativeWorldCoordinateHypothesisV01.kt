package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs

/**
 * Builds a relative 2D graph-gauge coordinate hypothesis from appearance
 * pair-geometry candidates. The lexicographically first source SHA is only a
 * numeric gauge, never a physical/world origin.
 */
object RelativeWorldCoordinateHypothesisV01 {
    const val SCHEMA = "D.RAW/RelativeWorldCoordinateHypothesis/0.1"

    private data class Edge(
        val left: String,
        val right: String,
        val leftToRight: DoubleArray,
    )

    fun build(graph: JSONObject): JSONObject {
        val nodesArray = graph.optJSONArray("nodes") ?: JSONArray()
        val roots = ArrayList<String>()
        for (i in 0 until nodesArray.length()) {
            val sha =
                nodesArray.optJSONObject(i)
                    ?.optString("source_sha256")
                    ?: continue
            if (sha.isNotBlank()) roots += sha
        }
        val sortedRoots = roots.distinct().sorted()
        if (sortedRoots.isEmpty()) {
            return unavailable("NO_OBSERVATION_ROOTS")
        }

        val edges = readEdges(graph)
        val gauge = sortedRoots.first()
        val transforms = linkedMapOf<String, DoubleArray>()
        transforms[gauge] = identity()

        var progress = true
        while (progress) {
            progress = false
            for (edge in edges) {
                val leftKnown = transforms[edge.left]
                val rightKnown = transforms[edge.right]

                if (leftKnown != null && rightKnown == null) {
                    val inverse = invert(edge.leftToRight)
                        ?: continue
                    transforms[edge.right] =
                        compose(leftKnown, inverse)
                    progress = true
                } else if (
                    leftKnown == null &&
                    rightKnown != null
                ) {
                    transforms[edge.left] =
                        compose(rightKnown, edge.leftToRight)
                    progress = true
                }
            }
        }

        val observations = JSONArray()
        for (sha in sortedRoots) {
            val transform = transforms[sha]
            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put(
                        "connected_to_numeric_gauge",
                        transform != null,
                    )
                    .put(
                        "source_normalized_to_graph_gauge_affine_2x3",
                        if (transform != null) {
                            matrixJson(transform)
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "transform_authority",
                        if (transform != null) {
                            "APPEARANCE_DERIVED_GRAPH_HYPOTHESIS_ONLY"
                        } else {
                            "UNKNOWN"
                        },
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (transforms.size >= 2) {
                    "RELATIVE_WORLD_COORDINATE_HYPOTHESIS_AVAILABLE"
                } else {
                    "DISCONNECTED_OR_UNREGISTERED_FAIL_CLOSED"
                },
            )
            .put(
                "numeric_gauge_source_sha256",
                gauge,
            )
            .put(
                "numeric_gauge_has_physical_origin_authority",
                false,
            )
            .put(
                "numeric_gauge_has_camera_center_authority",
                false,
            )
            .put(
                "numeric_gauge_has_panorama_center_authority",
                false,
            )
            .put(
                "connected_observation_count",
                transforms.size,
            )
            .put(
                "total_observation_count",
                sortedRoots.size,
            )
            .put("observations", observations)
            .put(
                "coordinate_semantics",
                JSONObject()
                    .put(
                        "dimension",
                        "RELATIVE_2D_FRONT_SIDE_GRAPH_GAUGE",
                    )
                    .put(
                        "absolute_metric_scale_proven",
                        false,
                    )
                    .put("depth_proven", false)
                    .put(
                        "camera_pose_proven",
                        false,
                    )
                    .put(
                        "same_world_structure_proven",
                        false,
                    )
                    .put(
                        "suitable_as_scientific_world_registration",
                        false,
                    ),
            )
            .put("world_registration_promoted", false)
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun readEdges(graph: JSONObject): List<Edge> {
        val out = ArrayList<Edge>()
        val edges = graph.optJSONArray("edges") ?: return out
        for (i in 0 until edges.length()) {
            val edge = edges.optJSONObject(i) ?: continue
            val pair =
                edge.optJSONObject("pair_geometry") ?: continue
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
            out += Edge(
                left = edge.optString("left_source_sha256"),
                right = edge.optString("right_source_sha256"),
                leftToRight = parsed,
            )
        }
        return out.filter {
            it.left.isNotBlank() &&
                it.right.isNotBlank() &&
                it.left != it.right
        }
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

    private fun identity(): DoubleArray =
        doubleArrayOf(
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
        )

    private fun compose(
        a: DoubleArray,
        b: DoubleArray,
    ): DoubleArray =
        doubleArrayOf(
            a[0] * b[0] + a[1] * b[3],
            a[0] * b[1] + a[1] * b[4],
            a[0] * b[2] + a[1] * b[5] + a[2],
            a[3] * b[0] + a[4] * b[3],
            a[3] * b[1] + a[4] * b[4],
            a[3] * b[2] + a[4] * b[5] + a[5],
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

    private fun matrixJson(m: DoubleArray): JSONArray =
        JSONArray()
            .put(JSONArray().put(m[0]).put(m[1]).put(m[2]))
            .put(JSONArray().put(m[3]).put(m[4]).put(m[5]))

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("world_registration_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

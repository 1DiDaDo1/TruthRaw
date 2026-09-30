package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Projects appearance-derived feature-track members into the relative graph
 * gauge exposed by RelativeWorldCoordinateHypothesisV01.
 *
 * The resulting point clouds are world-coordinate hypotheses only. Their
 * dispersion is a consistency diagnostic, not a physical point estimate.
 */
object RelativeWorldFeatureTrackProjectionV01 {
    const val SCHEMA = "D.RAW/RelativeWorldFeatureTrackProjection/0.1"

    fun build(
        tracks: JSONObject,
        relativeWorld: JSONObject,
    ): JSONObject {
        val transforms = readTransforms(relativeWorld)
        val trackArray =
            tracks.optJSONArray("tracks") ?: JSONArray()
        val projectedTracks = JSONArray()
        val rmsValues = ArrayList<Double>()

        for (i in 0 until trackArray.length()) {
            val track = trackArray.optJSONObject(i) ?: continue
            val members =
                track.optJSONArray("members") ?: continue

            val projected = JSONArray()
            val points = ArrayList<Pair<Double, Double>>()
            var missingTransformCount = 0

            for (m in 0 until members.length()) {
                val member = members.optJSONObject(m) ?: continue
                val sha = member.optString("source_sha256")
                val x =
                    member.optDouble(
                        "x_normalized",
                        Double.NaN,
                    )
                val y =
                    member.optDouble(
                        "y_normalized",
                        Double.NaN,
                    )
                val transform = transforms[sha]
                if (
                    transform == null ||
                    !x.isFinite() ||
                    !y.isFinite()
                ) {
                    missingTransformCount++
                    continue
                }

                val p = apply(transform, x, y)
                points += p
                projected.put(
                    JSONObject()
                        .put("source_sha256", sha)
                        .put(
                            "feature_index",
                            member.optInt(
                                "feature_index",
                                -1,
                            ),
                        )
                        .put("graph_gauge_x", p.first)
                        .put("graph_gauge_y", p.second),
                )
            }

            val centroid =
                if (points.isNotEmpty()) {
                    val cx =
                        points.sumOf { it.first } /
                            points.size.toDouble()
                    val cy =
                        points.sumOf { it.second } /
                            points.size.toDouble()
                    cx to cy
                } else {
                    Double.NaN to Double.NaN
                }

            val rms =
                if (
                    points.size >= 2 &&
                    centroid.first.isFinite() &&
                    centroid.second.isFinite()
                ) {
                    sqrt(
                        points.sumOf { p ->
                            val dx = p.first - centroid.first
                            val dy = p.second - centroid.second
                            dx * dx + dy * dy
                        } / points.size.toDouble(),
                    )
                } else {
                    Double.NaN
                }

            val maxRadius =
                if (
                    points.isNotEmpty() &&
                    centroid.first.isFinite() &&
                    centroid.second.isFinite()
                ) {
                    points.maxOf { p ->
                        val dx = p.first - centroid.first
                        val dy = p.second - centroid.second
                        sqrt(dx * dx + dy * dy)
                    }
                } else {
                    Double.NaN
                }

            if (rms.isFinite()) rmsValues += rms

            projectedTracks.put(
                JSONObject()
                    .put(
                        "track_id_sha256",
                        track.optString(
                            "track_id_sha256",
                        ),
                    )
                    .put(
                        "source_track_member_count",
                        members.length(),
                    )
                    .put(
                        "projected_member_count",
                        projected.length(),
                    )
                    .put(
                        "missing_transform_count",
                        missingTransformCount,
                    )
                    .put(
                        "graph_gauge_centroid_x",
                        finiteOrNull(centroid.first),
                    )
                    .put(
                        "graph_gauge_centroid_y",
                        finiteOrNull(centroid.second),
                    )
                    .put(
                        "graph_gauge_rms_dispersion",
                        finiteOrNull(rms),
                    )
                    .put(
                        "graph_gauge_max_dispersion",
                        finiteOrNull(maxRadius),
                    )
                    .put(
                        "same_physical_world_point_proven",
                        false,
                    )
                    .put(
                        "world_point_estimate_promoted",
                        false,
                    )
                    .put("projected_members", projected),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (projectedTracks.length() > 0) {
                    "RELATIVE_WORLD_TRACK_PROJECTIONS_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put(
                "numeric_gauge_source_sha256",
                relativeWorld.opt(
                    "numeric_gauge_source_sha256",
                ) ?: JSONObject.NULL,
            )
            .put(
                "numeric_gauge_has_physical_origin_authority",
                false,
            )
            .put(
                "projected_track_count",
                projectedTracks.length(),
            )
            .put(
                "median_track_rms_dispersion",
                finiteOrNull(
                    median(rmsValues.sorted()),
                ),
            )
            .put(
                "max_track_rms_dispersion",
                finiteOrNull(
                    rmsValues.maxOrNull()
                        ?: Double.NaN,
                ),
            )
            .put("tracks", projectedTracks)
            .put(
                "interpretation",
                JSONObject()
                    .put(
                        "automatic_world_point_threshold_used",
                        false,
                    )
                    .put(
                        "low_dispersion_proves_same_world_point",
                        false,
                    )
                    .put(
                        "may_support_future_geometry_validation",
                        true,
                    )
                    .put(
                        "world_registration_promoted",
                        false,
                    ),
            )
            .put("creates_sensor_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun readTransforms(
        relativeWorld: JSONObject,
    ): Map<String, DoubleArray> {
        val out = linkedMapOf<String, DoubleArray>()
        val observations =
            relativeWorld.optJSONArray("observations")
                ?: return out
        for (i in 0 until observations.length()) {
            val item =
                observations.optJSONObject(i) ?: continue
            val sha = item.optString("source_sha256")
            val matrix =
                item.optJSONArray(
                    "source_normalized_to_graph_gauge_affine_2x3",
                ) ?: continue
            val parsed = parseMatrix(matrix) ?: continue
            if (sha.isNotBlank()) out[sha] = parsed
        }
        return out
    }

    private fun parseMatrix(
        array: JSONArray,
    ): DoubleArray? {
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
        return out.takeIf { values ->
            values.all { it.isFinite() }
        }
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

    private fun median(values: List<Double>): Double {
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

    private fun finiteOrNull(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL
}

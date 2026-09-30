package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.sqrt

/**
 * Candidate triangulation from explicitly supplied camera centres and rays.
 *
 * No intrinsics/pose are invented. The relation record must supply them.
 */
object GeometryDepthCandidateSolverV01 {
    const val SCHEMA = "D.RAW/GeometryDepthCandidate/0.1"

    private data class Ray(
        val sourceSha256: String,
        val center: DoubleArray,
        val direction: DoubleArray,
    )

    fun evaluate(records: List<JSONObject>): JSONObject {
        val roots = linkedSetOf<String>()
        val tracks = ArrayList<JSONObject>()
        for (record in records) {
            if (
                record.optString("axis_scope") !=
                "GEOMETRY_DEPTH_VISIBILITY"
            ) {
                continue
            }
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "GEOMETRY_DEPTH_VISIBILITY",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            if (!payload.optBoolean("pose_relation_admitted", false)) continue
            val rs = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }
            val arr = payload.optJSONArray("tracks") ?: continue
            for (i in 0 until arr.length()) arr.optJSONObject(i)?.let(tracks::add)
        }

        if (tracks.isEmpty()) {
            return unavailable("NO_POSE_ADMITTED_TRACKS", roots)
        }

        val outTracks = JSONArray()
        var solved = 0
        for (track in tracks) {
            val id = track.optString("track_id")
            val obs = track.optJSONArray("observations") ?: continue
            val rays = ArrayList<Ray>()
            for (i in 0 until obs.length()) {
                val o = obs.optJSONObject(i) ?: continue
                val sha = o.optString("source_sha256")
                val center = vector3(o.optJSONArray("camera_center"))
                val direction =
                    vector3(o.optJSONArray("ray_direction"))
                        ?.let(ResearchMathV01::normalize3)
                if (sha.isNotBlank() && center != null && direction != null) {
                    rays += Ray(sha, center, direction)
                }
            }
            if (rays.size < 2) continue

            val midpoints = ArrayList<DoubleArray>()
            val separations = ArrayList<Double>()
            for (a in 0 until rays.size) {
                for (b in a + 1 until rays.size) {
                    val result = triangulate(rays[a], rays[b]) ?: continue
                    midpoints += result.first
                    separations += result.second
                }
            }
            if (midpoints.isEmpty()) continue

            val mean = DoubleArray(3)
            for (p in midpoints) {
                for (c in 0..2) mean[c] += p[c]
            }
            for (c in 0..2) mean[c] /= midpoints.size.toDouble()
            val medianSeparation =
                ResearchMathV01.median(separations) ?: continue
            val maxSeparation = separations.maxOrNull() ?: continue

            outTracks.put(
                JSONObject()
                    .put("track_id", id)
                    .put("ray_count", rays.size)
                    .put("pair_solution_count", midpoints.size)
                    .put("point_candidate", vectorJson(mean))
                    .put("median_ray_pair_separation", medianSeparation)
                    .put("max_ray_pair_separation", maxSeparation)
                    .put("authority", "CALIBRATED_POSE_RELATION_CANDIDATE_ONLY"),
            )
            solved++
        }

        if (solved == 0) {
            return unavailable("NO_NONDEGENERATE_TRIANGULATION", roots)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "GEOMETRY_DEPTH_CANDIDATES_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("solved_track_count", solved)
            .put("tracks", outTracks)
            .put("same_physical_world_point_proven", false)
            .put("visibility_graph_estimated", false)
            .put("occlusion_relation_estimated", false)
            .put("geometry_promoted", false)
            .put("photogrammetric_geometry_upgrades_radiometric_authority", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun triangulate(
        a: Ray,
        b: Ray,
    ): Pair<DoubleArray, Double>? {
        val d1 = a.direction
        val d2 = b.direction
        val w0 = doubleArrayOf(
            a.center[0] - b.center[0],
            a.center[1] - b.center[1],
            a.center[2] - b.center[2],
        )
        val aa = ResearchMathV01.dot3(d1, d1) ?: return null
        val bb = ResearchMathV01.dot3(d1, d2) ?: return null
        val cc = ResearchMathV01.dot3(d2, d2) ?: return null
        val dd = ResearchMathV01.dot3(d1, w0) ?: return null
        val ee = ResearchMathV01.dot3(d2, w0) ?: return null
        val denom = aa * cc - bb * bb
        if (!denom.isFinite() || abs(denom) <= 1.0e-12) return null

        val s = (bb * ee - cc * dd) / denom
        val t = (aa * ee - bb * dd) / denom
        if (!s.isFinite() || !t.isFinite()) return null

        val p1 = DoubleArray(3) { c -> a.center[c] + s * d1[c] }
        val p2 = DoubleArray(3) { c -> b.center[c] + t * d2[c] }
        val midpoint = DoubleArray(3) { c -> 0.5 * (p1[c] + p2[c]) }
        var separation2 = 0.0
        for (c in 0..2) {
            val d = p1[c] - p2[c]
            separation2 += d * d
        }
        val separation = sqrt(separation2)
        if (!separation.isFinite()) return null
        return midpoint to separation
    }

    private fun vector3(a: JSONArray?): DoubleArray? {
        if (a == null || a.length() != 3) return null
        val out = DoubleArray(3)
        for (i in 0..2) {
            val v = a.optDouble(i, Double.NaN)
            if (!v.isFinite()) return null
            out[i] = v
        }
        return out
    }

    private fun vectorJson(v: DoubleArray): JSONArray =
        JSONArray().put(v[0]).put(v[1]).put(v[2])

    private fun unavailable(
        reason: String,
        roots: Set<String>,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("geometry_promoted", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

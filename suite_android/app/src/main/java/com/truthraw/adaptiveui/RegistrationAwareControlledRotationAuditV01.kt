package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.ln
import kotlin.math.max
import kotlin.math.sqrt

/**
 * Diagnostic-only comparison between the nominal controlled-rotation field
 * model and appearance-derived pair geometry. It never promotes registration,
 * calibration, correction or writeback.
 */
object RegistrationAwareControlledRotationAuditV01 {
    const val SCHEMA =
        "D.RAW/RegistrationAwareControlledRotationAudit/0.1"

    private const val EPS = 1.0e-12
    private const val LN2 = 0.6931471805599453

    private data class Edge(
        val left: String,
        val right: String,
        val matrix: DoubleArray,
        val rms: Double,
        val inliers: Int,
    )

    fun evaluate(
        graph: JSONObject,
        records: List<JSONObject>,
    ): JSONObject {
        val edges = readEdges(graph)
        val reports = JSONArray()
        var admitted = 0

        for ((index, record) in records.withIndex()) {
            if (record.optString("axis_scope") != "FIELD_RESPONSE") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "FIELD_RESPONSE",
                )
            val payload = record.optJSONObject("axis_payload") ?: continue
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED" ||
                !payload.optBoolean("controlled_rotation_relation", false)
            ) {
                continue
            }
            admitted++
            reports.put(evaluateRecord(index, record, payload, edges))
        }

        if (admitted == 0) {
            return unavailable(
                "NO_ADMITTED_CONTROLLED_ROTATION_RELATION_RECORD",
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "REGISTRATION_AWARE_CONTROLLED_ROTATION_AUDIT_AVAILABLE",
            )
            .put("admitted_record_count", admitted)
            .put("record_reports", reports)
            .put(
                "authority_boundary",
                JSONObject()
                    .put(
                        "pair_geometry_authority",
                        "APPEARANCE_DERIVED_GEOMETRY_CANDIDATE_ONLY",
                    )
                    .put("pair_geometry_used_for_diagnostic_only", true)
                    .put("nominal_field_solver_modified", false)
                    .put("registration_adjusted_field_solver_executed", false)
                    .put("geometry_deviation_explains_field_error_proven", false)
                    .put("same_world_structure_proven", false)
                    .put("world_registration_proven", false)
                    .put("world_registration_promoted", false)
                    .put("field_response_calibration_promoted", false)
                    .put("correction_authorized", false),
            )
            .put("automatic_numeric_threshold_used", false)
            .put("automatic_registration_winner_used", false)
            .put("image_transform_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun evaluateRecord(
        recordIndex: Int,
        record: JSONObject,
        payload: JSONObject,
        edges: List<Edge>,
    ): JSONObject {
        val candidate =
            FieldResponseRotationSeparationCandidateV01.evaluate(
                listOf(record),
            )
        if (
            candidate.optString("status") !=
            "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE"
        ) {
            return recordUnavailable(
                recordIndex,
                "INDEPENDENT_FIELD_SEPARATION_CANDIDATE_UNAVAILABLE",
            )
        }

        val world =
            doubleMap(
                candidate.optJSONObject("world_component_candidates_ev"),
            )
        val sensor =
            doubleMap(
                candidate.optJSONObject("sensor_component_candidates_ev"),
            )
        val samples = payload.optJSONArray("samples") ?: JSONArray()
        val errors = linkedMapOf<String, MutableList<Double>>()
        val roles = linkedMapOf<String, Pair<String, Int>>()

        for (i in 0 until samples.length()) {
            val s = samples.optJSONObject(i) ?: continue
            val source = s.optString("source_sha256").trim().lowercase()
            val role = s.optString("observation_role")
            val q = quarterTurn(role) ?: continue
            val actual = s.optDouble("relative_signal_ev", Double.NaN)
            val w = world[s.optString("world_cell_id")] ?: continue
            val r = sensor[s.optString("sensor_cell_id")] ?: continue
            if (!actual.isFinite()) continue
            errors.getOrPut(source) { ArrayList() } += actual - (w + r)
            roles[source] = role to q
        }

        val anchor =
            roles.entries.firstOrNull { it.value.second == 0 }?.key
                ?: return recordUnavailable(
                    recordIndex,
                    "ROTATION_0_DEG_ANCHOR_REQUIRED",
                )
        val sign =
            payload.optJSONObject("rotation_mapping")
                ?.optInt("selected_shift_sign", 0) ?: 0
        val observations = JSONArray()
        var geometryCount = 0

        for (
            source in roles.keys.sortedBy {
                roles[it]?.second ?: Int.MAX_VALUE
            }
        ) {
            val role = roles[source] ?: continue
            val q = role.second
            val es = errors[source].orEmpty()
            val fieldRmse =
                if (es.isNotEmpty()) {
                    sqrt(es.sumOf { it * it } / es.size.toDouble())
                } else {
                    Double.NaN
                }

            if (q == 0) {
                observations.put(
                    baseObservation(
                        source,
                        role.first,
                        q,
                        es.size,
                        fieldRmse,
                    )
                        .put("pair_geometry_status", "ANCHOR_IDENTITY_REFERENCE")
                        .put("appearance_rotation_degrees", 0.0)
                        .put("geometry_implied_world_sector_shift", 0.0)
                        .put("nominal_record_world_sector_shift", 0.0)
                        .put("wrapped_sector_shift_difference", 0.0)
                        .put("abs_wrapped_sector_shift_difference", 0.0)
                        .put("pair_geometry_rms_normalized", 0.0)
                        .put("center_displacement_normalized", 0.0)
                        .put("area_scale_abs", 1.0)
                        .put("abs_log2_area_scale", 0.0)
                        .put("linear_anisotropy_ratio", 1.0)
                        .put("linear_determinant_negative", false),
                )
                continue
            }

            val transformed = transform(edges, anchor, source)
            if (transformed == null) {
                observations.put(
                    baseObservation(
                        source,
                        role.first,
                        q,
                        es.size,
                        fieldRmse,
                    ).put(
                        "pair_geometry_status",
                        "PAIR_GEOMETRY_CANDIDATE_UNAVAILABLE",
                    ),
                )
                continue
            }
            geometryCount++

            val m = transformed.first
            val edge = transformed.second
            val det = m[0] * m[4] - m[1] * m[3]
            val area = abs(det)
            val rotation = Math.toDegrees(atan2(m[3], m[0]))
            val geometryShift = wrap12(-rotation / 30.0)
            val nominalShift =
                if (sign == 1 || sign == -1) {
                    wrap12(sign * q * 3.0)
                } else {
                    Double.NaN
                }
            val shiftDelta =
                if (nominalShift.isFinite()) {
                    wrap12(geometryShift - nominalShift)
                } else {
                    Double.NaN
                }
            val cx = m[0] * 0.5 + m[1] * 0.5 + m[2]
            val cy = m[3] * 0.5 + m[4] * 0.5 + m[5]
            val centerMove =
                sqrt((cx - 0.5) * (cx - 0.5) + (cy - 0.5) * (cy - 0.5))
            val sv = singularValues(m[0], m[1], m[3], m[4])
            val anisotropy =
                if (sv != null && sv.second > EPS) {
                    sv.first / sv.second
                } else {
                    Double.NaN
                }
            val areaLog2 =
                if (area > EPS) abs(ln(area) / LN2) else Double.NaN

            observations.put(
                baseObservation(
                    source,
                    role.first,
                    q,
                    es.size,
                    fieldRmse,
                )
                    .put("pair_geometry_status", "PAIR_GEOMETRY_CANDIDATE_AVAILABLE")
                    .put("pair_geometry_inlier_count", edge.inliers)
                    .put("pair_geometry_rms_normalized", finite(edge.rms))
                    .put("appearance_rotation_degrees", finite(rotation))
                    .put("geometry_implied_world_sector_shift", finite(geometryShift))
                    .put("nominal_record_world_sector_shift", finite(nominalShift))
                    .put("wrapped_sector_shift_difference", finite(shiftDelta))
                    .put("abs_wrapped_sector_shift_difference", finite(abs(shiftDelta)))
                    .put("center_displacement_normalized", finite(centerMove))
                    .put("area_scale_abs", finite(area))
                    .put("abs_log2_area_scale", finite(areaLog2))
                    .put("linear_anisotropy_ratio", finite(anisotropy))
                    .put("linear_determinant_negative", det < 0.0),
            )
        }

        return JSONObject()
            .put("record_index", recordIndex)
            .put(
                "status",
                "CONTROLLED_ROTATION_RECORD_REGISTRATION_AUDIT_AVAILABLE",
            )
            .put(
                "record_source_sha256_roots",
                record.optJSONArray("source_sha256_roots") ?: JSONArray(),
            )
            .put(
                "selected_nominal_shift_sign",
                if (sign == 1 || sign == -1) sign else JSONObject.NULL,
            )
            .put(
                "independent_field_solver",
                JSONObject()
                    .put("training_sample_count", candidate.optInt("training_sample_count", 0))
                    .put("held_out_sample_count", candidate.optInt("held_out_sample_count", 0))
                    .put("training_rmse_ev", candidate.opt("training_rmse_ev") ?: JSONObject.NULL)
                    .put("held_out_rmse_ev", candidate.opt("held_out_rmse_ev") ?: JSONObject.NULL)
                    .put("candidate_applied", false)
                    .put("scientific_writeback_allowed", false),
            )
            .put("anchor_source_sha256", anchor)
            .put("geometry_candidate_observation_count", geometryCount)
            .put("observation_diagnostics", observations)
            .put(
                "interpretation",
                JSONObject()
                    .put("appearance_geometry_may_diagnose_rotation_axis_or_framing_error", true)
                    .put("appearance_geometry_is_world_registration_proof", false)
                    .put("geometry_deviation_explains_field_error_proven", false)
                    .put("registration_resampling_performed", false)
                    .put("automatic_threshold_or_winner_used", false),
            )
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun baseObservation(
        source: String,
        role: String,
        q: Int,
        count: Int,
        rmse: Double,
    ): JSONObject =
        JSONObject()
            .put("source_sha256", source)
            .put("observation_role", role)
            .put("quarter_turn_index", q)
            .put("field_solver_sample_count", count)
            .put("field_solver_rmse_ev", finite(rmse))

    private fun quarterTurn(role: String): Int? =
        when {
            role.contains("ROTATION_0_DEG") -> 0
            role.contains("ROTATION_90_DEG") -> 1
            role.contains("ROTATION_180_DEG") -> 2
            role.contains("ROTATION_270_DEG") -> 3
            else -> null
        }

    private fun doubleMap(obj: JSONObject?): Map<String, Double> {
        if (obj == null) return emptyMap()
        val out = linkedMapOf<String, Double>()
        val keys = obj.keys()
        while (keys.hasNext()) {
            val key = keys.next()
            val value = obj.optDouble(key, Double.NaN)
            if (value.isFinite()) out[key] = value
        }
        return out
    }

    private fun readEdges(graph: JSONObject): List<Edge> {
        val out = ArrayList<Edge>()
        val arr = graph.optJSONArray("edges") ?: return out
        for (i in 0 until arr.length()) {
            val item = arr.optJSONObject(i) ?: continue
            val pair = item.optJSONObject("pair_geometry") ?: continue
            if (pair.optString("status") != "PAIR_GEOMETRY_CANDIDATE_AVAILABLE") continue
            val geometry = pair.optJSONObject("geometry_candidate") ?: continue
            val matrix = parseMatrix(
                geometry.optJSONArray("left_to_right_matrix_2x3") ?: continue,
            ) ?: continue
            val left = item.optString("left_source_sha256")
            val right = item.optString("right_source_sha256")
            if (left.isBlank() || right.isBlank() || left == right) continue
            out += Edge(
                left = left,
                right = right,
                matrix = matrix,
                rms = geometry.optDouble("rms_residual_normalized", Double.NaN),
                inliers = pair.optJSONObject("matching")
                    ?.optInt("robust_inlier_count", 0) ?: 0,
            )
        }
        return out
    }

    private fun transform(
        edges: List<Edge>,
        from: String,
        to: String,
    ): Pair<DoubleArray, Edge>? {
        for (edge in edges) {
            if (edge.left == from && edge.right == to) return edge.matrix to edge
            if (edge.left == to && edge.right == from) {
                val inverse = invert(edge.matrix) ?: return null
                return inverse to edge
            }
        }
        return null
    }

    private fun parseMatrix(a: JSONArray): DoubleArray? {
        if (a.length() != 2) return null
        val r0 = a.optJSONArray(0) ?: return null
        val r1 = a.optJSONArray(1) ?: return null
        if (r0.length() != 3 || r1.length() != 3) return null
        val out = doubleArrayOf(
            r0.optDouble(0, Double.NaN),
            r0.optDouble(1, Double.NaN),
            r0.optDouble(2, Double.NaN),
            r1.optDouble(0, Double.NaN),
            r1.optDouble(1, Double.NaN),
            r1.optDouble(2, Double.NaN),
        )
        return out.takeIf { v -> v.all { it.isFinite() } }
    }

    private fun invert(m: DoubleArray): DoubleArray? {
        val det = m[0] * m[4] - m[1] * m[3]
        if (!det.isFinite() || abs(det) < 1.0e-10) return null
        val id = 1.0 / det
        val a = m[4] * id
        val b = -m[1] * id
        val d = -m[3] * id
        val e = m[0] * id
        val c = -(a * m[2] + b * m[5])
        val f = -(d * m[2] + e * m[5])
        return doubleArrayOf(a, b, c, d, e, f)
    }

    private fun singularValues(
        a: Double,
        b: Double,
        d: Double,
        e: Double,
    ): Pair<Double, Double>? {
        val trace = a * a + d * d + b * b + e * e
        val det = a * e - b * d
        val disc = max(0.0, trace * trace - 4.0 * det * det)
        val root = sqrt(disc)
        val hi = 0.5 * (trace + root)
        val lo = 0.5 * (trace - root)
        if (!hi.isFinite() || !lo.isFinite() || hi < 0.0 || lo < 0.0) return null
        return sqrt(hi) to sqrt(lo)
    }

    private fun wrap12(value: Double): Double {
        if (!value.isFinite()) return Double.NaN
        var x = value % 12.0
        if (x > 6.0) x -= 12.0
        if (x <= -6.0) x += 12.0
        return x
    }

    private fun finite(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL

    private fun recordUnavailable(index: Int, reason: String): JSONObject =
        JSONObject()
            .put("record_index", index)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("registration_adjusted_field_solver_executed", false)
            .put("world_registration_promoted", false)
            .put("field_response_calibration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("registration_adjusted_field_solver_executed", false)
            .put("world_registration_promoted", false)
            .put("field_response_calibration_promoted", false)
            .put("correction_authorized", false)
            .put("candidate_applied", false)
            .put("image_transform_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

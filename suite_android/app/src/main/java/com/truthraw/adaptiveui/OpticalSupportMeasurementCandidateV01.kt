package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Deterministic ingestion and summary of relation-based SFR/MTF-like
 * measurements. PSF kernels may be carried as observations but are not
 * deconvolved here.
 */
object OpticalSupportMeasurementCandidateV01 {
    const val SCHEMA = "D.RAW/OpticalSupportMeasurementCandidate/0.1"

    fun evaluate(records: List<JSONObject>): JSONObject {
        val points = ArrayList<JSONObject>()
        val roots = linkedSetOf<String>()
        for (record in records) {
            if (record.optString("axis_scope") != "OPTICAL_SUPPORT") continue
            val validation = CalibrationObservationRecordValidatorV01.validate(record)
            if (
                validation.optString("status") !=
                "CALIBRATION_OBSERVATION_RECORD_VALID"
            ) {
                continue
            }
            val rs = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            val arr = payload.optJSONArray("measurement_points") ?: continue
            for (i in 0 until arr.length()) arr.optJSONObject(i)?.let(points::add)
        }

        if (points.isEmpty()) {
            return unavailable("NO_OPTICAL_MEASUREMENT_POINTS", roots)
        }

        val groups = linkedMapOf<String, MutableList<Pair<Double, Double>>>()
        var fieldCoordinateCount = 0
        var focusStateCount = 0
        for (p in points) {
            val kind = p.optString("kind").uppercase()
            val f = p.optDouble("frequency_cycles_per_pixel", Double.NaN)
            val t = p.optDouble("transfer", Double.NaN)
            if (
                kind in setOf("SFR", "MTF") &&
                f.isFinite() && f >= 0.0 &&
                t.isFinite() && t >= 0.0
            ) {
                groups.getOrPut(kind) { ArrayList() }.add(f to t)
            }
            val rho = p.optDouble("rho", Double.NaN)
            val az = p.optDouble("azimuth_radians", Double.NaN)
            if (rho.isFinite() && az.isFinite()) fieldCoordinateCount++
            if (p.opt("focus_state") != null && p.opt("focus_state") != JSONObject.NULL) {
                focusStateCount++
            }
        }

        val curves = JSONArray()
        for ((kind, raw) in groups) {
            val sorted = raw.sortedBy { it.first }
            val curve = JSONArray()
            for ((f, t) in sorted) {
                curve.put(JSONArray().put(f).put(t))
            }
            curves.put(
                JSONObject()
                    .put("kind", kind)
                    .put("point_count", sorted.size)
                    .put("curve", curve)
                    .put("frequency_at_transfer_0_5_candidate", crossing(sorted, 0.5))
                    .put("frequency_at_transfer_0_1_candidate", crossing(sorted, 0.1)),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "OPTICAL_SUPPORT_MEASUREMENT_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("measurement_point_count", points.size)
            .put("field_coordinate_point_count", fieldCoordinateCount)
            .put("focus_state_point_count", focusStateCount)
            .put("frequency_curves", curves)
            .put("sfr_measurement_attached", groups.containsKey("SFR"))
            .put("mtf_measurement_attached", groups.containsKey("MTF"))
            .put(
                "psf_measurement_attached",
                points.any { it.optString("kind").uppercase() == "PSF" },
            )
            .put("optical_support_calibration_promoted", false)
            .put("deconvolution_authorized", false)
            .put("inverse_optics_authorized", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun crossing(
        curve: List<Pair<Double, Double>>,
        target: Double,
    ): Any {
        if (curve.size < 2) return JSONObject.NULL
        for (i in 1 until curve.size) {
            val (f0, t0) = curve[i - 1]
            val (f1, t1) = curve[i]
            if ((t0 >= target && t1 <= target) || (t0 <= target && t1 >= target)) {
                val dt = t1 - t0
                if (!dt.isFinite() || kotlin.math.abs(dt) <= 1.0e-15) return f0
                val u = (target - t0) / dt
                val f = f0 + u * (f1 - f0)
                return if (f.isFinite()) f else JSONObject.NULL
            }
        }
        return JSONObject.NULL
    }

    private fun unavailable(
        reason: String,
        roots: Set<String>,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("optical_support_calibration_promoted", false)
            .put("deconvolution_authorized", false)
            .put("scientific_writeback_allowed", false)
}

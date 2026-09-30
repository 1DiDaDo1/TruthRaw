package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.atan
import kotlin.math.PI

/**
 * Appearance-only mapping from image frequency to visual-angle frequency.
 *
 * A visibility sensitivity curve is used only when explicitly supplied as an
 * admitted appearance model. No human-vision curve is invented here.
 */
object PerceptualNoiseVisibilityCandidateV01 {
    const val SCHEMA = "D.RAW/PerceptualNoiseVisibilityCandidate/0.1"

    fun evaluate(request: JSONObject): JSONObject {
        val ppi = request.optDouble("display_pixel_density_ppi", Double.NaN)
        val distance = request.optDouble("viewing_distance_m", Double.NaN)
        val frequencies = request.optJSONArray("frequencies_cycles_per_pixel")
            ?: return unavailable("FREQUENCIES_REQUIRED")
        if (!ppi.isFinite() || ppi <= 0.0 || !distance.isFinite() || distance <= 0.0) {
            return unavailable("VALID_DISPLAY_DENSITY_AND_VIEWING_DISTANCE_REQUIRED")
        }

        val pixelPitchM = 0.0254 / ppi
        val angleRad = 2.0 * atan(0.5 * pixelPitchM / distance)
        val angleDeg = angleRad * 180.0 / PI
        if (!angleDeg.isFinite() || angleDeg <= 0.0) {
            return unavailable("VISUAL_ANGLE_NUMERIC_FAILURE")
        }
        val pixelsPerDegree = 1.0 / angleDeg

        val curve = request.optJSONArray("validated_sensitivity_curve")
        val curveAdmitted =
            request.optString("sensitivity_curve_authority") ==
                "VALIDATED_APPEARANCE_MODEL" &&
                curve != null &&
                curve.length() >= 2

        val mapped = JSONArray()
        for (i in 0 until frequencies.length()) {
            val cpp = frequencies.optDouble(i, Double.NaN)
            if (!cpp.isFinite() || cpp < 0.0) continue
            val cpd = cpp * pixelsPerDegree
            mapped.put(
                JSONObject()
                    .put("cycles_per_pixel", cpp)
                    .put("cycles_per_degree", cpd)
                    .put(
                        "visibility_weight",
                        if (curveAdmitted) {
                            interpolateCurve(curve!!, cpd) ?: JSONObject.NULL
                        } else {
                            JSONObject.NULL
                        },
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PERCEPTUAL_NOISE_VISIBILITY_CANDIDATE_AVAILABLE")
            .put("pixels_per_degree", pixelsPerDegree)
            .put("frequency_mapping", mapped)
            .put("validated_sensitivity_curve_used", curveAdmitted)
            .put("appearance_only", true)
            .put("scientific_noise_state_modified", false)
            .put("scientific_master_modified", false)
            .put("appearance_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun interpolateCurve(
        curve: JSONArray,
        x: Double,
    ): Double? {
        val points = ArrayList<Pair<Double, Double>>()
        for (i in 0 until curve.length()) {
            val p = curve.optJSONArray(i) ?: continue
            if (p.length() != 2) continue
            val px = p.optDouble(0, Double.NaN)
            val py = p.optDouble(1, Double.NaN)
            if (px.isFinite() && py.isFinite()) points += px to py
        }
        val sorted = points.sortedBy { it.first }
        if (sorted.isEmpty()) return null
        if (x <= sorted.first().first) return sorted.first().second
        if (x >= sorted.last().first) return sorted.last().second
        for (i in 1 until sorted.size) {
            val a = sorted[i - 1]
            val b = sorted[i]
            if (x in a.first..b.first) {
                val dx = b.first - a.first
                if (dx <= 0.0) return a.second
                val u = (x - a.first) / dx
                return (a.second + u * (b.second - a.second))
                    .takeIf(Double::isFinite)
            }
        }
        return null
    }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("appearance_only", true)
            .put("scientific_master_modified", false)
            .put("scientific_writeback_allowed", false)
}

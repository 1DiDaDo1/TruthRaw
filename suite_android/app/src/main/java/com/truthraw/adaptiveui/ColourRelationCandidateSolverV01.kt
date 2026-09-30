package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.sqrt

/**
 * Identity-independent 3x3 camera-RGB -> reference-XYZ candidate fit.
 *
 * The fit is a research candidate only. It requires relation records and never
 * turns RGB into spectral truth.
 */
object ColourRelationCandidateSolverV01 {
    const val SCHEMA = "D.RAW/ColourRelationCandidate/0.1"

    private data class Patch(
        val rgb: DoubleArray,
        val xyz: DoubleArray,
        val illuminant: String,
        val role: String,
    )

    fun evaluate(records: List<JSONObject>): JSONObject {
        val patches = ArrayList<Patch>()
        val roots = linkedSetOf<String>()
        for (record in records) {
            if (record.optString("axis_scope") != "COLOUR_RELATION") continue
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
            val arr = payload.optJSONArray("patches") ?: continue
            for (i in 0 until arr.length()) {
                val p = arr.optJSONObject(i) ?: continue
                val rgb = vector3(p.optJSONArray("camera_rgb")) ?: continue
                val xyz = vector3(p.optJSONArray("reference_xyz")) ?: continue
                val illum = p.optString("illuminant_id")
                if (illum.isBlank()) continue
                patches += Patch(
                    rgb = rgb,
                    xyz = xyz,
                    illuminant = illum,
                    role = p.optString("role", "TRAIN"),
                )
            }
        }

        val train = patches.filter { it.role == "TRAIN" }
        val held = patches.filter { it.role == "HELD_OUT" }
        val illuminants = train.map { it.illuminant }.toSet()
        if (train.size < 6) {
            return unavailable("INSUFFICIENT_TRAINING_PATCHES", roots)
        }
        if (illuminants.size < 2) {
            return unavailable("AT_LEAST_TWO_ILLUMINANTS_REQUIRED", roots)
        }
        if (held.isEmpty()) {
            return unavailable("HELD_OUT_PATCH_REQUIRED", roots)
        }

        val xtx = Array(3) { DoubleArray(3) }
        val xty = Array(3) { DoubleArray(3) }
        for (p in train) {
            for (r in 0..2) {
                for (c in 0..2) {
                    xtx[r][c] += p.rgb[r] * p.rgb[c]
                    xty[r][c] += p.rgb[r] * p.xyz[c]
                }
            }
        }

        val inv = ResearchMathV01.invert3x3(xtx)
            ?: return unavailable("COLOUR_NORMAL_EQUATION_SINGULAR", roots)
        val coeff = ResearchMathV01.multiply3x3(inv, xty)
            ?: return unavailable("COLOUR_MATRIX_SOLVE_FAILED", roots)

        // coeff[input][output] -> matrix[output][input]
        val matrix = Array(3) { out ->
            DoubleArray(3) { input -> coeff[input][out] }
        }

        val trainRmse = vectorRmse(train, matrix)
            ?: return unavailable("TRAINING_RMSE_FAILED", roots)
        val heldRmse = vectorRmse(held, matrix)
            ?: return unavailable("HELD_OUT_RMSE_FAILED", roots)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "COLOUR_RELATION_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("training_patch_count", train.size)
            .put("held_out_patch_count", held.size)
            .put("training_illuminant_count", illuminants.size)
            .put("camera_rgb_to_reference_xyz_candidate", matrixJson(matrix))
            .put("training_component_rmse", trainRmse)
            .put("held_out_component_rmse", heldRmse)
            .put("white_balance_equals_illuminant_spectrum", false)
            .put("three_channel_rgb_equals_spectral_truth", false)
            .put("colour_covariance_transport_required", true)
            .put("empirical_relation_attached", true)
            .put("calibration_promoted", false)
            .put("automatic_colour_correction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
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

    private fun vectorRmse(
        patches: List<Patch>,
        matrix: Array<DoubleArray>,
    ): Double? {
        if (patches.isEmpty()) return null
        var sum = 0.0
        var n = 0
        for (p in patches) {
            val predicted =
                ResearchMathV01.multiply3x3Vector(matrix, p.rgb) ?: return null
            for (c in 0..2) {
                val d = p.xyz[c] - predicted[c]
                sum += d * d
                n++
            }
        }
        return sqrt(sum / n.toDouble()).takeIf(Double::isFinite)
    }

    private fun matrixJson(m: Array<DoubleArray>): JSONArray {
        val out = JSONArray()
        for (r in 0..2) {
            out.put(JSONArray().put(m[r][0]).put(m[r][1]).put(m[r][2]))
        }
        return out
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
            .put("calibration_promoted", false)
            .put("automatic_colour_correction_applied", false)
            .put("scientific_writeback_allowed", false)
}

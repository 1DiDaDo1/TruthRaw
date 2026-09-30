package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.sqrt

/**
 * Uncertainty-weighted reconstructed-value candidate.
 *
 * Exact measured anchors are passed through unchanged. Unanchored coordinates
 * may return a RECONSTRUCTED_CANDIDATE, never MEASURED.
 */
object ScientificReconstructionCandidateV01 {
    const val SCHEMA = "D.RAW/ScientificReconstructionCandidate/0.1"

    fun evaluate(
        request: JSONObject,
        allowedSourceRoots: Set<String>? = null,
    ): JSONObject {
        val exact = request.optJSONObject("exact_measured_anchor")
        if (exact != null) {
            val values = vector3(exact.optJSONArray("values"))
                ?: return unavailable("INVALID_EXACT_MEASURED_ANCHOR")
            return JSONObject()
                .put("schema", SCHEMA)
                .put("status", "EXACT_MEASURED_ANCHOR_PASSTHROUGH")
                .put("values", vectorJson(values))
                .put("authority", "MEASURED")
                .put("measured_anchor_modified", false)
                .put("reconstruction_performed", false)
                .put("candidate_applied", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
        }

        val supports = request.optJSONArray("supports")
            ?: return unavailable("SUPPORTS_REQUIRED")
        val sumWeight = DoubleArray(3)
        val sumValue = DoubleArray(3)
        val roots = linkedSetOf<String>()

        for (i in 0 until supports.length()) {
            val s = supports.optJSONObject(i) ?: continue
            val values = vector3(s.optJSONArray("values")) ?: continue
            val variance = vector3(s.optJSONArray("variance")) ?: continue
            val supportWeight =
                s.optDouble("support_weight", Double.NaN)
            val sha =
                s.optString("source_sha256").trim().lowercase()
            if (
                !supportWeight.isFinite() || supportWeight <= 0.0 ||
                variance.any { !it.isFinite() || it <= 0.0 }
            ) {
                continue
            }
            if (
                allowedSourceRoots != null &&
                sha !in allowedSourceRoots
            ) {
                continue
            }
            if (sha.isNotBlank()) roots += sha
            for (c in 0..2) {
                val w = supportWeight / variance[c]
                if (!w.isFinite() || w <= 0.0) continue
                sumWeight[c] += w
                sumValue[c] += w * values[c]
            }
        }

        if (
            allowedSourceRoots != null &&
            roots.isEmpty()
        ) {
            return unavailable("NO_SUPPORT_BOUND_TO_PROMOTED_SOURCE_ROOTS")
        }

        if (sumWeight.any { !it.isFinite() || it <= 0.0 }) {
            return unavailable("INSUFFICIENT_UNCERTAINTY_BOUND_SUPPORT")
        }

        val value = DoubleArray(3)
        val sigma = DoubleArray(3)
        for (c in 0..2) {
            value[c] = sumValue[c] / sumWeight[c]
            sigma[c] = sqrt(1.0 / sumWeight[c])
            if (!value[c].isFinite() || !sigma[c].isFinite()) {
                return unavailable("RECONSTRUCTION_NUMERIC_FAILURE")
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "RECONSTRUCTED_VALUE_CANDIDATE_AVAILABLE")
            .put("values", vectorJson(value))
            .put("sigma_candidate", vectorJson(sigma))
            .put("authority", "RECONSTRUCTED_CANDIDATE")
            .put("provenance_source_sha256", JSONArray(roots.toList()))
            .put("measured_anchor_modified", false)
            .put("reconstruction_performed", true)
            .put("reconstruction_promoted", false)
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

    private fun vectorJson(v: DoubleArray): JSONArray =
        JSONArray().put(v[0]).put(v[1]).put(v[2])

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("authority", "UNKNOWN")
            .put("reconstruction_performed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

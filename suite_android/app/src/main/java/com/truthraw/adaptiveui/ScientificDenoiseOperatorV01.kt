package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Gated derived-output scientific denoise/reconstruction operator.
 *
 * The research candidate can be inspected at any time. An admitted derived
 * result requires an explicit route-specific admission object. Scientific
 * Master writeback remains forbidden here even after admission.
 */
object ScientificDenoiseOperatorV01 {
    const val SCHEMA = "D.RAW/ScientificDenoiseOperator/0.1"

    fun researchCandidate(request: JSONObject): JSONObject =
        ScientificReconstructionCandidateV01.evaluate(request)
            .put("operator_schema", SCHEMA)
            .put("research_candidate_only", true)
            .put("noise_reduction_applied", false)
            .put("scientific_writeback_allowed", false)

    fun resolveDerived(
        admission: JSONObject,
        route: String,
        request: JSONObject,
    ): JSONObject {
        val routeState =
            admission.optJSONObject("routes")
                ?.optJSONObject(route)
                ?: return blocked("UNKNOWN_DENOISE_ROUTE", route)
        if (
            !routeState.optBoolean("evidence_ready", false) ||
            !routeState.optBoolean("scientific_denoise_admitted", false) ||
            !admission.optBoolean("scientific_denoise_admitted", false)
        ) {
            return blocked("SCIENTIFIC_DENOISE_NOT_ADMITTED", route)
        }

        val candidate = ScientificReconstructionCandidateV01.evaluate(request)
        if (
            candidate.optString("status") !=
            "RECONSTRUCTED_VALUE_CANDIDATE_AVAILABLE"
        ) {
            return blocked("RECONSTRUCTION_CANDIDATE_UNAVAILABLE", route)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "DERIVED_DENOISED_RECONSTRUCTION_AVAILABLE")
            .put("route", route)
            .put("result", candidate)
            .put("authority", "RECONSTRUCTED")
            .put("measured_anchors_modified", false)
            .put("noise_reduction_applied", true)
            .put("scientific_master_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun blocked(
        reason: String,
        route: String,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "BLOCKED_FAIL_CLOSED")
            .put("reason", reason)
            .put("route", route)
            .put("noise_reduction_applied", false)
            .put("scientific_master_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

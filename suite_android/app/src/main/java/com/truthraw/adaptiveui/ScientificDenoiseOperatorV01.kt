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

    fun resolveWithPromotionState(
        route: String,
        request: JSONObject,
        radiometric: JSONObject,
        noiseComponents: JSONObject,
        opticalSupport: JSONObject,
        temporalFootprint: JSONObject,
        geometryDepth: JSONObject,
        worldSpaceNoise: JSONObject,
        promotionState: JSONObject,
    ): JSONObject {
        val noiseTransport =
            ScientificNoiseTransportV01.describe(promotionState)
        val admission =
            ScientificDenoiseAdmissionV01.describe(
                radiometric = radiometric,
                noiseComponents = noiseComponents,
                noiseTransport = noiseTransport,
                opticalSupport = opticalSupport,
                temporalFootprint = temporalFootprint,
                geometryDepth = geometryDepth,
                worldSpaceNoise = worldSpaceNoise,
                promotionState = promotionState,
                allowPromotionProjection = true,
            )
        val roots =
            linkedSetOf<String>().apply {
                val arr =
                    promotionState.optJSONArray(
                        "source_sha256_roots",
                    )
                if (arr != null) {
                    for (i in 0 until arr.length()) {
                        arr.optString(i)
                            .trim()
                            .lowercase()
                            .takeIf(String::isNotBlank)
                            ?.let(::add)
                    }
                }
            }
        return resolveDerived(
            admission = admission,
            route = route,
            request = request,
            allowedSourceRoots = roots,
        )
    }

    fun resolveDerived(
        admission: JSONObject,
        route: String,
        request: JSONObject,
        allowedSourceRoots: Set<String>? = null,
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

        val candidate =
            ScientificReconstructionCandidateV01.evaluate(
                request = request,
                allowedSourceRoots = allowedSourceRoots,
            )
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

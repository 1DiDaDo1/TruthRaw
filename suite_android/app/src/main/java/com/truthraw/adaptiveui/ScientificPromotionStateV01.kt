package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Typed bridge between future INTERNAL held-out validation and candidate
 * runtimes. No user-imported calibration record can create this state.
 *
 * Current runtime uses blocked() until an internal validation producer emits
 * the exact promotion-decision schema and passes all invariant checks.
 */
object ScientificPromotionStateV01 {
    const val SCHEMA = "D.RAW/ScientificPromotionState/0.1"
    const val INTERNAL_DECISION_SCHEMA =
        "D.RAW/InternalScientificPromotionDecision/0.1"

    fun blocked(reason: String = "NO_INTERNAL_PROMOTION_DECISION"): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "NOT_PROMOTED_FAIL_CLOSED")
            .put("reason", reason)
            .put("world_registration_promoted", false)
            .put("world_to_source_bridge_promoted", false)
            .put("radiometric_calibration_promoted", false)
            .put("field_response_calibration_promoted", false)
            .put("colour_calibration_promoted", false)
            .put("noise_component_calibration_promoted", false)
            .put("numeric_noise_transport_validated", false)
            .put("optical_support_calibration_promoted", false)
            .put("temporal_relation_promoted", false)
            .put("geometry_promoted", false)
            .put("world_space_noise_separation_promoted", false)
            .put("scientific_denoise_single_frame_approved", false)
            .put("scientific_denoise_optics_approved", false)
            .put("scientific_denoise_world_space_approved", false)
            .put("scientific_writeback_allowed", false)
            .put("creates_new_evidence", false)

    fun fromInternalValidationDecision(
        report: JSONObject,
        activeSourceRoots: Set<String>,
    ): JSONObject {
        if (
            report.optString("schema") != INTERNAL_DECISION_SCHEMA ||
            !report.optBoolean("generated_by_internal_validator", false) ||
            !report.optBoolean("held_out_validation_passed", false) ||
            report.optBoolean("measured_samples_modified", true) ||
            report.optBoolean("scientific_writeback_requested", true)
        ) {
            return blocked("INTERNAL_PROMOTION_DECISION_INVARIANT_FAILED")
        }

        val roots = report.optJSONArray("source_sha256_roots") ?: JSONArray()
        if (roots.length() == 0) {
            return blocked("PROMOTION_DECISION_SOURCE_ROOTS_REQUIRED")
        }
        for (i in 0 until roots.length()) {
            val root = roots.optString(i).trim().lowercase()
            if (root !in activeSourceRoots) {
                return blocked("PROMOTION_DECISION_NOT_BOUND_TO_ACTIVE_SESSION")
            }
        }

        val decisions = report.optJSONObject("decisions")
            ?: return blocked("PROMOTION_DECISIONS_MISSING")

        fun decision(key: String): Boolean =
            decisions.optBoolean(key, false)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "INTERNAL_PROMOTION_DECISION_BOUND")
            .put("held_out_validation_passed", true)
            .put("world_registration_promoted", decision("world_registration_promoted"))
            .put("world_to_source_bridge_promoted", decision("world_to_source_bridge_promoted"))
            .put("radiometric_calibration_promoted", decision("radiometric_calibration_promoted"))
            .put("field_response_calibration_promoted", decision("field_response_calibration_promoted"))
            .put("colour_calibration_promoted", decision("colour_calibration_promoted"))
            .put("noise_component_calibration_promoted", decision("noise_component_calibration_promoted"))
            .put("numeric_noise_transport_validated", decision("numeric_noise_transport_validated"))
            .put("optical_support_calibration_promoted", decision("optical_support_calibration_promoted"))
            .put("temporal_relation_promoted", decision("temporal_relation_promoted"))
            .put("geometry_promoted", decision("geometry_promoted"))
            .put("world_space_noise_separation_promoted", decision("world_space_noise_separation_promoted"))
            .put("scientific_denoise_single_frame_approved", decision("scientific_denoise_single_frame_approved"))
            .put("scientific_denoise_optics_approved", decision("scientific_denoise_optics_approved"))
            .put("scientific_denoise_world_space_approved", decision("scientific_denoise_world_space_approved"))
            .put("scientific_writeback_allowed", false)
            .put("creates_new_evidence", false)
    }
}

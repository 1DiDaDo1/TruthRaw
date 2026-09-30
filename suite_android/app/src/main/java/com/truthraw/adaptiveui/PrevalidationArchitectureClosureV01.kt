package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Formal closure record for the implementation-before-validation phase.
 *
 * Closure means the agreed safe architecture is represented in code and
 * fail-closed contracts. It does NOT mean physical validation or scientific
 * promotion.
 */
object PrevalidationArchitectureClosureV01 {
    const val SCHEMA = "D.RAW/PrevalidationArchitectureClosure/0.1"

    fun build(
        axisAuthority: JSONObject,
        unknownGuard: JSONObject,
        queryPlanner: JSONObject,
        candidateLedger: JSONObject,
        capabilityMatrix: JSONObject,
        fieldSeparation: JSONObject,
        promotionFirewall: JSONObject,
    ): JSONObject {
        val checks = JSONArray()

        fun check(
            id: String,
            actual: String,
            expected: Set<String>,
        ): Boolean {
            val ok = actual in expected
            checks.put(
                JSONObject()
                    .put("id", id)
                    .put("actual_status", actual)
                    .put("implemented_contract_present", ok),
            )
            return ok
        }

        val allImplemented =
            listOf(
                check(
                    "AXIS_AUTHORITY",
                    axisAuthority.optString("status"),
                    setOf("AXIS_AUTHORITY_MATRIX_AVAILABLE"),
                ),
                check(
                    "UNKNOWN_PROPAGATION",
                    unknownGuard.optString("status"),
                    setOf("UNKNOWN_PROPAGATION_GUARD_AVAILABLE"),
                ),
                check(
                    "QUERY_PLANNER",
                    queryPlanner.optString("status"),
                    setOf(
                        "QUERY_PLAN_CONTEXT_AVAILABLE_NO_WORLD_SOURCE_RESOLUTION",
                    ),
                ),
                check(
                    "CANDIDATE_LEDGER",
                    candidateLedger.optString("status"),
                    setOf("PREVALIDATION_CANDIDATE_LEDGER_AVAILABLE"),
                ),
                check(
                    "CAPABILITY_MATRIX",
                    capabilityMatrix.optString("status"),
                    setOf("CAPABILITY_MATRIX_AVAILABLE"),
                ),
                check(
                    "FIELD_SEPARATION_CANDIDATES",
                    fieldSeparation.optString("status"),
                    setOf("FIELD_SEPARATION_CANDIDATE_SET_AVAILABLE"),
                ),
                check(
                    "PROMOTION_FIREWALL",
                    promotionFirewall.optString("status"),
                    setOf("RESEARCH_PROMOTION_FIREWALL_PASS"),
                ),
            ).all { it }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (allImplemented) {
                    "PREVALIDATION_ARCHITECTURE_CLOSURE_COMPLETE"
                } else {
                    "PREVALIDATION_ARCHITECTURE_CLOSURE_INCOMPLETE"
                },
            )
            .put("implementation_checks", checks)
            .put(
                "closure_meaning",
                "SAFE_ARCHITECTURE_IMPLEMENTED_AND_FAIL_CLOSED",
            )
            .put(
                "closure_is_physical_validation",
                false,
            )
            .put(
                "closure_is_scientific_promotion",
                false,
            )
            .put(
                "phone_test_required_to_preserve_architecture",
                false,
            )
            .put(
                "future_physical_validation_still_required_for_promotion",
                true,
            )
            .put(
                "normal_user_calibration_required",
                false,
            )
            .put("world_registration_promoted", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("deconvolution_authorized", false)
            .put("multi_frame_scientific_fusion_applied", false)
            .put("restoration_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

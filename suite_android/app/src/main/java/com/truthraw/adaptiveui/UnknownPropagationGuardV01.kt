package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Explicit UNKNOWN propagation rules used by research/prevalidation runtime.
 */
object UnknownPropagationGuardV01 {
    const val SCHEMA = "D.RAW/UnknownPropagationGuard/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_PROPAGATION_GUARD_AVAILABLE")
            .put(
                "forbidden_substitutions",
                JSONArray()
                    .put("UNKNOWN_TO_ZERO")
                    .put("UNKNOWN_TO_ONE")
                    .put("UNKNOWN_TO_IDENTITY_TRANSFORM")
                    .put("UNKNOWN_TO_EMPTY_UNCERTAINTY")
                    .put("UNKNOWN_TO_MEASURED")
                    .put("UNKNOWN_TO_CALIBRATED")
                    .put("UNKNOWN_TO_RECONSTRUCTED_WITHOUT_SUPPORT"),
            )
            .put(
                "rules",
                JSONObject()
                    .put(
                        "missing_numeric_value_may_default_to_zero",
                        false,
                    )
                    .put(
                        "missing_gain_may_default_to_one_as_scientific_truth",
                        false,
                    )
                    .put(
                        "missing_transform_may_default_to_identity_as_world_relation",
                        false,
                    )
                    .put(
                        "missing_uncertainty_may_default_to_zero",
                        false,
                    )
                    .put(
                        "unsupported_query_must_remain_unknown",
                        true,
                    )
                    .put(
                        "unknown_may_block_promotion",
                        true,
                    )
                    .put(
                        "unknown_may_be_preserved_indefinitely",
                        true,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

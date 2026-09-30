package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * One read-only snapshot that makes the current scientific authority state
 * explicit for future chats/builds.
 */
object FreeWorldScientificStateSnapshotV01 {
    const val SCHEMA = "D.RAW/FreeWorldScientificStateSnapshot/0.1"

    fun build(
        lineage: JSONObject,
        axisAuthority: JSONObject,
        capabilityMatrix: JSONObject,
        gateRegistry: JSONObject,
        unknownGuard: JSONObject,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SCIENTIFIC_STATE_SNAPSHOT_AVAILABLE")
            .put(
                "source_sha256_roots",
                lineage.optJSONArray("source_sha256_roots")
                    ?: org.json.JSONArray(),
            )
            .put(
                "source_root_count",
                lineage.optInt("source_root_count", 0),
            )
            .put("axis_authority_matrix", axisAuthority)
            .put("capability_matrix", capabilityMatrix)
            .put("promotion_gate_registry", gateRegistry)
            .put("unknown_propagation_guard", unknownGuard)
            .put(
                "scientific_state_law",
                JSONObject()
                    .put(
                        "implementation_equals_validation",
                        false,
                    )
                    .put(
                        "validation_equals_measurement",
                        false,
                    )
                    .put(
                        "measurement_equals_promotion",
                        false,
                    )
                    .put(
                        "appearance_equals_measurement",
                        false,
                    )
                    .put(
                        "metadata_hint_equals_measurement",
                        false,
                    )
                    .put(
                        "unknown_is_valid_state",
                        true,
                    )
                    .put(
                        "derived_manifest_is_physical_evidence_root",
                        false,
                    ),
            )
            .put(
                "current_global_promotion",
                JSONObject()
                    .put("world_registration_promoted", false)
                    .put("calibration_promoted", false)
                    .put("correction_authorized", false)
                    .put("deconvolution_authorized", false)
                    .put("temporal_fusion_applied", false)
                    .put("restoration_applied", false)
                    .put(
                        "scientific_writeback_allowed",
                        false,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

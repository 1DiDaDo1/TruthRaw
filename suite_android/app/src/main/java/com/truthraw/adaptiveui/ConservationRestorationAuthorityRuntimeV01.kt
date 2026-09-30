package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Runtime authority vocabulary for conservation/restoration operations.
 */
object ConservationRestorationAuthorityRuntimeV01 {
    const val SCHEMA = "D.RAW/ConservationRestorationAuthorityRuntime/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "RESTORATION_AUTHORITY_RUNTIME_CONTRACT_AVAILABLE",
            )
            .put(
                "roles",
                JSONArray()
                    .put("ORIGINAL_MEASURED_SUPPORT")
                    .put("CONDITION_OBSERVED")
                    .put("STABILIZED_DERIVED")
                    .put("LOSS_COMPENSATION_RECONSTRUCTED")
                    .put("AESTHETIC_REINTEGRATION_ONLY")
                    .put("UNRESOLVED_LOSS"),
            )
            .put(
                "rules",
                JSONObject()
                    .put(
                        "valid_measured_support_may_be_overpainted_without_invalidation_reason",
                        false,
                    )
                    .put(
                        "loss_compensation_becomes_measured",
                        false,
                    )
                    .put(
                        "unresolved_loss_may_be_silently_filled",
                        false,
                    )
                    .put(
                        "appearance_similarity_proves_physical_correctness",
                        false,
                    )
                    .put(
                        "restoration_must_remain_replayable_or_removable",
                        true,
                    )
                    .put(
                        "source_and_scientific_master_identity_must_remain_recoverable",
                        true,
                    )
                    .put(
                        "authority_mask_or_equivalent_provenance_required",
                        true,
                    ),
            )
            .put("restoration_applied", false)
            .put("source_sample_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

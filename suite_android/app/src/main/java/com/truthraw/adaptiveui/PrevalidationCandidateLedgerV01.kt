package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Machine-readable queue of preserved candidate machinery that still needs
 * future evidence before scientific promotion.
 */
object PrevalidationCandidateLedgerV01 {
    const val SCHEMA = "D.RAW/PrevalidationCandidateLedger/0.1"

    fun build(
        graph: JSONObject,
        tracks: JSONObject,
        cycles: JSONObject,
        fieldSeparation: JSONObject,
    ): JSONObject {
        val items = JSONArray()

        fun add(
            id: String,
            implemented: Boolean,
            currentStatus: String,
            futureGate: String,
        ) {
            items.put(
                JSONObject()
                    .put("id", id)
                    .put("implemented", implemented)
                    .put("current_status", currentStatus)
                    .put("future_gate", futureGate)
                    .put("scientifically_promoted", false),
            )
        }

        add(
            "PAIR_GEOMETRY",
            true,
            "APPEARANCE_DERIVED_CANDIDATES=" +
                graph.optInt("geometry_candidate_edge_count", 0),
            "VALIDATED_PAIR_GEOMETRY",
        )
        add(
            "MULTI_OBSERVATION_FEATURE_TRACKS",
            true,
            "TRACK_HYPOTHESES=" +
                tracks.optInt("track_count", 0),
            "MULTI_OBSERVATION_TRACK_CONSISTENCY",
        )
        add(
            "GRAPH_CYCLE_CONSISTENCY",
            true,
            "CLOSED_TRIANGLES=" +
                cycles.optInt("closed_triangle_count", 0),
            "GRAPH_LOOP_CONSISTENCY",
        )
        add(
            "FIELD_RESPONSE_SEPARATION",
            true,
            "ELIGIBLE_FUTURE_PAIRS=" +
                fieldSeparation.optInt(
                    "eligible_future_experiment_pair_count",
                    0,
                ),
            "WORLD_VS_SENSOR_SEPARATION",
        )
        add(
            "WORLD_REGISTRATION",
            true,
            "RELATIVE_GRAPH_GAUGE_HYPOTHESES_ONLY",
            "HELD_OUT_WORLD_REGISTRATION_VALIDATION",
        )
        add(
            "WORLD_TO_SOURCE_BRIDGE",
            true,
            "CONTRACT_ONLY_NOT_ADMITTED",
            "VALIDATED_WORLD_TO_SOURCE_BRIDGE",
        )
        add(
            "CONTINUOUS_FREE_WORLD_SOLVER",
            true,
            "TYPED_ABI_PLUS_FAIL_CLOSED_UNKNOWN_RUNTIME",
            "ADMITTED_RECONSTRUCTION_WITH_SUPPORT_UNCERTAINTY_CENSOR_BOUNDS",
        )
        add(
            "COLOUR_CALIBRATION",
            true,
            "RUNTIME_CONTRACT_ONLY",
            "REFERENCE_TARGET_MULTI_ILLUMINANT_HELD_OUT_VALIDATION",
        )
        add(
            "OPTICAL_SUPPORT",
            true,
            "RUNTIME_CONTRACT_ONLY",
            "MEASURED_SFR_MTF_PSF_OR_EQUIVALENT",
        )
        add(
            "TEMPORAL_RELATION",
            true,
            "SCAFFOLD_ONLY",
            "SOURCE_BOUND_PHYSICAL_SEQUENCE_VALIDATION",
        )
        add(
            "RESTORATION",
            true,
            "AUTHORITY_RUNTIME_ONLY",
            "EXPLICIT_RESTORATION_ADMISSION",
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PREVALIDATION_CANDIDATE_LEDGER_AVAILABLE")
            .put("items", items)
            .put(
                "all_items_preserved_without_forced_promotion",
                true,
            )
            .put(
                "phone_test_required_to_preserve_items",
                false,
            )
            .put(
                "automatic_promotion_allowed",
                false,
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

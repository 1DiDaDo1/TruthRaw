package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Executable schema contract for future raster-independent Free World queries.
 *
 * v0.1 does not synthesize a queried pixel. It freezes what a future query
 * must return so resolution freedom cannot erase authority/provenance.
 */
object FreeWorldContinuousQueryContractV01 {
    const val SCHEMA = "D.RAW/FreeWorldContinuousQueryContract/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "QUERY_CONTRACT_AVAILABLE_NO_PIXEL_SOLVER")
            .put(
                "query_coordinates",
                JSONArray()
                    .put("RELATIVE_WORLD_X")
                    .put("RELATIVE_WORLD_Y")
                    .put("OPTIONAL_TIME")
                    .put("OPTIONAL_VIEW_DIRECTION")
                    .put("REQUESTED_OUTPUT_FOOTPRINT"),
            )
            .put(
                "required_result_fields",
                JSONArray()
                    .put("VALUE_OR_UNKNOWN")
                    .put("VALUE_DOMAIN")
                    .put("SOURCE_SUPPORT")
                    .put("RECONSTRUCTION_SUPPORT")
                    .put("SPATIAL_FOOTPRINT")
                    .put("TEMPORAL_FOOTPRINT")
                    .put("AUTHORITY")
                    .put("UNCERTAINTY")
                    .put("CENSOR_BOUNDS")
                    .put("PROVENANCE_ROOTS"),
            )
            .put(
                "authority_classes",
                JSONArray()
                    .put("MEASURED")
                    .put("CALIBRATED_ESTIMATE")
                    .put("RECONSTRUCTED")
                    .put("CENSORED")
                    .put("UNKNOWN")
                    .put("APPEARANCE_ONLY"),
            )
            .put(
                "resolution_policy",
                JSONObject()
                    .put(
                        "source_raster_is_world_resolution_limit",
                        false,
                    )
                    .put(
                        "output_resolution_may_change_authority",
                        false,
                    )
                    .put(
                        "empty_fine_lattice_positions_begin_unknown",
                        true,
                    )
                    .put(
                        "interpolation_may_create_measured_samples",
                        false,
                    ),
            )
            .put(
                "precision_policy",
                JSONObject()
                    .put(
                        "branch_sensitive_compute",
                        "FLOAT64_WHERE_REQUIRED",
                    )
                    .put(
                        "controlled_scientific_storage",
                        "FLOAT32_WHERE_VALIDATED",
                    )
                    .put(
                        "precision_is_authority",
                        false,
                    ),
            )
            .put(
                "view_policy",
                JSONObject()
                    .put(
                        "human_vision_viewing_conditions_downstream",
                        true,
                    )
                    .put(
                        "display_target_downstream",
                        true,
                    )
                    .put(
                        "output_acutance_downstream",
                        true,
                    )
                    .put(
                        "view_transform_may_write_scientific_master",
                        false,
                    ),
            )
            .put("pixel_solver_implemented", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

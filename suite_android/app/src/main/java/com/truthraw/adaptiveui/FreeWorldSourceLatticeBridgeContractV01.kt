package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Contract separating relative-world coordinates from exact source-lattice
 * coordinates. No bridge is admitted until an independently validated
 * registration relation exists.
 */
object FreeWorldSourceLatticeBridgeContractV01 {
    const val SCHEMA =
        "D.RAW/FreeWorldSourceLatticeBridgeContract/0.1"

    fun describe(
        promotionState: JSONObject =
            ScientificPromotionStateV01.blocked(),
        validatedMapping: JSONObject? = null,
        activeSourceRoots: Set<String> = emptySet(),
    ): JSONObject {
        val mappingRoots =
            linkedSetOf<String>().apply {
                val arr =
                    validatedMapping?.optJSONArray(
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
        val rootsBound =
            activeSourceRoots.isNotEmpty() &&
                mappingRoots.isNotEmpty() &&
                mappingRoots.all { it in activeSourceRoots }
        val mapping = validatedMapping
        val mappingValid =
            mapping != null &&
                mapping.optString("schema") ==
                    "D.RAW/ValidatedWorldToSourceRelation/0.1" &&
                mapping.optBoolean(
                    "held_out_validation_passed",
                    false,
                ) &&
                mapping.has("mapping_uncertainty") &&
                !mapping.optBoolean(
                    "appearance_only_authority",
                    true,
                ) &&
                rootsBound
        val promoted =
            promotionState.optBoolean(
                "world_to_source_bridge_promoted",
                false,
            )
        val admitted = promoted && mappingValid

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (admitted) {
                    "VALIDATED_WORLD_TO_SOURCE_BRIDGE_ADMITTED"
                } else {
                    "BRIDGE_CONTRACT_AVAILABLE_NO_ADMITTED_WORLD_TO_SOURCE_MAP"
                },
            )
            .put(
                "source_coordinate_domain",
                RasterIndependentSampleLatticeV01.SCHEMA,
            )
            .put(
                "world_coordinate_domain",
                "D.RAW/RELATIVE_FREE_WORLD_COORDINATE_DOMAIN",
            )
            .put(
                "required_before_world_query_can_reach_measured_anchor",
                "VALIDATED_EXPLICIT_WORLD_TO_SOURCE_RELATION_WITH_UNCERTAINTY",
            )
            .put(
                "appearance_pair_geometry_is_sufficient",
                false,
            )
            .put(
                "relative_graph_gauge_is_sufficient",
                false,
            )
            .put(
                "camera_or_lens_identity_is_sufficient",
                false,
            )
            .put(
                "stitched_panorama_geometry_is_sufficient",
                false,
            )
            .put(
                "exact_source_anchor_resolver_available",
                true,
            )
            .put(
                "world_to_source_bridge_admitted",
                admitted,
            )
            .put("promotion_state_granted", promoted)
            .put("validated_mapping_attached", mappingValid)
            .put("validated_mapping_source_roots_bound", rootsBound)
            .put(
                "unmapped_world_query_must_remain_unknown",
                true,
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

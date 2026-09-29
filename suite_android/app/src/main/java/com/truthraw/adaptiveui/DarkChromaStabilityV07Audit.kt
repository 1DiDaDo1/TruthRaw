package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Dark Chroma Stability v0.7.
 *
 * This version does not add a denoise or colour-correction rule.
 * It moves the exact v0.6 sampled support geometry into the generic
 * raster-independent D.RAW sample coordinate world.
 */
object DarkChromaStabilityV07Audit {
    fun analyze(
        sourceSha256: String,
        v06: JSONObject?,
        lattice: JSONObject?,
        latticeGeometry: JSONObject?,
    ): JSONObject {
        if (
            v06 == null ||
            v06.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.6" ||
            v06.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "V0_6_BINDING_MISMATCH")
        }
        if (
            lattice == null ||
            lattice.optString("schema") !=
            "D.RAW/RasterIndependentSampleLattice/0.1" ||
            lattice.optString("source_sha256") != sourceSha256 ||
            lattice.optString("status") != "AVAILABLE"
        ) {
            return unavailable(sourceSha256, "SAMPLE_LATTICE_BINDING_MISMATCH")
        }

        val geometryAvailable =
            latticeGeometry != null &&
                latticeGeometry.optString("schema") ==
                    "D.RAW/N2RasterIndependentSampleGeometry/0.1" &&
                latticeGeometry.optString("source_sha256") ==
                    sourceSha256 &&
                latticeGeometry.optString("status") ==
                    "AUDIT_ONLY_LATTICE_BINDING_AVAILABLE"

        val geometrySkipped =
            latticeGeometry?.optString("status") ==
                "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE"

        val v06Global =
            v06.optJSONObject("global") ?: JSONObject()
        val queries =
            latticeGeometry?.optJSONArray("queries") ?: JSONArray()

        return JSONObject()
            .put(
                "schema",
                "D.RAW/Frontside/DarkChromaStability/0.7",
            )
            .put(
                "status",
                when {
                    geometryAvailable ->
                        "AUDIT_ONLY_RASTER_INDEPENDENT_GEOMETRY_AVAILABLE"
                    geometrySkipped ->
                        "AUDIT_ONLY_RASTER_INDEPENDENT_GEOMETRY_SKIPPED"
                    else ->
                        "AUDIT_ONLY_RASTER_INDEPENDENT_GEOMETRY_FAIL_CLOSED"
                },
            )
            .put("source_sha256", sourceSha256)
            .put(
                "authority",
                "EXACT_SOURCE_ANCHORS_PLUS_VERSIONED_RASTER_INDEPENDENT_COORDINATE_GEOMETRY",
            )
            .put(
                "coordinate_units_per_source_pixel",
                lattice.optLong(
                    "coordinate_units_per_source_pixel",
                    0L,
                ),
            )
            .put(
                "source_measured_anchor_count",
                lattice.opt("measured_anchor_count"),
            )
            .put(
                "unanchored_lattice_positions_authority",
                lattice.optString(
                    "unanchored_lattice_positions_authority",
                    "UNKNOWN",
                ),
            )
            .put(
                "distance_geometry_available",
                geometryAvailable,
            )
            .put(
                "distance_bound_candidate_count",
                if (geometryAvailable) queries.length() else 0,
            )
            .put(
                "v0_6_visible_candidate_tiles",
                v06Global.optLong("visible_candidate_tiles", 0L),
            )
            .put(
                "lattice_contract",
                JSONObject()
                    .put(
                        "source_raster_replaced_as_evidence",
                        false,
                    )
                    .put(
                        "source_raster_used_only_as_measurement_sampling",
                        true,
                    )
                    .put(
                        "measured_source_samples_relocated",
                        false,
                    )
                    .put(
                        "measured_source_values_changed",
                        false,
                    )
                    .put(
                        "dense_micro_raster_materialized",
                        false,
                    )
                    .put(
                        "positions_between_measured_anchors_exist_as_coordinates",
                        true,
                    )
                    .put(
                        "positions_between_measured_anchors_have_measured_values",
                        false,
                    )
                    .put(
                        "unknown_positions_remain_unknown",
                        true,
                    )
                    .put(
                        "coordinate_precision_increases_optical_resolution",
                        false,
                    )
                    .put("upscaling_performed", false)
                    .put("interpolation_performed", false)
                    .put("new_measurements_created", false)
                    .put("noise_correction_enabled", false)
                    .put("distance_threshold_admitted", false)
                    .put("can_reduce_protection", false)
                    .put("can_enable_correction", false),
            )
            .put(
                "scientific_intent",
                "ALLOW_NOISE_STRUCTURE_AND_RECONSTRUCTION_REASONING_IN_A_COORDINATE_DOMAIN_FINER_THAN_THE_SOURCE_RASTER_WITHOUT_INVENTING_SOURCE_SAMPLES",
            )
            .put(
                "future_reconstruction_rule",
                "ONLY_VERSIONED_RECONSTRUCTION_MAY_ASSIGN_VALUES_TO_UNANCHORED_COORDINATES_WITH_EXPLICIT_PROVENANCE_AND_UNCERTAINTY",
            )
            .put(
                "single_observation_contract",
                JSONObject()
                    .put("source_observation_count", 1)
                    .put("selected_source_sha256", sourceSha256)
                    .put("other_physical_lenses_used", false)
                    .put("temporal_frames_used", false)
                    .put("burst_used", false)
                    .put("multi_observation_fusion_allowed", false),
            )
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("replacement_colour_estimated", false)
            .put("pixel_value_replacement_proposed", false)
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put(
                "schema",
                "D.RAW/Frontside/DarkChromaStability/0.7",
            )
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

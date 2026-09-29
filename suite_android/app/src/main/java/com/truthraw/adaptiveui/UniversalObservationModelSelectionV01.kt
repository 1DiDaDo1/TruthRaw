package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Universal, source-agnostic candidate-bank policy for the next local
 * reconstruction audit.
 *
 * Important:
 * - the original source raster remains the exact MEASURED anchor geometry;
 * - the source raster is NOT the resolution boundary of the scientific world;
 * - selection features are derived only from the current sealed observation;
 * - no lens, camera-model or vendor calibration is consulted;
 * - no held-out target value or hold-out error is used to choose candidates;
 * - this object never writes a reconstructed value into Scientific Master.
 *
 * This v0.1 policy does not declare a winning predictor. It narrows or expands
 * the deterministic model bank that a later native hold-out audit is allowed
 * to evaluate at a location.
 */
object UniversalObservationModelSelectionV01 {
    const val SCHEMA = "D.RAW/UniversalObservationModelSelection/0.1"

    private const val STRUCTURE_NEAR_SOURCE_PX = 4L
    private const val STRUCTURE_LOCAL_SOURCE_PX = 8L
    private const val CENSOR_GUARD_SOURCE_PX = 2L

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        base(sourceSha256)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("query_count", 0)
            .put("queries", JSONArray())

    fun analyze(
        sourceSha256: String,
        sampleLattice: JSONObject?,
        latticeGeometry: JSONObject?,
        anchorAudit: JSONObject?,
    ): JSONObject {
        if (
            sampleLattice == null ||
            sampleLattice.optString("schema") !=
                RasterIndependentSampleLatticeV01.SCHEMA ||
            sampleLattice.optString("source_sha256") != sourceSha256 ||
            sampleLattice.optString("status") != "AVAILABLE"
        ) {
            return unavailable(sourceSha256, "SAMPLE_LATTICE_NOT_AVAILABLE")
        }

        if (
            latticeGeometry == null ||
            latticeGeometry.optString("schema") !=
                "D.RAW/N2RasterIndependentSampleGeometry/0.1" ||
            latticeGeometry.optString("source_sha256") != sourceSha256 ||
            latticeGeometry.optString("status") !=
                "AUDIT_ONLY_LATTICE_BINDING_AVAILABLE"
        ) {
            return unavailable(
                sourceSha256,
                "RASTER_INDEPENDENT_SUPPORT_GEOMETRY_NOT_AVAILABLE",
            )
        }

        if (
            anchorAudit == null ||
            anchorAudit.optString("schema") !=
                "D.RAW/Frontside/AnchorConstrainedLocalReconstruction/0.1" ||
            anchorAudit.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(
                sourceSha256,
                "ANCHOR_RECONSTRUCTION_AUDIT_BINDING_MISMATCH",
            )
        }

        val units =
            sampleLattice.optLong(
                "coordinate_units_per_source_pixel",
                RasterIndependentSampleLatticeV01.UNITS_PER_SOURCE_PIXEL,
            )
        if (units <= 0L) {
            return unavailable(sourceSha256, "INVALID_LATTICE_SCALE")
        }

        val sourceQueries =
            latticeGeometry.optJSONArray("queries") ?: JSONArray()
        if (sourceQueries.length() <= 0) {
            return unavailable(sourceSha256, "NO_LATTICE_QUERIES")
        }

        val anchorStatus = anchorAudit.optString("status")
        val priorHoldoutAvailable =
            anchorStatus == "AUDIT_ONLY_HOLDOUT_VALIDATION_AVAILABLE"

        val outQueries = JSONArray()
        for (i in 0 until sourceQueries.length()) {
            val q = sourceQueries.optJSONObject(i) ?: continue
            val id = q.optInt("id", -1)
            if (id < 0) continue

            val nearestStructureCenter =
                distanceUnits(q.opt("nearest_structure_from_center"))
            val nearestStructureRect =
                distanceUnits(q.opt("nearest_structure_to_rect"))
            val nearestCensoredRect =
                distanceUnits(q.opt("nearest_censored_to_rect"))
            val nearestBoundaryRect =
                distanceUnits(q.opt("nearest_censor_boundary_to_rect"))
            val r8StructureFraction =
                firstFiniteFraction(
                    q.optJSONArray("center_structure_fraction"),
                )

            val censorGuardUnits =
                Math.multiplyExact(CENSOR_GUARD_SOURCE_PX, units)
            val structureNearUnits =
                Math.multiplyExact(STRUCTURE_NEAR_SOURCE_PX, units)
            val structureLocalUnits =
                Math.multiplyExact(STRUCTURE_LOCAL_SOURCE_PX, units)

            val censorConstrained =
                (nearestCensoredRect != null &&
                    nearestCensoredRect <= censorGuardUnits) ||
                    (nearestBoundaryRect != null &&
                        nearestBoundaryRect <= censorGuardUnits)

            val regime = when {
                censorConstrained ->
                    "CENSOR_CONSTRAINED"

                nearestStructureRect != null &&
                    nearestStructureRect <= structureNearUnits ->
                    "STRUCTURE_NEAR"

                nearestStructureCenter != null &&
                    nearestStructureCenter > structureLocalUnits &&
                    r8StructureFraction != null &&
                    r8StructureFraction == 0.0 ->
                    "STRUCTURE_SPARSE_LOCAL"

                nearestStructureCenter != null ||
                    r8StructureFraction != null ->
                    "STRUCTURE_MIXED_OR_SUPPORTED"

                else ->
                    "SUPPORT_UNKNOWN"
            }

            val candidates = JSONArray()
            when (regime) {
                "CENSOR_CONSTRAINED" -> {
                    candidates.put("NO_RECONSTRUCTION_CANDIDATE")
                    candidates.put("CENTER_EXCLUDED_MULTISCALE_REFERENCE")
                }

                "STRUCTURE_NEAR" -> {
                    candidates.put("CENTER_EXCLUDED_MULTISCALE_REFERENCE")
                    candidates.put("DIRECTIONAL_LINE_RESEARCH_CANDIDATE")
                    candidates.put("NO_RECONSTRUCTION_CANDIDATE")
                }

                "STRUCTURE_SPARSE_LOCAL" -> {
                    candidates.put("ROBUST_LOCAL_CONSTANT_RESEARCH_CANDIDATE")
                    candidates.put("AFFINE_PLANE_V0_1_RESEARCH_CANDIDATE")
                    candidates.put("CENTER_EXCLUDED_MULTISCALE_REFERENCE")
                    candidates.put("NO_RECONSTRUCTION_CANDIDATE")
                }

                "STRUCTURE_MIXED_OR_SUPPORTED" -> {
                    candidates.put("CENTER_EXCLUDED_MULTISCALE_REFERENCE")
                    candidates.put("DIRECTIONAL_LINE_RESEARCH_CANDIDATE")
                    candidates.put("AFFINE_PLANE_V0_1_RESEARCH_CANDIDATE")
                    candidates.put("LOW_ORDER_CURVATURE_RESEARCH_CANDIDATE")
                    candidates.put("NO_RECONSTRUCTION_CANDIDATE")
                }

                else -> {
                    candidates.put("CENTER_EXCLUDED_MULTISCALE_REFERENCE")
                    candidates.put("NO_RECONSTRUCTION_CANDIDATE")
                }
            }

            outQueries.put(
                JSONObject()
                    .put("id", id)
                    .put(
                        "source_rect",
                        q.optJSONArray("source_rect") ?: JSONArray(),
                    )
                    .put(
                        "lattice_rect_units",
                        q.optJSONArray("lattice_rect_units") ?: JSONArray(),
                    )
                    .put(
                        "lattice_center_units",
                        q.optJSONArray("lattice_center_units") ?: JSONArray(),
                    )
                    .put(
                        "local_evidence_regime",
                        regime,
                    )
                    .put(
                        "nearest_structure_from_center_lattice_units",
                        nearestStructureCenter ?: JSONObject.NULL,
                    )
                    .put(
                        "nearest_structure_to_rect_lattice_units",
                        nearestStructureRect ?: JSONObject.NULL,
                    )
                    .put(
                        "nearest_censored_to_rect_lattice_units",
                        nearestCensoredRect ?: JSONObject.NULL,
                    )
                    .put(
                        "nearest_censor_boundary_to_rect_lattice_units",
                        nearestBoundaryRect ?: JSONObject.NULL,
                    )
                    .put(
                        "r8_structure_fraction",
                        r8StructureFraction ?: JSONObject.NULL,
                    )
                    .put(
                        "eligible_model_bank",
                        candidates,
                    )
                    .put(
                        "single_winner_predeclared",
                        false,
                    )
                    .put(
                        "heldout_target_used_for_selection",
                        false,
                    )
                    .put(
                        "holdout_error_used_for_selection",
                        false,
                    )
                    .put(
                        "lens_calibration_used",
                        false,
                    )
                    .put(
                        "camera_model_used",
                        false,
                    )
                    .put(
                        "vendor_mapping_used",
                        false,
                    )
                    .put(
                        "selection_applied_to_scientific_master",
                        false,
                    ),
            )
        }

        if (outQueries.length() <= 0) {
            return unavailable(sourceSha256, "NO_VALID_MODEL_SELECTION_QUERIES")
        }

        return base(sourceSha256)
            .put("status", "PROSPECTIVE_AUDIT_POLICY_AVAILABLE")
            .put(
                "authority",
                "OBSERVATION_DERIVED_MODEL_BANK_POLICY_AUDIT_ONLY",
            )
            .put(
                "source_raster_role",
                "EXACT_MEASURED_ANCHOR_GEOMETRY_AND_FULL_RESOLUTION_SOURCE_SUPPORT",
            )
            .put("source_raster_used_only_for_noise", false)
            .put(
                "scientific_solution_domain",
                "RASTER_INDEPENDENT_SPARSE_FIXED_POINT_LATTICE",
            )
            .put(
                "source_raster_is_world_resolution_authority",
                false,
            )
            .put(
                "unanchored_lattice_positions_begin_unknown",
                true,
            )
            .put(
                "frontside_and_backside_observation_features_only",
                true,
            )
            .put("lens_calibration_used", false)
            .put("camera_model_used", false)
            .put("vendor_mapping_used", false)
            .put("heldout_target_used_for_selection", false)
            .put("holdout_error_used_for_selection", false)
            .put(
                "prior_anchor_holdout_available",
                priorHoldoutAvailable,
            )
            .put(
                "existing_2026_09_29_holdout_is_development_evidence_only",
                true,
            )
            .put(
                "independent_new_capture_required_for_selector_validation",
                true,
            )
            .put("query_count", outQueries.length())
            .put("queries", outQueries)
            .put("affine_plane_promoted", false)
            .put("winner_declared", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun base(sourceSha256: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("source_sha256", sourceSha256)
            .put("device_specific_mapping_used", false)
            .put("lens_specific_calibration_required", false)
            .put("ai_or_learned_models_used", false)
            .put("heldout_target_used_for_selection", false)
            .put("holdout_error_used_for_selection", false)
            .put("measured_anchors_modified", false)
            .put("unanchored_values_promoted_to_measured", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun distanceUnits(value: Any?): Long? {
        val o = value as? JSONObject ?: return null
        if (!o.has("distance_lattice_units") ||
            o.isNull("distance_lattice_units")
        ) {
            return null
        }
        val v = o.optLong("distance_lattice_units", Long.MIN_VALUE)
        return v.takeIf { it != Long.MIN_VALUE && it >= 0L }
    }

    private fun firstFiniteFraction(values: JSONArray?): Double? {
        if (values == null || values.length() <= 0) return null
        val v = values.optDouble(0, Double.NaN)
        return v.takeIf { it.isFinite() && it >= 0.0 && it <= 1.0 }
    }
}

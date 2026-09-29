package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Universal Multi-Observation Relation Protocol v0.1.
 *
 * This protocol never identifies a camera or lens by name. It defines how
 * multiple sealed observations may be related without silently merging their
 * evidence or upgrading authority.
 *
 * A relation is a separate scientific object. Two observations remain two
 * immutable evidence roots.
 */
object UniversalMultiObservationRelationProtocolV01 {
    const val SCHEMA = "D.RAW/UniversalMultiObservationRelationProtocol/0.1"

    private val allowedRelationEvidenceClasses = setOf(
        "SEALED_CAPTURE_SESSION_PROVENANCE",
        "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
        "OBSERVATION_REPEATABILITY_CANDIDATE",
        "USER_GROUPING_HINT_ONLY",
        "NONE",
    )

    fun protocol(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "FROZEN_PROTOCOL_AVAILABLE")
            .put(
                "law",
                "RELATE_OBSERVATIONS_WITH_EXPLICIT_EVIDENCE_NEVER_BY_CAMERA_OR_LENS_NAME",
            )
            .put(
                "observation_identity",
                JSONObject()
                    .put("source_sha256_is_primary_observation_root", true)
                    .put("distinct_source_sha_required_for_distinct_observations", true)
                    .put("physical_frame_count_may_not_be_inferred_from_derived_views", true)
                    .put("independent_evidence_count_may_not_be_upgraded_by_relation", true)
                    .put("relation_merges_source_evidence", false),
            )
            .put(
                "identity_independence",
                JSONObject()
                    .put("camera_model_name_may_prove_relation", false)
                    .put("lens_model_name_may_prove_relation", false)
                    .put("vendor_name_may_prove_relation", false)
                    .put("file_extension_may_prove_relation", false)
                    .put("focal_length_metadata_may_prove_relation", false)
                    .put("visual_similarity_may_prove_relation", false)
                    .put("device_specific_profile_key_allowed", false),
            )
            .put(
                "relation_evidence_classes",
                JSONArray()
                    .put(relationClass(
                        "SEALED_CAPTURE_SESSION_PROVENANCE",
                        "PROVENANCE_BOUND_RELATION",
                        "Same acquisition session/route is bound by D.RAW provenance rather than inferred from brand/model metadata.",
                        true,
                    ))
                    .put(relationClass(
                        "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
                        "CALIBRATION_PROVENANCE_BOUND_RELATION",
                        "A versioned calibration capture record explicitly binds source SHA-256 values, target/setup state and capture roles.",
                        true,
                    ))
                    .put(relationClass(
                        "OBSERVATION_REPEATABILITY_CANDIDATE",
                        "OBSERVATION_DERIVED_RELATION_CANDIDATE",
                        "Repeated structure/field behaviour may justify a candidate relation but does not prove same physical camera/lens identity.",
                        false,
                    ))
                    .put(relationClass(
                        "USER_GROUPING_HINT_ONLY",
                        "NON_AUTHORITY_GROUPING_HINT",
                        "User may group observations for an experiment, but grouping alone is not scientific proof of a shared capture system.",
                        false,
                    ))
                    .put(relationClass(
                        "NONE",
                        "RELATION_UNPROVEN",
                        "No admitted relation evidence is present.",
                        false,
                    )),
            )
            .put(
                "relation_edge_contract",
                JSONObject()
                    .put("edge_is_separate_from_observation_nodes", true)
                    .put("edge_must_list_source_sha256_roots", true)
                    .put("edge_must_list_relation_evidence_class", true)
                    .put("edge_must_list_axis_scope", true)
                    .put("edge_must_list_uncertainty_or_unknowns", true)
                    .put("edge_must_be_versioned", true)
                    .put("edge_may_modify_source_bytes", false)
                    .put("edge_may_modify_measured_anchors", false)
                    .put("edge_may_create_new_measured_samples", false)
                    .put("edge_may_write_scientific_master", false),
            )
            .put(
                "axis_scopes",
                JSONArray()
                    .put(axis(
                        "FIELD_RESPONSE_REPEATABILITY",
                        "Repeatable scene/lens/sensor field behaviour candidate. Lens-only vignetting remains unproven unless separately isolated.",
                    ))
                    .put(axis(
                        "COLOUR_RELATION",
                        "Empirical camera-channel/colorimetric relation. White balance and visible preview RGB are insufficient.",
                    ))
                    .put(axis(
                        "OPTICAL_SUPPORT_RELATION",
                        "SFR/MTF/PSF or chromatic/focus support relation. Output raster density does not establish this axis.",
                    ))
                    .put(axis(
                        "DARK_NOISE_OFFSET_RELATION",
                        "Offset/noise relation. May refine uncertainty only after independent dark/noise observations.",
                    ))
                    .put(axis(
                        "TEMPORAL_CAPTURE_RELATION",
                        "Physical timing/motion relation. Synthetic or copied frames do not create a temporal evidence edge.",
                    )),
            )
            .put(
                "set_level_minimums",
                JSONObject()
                    .put(
                        "field_repeatability_candidate",
                        JSONObject()
                            .put("minimum_independent_observations", 3)
                            .put("same_scene_required", false)
                            .put("scene_variation_preferred", true)
                            .put("camera_or_lens_name_used_as_key", false)
                            .put("lens_only_claim_allowed", false),
                    )
                    .put(
                        "flat_field_rotation_separation_candidate",
                        JSONObject()
                            .put("minimum_orientations", 4)
                            .put(
                                "preferred_orientation_degrees",
                                JSONArray().put(0).put(90).put(180).put(270),
                            )
                            .put("rotation_relation_must_be_recorded", true)
                            .put("automatic_vignetting_correction_allowed", false),
                    )
                    .put(
                        "colour_multi_illuminant_candidate",
                        JSONObject()
                            .put("minimum_characterized_illuminants", 2)
                            .put("reference_target_required", true)
                            .put("held_out_validation_required", true)
                            .put("spectral_truth_claim_from_three_channels_allowed", false)
                            .put("automatic_colour_correction_allowed", false),
                    )
                    .put(
                        "optical_support_candidate",
                        JSONObject()
                            .put("radial_and_tangential_sampling_required_for_directional_claim", true)
                            .put("multiple_field_positions_required", true)
                            .put("focus_state_must_be_recorded_when_known", true)
                            .put("deconvolution_authorized", false),
                    )
                    .put(
                        "dark_noise_candidate",
                        JSONObject()
                            .put("independent_dark_or_neutral_observations_required", true)
                            .put("uncertainty_refinement_only", true)
                            .put("measured_anchor_rewrite_allowed", false),
                    )
                    .put(
                        "temporal_candidate",
                        JSONObject()
                            .put("physical_capture_timing_relation_required", true)
                            .put("virtual_or_derived_frames_count_as_independent_evidence", false)
                            .put("synthetic_motion_blur_authority", "APPEARANCE_ONLY"),
                    ),
            )
            .put(
                "promotion_boundary",
                JSONObject()
                    .put("relation_protocol_itself_promotes_calibration", false)
                    .put("relation_protocol_itself_authorizes_correction", false)
                    .put("relation_protocol_itself_authorizes_deconvolution", false)
                    .put("relation_protocol_itself_authorizes_scientific_writeback", false)
                    .put("axis_specific_validation_required_after_relation_admission", true)
                    .put("unknown_must_remain_unknown_until_axis_specific_gate_passes", true),
            )
            .put("uses_ai_or_learned_model", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    fun evaluatePair(
        leftAtlas: JSONObject,
        rightAtlas: JSONObject,
        relationEvidence: JSONObject? = null,
    ): JSONObject {
        val leftSha = leftAtlas.optString("source_sha256")
        val rightSha = rightAtlas.optString("source_sha256")
        val evidenceClass =
            relationEvidence
                ?.optString("evidence_class", "NONE")
                ?.takeIf { it in allowedRelationEvidenceClasses }
                ?: "NONE"

        val distinctRoots =
            leftSha.isNotBlank() &&
                rightSha.isNotBlank() &&
                leftSha != rightSha

        val bothAtlas =
            leftAtlas.optString("schema") ==
                "D.RAW/UniversalObservationCalibrationAtlas/0.1" &&
                rightAtlas.optString("schema") ==
                "D.RAW/UniversalObservationCalibrationAtlas/0.1" &&
                leftAtlas.optString("status") == "OBSERVATION_ATLAS_AVAILABLE" &&
                rightAtlas.optString("status") == "OBSERVATION_ATLAS_AVAILABLE"

        val relationAuthority = when (evidenceClass) {
            "SEALED_CAPTURE_SESSION_PROVENANCE" ->
                "PROVENANCE_BOUND_RELATION"
            "EXPLICIT_CALIBRATION_CAPTURE_RECORD" ->
                "CALIBRATION_PROVENANCE_BOUND_RELATION"
            "OBSERVATION_REPEATABILITY_CANDIDATE" ->
                "OBSERVATION_DERIVED_RELATION_CANDIDATE"
            "USER_GROUPING_HINT_ONLY" ->
                "NON_AUTHORITY_GROUPING_HINT"
            else ->
                "RELATION_UNPROVEN"
        }

        val relationCanSupportCalibrationCandidate =
            bothAtlas &&
                distinctRoots &&
                (
                    evidenceClass == "SEALED_CAPTURE_SESSION_PROVENANCE" ||
                        evidenceClass == "EXPLICIT_CALIBRATION_CAPTURE_RECORD"
                    )

        return JSONObject()
            .put("schema", "D.RAW/UniversalMultiObservationRelation/0.1")
            .put(
                "status",
                if (bothAtlas && distinctRoots) {
                    "PAIR_RELATION_EVALUATED"
                } else {
                    "PAIR_RELATION_FAIL_CLOSED"
                },
            )
            .put(
                "observation_roots",
                JSONArray()
                    .put(leftSha.ifBlank { JSONObject.NULL })
                    .put(rightSha.ifBlank { JSONObject.NULL }),
            )
            .put("distinct_observation_roots", distinctRoots)
            .put("both_atlas_inputs_admitted", bothAtlas)
            .put("relation_evidence_class", evidenceClass)
            .put("relation_authority", relationAuthority)
            .put(
                "relation_can_support_axis_specific_calibration_candidate",
                relationCanSupportCalibrationCandidate,
            )
            .put("same_physical_camera_proven", false)
            .put("same_physical_lens_proven", false)
            .put("camera_model_name_used", false)
            .put("lens_model_name_used", false)
            .put("vendor_mapping_used", false)
            .put("source_evidence_merged", false)
            .put("independent_evidence_count_upgraded", false)
            .put("creates_new_evidence", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun relationClass(
        id: String,
        authority: String,
        description: String,
        canSupportAxisSpecificCalibrationCandidate: Boolean,
    ): JSONObject =
        JSONObject()
            .put("id", id)
            .put("authority", authority)
            .put("description", description)
            .put(
                "can_support_axis_specific_calibration_candidate",
                canSupportAxisSpecificCalibrationCandidate,
            )
            .put("same_physical_camera_proven", false)
            .put("same_physical_lens_proven", false)

    private fun axis(
        id: String,
        description: String,
    ): JSONObject =
        JSONObject()
            .put("id", id)
            .put("description", description)
            .put("relation_alone_promotes_authority", false)
            .put("relation_alone_authorizes_correction", false)
}

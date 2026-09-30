package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Machine-readable checkpoint of implemented Free World capabilities versus
 * scientifically promoted capabilities.
 *
 * It exists so later development cannot confuse "code exists" with
 * "physical interpretation validated".
 */
object FreeWorldCapabilityMatrixV01 {
    const val SCHEMA = "D.RAW/FreeWorldCapabilityMatrix/0.1"

    fun describe(
        graph: JSONObject,
        tracks: JSONObject,
        cycles: JSONObject,
        decomposition: JSONObject,
    ): JSONObject {
        val capabilities = JSONArray()

        fun capability(
            id: String,
            implementationStatus: String,
            scientificStatus: String,
            activeUseAllowed: Boolean,
            validationRequired: Boolean,
        ) {
            capabilities.put(
                JSONObject()
                    .put("id", id)
                    .put(
                        "implementation_status",
                        implementationStatus,
                    )
                    .put(
                        "scientific_status",
                        scientificStatus,
                    )
                    .put(
                        "active_scientific_use_allowed",
                        activeUseAllowed,
                    )
                    .put(
                        "physical_validation_required",
                        validationRequired,
                    ),
            )
        }

        capability(
            "UNIVERSAL_IDENTITY_INDEPENDENCE",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "RAW_LENS_CAMERA_VENDOR_IDENTITY_CANNOT_SELECT_SCIENTIFIC_TRUTH",
            true,
            false,
        )
        capability(
            "LOCAL_FEATURE_GEOMETRY",
            "IMPLEMENTED",
            "APPEARANCE_DERIVED_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "PAIR_GEOMETRY",
            "IMPLEMENTED",
            "AFFINE_APPEARANCE_HYPOTHESIS_ONLY",
            false,
            true,
        )
        capability(
            "PAIR_GEOMETRY_MODEL_BANK",
            "IMPLEMENTED",
            "TRANSLATION_SIMILARITY_AFFINE_HOMOGRAPHY_CANDIDATES_NO_WINNER",
            false,
            true,
        )
        capability(
            "MULTI_OBSERVATION_FEATURE_TRACKS",
            "IMPLEMENTED",
            "TRACK_HYPOTHESES_ONLY",
            false,
            true,
        )
        capability(
            "GRAPH_CYCLE_CONSISTENCY",
            "IMPLEMENTED",
            "DESCRIPTIVE_DIAGNOSTIC_ONLY",
            false,
            true,
        )
        capability(
            "RELATIVE_WORLD_GRAPH_GAUGE",
            "IMPLEMENTED",
            "NUMERIC_GAUGE_ONLY",
            false,
            true,
        )
        capability(
            "WORLD_SENSOR_FIELD_DECOMPOSITION",
            "SCAFFOLD_IMPLEMENTED",
            "NOT_ESTIMATED",
            false,
            true,
        )
        capability(
            "NATURAL_SELF_CALIBRATION",
            "ATLAS_IMPLEMENTED",
            "NO_AXIS_PROMOTED",
            false,
            true,
        )
        capability(
            "PHYSICAL_OBSERVATION_NOISE_CONTEXT",
            "IMPLEMENTED",
            "SOURCE_BOUND_CONTEXT_ONLY_NO_NOISE_CLASSIFICATION",
            true,
            false,
        )
        capability(
            "RADIOMETRIC_RESPONSE",
            "RUNTIME_FOUNDATION_IMPLEMENTED",
            "METADATA_CONTEXT_ONLY_NO_OECF_OR_LINEARITY_PROMOTION",
            false,
            true,
        )
        capability(
            "NOISE_COMPONENT_DECOMPOSITION",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "COMPONENTS_DECLARED_NO_DECOMPOSITION_PROMOTED",
            false,
            true,
        )
        capability(
            "SCIENTIFIC_NOISE_TRANSPORT",
            "DETERMINISTIC_NUMERIC_PRIMITIVES_IMPLEMENTED",
            "COVARIANCE_MATH_AVAILABLE_BUT_NOT_PHYSICALLY_VALIDATED_OR_APPLIED",
            false,
            true,
        )
        capability(
            "RADIOMETRIC_RESPONSE_CANDIDATE_SOLVER",
            "NUMERIC_CANDIDATE_SOLVER_IMPLEMENTED",
            "LINEAR_AND_PIECEWISE_RESPONSE_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "NOISE_COMPONENT_CANDIDATE_SOLVER",
            "NUMERIC_CANDIDATE_SOLVER_IMPLEMENTED",
            "SHOT_READ_AND_FIXED_PATTERN_SUMMARY_CANDIDATES_ONLY",
            false,
            true,
        )
        capability(
            "FIELD_RESPONSE_ROTATION_SEPARATION",
            "NUMERIC_CANDIDATE_SOLVER_IMPLEMENTED",
            "WORLD_SENSOR_ADDITIVE_CANDIDATE_ONLY_NO_VIGNETTING_CLAIM",
            false,
            true,
        )
        capability(
            "OPTICAL_SUPPORT",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "NO_MEASURED_SFR_MTF_PSF_ATTACHED",
            false,
            true,
        )
        capability(
            "OPTICAL_SUPPORT_MEASUREMENT_CANDIDATE",
            "NUMERIC_CANDIDATE_RUNTIME_IMPLEMENTED",
            "SFR_MTF_PSF_INPUT_SUMMARY_NO_CALIBRATION_PROMOTION",
            false,
            true,
        )
        capability(
            "NOISE_AWARE_INVERSE_OPTICS_CANDIDATE",
            "NUMERIC_CANDIDATE_RUNTIME_IMPLEMENTED",
            "FREQUENCY_GAINS_ONLY_NO_IMAGE_TRANSFORM_OR_DECONVOLUTION_AUTHORITY",
            false,
            true,
        )
        capability(
            "COLOUR_RELATION",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "NO_EMPIRICAL_OR_SPECTRAL_PROMOTION",
            false,
            true,
        )
        capability(
            "COLOUR_RELATION_CANDIDATE_SOLVER",
            "NUMERIC_3X3_FIT_IMPLEMENTED",
            "MULTI_ILLUMINANT_HELD_OUT_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "TEMPORAL_SEQUENCE_CANDIDATE_SOLVER",
            "NUMERIC_RELATION_RUNTIME_IMPLEMENTED",
            "EXPLICIT_ORDER_AND_READOUT_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "GEOMETRY_DEPTH_VISIBILITY",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "2D_APPEARANCE_GEOMETRY_ONLY_NO_3D_PROMOTION",
            false,
            true,
        )
        capability(
            "GEOMETRY_DEPTH_CANDIDATE_SOLVER",
            "RAY_TRIANGULATION_CANDIDATE_RUNTIME_IMPLEMENTED",
            "POSE_ADMITTED_GEOMETRY_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "LIGHT_TRANSPORT_AUTHORITY",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "MATERIAL_ILLUMINATION_GEOMETRY_REMAIN_UNPROMOTED",
            false,
            true,
        )
        capability(
            "MULTI_OBSERVATION_RESIDUAL_RELATION",
            "SCAFFOLD_IMPLEMENTED",
            "NO_RESIDUALS_COMPUTED_NO_WORLD_SOURCE_RELATION_PROMOTED",
            false,
            true,
        )
        capability(
            "SCIENTIFIC_DENOISE_ADMISSION",
            "FAIL_CLOSED_POLICY_IMPLEMENTED",
            "ALL_SCIENTIFIC_DENOISE_ROUTES_BLOCKED_UNTIL_AXIS_GATES_PASS",
            false,
            true,
        )
        capability(
            "WORLD_SPACE_RESIDUAL_CANDIDATE_SOLVER",
            "NUMERIC_CANDIDATE_RUNTIME_IMPLEMENTED",
            "WORLD_SENSOR_RESIDUAL_CANDIDATES_NO_PROMOTION",
            false,
            true,
        )
        capability(
            "SCIENTIFIC_RECONSTRUCTION_CANDIDATE",
            "UNCERTAINTY_WEIGHTED_RUNTIME_IMPLEMENTED",
            "EXACT_ANCHOR_PASSTHROUGH_OR_RECONSTRUCTED_CANDIDATE_ONLY",
            false,
            true,
        )
        capability(
            "SCIENTIFIC_DENOISE_OPERATOR",
            "GATED_DERIVED_OUTPUT_OPERATOR_IMPLEMENTED",
            "BLOCKED_UNTIL_ROUTE_SPECIFIC_PROMOTION",
            false,
            true,
        )
        capability(
            "WORLD_SPACE_NOISE_SEPARATION",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "NO_WORLD_SENSOR_TEMPORAL_RESIDUAL_DECOMPOSITION_PERFORMED",
            false,
            true,
        )
        capability(
            "PERCEPTUAL_NOISE_VISIBILITY_CANDIDATE",
            "VISUAL_ANGLE_RUNTIME_IMPLEMENTED",
            "APPEARANCE_ONLY_NO_HVS_CURVE_INVENTED",
            true,
            false,
        )
        capability(
            "PERCEPTUAL_NOISE_APPEARANCE",
            "DOWNSTREAM_CONTRACT_IMPLEMENTED",
            "APPEARANCE_ONLY",
            true,
            false,
        )
        capability(
            "TEMPORAL_MULTIVIEW",
            "SCAFFOLD_IMPLEMENTED",
            "NO_PHYSICAL_SEQUENCE_RELATION_PROMOTED",
            false,
            true,
        )
        capability(
            "SOURCE_LATTICE_EXACT_ANCHOR_QUERY",
            "IMPLEMENTED",
            "MEASURED_ONLY_AT_EXACT_ADMITTED_ANCHORS",
            true,
            false,
        )
        capability(
            "FREE_WORLD_CONTINUOUS_QUERY",
            "TYPED_ABI_AND_FAIL_CLOSED_RUNTIME_IMPLEMENTED",
            "UNKNOWN_UNTIL_WORLD_SOURCE_BRIDGE_AND_SOLVER_ADMITTED",
            false,
            true,
        )
        capability(
            "RESTORATION_AUTHORITY",
            "RUNTIME_CONTRACT_IMPLEMENTED",
            "NO_RESTORATION_APPLIED",
            false,
            true,
        )
        capability(
            "VIEW_APPEARANCE",
            "DOWNSTREAM_BOUNDARY_IMPLEMENTED",
            "APPEARANCE_ONLY",
            true,
            false,
        )
        capability(
            "RESEARCH_PROMOTION_FIREWALL",
            "IMPLEMENTED",
            "ENFORCED_ON_FOUNDATION_EXPORT",
            true,
            false,
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "CAPABILITY_MATRIX_AVAILABLE",
            )
            .put(
                "observation_graph_candidate_edges",
                graph.optInt(
                    "geometry_candidate_edge_count",
                    0,
                ),
            )
            .put(
                "feature_track_hypothesis_count",
                tracks.optInt("track_count", 0),
            )
            .put(
                "closed_cycle_count",
                cycles.optInt(
                    "closed_triangle_count",
                    0,
                ),
            )
            .put(
                "decomposition_world_component_estimated",
                decomposition.optBoolean(
                    "world_fixed_component_estimated",
                    false,
                ),
            )
            .put(
                "decomposition_sensor_component_estimated",
                decomposition.optBoolean(
                    "sensor_fixed_component_estimated",
                    false,
                ),
            )
            .put("capabilities", capabilities)
            .put(
                "global_law",
                JSONObject()
                    .put(
                        "implemented_does_not_mean_validated",
                        true,
                    )
                    .put(
                        "validated_does_not_mean_measured",
                        true,
                    )
                    .put(
                        "representation_freedom_does_not_upgrade_authority",
                        true,
                    )
                    .put(
                        "unknown_is_valid_state",
                        true,
                    ),
            )
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("deconvolution_authorized", false)
            .put("scientific_writeback_allowed", false)
            .put("creates_new_evidence", false)
    }
}

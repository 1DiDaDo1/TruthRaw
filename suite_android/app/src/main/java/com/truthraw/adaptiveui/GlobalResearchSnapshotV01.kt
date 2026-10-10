package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import java.time.Instant

/**
 * Source-independent implementation/navigation snapshot.
 *
 * It contains no photo measurement and must never be interpreted as
 * calibration evidence or a session capability result.
 */
object GlobalResearchSnapshotV01 {
    const val SCHEMA = "D.RAW/GlobalResearchSnapshot/0.1"

    fun build(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("generated_at_utc", Instant.now().toString())
            .put("status", "IMPLEMENTATION_MAP_NOT_PHOTO_EVIDENCE")
            .put(
                "normal_workflow",
                JSONArray()
                    .put("CHOOSE_OR_KEEP_ROUTE")
                    .put("OPEN_RAW_OR_DNG_OR_USE_UNIVERSAL_CAMERA")
                    .put("SEAL_SOURCE")
                    .put("UNIVERSAL_INTAKE")
                    .put("SCIENTIFIC_MASTER")
                    .put("ROUTE_SPECIFIC_VIEW_OR_EXPORT"),
            )
            .put(
                "routes",
                JSONObject()
                    .put(
                        "PURE",
                        JSONObject()
                            .put("role", "SCIENTIFIC_VIEW")
                            .put("stronger_evidence_than_other_routes", false),
                    )
                    .put(
                        "ADVANCED",
                        JSONObject()
                            .put(
                                "role",
                                "APPEARANCE_AND_RESTORATION_VIEW",
                            )
                            .put(
                                "appearance_may_write_scientific_master",
                                false,
                            ),
                    )
                    .put(
                        "PRO",
                        JSONObject()
                            .put(
                                "role",
                                "OPEN_SCENE_RESEARCH_AND_PROFESSIONAL_WORKBENCH",
                            )
                            .put("stronger_evidence_than_pure", false),
                    ),
            )
            .put(
                "research_workflow",
                JSONArray()
                    .put("SELECT_ONE_OR_MORE_SEALED_OBSERVATIONS")
                    .put("ANALYSE_ALL_SELECTED_SOURCES_UNIVERSALLY")
                    .put(
                        "OPTIONALLY_IMPORT_RELATION_BASED_CALIBRATION_OBSERVATION_RECORDS",
                    )
                    .put(
                        "EXPORT_OBSERVATION_ATLAS_OR_FIELD_CHART_WHEN_NEEDED",
                    )
                    .put(
                        "EXPORT_FREE_WORLD_FOUNDATION_FOR_SESSION_CANDIDATE_STATE",
                    )
                    .put(
                        "VALIDATE_AXIS_SPECIFIC_CANDIDATES_BEFORE_PROMOTION",
                    ),
            )
            .put(
                "implemented_candidate_runtime",
                JSONArray()
                    .put("RADIOMETRIC_RESPONSE")
                    .put("RADIOMETRIC_RELIABILITY_ENVELOPE")
                    .put("NOISE_COMPONENT_DECOMPOSITION")
                    .put("SPARSE_CFA_REPEATED_OBSERVATION_NOISE")
                    .put("NOISE_SPECTRUM_NPS")
                    .put("FIELD_RESPONSE_WORLD_SENSOR_SEPARATION")
                    .put("OPTICAL_SUPPORT_SFR_MTF_PSF")
                    .put("NOISE_OPTICS_JOINT_CANDIDATE")
                    .put("NOISE_AWARE_INVERSE_OPTICS")
                    .put("COLOUR_RELATION_MULTI_ILLUMINANT")
                    .put("COLOUR_COVARIANCE_J_C_JT_TRANSPORT")
                    .put("COLOUR_CALIBRATION_DOMAIN_AUTHORITY")
                    .put("TEMPORAL_SEQUENCE_AND_READOUT")
                    .put("GEOMETRY_DEPTH_VISIBILITY_CANDIDATES")
                    .put("WORLD_SPACE_RESIDUAL_SEPARATION")
                    .put("UNCERTAINTY_WEIGHTED_RECONSTRUCTION")
                    .put("MULTIDIMENSIONAL_MODEL_AUTHORITY_ADMISSION")
                    .put("TYPED_INTERNAL_PROMOTION_STATE")
                    .put("VALIDATED_WORLD_TO_SOURCE_PROMOTION_BRIDGE")
                    .put("GATED_SCIENTIFIC_DENOISE")
                    .put(
                        "PERCEPTUAL_NOISE_VISIBILITY_APPEARANCE",
                    ),
            )
            .put(
                "provenance_and_record_binding",
                JSONObject()
                    .put("physical_rawsensor_to_processing_dng_lineage", true)
                    .put("processing_dng_is_second_physical_frame", false)
                    .put("canonical_calibration_record_identity", true)
                    .put("records_must_bind_active_session_roots", true)
                    .put("physical_and_derived_alias_double_count_allowed", false)
                    .put("cross_session_candidate_record_use_allowed", false),
            )
            .put(
                "session_capability_matrix_location",
                "FreeWorldObservationGeometryFoundationV01 export",
            )
            .put(
                "promotion_gate_registry",
                ScientificPromotionGateRegistryV01.describe(),
            )
            .put(
                "universal_identity_independence",
                UniversalIdentityIndependenceV01.describe(),
            )
            .put(
                "colour_domain_authority",
                ColourDomainAuthorityV01.describe(),
            )
            .put(
                "prospective_model_authority_admission",
                UniversalObservationAuthorityAdmissionV02.describe(),
            )
            .put(
                "test_status_ui_contract",
                JSONObject()
                    .put("running_indicator", "GREEN_DOT")
                    .put("success_indicator", "GREEN_DOT")
                    .put("error_indicator", "RED_DOT")
                    .put("running_timer", "LIVE_ELAPSED")
                    .put("terminal_timer", "FINAL_DURATION"),
            )
            .put(
                "permanent_laws",
                JSONArray()
                    .put("MEASURED_NE_RECONSTRUCTED_NE_APPEARANCE")
                    .put("BLACK_LEVEL_NE_ZERO_LINE")
                    .put("FINITE_NE_PROVEN_RELIABLE")
                    .put("WHITE_LEVEL_NE_PROVEN_LINEARITY_BOUNDARY")
                    .put("MISSING_COVARIANCE_NE_ZERO_COVARIANCE")
                    .put("COVARIANCE_MUST_BE_POSITIVE_SEMIDEFINITE")
                    .put(
                        "OUTPUT_RASTER_DENSITY_NE_OPTICAL_RESOLUTION",
                    )
                    .put(
                        "CAMERA_LENS_VENDOR_RAW_IDENTITY_IS_NOT_SCIENTIFIC_MODEL_KEY",
                    )
                    .put("UNKNOWN_RESIDUAL_IS_NOT_AUTOMATICALLY_NOISE")
                    .put("KNOWN_EXPOSURE_DURATION_NE_KNOWN_ROW_TIMING")
                    .put("FIXED_3X3_NE_UNIVERSAL_SPECTRAL_TRUTH")
                    .put(
                        "REPRESENTATION_MAY_EXCEED_SOURCE_KNOWLEDGE_CLAIMS_MAY_NOT",
                    ),
            )
            .put(
                "validation_state",
                JSONObject()
                    .put(
                        "build_status_authority",
                        "EXTERNAL_CI_ARTIFACT_STATE_NOT_EMBEDDED_IN_SCIENTIFIC_SNAPSHOT",
                    )
                    .put(
                        "new_multidisciplinary_wave_device_validated",
                        false,
                    )
                    .put(
                        "candidate_availability_equals_promotion",
                        false,
                    )
                    .put(
                        "scientific_writeback_allowed_by_snapshot",
                        false,
                    ),
            )
            .put("contains_photo_measurement", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Universal Observation & Calibration Atlas v0.1.
 *
 * This layer is intentionally identity-agnostic. It never requires a camera
 * model, lens model, vendor map or prior calibration in order to accept and
 * describe one sealed observation.
 *
 * It combines what D.RAW can currently know from the back side of the source
 * with what the deterministic front-side inspector can see, while keeping
 * every authority axis separate. Optional future calibration observations may
 * strengthen individual axes only after an explicit relation is admitted.
 */
object UniversalObservationCalibrationAtlasV01 {
    const val SCHEMA = "D.RAW/UniversalObservationCalibrationAtlas/0.1"

    fun describe(
        sourceSha256: String,
        sourceClass: String,
        sourceRoute: String,
        metadata: JSONObject?,
        raster: JSONObject?,
        sampleLattice: JSONObject?,
        frontside: JSONObject?,
        backsideSignalSupport: JSONObject?,
        opticalFieldChart: JSONObject?,
    ): JSONObject {
        val m = metadata ?: JSONObject()
        val r = raster ?: JSONObject()
        val f = frontside ?: JSONObject()
        val b = backsideSignalSupport ?: JSONObject()
        val field = opticalFieldChart ?: JSONObject()
        val lattice = sampleLattice ?: JSONObject()

        val frontsideAvailable =
            f.optString("status") == "FRONTSIDE_STRUCTURAL_INSPECTION_AVAILABLE"
        val backsideMeasured =
            b.optString("status") == "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
        val fieldAvailable =
            field.optString("status") == "FIELD_CHART_AVAILABLE"

        val sourceMetadataColourPresent =
            present(m, "as_shot_neutral") ||
                m.optBoolean("color_matrix_1_present", false) ||
                m.optBoolean("color_matrix_2_present", false) ||
                m.optBoolean("camera_calibration_1_present", false) ||
                m.optBoolean("camera_calibration_2_present", false) ||
                m.optBoolean("forward_matrix_1_present", false) ||
                m.optBoolean("forward_matrix_2_present", false)

        val exposureSeconds = finitePositive(m, "exposure_time_seconds")
        val fNumber = finitePositive(m, "f_number")
        val iso = finitePositive(m, "iso")

        val optionalObservations = JSONArray()
            .put(optionalObservation(
                "FLAT_FIELD_RELATIVE_ILLUMINATION",
                "Repeatable camera-system field response; scene/lens/sensor separation requires independent observations.",
            ))
            .put(optionalObservation(
                "COLOUR_REFERENCE_MULTI_ILLUMINANT",
                "Empirical camera-channel to colorimetric relation with held-out validation; never required for universal intake.",
            ))
            .put(optionalObservation(
                "OPTICAL_SFR_MTF_PSF",
                "Independent radial/tangential optical-support evidence; output raster density remains separate.",
            ))
            .put(optionalObservation(
                "DARK_NOISE_OFFSET",
                "Independent offset/noise support; may refine uncertainty but cannot rewrite measured anchors.",
            ))
            .put(optionalObservation(
                "TEMPORAL_MOTION_FOOTPRINT",
                "Additional physical timing/motion observation; synthetic motion blur remains appearance-only.",
            ))

        val fieldSignal =
            field.optJSONObject("measured_composite_field_signal")
                ?: JSONObject().put("status", "UNKNOWN")

        val gainMapHint =
            field.optJSONObject("source_opcode_provenance_hint")
                ?.optJSONObject("opcode_list_2")
                ?: JSONObject().put("opcode_header_parse_status", "UNAVAILABLE")

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "OBSERVATION_ATLAS_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put("source_class", sourceClass)
            .put("source_route", sourceRoute)
            .put(
                "universal_identity_policy",
                JSONObject()
                    .put("camera_identity_required", false)
                    .put("lens_identity_required", false)
                    .put("vendor_identity_required", false)
                    .put("prior_user_calibration_required", false)
                    .put("raw_format_identity_may_route_decoder", true)
                    .put("raw_format_identity_may_define_scientific_truth", false)
                    .put("device_specific_map_required", false)
                    .put(
                        "law",
                        "OBSERVE_WHAT_IS_PRESENT_BEFORE_ASKING_WHO_PRODUCED_IT",
                    ),
            )
            .put(
                "backside",
                JSONObject()
                    .put("authority", "SOURCE_AND_MEASUREMENT_SIDE")
                    .put("measured_signal_available", backsideMeasured)
                    .put("measured_signal_status", b.optString("status", "UNKNOWN"))
                    .put("sample_lattice_status", lattice.optString("status", "UNKNOWN"))
                    .put("cfa_pattern_present", present(r, "cfa_pattern"))
                    .put("black_level_present", present(r, "black_level"))
                    .put("white_level_present", present(r, "white_level"))
                    .put("noise_profile_present", m.optBoolean("noise_profile_present", false))
                    .put("active_area_present", present(r, "active_area")),
            )
            .put(
                "frontside",
                JSONObject()
                    .put("authority", "APPEARANCE_DERIVED_ONLY")
                    .put("inspection_available", frontsideAvailable)
                    .put("status", f.optString("status", "UNKNOWN"))
                    .put(
                        "structural_centroid_normalized",
                        f.optJSONObject("proportions")
                            ?.opt("structural_centroid_normalized")
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "appearance_statistics",
                        f.optJSONObject("appearance_statistics") ?: JSONObject(),
                    )
                    .put(
                        "visible_colour_statistics",
                        f.optJSONObject("visible_colour_statistics") ?: JSONObject(),
                    )
                    .put("may_create_sensor_evidence", false)
                    .put("may_write_scientific_state", false),
            )
            .put(
                "colour_state",
                JSONObject()
                    .put(
                        "base_authority",
                        if (sourceMetadataColourPresent) {
                            "SOURCE_METADATA_BOUND_COLOUR_AVAILABLE"
                        } else {
                            "UNKNOWN_OR_UNCALIBRATED"
                        },
                    )
                    .put("source_metadata_bound_colour_available", sourceMetadataColourPresent)
                    .put("frontside_visible_colour_available", frontsideAvailable)
                    .put("frontside_used_as_colorimetric_calibration", false)
                    .put("empirical_multi_illuminant_calibration_attached", false)
                    .put("spectral_calibration_attached", false)
                    .put("white_balance_is_not_spectral_calibration", true)
                    .put("three_channel_rgb_proves_full_spectrum", false)
                    .put("automatic_colour_correction_from_atlas_allowed", false),
            )
            .put(
                "illumination_state",
                JSONObject()
                    .put("exposure_time_seconds", exposureSeconds ?: JSONObject.NULL)
                    .put("f_number", fNumber ?: JSONObject.NULL)
                    .put("iso", iso ?: JSONObject.NULL)
                    .put("capture_values_authority", "SOURCE_METADATA_PROVENANCE_WHEN_PRESENT")
                    .put("scene_light_kind", "UNKNOWN")
                    .put("spectral_power_distribution", "UNKNOWN")
                    .put("light_direction", "UNKNOWN")
                    .put("spatial_extent", "UNKNOWN")
                    .put("flicker_temporal_modulation", "UNKNOWN")
                    .put("composite_field_signal_status", fieldSignal.optString("status", "UNKNOWN"))
                    .put("lens_only_falloff_proven", false)
                    .put("camera_system_relative_illumination_calibrated", false)
                    .put("automatic_light_falloff_correction_allowed", false),
            )
            .put(
                "field_response",
                JSONObject()
                    .put("coordinate_chart_available", fieldAvailable)
                    .put("coordinate_chart_status", field.optString("status", "UNKNOWN"))
                    .put(
                        "authority",
                        if (fieldAvailable) {
                            "COMPOSITE_SCENE_LENS_SENSOR_OBSERVATION_ONLY"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put("rho_azimuth_radial_tangential_basis", fieldAvailable)
                    .put("gain_map_metadata_present", gainMapHint.optBoolean("gain_map_present", false))
                    .put("gain_map_authority", "SOURCE_METADATA_PROVENANCE_HINT_ONLY")
                    .put("gain_map_applied", false)
                    .put("lens_only_vignetting_proven", false)
                    .put("optical_axis_proven", false)
                    .put("correction_gain_allowed", false),
            )
            .put(
                "optical_support",
                JSONObject()
                    .put("sfr_mtf_psf_calibration_attached", false)
                    .put("radial_support_status", "UNKNOWN")
                    .put("tangential_support_status", "UNKNOWN")
                    .put("field_curvature_status", "UNKNOWN")
                    .put("chromatic_displacement_status", "UNKNOWN")
                    .put("focal_length_metadata_can_prove_optical_support", false)
                    .put("higher_output_raster_can_create_optical_support", false)
                    .put("deconvolution_authorized", false),
            )
            .put(
                "temporal_exposure_footprint",
                JSONObject()
                    .put("integration_duration_seconds", exposureSeconds ?: JSONObject.NULL)
                    .put(
                        "integration_duration_authority",
                        if (exposureSeconds != null) {
                            "SOURCE_METADATA_BOUND_CAPTURE_DURATION"
                        } else {
                            "UNKNOWN"
                        },
                    )
                    .put("camera_or_subject_motion_path", "UNKNOWN")
                    .put("rolling_shutter_temporal_geometry", "UNKNOWN")
                    .put("physical_motion_blur_separated", false)
                    .put("synthetic_motion_blur_authority", "APPEARANCE_ONLY")
                    .put("virtual_exposure_creates_new_evidence", false),
            )
            .put(
                "optional_calibration_observation_pack",
                JSONObject()
                    .put("required_for_universal_intake", false)
                    .put("normal_user_must_calibrate_camera", false)
                    .put("camera_or_lens_name_used_as_calibration_key", false)
                    .put(
                        "relation_policy",
                        "OPTIONAL_EXTRA_OBSERVATIONS_MAY_STRENGTHEN_ONLY_THEIR_OWN_AUTHORITY_AXIS_AFTER_EXPLICIT_RELATION_ADMISSION",
                    )
                    .put("observations", optionalObservations),
            )
            .put(
                "precision_contract",
                JSONObject()
                    .put("exact_integer_or_packed_source_evidence_preserved", true)
                    .put("float64_branch_sensitive_compute_preserved", true)
                    .put("controlled_float32_scientific_storage_preserved", true)
                    .put("precision_may_change_authority", false),
            )
            .put(
                "zero_line_truthrange",
                JSONObject()
                    .put("architecture_preserved", true)
                    .put("black_level_is_zero_line", false)
                    .put("zero_line_is_reference_gauge", true)
                    .put("source_white_level_is_truthrange_ceiling", false),
            )
            .put(
                "restoration_contract",
                JSONObject()
                    .put("measured_support_overpaint_allowed", false)
                    .put("loss_compensation_must_remain_reconstructed", true)
                    .put("appearance_reintegration_scientific_writeback_allowed", false),
            )
            .put("lens_profile_lookup_used", false)
            .put("camera_model_routing_used", false)
            .put("vendor_mapping_used", false)
            .put("ai_ml_neural_generative_used", false)
            .put("source_sample_values_modified", false)
            .put("source_sample_positions_modified", false)
            .put("new_measured_samples_created", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun optionalObservation(
        id: String,
        role: String,
    ): JSONObject =
        JSONObject()
            .put("id", id)
            .put("attached", false)
            .put("required", false)
            .put("role", role)
            .put("may_identify_camera_or_lens_by_name", false)
            .put("may_strengthen_authority_without_relation_proof", false)

    private fun present(
        obj: JSONObject,
        key: String,
    ): Boolean {
        if (!obj.has(key)) return false
        val value = obj.opt(key)
        return value != null && value !== JSONObject.NULL
    }

    private fun finitePositive(
        obj: JSONObject,
        key: String,
    ): Double? {
        val value = obj.optDouble(key, Double.NaN)
        return value.takeIf { it.isFinite() && it > 0.0 }
    }
}

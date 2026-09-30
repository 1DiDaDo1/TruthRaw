package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Mechanical statement of D.RAW's universal-input identity law.
 *
 * Container/capture identities may route transport or decoding only. Scientific
 * interpretation must be driven by admitted observations, relations, authority
 * and uncertainty rather than product or file-format identity.
 */
object UniversalIdentityIndependenceV01 {
    const val SCHEMA = "D.RAW/UniversalIdentityIndependence/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNIVERSAL_IDENTITY_INDEPENDENCE_CONTRACT_AVAILABLE")
            .put(
                "raw_container_policy",
                JSONObject()
                    .put("raw_format_may_route_decoder", true)
                    .put("container_layout_may_route_parser", true)
                    .put("raw_format_may_select_scientific_model", false)
                    .put("raw_format_may_select_denoise_strength", false)
                    .put("raw_format_may_define_colour_truth", false)
                    .put("raw_format_may_define_optical_truth", false)
                    .put("raw_format_may_define_world_geometry", false),
            )
            .put(
                "camera_lens_identity_policy",
                JSONObject()
                    .put("camera_name_may_be_provenance", true)
                    .put("lens_name_may_be_provenance", true)
                    .put("vendor_name_may_be_provenance", true)
                    .put("physical_camera_id_may_be_acquisition_provenance", true)
                    .put("camera_name_may_be_calibration_key", false)
                    .put("lens_name_may_be_calibration_key", false)
                    .put("vendor_name_may_be_calibration_key", false)
                    .put("physical_camera_id_may_be_scientific_model_key", false),
            )
            .put(
                "scientific_relation_policy",
                JSONObject()
                    .put("sealed_source_sha256_is_observation_identity", true)
                    .put("relations_require_explicit_evidence", true)
                    .put("optical_route_may_share_world_without_sharing_calibration", true)
                    .put("same_format_implies_same_response", false)
                    .put("same_lens_name_implies_same_response", false)
                    .put("same_camera_name_implies_same_response", false)
                    .put("unknown_is_valid_state", true),
            )
            .put("normal_user_calibration_required", false)
            .put("device_profile_required", false)
            .put("lens_profile_lookup_required", false)
            .put("vendor_mapping_required", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

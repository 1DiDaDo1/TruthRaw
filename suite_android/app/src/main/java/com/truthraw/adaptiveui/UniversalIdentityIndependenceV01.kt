package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Mechanical statement of D.RAW's universal-input identity law.
 *
 * Container/capture identities may route transport or decoding only. Scientific
 * interpretation must be driven by admitted observations, relations, authority
 * and uncertainty rather than product or file-format identity.
 *
 * A sealed observation does not need a known camera, lens, vendor or acquisition
 * origin in order to be structurally inspected. Missing origin only blocks claims
 * that specifically require origin-bound evidence; it never licenses a guessed
 * identity. D.RAW may build a richer Free World representation from the readable
 * observation, but richer representation can never become richer MEASURED
 * authority.
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
                "unknown_origin_policy",
                JSONObject()
                    .put("source_origin_may_be_unknown", true)
                    .put("unknown_camera_identity_blocks_source_sealing", false)
                    .put("unknown_lens_identity_blocks_source_sealing", false)
                    .put("unknown_vendor_identity_blocks_source_sealing", false)
                    .put(
                        "unknown_origin_blocks_read_only_structural_inspection",
                        false,
                    )
                    .put(
                        "readable_observation_content_drives_admissible_scientific_interpretation",
                        true,
                    )
                    .put(
                        "origin_dependent_claim_without_origin_evidence_must_remain_unknown",
                        true,
                    )
                    .put("missing_origin_identity_may_be_synthesized", false),
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
            .put(
                "free_world_derivation_policy",
                JSONObject()
                    .put("free_world_representation_may_be_richer_than_source", true)
                    .put(
                        "coordinate_or_raster_density_may_exceed_source_sampling",
                        true,
                    )
                    .put("higher_representation_density_creates_new_measurement", false)
                    .put("reconstructed_value_may_be_relabeled_measured", false)
                    .put("unknown_value_may_be_relabeled_measured", false)
                    .put("source_lineage_must_remain_recoverable", true)
                    .put("sealed_source_values_may_be_modified", false)
                    .put("derived_world_state_may_scientifically_write_back", false)
                    .put(
                        "authority_and_uncertainty_required_for_derived_values",
                        true,
                    )
                    .put("source_sensor_world_view_spaces_must_remain_distinct", true),
            )
            .put("normal_user_calibration_required", false)
            .put("device_profile_required", false)
            .put("lens_profile_lookup_required", false)
            .put("vendor_mapping_required", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Executable data contract for optical-support observations.
 */
object OpticalSupportAtlasV01 {
    const val SCHEMA = "D.RAW/OpticalSupportAtlas/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "OPTICAL_SUPPORT_ATLAS_CONTRACT_AVAILABLE")
            .put(
                "supported_measurement_kinds",
                JSONArray()
                    .put("SFR")
                    .put("MTF")
                    .put("PSF")
                    .put("RADIAL_SUPPORT")
                    .put("TANGENTIAL_SUPPORT")
                    .put("CHROMATIC_DISPLACEMENT")
                    .put("FIELD_CURVATURE")
                    .put("FOCUS_STATE"),
            )
            .put(
                "field_coordinate_requirements",
                JSONArray()
                    .put("SOURCE_XY")
                    .put("RHO")
                    .put("AZIMUTH")
                    .put("RADIAL_BASIS")
                    .put("TANGENTIAL_BASIS"),
            )
            .put(
                "authority_classes",
                JSONArray()
                    .put("MEASURED")
                    .put("CALIBRATED_ESTIMATE")
                    .put("INFERRED")
                    .put("UNKNOWN"),
            )
            .put(
                "scientific_support_propagation_minimum_authority",
                "CALIBRATED_ESTIMATE_OR_MEASURED_ONLY",
            )
            .put(
                "optical_frequency_support_policy",
                JSONObject()
                    .put("output_raster_density_equals_optical_resolution", false)
                    .put("raster_upsampling_may_upgrade_spatial_authority", false)
                    .put("unsupported_frequency_is_unknown", true)
                    .put("measured_or_calibrated_support_required_for_frequency_claim", true)
                    .put("inverse_optics_requires_noise_transport", true)
                    .put("near_zero_transfer_may_be_blindly_inverted", false)
                    .put("field_and_focus_dependence_must_be_preserved", true)
                    .put("channel_or_wavelength_dependence_may_be_collapsed_without_evidence", false),
            )
            .put(
                "candidate_measurement_runtime",
                "OpticalSupportMeasurementCandidateV01",
            )
            .put(
                "noise_aware_inverse_optics_candidate",
                "NoiseAwareInverseOpticsCandidateV01",
            )
            .put("sfr_measurement_attached", false)
            .put("mtf_measurement_attached", false)
            .put("psf_measurement_attached", false)
            .put("chromatic_displacement_attached", false)
            .put("field_curvature_attached", false)
            .put("spatial_authority_map_promoted", false)
            .put("deconvolution_applied", false)
            .put("inverse_optics_authorized", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Carries the older Deep-Scene/Light-Transport separation into the current
 * promotion-gated Free World architecture.
 */
object LightTransportAuthorityContractV01 {
    const val SCHEMA = "D.RAW/LightTransportAuthorityContract/0.1"

    fun describe(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "LIGHT_TRANSPORT_AUTHORITY_CONTRACT_AVAILABLE")
            .put(
                "separate_authority_axes",
                JSONArray()
                    .put("OBSERVED_RADIOMETRY")
                    .put("GEOMETRY_NORMAL")
                    .put("MATERIAL")
                    .put("ILLUMINATION")
                    .put("SPECTRAL_HYPOTHESIS")
                    .put("VISIBILITY"),
            )
            .put("three_channel_rgb_is_spectrometer", false)
            .put("physically_plausible_rendering_is_measurement", false)
            .put("lambertian_reference_may_be_used_as_bounded_hypothesis", true)
            .put("lambertian_reference_proves_real_material", false)
            .put("rendering_equation_integral_solved", false)
            .put("multiple_scattering_solved", false)
            .put("inferred_material_or_light_may_upgrade_measured_radiometry", false)
            .put("counterfactual_scene_may_repair_scientific_master", false)
            .put("light_transport_applied_to_scientific_master", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

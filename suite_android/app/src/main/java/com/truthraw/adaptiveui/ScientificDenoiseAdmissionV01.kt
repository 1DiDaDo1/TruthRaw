package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Fail-closed admission policy for future scientific denoise.
 *
 * It never denoises. It reports which independently validated authority axes
 * would be required before a residual can be altered scientifically.
 */
object ScientificDenoiseAdmissionV01 {
    const val SCHEMA = "D.RAW/ScientificDenoiseAdmission/0.1"

    fun describe(
        radiometric: JSONObject,
        noiseComponents: JSONObject,
        opticalSupport: JSONObject,
        temporalFootprint: JSONObject,
        geometryDepth: JSONObject,
        worldSpaceNoise: JSONObject,
    ): JSONObject {
        val radiometricReady =
            radiometric.optBoolean("radiometric_calibration_promoted", false)
        val noiseReady =
            noiseComponents.optBoolean("noise_component_decomposition_performed", false) &&
                noiseComponents.optBoolean("noise_reduction_applied", false).not()
        val opticalReady =
            opticalSupport.optBoolean("sfr_measurement_attached", false) ||
                opticalSupport.optBoolean("mtf_measurement_attached", false) ||
                opticalSupport.optBoolean("psf_measurement_attached", false)
        val temporalReady =
            temporalFootprint.optBoolean("physical_sequence_order_proven", false)
        val geometryReady =
            geometryDepth.optBoolean("geometry_promoted", false)
        val worldSeparationReady =
            worldSpaceNoise.optBoolean("world_fixed_signal_estimated", false) ||
                worldSpaceNoise.optBoolean("sensor_fixed_pattern_estimated", false) ||
                worldSpaceNoise.optBoolean("temporal_random_residual_estimated", false)

        val blockers = JSONArray()
        if (!radiometricReady) blockers.put("RADIOMETRIC_RESPONSE_NOT_PROMOTED")
        if (!noiseReady) blockers.put("NOISE_COMPONENT_DECOMPOSITION_NOT_PROMOTED")
        if (!opticalReady) blockers.put("OPTICAL_FREQUENCY_SUPPORT_NOT_PROMOTED")
        if (!temporalReady) blockers.put("PHYSICAL_TEMPORAL_RELATION_NOT_PROMOTED")
        if (!geometryReady) blockers.put("GEOMETRY_DEPTH_VISIBILITY_NOT_PROMOTED")
        if (!worldSeparationReady) blockers.put("WORLD_SENSOR_TEMPORAL_RESIDUAL_NOT_SEPARATED")
        blockers.put("EXPLICIT_SCIENTIFIC_DENOISE_APPROVAL_GATE_NOT_GRANTED")

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SCIENTIFIC_DENOISE_ADMISSION_BLOCKED")
            .put(
                "required_authority_axes",
                JSONArray()
                    .put("RADIOMETRIC_RESPONSE")
                    .put("NOISE_COMPONENT")
                    .put("OPTICAL_SUPPORT")
                    .put("TEMPORAL")
                    .put("GEOMETRY_DEPTH_VISIBILITY_WHERE_REQUIRED")
                    .put("WORLD_SENSOR_RESIDUAL_SEPARATION_WHERE_REQUIRED")
                    .put("RECONSTRUCTION_UNCERTAINTY"),
            )
            .put(
                "readiness",
                JSONObject()
                    .put("radiometric_response_ready", radiometricReady)
                    .put("noise_component_ready", noiseReady)
                    .put("optical_support_ready", opticalReady)
                    .put("temporal_relation_ready", temporalReady)
                    .put("geometry_depth_visibility_ready", geometryReady)
                    .put("world_space_residual_separation_ready", worldSeparationReady),
            )
            .put("blockers", blockers)
            .put(
                "admission_law",
                JSONObject()
                    .put("frontside_appearance_can_authorize_scientific_denoise", false)
                    .put("camera_or_lens_identity_can_authorize_scientific_denoise", false)
                    .put("raw_format_identity_can_authorize_scientific_denoise", false)
                    .put("unknown_residual_can_be_removed", false)
                    .put("measured_anchor_may_be_overwritten", false)
                    .put("reconstructed_value_may_be_promoted_to_measured", false),
            )
            .put("scientific_denoise_admitted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

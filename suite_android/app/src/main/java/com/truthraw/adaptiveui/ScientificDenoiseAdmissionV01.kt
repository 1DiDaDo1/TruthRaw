package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Fail-closed admission policy for future scientific denoise.
 *
 * It never denoises. It separates three evidence routes so a valid single-frame
 * denoise is not made dependent on 3D/multi-frame evidence, while stronger
 * optics-aware and world-space routes require their additional authority.
 */
object ScientificDenoiseAdmissionV01 {
    const val SCHEMA = "D.RAW/ScientificDenoiseAdmission/0.1"

    fun describe(
        radiometric: JSONObject,
        noiseComponents: JSONObject,
        noiseTransport: JSONObject,
        opticalSupport: JSONObject,
        temporalFootprint: JSONObject,
        geometryDepth: JSONObject,
        worldSpaceNoise: JSONObject,
    ): JSONObject {
        val radiometricReady =
            radiometric.optBoolean("radiometric_calibration_promoted", false)
        val noiseReady =
            noiseComponents.optBoolean(
                "noise_component_calibration_promoted",
                false,
            )
        val numericNoiseTransportReady =
            noiseTransport.optBoolean(
                "numeric_transport_validated",
                false,
            )
        val opticalReady =
            opticalSupport.optBoolean(
                "optical_support_calibration_promoted",
                false,
            )
        val temporalReady =
            temporalFootprint.optBoolean("physical_sequence_order_proven", false)
        val geometryReady =
            geometryDepth.optBoolean("geometry_promoted", false)
        val worldSeparationReady =
            worldSpaceNoise.optBoolean("world_fixed_signal_estimated", false) &&
                (
                    worldSpaceNoise.optBoolean(
                        "sensor_fixed_pattern_estimated",
                        false,
                    ) ||
                        worldSpaceNoise.optBoolean(
                            "temporal_random_residual_estimated",
                            false,
                        )
                )

        val singleFramePrerequisites =
            JSONObject()
                .put("radiometric_response_promoted", radiometricReady)
                .put("noise_component_decomposition_promoted", noiseReady)
                .put("numeric_noise_transport_validated", numericNoiseTransportReady)
                .put("reconstruction_uncertainty_required", true)
                .put("measured_anchor_preservation_required", true)
        val singleFrameReady =
            radiometricReady &&
                noiseReady &&
                numericNoiseTransportReady

        val opticsPrerequisites =
            JSONObject()
                .put("single_frame_route_ready", singleFrameReady)
                .put("optical_frequency_support_promoted", opticalReady)
                .put("frequency_dependent_noise_gain_required", true)
        val opticsReady =
            singleFrameReady && opticalReady

        val worldPrerequisites =
            JSONObject()
                .put("single_frame_route_ready", singleFrameReady)
                .put("physical_temporal_relation_promoted", temporalReady)
                .put("geometry_depth_visibility_promoted", geometryReady)
                .put("world_sensor_residual_separation_promoted", worldSeparationReady)
                .put("validated_world_to_source_bridge_required", true)
                .put("occlusion_and_view_dependence_required", true)
        val worldReady =
            singleFrameReady &&
                temporalReady &&
                geometryReady &&
                worldSeparationReady

        val blockers = JSONArray()
        if (!radiometricReady) blockers.put("RADIOMETRIC_RESPONSE_NOT_PROMOTED")
        if (!noiseReady) blockers.put("NOISE_COMPONENT_DECOMPOSITION_NOT_PROMOTED")
        if (!numericNoiseTransportReady) blockers.put("NUMERIC_NOISE_TRANSPORT_NOT_VALIDATED")
        blockers.put("EXPLICIT_SCIENTIFIC_DENOISE_APPROVAL_GATE_NOT_GRANTED")

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SCIENTIFIC_DENOISE_ADMISSION_BLOCKED")
            .put(
                "routes",
                JSONObject()
                    .put(
                        "single_frame_component_aware",
                        JSONObject()
                            .put("prerequisites", singleFramePrerequisites)
                            .put("evidence_ready", singleFrameReady)
                            .put("scientific_denoise_admitted", false),
                    )
                    .put(
                        "optics_aware_or_inverse_optics",
                        JSONObject()
                            .put("prerequisites", opticsPrerequisites)
                            .put("evidence_ready", opticsReady)
                            .put("scientific_denoise_admitted", false),
                    )
                    .put(
                        "multi_observation_world_space",
                        JSONObject()
                            .put("prerequisites", worldPrerequisites)
                            .put("evidence_ready", worldReady)
                            .put("scientific_denoise_admitted", false),
                    ),
            )
            .put(
                "admission_law",
                JSONObject()
                    .put("frontside_appearance_can_authorize_scientific_denoise", false)
                    .put("camera_or_lens_identity_can_authorize_scientific_denoise", false)
                    .put("raw_format_identity_can_authorize_scientific_denoise", false)
                    .put("unknown_residual_can_be_removed", false)
                    .put("measured_anchor_may_be_overwritten", false)
                    .put("reconstructed_value_may_be_promoted_to_measured", false)
                    .put("single_frame_route_requires_3d_geometry", false)
                    .put("single_frame_route_requires_multiple_frames", false)
                    .put("multi_observation_route_may_merge_source_evidence_roots", false),
            )
            .put("blockers", blockers)
            .put("scientific_denoise_admitted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

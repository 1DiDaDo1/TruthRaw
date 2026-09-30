package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Identity-independent, non-promoting natural self-calibration atlas.
 *
 * Every axis remains separate. The atlas may report readiness/candidates from
 * accumulated observations but never turns them into an applied calibration.
 */
object NaturalSelfCalibrationAtlasV01 {
    const val SCHEMA = "D.RAW/NaturalSelfCalibrationAtlas/0.1"

    fun build(
        profiles: List<JSONObject>,
        graph: JSONObject,
        fieldRepeatability: JSONObject? = null,
    ): JSONObject {
        var measuredFieldCount = 0
        var metadataColourCount = 0
        var noiseMetadataCount = 0
        var captureTimeHintCount = 0
        var opticalMetadataHintCount = 0

        val roots = JSONArray()
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue
            roots.put(sha)

            val field =
                profile.optJSONObject(
                    "observation_optical_field_chart",
                ) ?: JSONObject()
            val signal =
                field.optJSONObject(
                    "measured_composite_field_signal",
                ) ?: JSONObject()
            if (
                field.optString("status") == "FIELD_CHART_AVAILABLE" &&
                signal.optString("status") ==
                "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            ) {
                measuredFieldCount++
            }

            val metadata =
                profile.optJSONObject("source_metadata") ?: JSONObject()
            if (
                metadata.optBoolean(
                    "color_matrix_1_present",
                    false,
                ) ||
                metadata.optBoolean(
                    "color_matrix_2_present",
                    false,
                )
            ) {
                metadataColourCount++
            }
            if (
                metadata.optBoolean(
                    "noise_profile_present",
                    false,
                )
            ) {
                noiseMetadataCount++
            }
            if (
                metadata.has("capture_time_preferred_text") &&
                metadata.opt("capture_time_preferred_text") !=
                JSONObject.NULL
            ) {
                captureTimeHintCount++
            }
            if (
                metadata.has("focal_length_mm") &&
                metadata.opt("focal_length_mm") !=
                JSONObject.NULL
            ) {
                opticalMetadataHintCount++
            }
        }

        val repeatabilityAvailable =
            fieldRepeatability?.optString("status") ==
                "READ_ONLY_REPEATABILITY_AUDIT_AVAILABLE"
        val cfaMad =
            fieldRepeatability
                ?.optJSONObject("cfa_phase_repeatability")
                ?.opt("median_phase_bin_cross_observation_mad_ev")
                ?: JSONObject.NULL
        val radiometricResponse =
            RadiometricResponseAtlasV01.build(profiles)
        val noiseComponents =
            NoiseComponentAtlasV01.build(profiles)

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "NATURAL_SELF_CALIBRATION_ATLAS_FOUNDATION_AVAILABLE",
            )
            .put("observation_roots", roots)
            .put("observation_count", seen.size)
            .put(
                "identity_policy",
                JSONObject()
                    .put(
                        "camera_name_required",
                        false,
                    )
                    .put("lens_name_required", false)
                    .put("vendor_required", false)
                    .put(
                        "device_profile_key_allowed",
                        false,
                    )
                    .put(
                        "normal_user_calibration_required",
                        false,
                    ),
            )
            .put(
                "radiometric_response_axis",
                radiometricResponse,
            )
            .put(
                "field_response_axis",
                JSONObject()
                    .put(
                        "measured_observation_count",
                        measuredFieldCount,
                    )
                    .put(
                        "multi_scene_repeatability_available",
                        repeatabilityAvailable,
                    )
                    .put(
                        "authority",
                        if (repeatabilityAvailable) {
                            "OBSERVED_REPEATABILITY_DESCRIPTIVE_ONLY"
                        } else {
                            "UNKNOWN_OR_SINGLE_OBSERVATION"
                        },
                    )
                    .put(
                        "camera_system_response_proven",
                        false,
                    )
                    .put(
                        "lens_only_vignetting_proven",
                        false,
                    )
                    .put("calibration_promoted", false),
            )
            .put(
                "cfa_phase_axis",
                JSONObject()
                    .put(
                        "repeatability_metric_available",
                        repeatabilityAvailable,
                    )
                    .put(
                        "median_phase_bin_cross_observation_mad_ev",
                        cfaMad,
                    )
                    .put(
                        "sensor_phase_response_proven",
                        false,
                    )
                    .put(
                        "scene_colour_component_separated",
                        false,
                    )
                    .put("calibration_promoted", false),
            )
            .put(
                "colour_axis",
                JSONObject()
                    .put(
                        "source_metadata_colour_observation_count",
                        metadataColourCount,
                    )
                    .put(
                        "multi_illuminant_reference_target_observations",
                        0,
                    )
                    .put(
                        "held_out_validation_available",
                        false,
                    )
                    .put(
                        "spectral_truth_from_three_channels_claimed",
                        false,
                    )
                    .put("calibration_promoted", false),
            )
            .put(
                "optical_support_axis",
                JSONObject()
                    .put(
                        "metadata_focal_hint_observation_count",
                        opticalMetadataHintCount,
                    )
                    .put("sfr_measured", false)
                    .put("mtf_measured", false)
                    .put("psf_measured", false)
                    .put(
                        "field_curvature_measured",
                        false,
                    )
                    .put(
                        "chromatic_displacement_measured",
                        false,
                    )
                    .put(
                        "deconvolution_authorized",
                        false,
                    ),
            )
            .put(
                "dark_noise_axis",
                JSONObject()
                    .put(
                        "noise_metadata_observation_count",
                        noiseMetadataCount,
                    )
                    .put(
                        "independent_dark_set_count",
                        0,
                    )
                    .put(
                        "measured_offset_noise_model_promoted",
                        false,
                    ),
            )
            .put(
                "noise_component_axis",
                noiseComponents,
            )
            .put(
                "temporal_axis",
                JSONObject()
                    .put(
                        "capture_time_metadata_hint_count",
                        captureTimeHintCount,
                    )
                    .put(
                        "physical_timing_relation_proven",
                        false,
                    )
                    .put(
                        "rolling_shutter_model_proven",
                        false,
                    )
                    .put(
                        "synthetic_frame_is_independent_evidence",
                        false,
                    ),
            )
            .put(
                "graph_binding",
                JSONObject()
                    .put(
                        "graph_schema",
                        graph.optString("schema", "UNKNOWN"),
                    )
                    .put(
                        "graph_identity_sha256",
                        graph.opt(
                            "graph_identity_sha256",
                        ) ?: JSONObject.NULL,
                    )
                    .put(
                        "graph_identity_is_calibration_identity",
                        false,
                    ),
            )
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("deconvolution_authorized", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

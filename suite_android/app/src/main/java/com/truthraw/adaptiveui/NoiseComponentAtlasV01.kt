package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Component-separated sensor/noise research contract.
 *
 * No denoise is performed. The purpose is to stop unrelated uncertainty
 * sources from being collapsed into one scalar "noise" value.
 */
object NoiseComponentAtlasV01 {
    const val SCHEMA = "D.RAW/NoiseComponentAtlas/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        var noiseProfileHintCount = 0
        var measuredBacksideCount = 0
        val roots = JSONArray()
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue
            roots.put(sha)

            val metadata = profile.optJSONObject("source_metadata") ?: JSONObject()
            if (metadata.optBoolean("noise_profile_present", false)) {
                noiseProfileHintCount++
            }

            val backside =
                profile.optJSONObject("backside_signal_support")
                    ?: profile.optJSONObject("backside_signal_support_audit")
                    ?: JSONObject()
            if (
                backside.optString("status") ==
                "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
            ) {
                measuredBacksideCount++
            }
        }

        fun component(
            id: String,
            family: String,
            dependency: String,
        ): JSONObject =
            JSONObject()
                .put("id", id)
                .put("family", family)
                .put("dependency", dependency)
                .put("authority", "UNKNOWN_UNTIL_INDEPENDENT_PHYSICAL_OBSERVATIONS")
                .put("model_promoted", false)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "NOISE_COMPONENT_ATLAS_CONTRACT_AVAILABLE")
            .put("observation_roots", roots)
            .put("noise_profile_metadata_hint_count", noiseProfileHintCount)
            .put("measured_backside_observation_count", measuredBacksideCount)
            .put("independent_dark_observation_set_count", 0)
            .put("independent_flat_field_observation_set_count", 0)
            .put(
                "components",
                JSONArray()
                    .put(component("PHOTON_SHOT", "TEMPORAL_SIGNAL_DEPENDENT", "SCENE_SIGNAL_AND_EXPOSURE"))
                    .put(component("READ", "TEMPORAL_SIGNAL_INDEPENDENT", "READOUT_AND_GAIN_CONTEXT"))
                    .put(component("DARK_CURRENT_SHOT", "TEMPORAL_DARK", "INTEGRATION_TIME_AND_TEMPERATURE_WHEN_MEASURED"))
                    .put(component("DSNU", "FIXED_PATTERN_DARK", "SENSOR_COORDINATE_AND_DARK_CONTEXT"))
                    .put(component("PRNU", "FIXED_PATTERN_SIGNAL_DEPENDENT", "SENSOR_COORDINATE_AND_SIGNAL"))
                    .put(component("ROW_COLUMN_PATTERN", "FIXED_OR_CORRELATED_PATTERN", "READOUT_GEOMETRY"))
                    .put(component("QUANTIZATION", "DIGITAL_CODING", "CODE_STEP_AND_GAIN"))
                    .put(component("CFA_PHASE", "PHASE_DEPENDENT", "CFA_PHASE_AND_CHANNEL_RESPONSE"))
                    .put(component("SPATIAL_FREQUENCY_RESIDUAL", "CORRELATED_RESIDUAL", "NPS_OR_EQUIVALENT"))
                    .put(component("UNKNOWN_RESIDUAL", "UNCLASSIFIED", "EXPLICIT_UNKNOWN")),
            )
            .put(
                "separation_law",
                JSONObject()
                    .put("temporal_noise_equals_fixed_pattern_noise", false)
                    .put("noise_profile_metadata_is_full_noise_calibration", false)
                    .put("frontside_texture_may_be_declared_noise", false)
                    .put("unknown_residual_may_be_forced_to_zero", false)
                    .put("denoise_requires_component_or_residual_authority", true),
            )
            .put(
                "candidate_solver",
                "NoiseComponentDecompositionCandidateV01",
            )
            .put("noise_component_decomposition_performed", false)
            .put("numeric_noise_transport_validated", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

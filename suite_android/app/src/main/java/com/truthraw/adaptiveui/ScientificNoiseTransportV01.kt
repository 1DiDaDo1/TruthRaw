package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Defines how uncertainty/noise must travel through future scientific
 * transforms. This is a contract only; no numeric covariance is synthesized.
 */
object ScientificNoiseTransportV01 {
    const val SCHEMA = "D.RAW/ScientificNoiseTransport/0.1"

    fun describe(
        promotionState: JSONObject =
            ScientificPromotionStateV01.blocked(),
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SCIENTIFIC_NOISE_TRANSPORT_CONTRACT_AVAILABLE")
            .put(
                "supported_representations",
                JSONArray()
                    .put("PER_CHANNEL_VARIANCE")
                    .put("CROSS_CHANNEL_COVARIANCE")
                    .put("SPATIAL_COVARIANCE_OR_KERNEL")
                    .put("NOISE_POWER_SPECTRUM")
                    .put("CFA_PHASE_CONDITIONAL_UNCERTAINTY"),
            )
            .put(
                "transport_rules",
                JSONObject()
                    .put("scalar_gain_variance_rule", "VAR_OUT=GAIN^2*VAR_IN")
                    .put("linear_transform_covariance_rule", "COV_OUT=J*COV_IN*J_TRANSPOSE")
                    .put("colour_transform_must_transport_covariance", true)
                    .put("field_gain_must_transport_uncertainty", true)
                    .put("reconstruction_must_add_reconstruction_uncertainty", true)
                    .put("deconvolution_must_account_for_frequency_dependent_noise_gain", true)
                    .put("missing_covariance_may_be_assumed_zero", false),
            )
            .put(
                "inverse_optics_policy",
                JSONObject()
                    .put("near_zero_optical_transfer_may_be_blindly_inverted", false)
                    .put("optical_support_and_noise_support_must_be_jointly_considered", true)
                    .put("output_raster_density_is_not_recovered_frequency_support", true),
            )
            .put("numeric_primitives_implemented", true)
            .put("numeric_primitive_implementation", ScientificNoiseMathV01.METHOD_ID)
            .put(
                "numeric_transport_validated",
                promotionState.optBoolean(
                    "numeric_noise_transport_validated",
                    false,
                ),
            )
            .put("numeric_noise_transport_performed", false)
            .put(
                "promotion_state_status",
                promotionState.optString(
                    "status",
                    "NOT_PROMOTED_FAIL_CLOSED",
                ),
            )
            .put("noise_reduction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

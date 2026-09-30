package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs

/**
 * Frequency-domain Wiener-like inverse-optics candidate.
 *
 * This computes gains only. It does not transform an image and cannot authorize
 * deconvolution.
 */
object NoiseAwareInverseOpticsCandidateV01 {
    const val SCHEMA = "D.RAW/NoiseAwareInverseOpticsCandidate/0.1"

    fun evaluate(request: JSONObject): JSONObject {
        val bins = request.optJSONArray("frequency_bins")
            ?: return unavailable("FREQUENCY_BINS_REQUIRED")
        val minimumTransfer =
            request.optDouble("minimum_transfer_magnitude", Double.NaN)
        val maximumGain =
            request.optDouble("maximum_gain", Double.NaN)
        if (
            !minimumTransfer.isFinite() || minimumTransfer <= 0.0 ||
            !maximumGain.isFinite() || maximumGain <= 0.0
        ) {
            return unavailable("VALID_TRANSFER_AND_GAIN_BOUNDS_REQUIRED")
        }

        val out = JSONArray()
        var admitted = 0
        var blocked = 0
        for (i in 0 until bins.length()) {
            val b = bins.optJSONObject(i) ?: continue
            val f = b.optDouble("frequency", Double.NaN)
            val h = b.optDouble("transfer_magnitude", Double.NaN)
            val noise = b.optDouble("noise_psd", Double.NaN)
            val signal = b.optDouble("signal_psd", Double.NaN)
            if (
                !f.isFinite() || f < 0.0 ||
                !h.isFinite() || h < 0.0 ||
                !noise.isFinite() || noise < 0.0 ||
                !signal.isFinite() || signal <= 0.0
            ) {
                continue
            }

            val safe = h >= minimumTransfer
            val gain =
                if (safe) {
                    val denom = h * h + noise / signal
                    if (denom.isFinite() && denom > 0.0) {
                        (h / denom).coerceIn(-maximumGain, maximumGain)
                    } else {
                        Double.NaN
                    }
                } else {
                    Double.NaN
                }

            if (gain.isFinite()) admitted++ else blocked++
            out.put(
                JSONObject()
                    .put("frequency", f)
                    .put("transfer_magnitude", h)
                    .put("noise_psd", noise)
                    .put("signal_psd", signal)
                    .put(
                        "candidate_gain",
                        if (gain.isFinite()) gain else JSONObject.NULL,
                    )
                    .put(
                        "status",
                        if (gain.isFinite()) {
                            "GAIN_CANDIDATE_AVAILABLE"
                        } else if (abs(h) < minimumTransfer) {
                            "BLOCKED_NEAR_ZERO_OPTICAL_TRANSFER"
                        } else {
                            "BLOCKED_INVALID_NUMERIC_STATE"
                        },
                    ),
            )
        }

        if (admitted == 0) return unavailable("NO_SAFE_INVERSE_OPTICS_BINS")

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "NOISE_AWARE_INVERSE_OPTICS_CANDIDATE_AVAILABLE")
            .put("frequency_bins", out)
            .put("candidate_bin_count", admitted)
            .put("blocked_bin_count", blocked)
            .put("method", "WIENER_LIKE_H_OVER_H2_PLUS_NOISE_OVER_SIGNAL")
            .put("image_transform_applied", false)
            .put("deconvolution_authorized", false)
            .put("inverse_optics_authorized", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("image_transform_applied", false)
            .put("deconvolution_authorized", false)
            .put("scientific_writeback_allowed", false)
}

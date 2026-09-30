package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs

/**
 * End-to-end candidate bridge:
 * controlled NPS + controlled MTF/SFR + explicit signal PSD
 * -> NoiseAwareInverseOpticsCandidateV01.
 *
 * It refuses to invent signal PSD or optical transfer.
 */
object NoiseOpticsJointCandidateV01 {
    const val SCHEMA = "D.RAW/NoiseOpticsJointCandidate/0.1"
    private const val FREQUENCY_EPS = 1.0e-9

    fun evaluate(
        noiseSpectrum: JSONObject,
        opticalSupport: JSONObject,
        records: List<JSONObject>,
    ): JSONObject {
        if (
            noiseSpectrum.optString("status") !=
            "NOISE_SPECTRUM_MEASUREMENT_CANDIDATE_AVAILABLE"
        ) {
            return unavailable("NOISE_SPECTRUM_CANDIDATE_UNAVAILABLE")
        }
        if (
            opticalSupport.optString("status") !=
            "OPTICAL_SUPPORT_MEASUREMENT_CANDIDATE_AVAILABLE"
        ) {
            return unavailable("OPTICAL_SUPPORT_CANDIDATE_UNAVAILABLE")
        }

        val transfer = opticalCurve(opticalSupport)
        if (transfer.isEmpty()) {
            return unavailable("NO_MTF_OR_SFR_FREQUENCY_CURVE")
        }

        val signal = explicitSignalPsd(records)
        if (signal.isEmpty()) {
            return unavailable("EXPLICIT_SIGNAL_PSD_REQUIRED")
        }

        val bins = JSONArray()
        val channels =
            noiseSpectrum.optJSONArray("channels") ?: JSONArray()
        for (i in 0 until channels.length()) {
            val channel = channels.optJSONObject(i) ?: continue
            val channelId =
                channel.optString("channel_or_phase", "UNSPECIFIED")
            val curve = channel.optJSONArray("curve") ?: continue
            for (j in 0 until curve.length()) {
                val point = curve.optJSONObject(j) ?: continue
                val f =
                    point.optDouble(
                        "frequency_cycles_per_pixel",
                        Double.NaN,
                    )
                val noise =
                    point.optDouble("median_power", Double.NaN)
                if (!f.isFinite() || !noise.isFinite() || noise < 0.0) {
                    continue
                }
                val h = nearest(transfer, f) ?: continue
                val s =
                    signal[channelId]?.let { nearest(it, f) }
                        ?: signal["UNSPECIFIED"]?.let { nearest(it, f) }
                        ?: continue
                bins.put(
                    JSONObject()
                        .put("frequency", f)
                        .put("channel_or_phase", channelId)
                        .put("transfer_magnitude", h)
                        .put("noise_psd", noise)
                        .put("signal_psd", s),
                )
            }
        }

        if (bins.length() == 0) {
            return unavailable("NO_JOINT_FREQUENCY_BINS")
        }

        val inverse =
            NoiseAwareInverseOpticsCandidateV01.evaluate(
                JSONObject().put("frequency_bins", bins),
            )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (
                    inverse.optString("status") ==
                    "NOISE_AWARE_INVERSE_OPTICS_CANDIDATE_AVAILABLE"
                ) {
                    "NOISE_OPTICS_JOINT_CANDIDATE_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("joint_input_bin_count", bins.length())
            .put("inverse_optics_candidate", inverse)
            .put("signal_psd_inferred_from_image", false)
            .put("optical_transfer_inferred_from_sharpness", false)
            .put("inverse_optics_authorized", false)
            .put("image_transform_applied", false)
            .put("candidate_applied", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun opticalCurve(
        opticalSupport: JSONObject,
    ): List<Pair<Double, Double>> {
        val curves =
            opticalSupport.optJSONArray("frequency_curves") ?: return emptyList()
        var selected: JSONArray? = null
        for (preferred in listOf("MTF", "SFR")) {
            for (i in 0 until curves.length()) {
                val curve = curves.optJSONObject(i) ?: continue
                if (curve.optString("kind").uppercase() == preferred) {
                    selected = curve.optJSONArray("curve")
                    break
                }
            }
            if (selected != null) break
        }
        val arr = selected ?: return emptyList()
        val out = ArrayList<Pair<Double, Double>>()
        for (i in 0 until arr.length()) {
            val point = arr.optJSONArray(i) ?: continue
            val f = point.optDouble(0, Double.NaN)
            val h = point.optDouble(1, Double.NaN)
            if (f.isFinite() && f >= 0.0 && h.isFinite() && h >= 0.0) {
                out += f to h
            }
        }
        return out.sortedBy { it.first }
    }

    private fun explicitSignalPsd(
        records: List<JSONObject>,
    ): Map<String, List<Pair<Double, Double>>> {
        val grouped =
            linkedMapOf<String, MutableList<Pair<Double, Double>>>()
        for (record in records) {
            val axis = record.optString("axis_scope")
            if (
                axis != "OPTICAL_SUPPORT" &&
                axis != "NOISE_COMPONENT_SEPARATION"
            ) {
                continue
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            val bins = payload.optJSONArray("signal_psd_bins") ?: continue
            for (i in 0 until bins.length()) {
                val b = bins.optJSONObject(i) ?: continue
                val f =
                    b.optDouble(
                        "frequency_cycles_per_pixel",
                        Double.NaN,
                    )
                val power = b.optDouble("power", Double.NaN)
                val channel =
                    b.optString("channel_or_phase", "UNSPECIFIED")
                if (
                    f.isFinite() && f >= 0.0 &&
                    power.isFinite() && power > 0.0
                ) {
                    grouped.getOrPut(channel) { ArrayList() }
                        .add(f to power)
                }
            }
        }
        return grouped.mapValues { (_, v) -> v.sortedBy { it.first } }
    }

    private fun nearest(
        curve: List<Pair<Double, Double>>,
        frequency: Double,
    ): Double? {
        var best: Pair<Double, Double>? = null
        var bestDistance = Double.POSITIVE_INFINITY
        for (point in curve) {
            val d = abs(point.first - frequency)
            if (d < bestDistance) {
                best = point
                bestDistance = d
            }
        }
        val selected = best ?: return null
        return selected.second.takeIf {
            bestDistance <= FREQUENCY_EPS * (1.0 + abs(frequency))
        }
    }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("inverse_optics_authorized", false)
            .put("image_transform_applied", false)
            .put("candidate_applied", false)
            .put("scientific_writeback_allowed", false)
}

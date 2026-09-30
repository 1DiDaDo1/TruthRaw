package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.abs

/**
 * Builds radiometric response measurement points directly from controlled
 * sealed RAW profiles when constant illumination/gain/aperture relation is
 * explicitly admitted by a calibration observation record.
 */
object RadiometricProfileRelationCandidateV01 {
    const val SCHEMA = "D.RAW/RadiometricProfileRelationCandidate/0.1"

    fun evaluate(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
    ): JSONObject {
        val bySha =
            linkedMapOf<String, JSONObject>().apply {
                for (profile in profiles) {
                    val processing =
                        profile.optString("source_sha256")
                            .trim()
                            .lowercase()
                    if (processing.isNotBlank()) {
                        put(processing, profile)
                    }
                    val upstream =
                        profile.optString(
                            "upstream_sealed_source_sha256",
                        ).trim().lowercase()
                    if (upstream.isNotBlank()) {
                        put(upstream, profile)
                    }
                }
            }

        val syntheticRecords = ArrayList<JSONObject>()
        for (record in records) {
            if (record.optString("axis_scope") != "RADIOMETRIC_RESPONSE") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "RADIOMETRIC_RESPONSE",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            if (
                !payload.optBoolean("constant_illumination_relation_admitted", false) ||
                !payload.optBoolean("constant_gain_aperture_relation_admitted", false)
            ) {
                continue
            }

            val roots =
                record.optJSONArray(
                    "session_processing_source_sha256_roots",
                ) ?: record.optJSONArray("source_sha256_roots") ?: continue
            val roles = record.optJSONArray("observation_roles") ?: continue
            if (roots.length() != roles.length() || roots.length() < 3) continue

            data class Point(
                val sha: String,
                val exposure: Double,
                val signal: Double,
                val iso: Double,
                val fNumber: Double,
                val role: String,
            )

            val points = ArrayList<Point>()
            for (i in 0 until roots.length()) {
                val sha =
                    roots.optString(i).trim().lowercase()
                val profile = bySha[sha] ?: continue
                val metadata = profile.optJSONObject("source_metadata") ?: JSONObject()
                val backside =
                    profile.optJSONObject("backside_signal_support") ?: JSONObject()
                if (
                    backside.optString("status") !=
                    "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
                ) {
                    continue
                }
                val exposure =
                    metadata.optDouble("exposure_time_seconds", Double.NaN)
                val iso = metadata.optDouble("iso", Double.NaN)
                val f = metadata.optDouble("f_number", Double.NaN)
                val signal =
                    backside.optJSONObject("global")
                        ?.optDouble("p50_normalized_above_black", Double.NaN)
                        ?: Double.NaN
                if (
                    exposure.isFinite() && exposure > 0.0 &&
                    iso.isFinite() && iso > 0.0 &&
                    f.isFinite() && f > 0.0 &&
                    signal.isFinite()
                ) {
                    points += Point(
                        sha = sha,
                        exposure = exposure,
                        signal = signal,
                        iso = iso,
                        fNumber = f,
                        role = roles.optString(i, "TRAIN"),
                    )
                }
            }
            if (points.size < 3) continue

            val iso0 = points.first().iso
            val f0 = points.first().fNumber
            val constantContext =
                points.all {
                    relativeClose(it.iso, iso0) &&
                        relativeClose(it.fNumber, f0)
                }
            if (!constantContext) continue

            val minExposure =
                points.minOfOrNull { it.exposure } ?: continue
            if (!minExposure.isFinite() || minExposure <= 0.0) continue

            val measurements = JSONArray()
            for (p in points) {
                measurements.put(
                    JSONObject()
                        .put("source_sha256", p.sha)
                        .put("relative_exposure", p.exposure / minExposure)
                        .put("normalized_signal", p.signal)
                        .put("role", p.role)
                        .put("censored", false),
                )
            }

            val synthetic =
                JSONObject(record.toString())
                    .put(
                        "axis_payload",
                        JSONObject()
                            .put("measurement_points", measurements)
                            .put("derived_from_source_profiles", true)
                            .put("constant_iso_metadata", iso0)
                            .put("constant_f_number_metadata", f0),
                    )
            syntheticRecords += synthetic
        }

        if (syntheticRecords.isEmpty()) {
            return JSONObject()
                .put("schema", SCHEMA)
                .put("status", "UNKNOWN_FAIL_CLOSED")
                .put("reason", "NO_CONTROLLED_PROFILE_RELATION_AVAILABLE")
                .put("radiometric_calibration_promoted", false)
                .put("scientific_writeback_allowed", false)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "PROFILE_DERIVED_RADIOMETRIC_CANDIDATE_AVAILABLE")
            .put(
                "candidate",
                RadiometricResponseCandidateSolverV01.evaluate(syntheticRecords),
            )
            .put("iso_metadata_used_as_measured_gain", false)
            .put("camera_lens_vendor_or_raw_identity_used_as_key", false)
            .put("radiometric_calibration_promoted", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun relativeClose(
        a: Double,
        b: Double,
    ): Boolean {
        val scale = maxOf(1.0, abs(a), abs(b))
        return abs(a - b) <= 1.0e-9 * scale
    }
}

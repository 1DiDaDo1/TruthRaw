package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Connects an admitted colour-relation candidate to explicitly supplied RGB
 * covariance. Missing covariance remains UNKNOWN; it is never synthesized.
 */
object ColourCovarianceTransportCandidateV01 {
    const val SCHEMA = "D.RAW/ColourCovarianceTransportCandidate/0.1"

    fun evaluate(
        colourRelation: JSONObject,
        records: List<JSONObject>,
    ): JSONObject {
        if (
            colourRelation.optString("status") !=
            "COLOUR_RELATION_CANDIDATE_AVAILABLE"
        ) {
            return unavailable("COLOUR_RELATION_CANDIDATE_UNAVAILABLE")
        }

        val matrix =
            matrix3x3(
                colourRelation.optJSONArray(
                    "camera_rgb_to_reference_xyz_candidate",
                ),
            ) ?: return unavailable("COLOUR_MATRIX_INVALID")

        val outputs = JSONArray()
        var count = 0

        for (record in records) {
            if (record.optString("axis_scope") != "COLOUR_RELATION") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "COLOUR_RELATION",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }

            val payload = record.optJSONObject("axis_payload") ?: continue
            val samples = payload.optJSONArray("covariance_samples")
            if (samples != null) {
                for (i in 0 until samples.length()) {
                    val sample = samples.optJSONObject(i) ?: continue
                    addSample(sample, matrix, outputs)?.let { count++ }
                }
            }

            val patches = payload.optJSONArray("patches") ?: continue
            for (i in 0 until patches.length()) {
                val patch = patches.optJSONObject(i) ?: continue
                if (patch.has("camera_rgb_covariance_3x3")) {
                    addSample(patch, matrix, outputs)?.let { count++ }
                }
            }
        }

        if (count == 0) {
            return unavailable("NO_EXPLICIT_INPUT_COVARIANCE")
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "COLOUR_COVARIANCE_TRANSPORT_CANDIDATE_AVAILABLE",
            )
            .put("sample_count", count)
            .put("samples", outputs)
            .put("transport_rule", "C_OUT=J*C_IN*J_TRANSPOSE")
            .put("missing_covariance_synthesized", false)
            .put("colour_calibration_promoted", false)
            .put("candidate_applied", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun addSample(
        sample: JSONObject,
        matrix: Array<DoubleArray>,
        outputs: JSONArray,
    ): Unit? {
        val covariance =
            matrix3x3(
                sample.optJSONArray("camera_rgb_covariance_3x3"),
            ) ?: return null
        if (!ScientificNoiseMathV01.validCovariance3x3(covariance)) {
            return null
        }
        val transformed =
            ScientificNoiseMathV01.transformCovariance3x3(
                covariance,
                matrix,
            ) ?: return null

        outputs.put(
            JSONObject()
                .put(
                    "sample_id",
                    sample.optString("sample_id")
                        .ifBlank { sample.optString("patch_id", "UNNAMED") },
                )
                .put(
                    "input_camera_rgb_covariance_3x3",
                    matrixJson(covariance),
                )
                .put(
                    "output_reference_xyz_covariance_3x3",
                    matrixJson(transformed),
                ),
        )
        return Unit
    }

    private fun matrix3x3(a: JSONArray?): Array<DoubleArray>? {
        if (a == null || a.length() != 3) return null
        val out = Array(3) { DoubleArray(3) }
        for (r in 0..2) {
            val row = a.optJSONArray(r) ?: return null
            if (row.length() != 3) return null
            for (c in 0..2) {
                val v = row.optDouble(c, Double.NaN)
                if (!v.isFinite()) return null
                out[r][c] = v
            }
        }
        return out
    }

    private fun matrixJson(m: Array<DoubleArray>): JSONArray =
        JSONArray().apply {
            for (r in 0..2) {
                put(
                    JSONArray()
                        .put(m[r][0])
                        .put(m[r][1])
                        .put(m[r][2]),
                )
            }
        }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("missing_covariance_synthesized", false)
            .put("colour_calibration_promoted", false)
            .put("candidate_applied", false)
            .put("scientific_writeback_allowed", false)
}

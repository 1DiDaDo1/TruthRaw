package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Per-observation authority matrix.
 *
 * This is deliberately descriptive and axis-separated. It prevents a strong
 * frontside/metadata signal on one axis from upgrading another axis.
 */
object ObservationAxisAuthorityMatrixV01 {
    const val SCHEMA = "D.RAW/ObservationAxisAuthorityMatrix/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue

            val sourceClass =
                profile.optString(
                    "scientific_source_class",
                    "UNKNOWN",
                )
            val front =
                profile.optJSONObject("scene_analysis")
                    ?: JSONObject()
            val lattice =
                profile.optJSONObject(
                    "raster_independent_sample_lattice",
                ) ?: JSONObject()
            val backside =
                profile.optJSONObject("backside_signal_support")
                    ?: JSONObject()
            val field =
                profile.optJSONObject(
                    "observation_optical_field_chart",
                ) ?: JSONObject()
            val measuredField =
                field.optJSONObject(
                    "measured_composite_field_signal",
                ) ?: JSONObject()
            val metadata =
                profile.optJSONObject("source_metadata")
                    ?: JSONObject()
            val atlas =
                profile.optJSONObject(
                    "universal_observation_calibration_atlas",
                ) ?: JSONObject()

            val sourceMeasured =
                sourceClass == "DNG_CFA_RAW" &&
                    lattice.optString("status") == "AVAILABLE"
            val backsideMeasured =
                backside.optString("status").contains(
                    "AVAILABLE",
                    ignoreCase = true,
                )
            val measuredFieldAvailable =
                field.optString("status") ==
                    "FIELD_CHART_AVAILABLE" &&
                    measuredField.optString("status") ==
                    "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE"
            val frontAvailable =
                front.optString("status") ==
                    "FRONTSIDE_STRUCTURAL_INSPECTION_AVAILABLE"

            val colourMetadata =
                listOf(
                    "color_matrix_1_present",
                    "color_matrix_2_present",
                    "forward_matrix_1_present",
                    "forward_matrix_2_present",
                    "as_shot_neutral_present",
                ).any { metadata.optBoolean(it, false) }

            val timeHint =
                metadata.has("capture_time_preferred_text") &&
                    metadata.opt("capture_time_preferred_text") !=
                    JSONObject.NULL

            val focalHint =
                metadata.has("focal_length_mm") &&
                    metadata.opt("focal_length_mm") !=
                    JSONObject.NULL

            observations.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put("source_class", sourceClass)
                    .put(
                        "axes",
                        JSONObject()
                            .put(
                                "sealed_source",
                                axis(
                                    if (sourceMeasured) {
                                        "MEASURED_SOURCE_BOUND"
                                    } else {
                                        "SEALED_OPAQUE_OR_NON_CFA_SOURCE"
                                    },
                                    "SOURCE_SHA256_ROOT",
                                ),
                            )
                            .put(
                                "source_sample_geometry",
                                axis(
                                    if (sourceMeasured) {
                                        "MEASURED"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "RASTER_INDEPENDENT_SAMPLE_LATTICE",
                                ),
                            )
                            .put(
                                "backside_signal_support",
                                axis(
                                    if (backsideMeasured) {
                                        "MEASURED_OR_SOURCE_BOUND_AUDIT"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "BACKSIDE_ONLY",
                                ),
                            )
                            .put(
                                "frontside_structure",
                                axis(
                                    if (frontAvailable) {
                                        "APPEARANCE_DERIVED_ONLY"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "FRONTSIDE_ONLY",
                                ),
                            )
                            .put(
                                "field_response",
                                axis(
                                    if (measuredFieldAvailable) {
                                        "MEASURED_COMPOSITE_SCENE_LENS_SENSOR"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "NOT_LENS_ONLY",
                                ),
                            )
                            .put(
                                "colour",
                                axis(
                                    if (colourMetadata) {
                                        "SOURCE_METADATA_HINT_ONLY"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "NO_COLORIMETRIC_PROMOTION",
                                ),
                            )
                            .put(
                                "optical_support",
                                axis(
                                    if (focalHint) {
                                        "SOURCE_METADATA_HINT_ONLY"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "NO_SFR_MTF_PSF_PROMOTION",
                                ),
                            )
                            .put(
                                "temporal",
                                axis(
                                    if (timeHint) {
                                        "SOURCE_METADATA_BOUND_HINT"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "NO_PHYSICAL_SEQUENCE_PROMOTION",
                                ),
                            )
                            .put(
                                "calibration_atlas",
                                axis(
                                    if (atlas.length() > 0) {
                                        "READ_ONLY_OBSERVATION_ATLAS"
                                    } else {
                                        "UNKNOWN"
                                    },
                                    "NON_PROMOTING",
                                ),
                            ),
                    )
                    .put(
                        "cross_axis_upgrade_allowed",
                        false,
                    )
                    .put(
                        "frontside_may_upgrade_backside",
                        false,
                    )
                    .put(
                        "metadata_hint_may_become_measurement",
                        false,
                    )
                    .put(
                        "appearance_may_become_measurement",
                        false,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (observations.length() > 0) {
                    "AXIS_AUTHORITY_MATRIX_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("observation_count", observations.length())
            .put("observations", observations)
            .put(
                "global_law",
                JSONObject()
                    .put(
                        "measured_reconstructed_appearance_separated",
                        true,
                    )
                    .put(
                        "stronger_axis_may_upgrade_weaker_axis",
                        false,
                    )
                    .put("unknown_is_valid_state", true)
                    .put(
                        "missing_metadata_means_zero",
                        false,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun axis(
        authority: String,
        note: String,
    ): JSONObject =
        JSONObject()
            .put("authority", authority)
            .put("note", note)
            .put("promoted", false)
}

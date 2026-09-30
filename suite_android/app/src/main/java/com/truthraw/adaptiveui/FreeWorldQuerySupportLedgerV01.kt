package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only ledger describing what kinds of support each sealed observation
 * can contribute to a future Free World continuous query.
 *
 * It deliberately does not resolve a value.
 */
object FreeWorldQuerySupportLedgerV01 {
    const val SCHEMA = "D.RAW/FreeWorldQuerySupportLedger/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val entries = JSONArray()
        val seen = linkedSetOf<String>()

        for (profile in profiles) {
            val sha = profile.optString("source_sha256")
            if (sha.isBlank() || !seen.add(sha)) continue

            val frontside =
                profile.optJSONObject("scene_analysis")
                    ?: JSONObject()
            val local =
                frontside.optJSONObject(
                    "deterministic_local_feature_geometry_v0_1",
                ) ?: JSONObject()
            val lattice =
                profile.optJSONObject("sample_lattice")
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

            entries.put(
                JSONObject()
                    .put("source_sha256", sha)
                    .put(
                        "source_class",
                        profile.optString(
                            "scientific_source_class",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "sample_lattice_status",
                        lattice.optString(
                            "status",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "measured_sensor_field_available",
                        field.optString("status") ==
                            "FIELD_CHART_AVAILABLE" &&
                            measuredField.optString("status") ==
                            "MEASURED_COMPOSITE_FIELD_SIGNAL_AVAILABLE",
                    )
                    .put(
                        "frontside_local_geometry_available",
                        local.optString("status") ==
                            "DETERMINISTIC_LOCAL_FEATURES_AVAILABLE",
                    )
                    .put(
                        "capture_time_metadata_hint_available",
                        metadata.opt(
                            "capture_time_preferred_text",
                        ) != null &&
                            metadata.opt(
                                "capture_time_preferred_text",
                            ) != JSONObject.NULL,
                    )
                    .put(
                        "exact_query_support_resolver_attached",
                        false,
                    )
                    .put(
                        "may_claim_measured_at_arbitrary_world_coordinate",
                        false,
                    )
                    .put(
                        "fine_lattice_unknown_positions_preserved",
                        true,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (entries.length() > 0) {
                    "QUERY_SUPPORT_LEDGER_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("observation_count", entries.length())
            .put("observations", entries)
            .put(
                "resolver_boundary",
                JSONObject()
                    .put(
                        "continuous_value_solver_implemented",
                        false,
                    )
                    .put(
                        "exact_source_footprint_resolver_implemented",
                        false,
                    )
                    .put(
                        "world_to_source_registration_promoted",
                        false,
                    )
                    .put(
                        "unknown_query_support_may_be_treated_as_zero",
                        false,
                    )
                    .put(
                        "unsupported_query_must_return_unknown",
                        true,
                    ),
            )
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}
}

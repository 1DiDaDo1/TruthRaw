package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Diagnostic-only attribution of the Scientific-Master PassArtifact route.
 *
 * The route is derived exclusively from the hash-bound native PassArtifact
 * diagnostics. Scientific-Master source-read counts are intentionally not used
 * as route evidence: they remain an independent performance observation.
 */
object ScientificMasterPassArtifactPerformanceAttributionV01 {
    const val SCHEMA =
        "D.RAW/ScientificMasterPassArtifactAttribution/0.1"

    private const val DIAGNOSTICS_SCHEMA =
        "D.RAW/ScientificMasterPassArtifactDiagnostics/0.1"
    private const val EXACT_GAUGE_TYPE = "EXACT_GAUGE_FLOAT32_BITS"
    private const val EXACT_GAUGE_VERSION = "0.3"
    private const val RETAINED_ROUTE = "retained_exact_float32_bits_v0.3"
    private const val FALLBACK_ROUTE = "canonical_v0.2_fallback"
    private const val NO_FALLBACK = "none"

    fun from(
        diagnostics: JSONObject?,
        telemetryOrigin: String,
        expectedCallerBudgetBytes: Long,
    ): JSONObject {
        val available = diagnostics?.optBoolean("available", false) == true
        val contradictions = JSONArray()

        if (!available) {
            return base(
                telemetryOrigin = telemetryOrigin,
                available = false,
                bindingVerified = false,
                routeAttribution = "UNKNOWN_FAIL_CLOSED",
                contradictions = contradictions,
            )
                .put("telemetry_unavailable", true)
        }

        val bindingVerified = diagnostics!!.optBoolean("binding_verified", false)
        val route = diagnostics.optString("route_used", "unknown")
        val fallbackReason = diagnostics.optString("fallback_reason", "unknown")
        val optimizationApplied =
            diagnostics.optBoolean("optimization_applied", false)
        val pass2ReadsAvoided =
            diagnostics.optLong("pass2_stage2_tile_reads_avoided", -1L)

        fun contradiction(condition: Boolean, code: String) {
            if (condition) contradictions.put(code)
        }

        contradiction(
            diagnostics.optString("schema") != DIAGNOSTICS_SCHEMA,
            "DIAGNOSTICS_SCHEMA_MISMATCH",
        )
        contradiction(!bindingVerified, "SCIENTIFIC_MASTER_HASH_BINDING_UNVERIFIED")
        contradiction(
            diagnostics.optString("artifact_type") != EXACT_GAUGE_TYPE,
            "ARTIFACT_TYPE_MISMATCH",
        )
        contradiction(
            diagnostics.optString("artifact_version") != EXACT_GAUGE_VERSION,
            "ARTIFACT_VERSION_MISMATCH",
        )
        contradiction(
            diagnostics.optLong("caller_budget_bytes", -1L) !=
                expectedCallerBudgetBytes,
            "CALLER_BUDGET_MISMATCH",
        )
        contradiction(
            diagnostics.optBoolean("candidate_applied", true),
            "CANDIDATE_APPLIED_FORBIDDEN",
        )
        contradiction(
            diagnostics.optBoolean("source_values_modified", true),
            "SOURCE_VALUES_MODIFIED_FORBIDDEN",
        )
        contradiction(
            diagnostics.optBoolean("creates_new_evidence", true),
            "CREATES_NEW_EVIDENCE_FORBIDDEN",
        )
        contradiction(
            diagnostics.optBoolean("scientific_writeback_allowed", true),
            "SCIENTIFIC_WRITEBACK_FORBIDDEN",
        )

        when (route) {
            RETAINED_ROUTE -> {
                contradiction(
                    !optimizationApplied,
                    "RETAINED_ROUTE_WITHOUT_OPTIMIZATION",
                )
                contradiction(
                    fallbackReason != NO_FALLBACK,
                    "RETAINED_ROUTE_WITH_FALLBACK_REASON",
                )
                contradiction(
                    pass2ReadsAvoided <= 0L,
                    "RETAINED_ROUTE_WITHOUT_AVOIDED_PASS2_READS",
                )
            }
            FALLBACK_ROUTE -> {
                contradiction(
                    optimizationApplied,
                    "CANONICAL_FALLBACK_WITH_OPTIMIZATION",
                )
                contradiction(
                    fallbackReason == NO_FALLBACK || fallbackReason == "unknown",
                    "CANONICAL_FALLBACK_WITHOUT_REASON",
                )
                contradiction(
                    pass2ReadsAvoided != 0L,
                    "CANONICAL_FALLBACK_WITH_AVOIDED_PASS2_READS",
                )
            }
            else -> contradictions.put("UNKNOWN_ROUTE")
        }

        val hasContradiction = contradictions.length() > 0
        val attribution =
            if (hasContradiction) {
                "UNKNOWN_FAIL_CLOSED"
            } else {
                when (route) {
                    RETAINED_ROUTE -> "EXACT_GAUGE_RETAINED_V0_3"
                    FALLBACK_ROUTE -> "CANONICAL_V0_2_FALLBACK"
                    else -> "UNKNOWN_FAIL_CLOSED"
                }
            }

        return base(
            telemetryOrigin = telemetryOrigin,
            available = true,
            bindingVerified = bindingVerified,
            routeAttribution = attribution,
            contradictions = contradictions,
        )
            .put("telemetry_unavailable", false)
            .put("artifact_type", diagnostics.optString("artifact_type"))
            .put("artifact_version", diagnostics.optString("artifact_version"))
            .put("route_used", route)
            .put("fallback_reason", fallbackReason)
            .put(
                "geometric_eligible_upper_bound",
                diagnostics.optLong("geometric_eligible_upper_bound", 0L),
            )
            .put(
                "eligible_retained_samples",
                diagnostics.optLong("eligible_retained_samples", 0L),
            )
            .put(
                "retained_bytes_requested",
                diagnostics.optLong("retained_bytes_requested", 0L),
            )
            .put(
                "retained_bytes_reserved",
                diagnostics.optLong("retained_bytes_reserved", 0L),
            )
            .put(
                "retained_bytes_used",
                diagnostics.optLong("retained_bytes_used", 0L),
            )
            .put(
                "caller_budget_bytes",
                diagnostics.optLong("caller_budget_bytes", 0L),
            )
            .put(
                "candidate_logical_resident_upper_bound",
                diagnostics.optLong(
                    "candidate_logical_resident_upper_bound",
                    0L,
                ),
            )
            .put(
                "stage2_gauge_scan_passes_actually_used",
                diagnostics.optLong(
                    "stage2_gauge_scan_passes_actually_used",
                    0L,
                ),
            )
            .put("pass2_stage2_tile_reads_avoided", pass2ReadsAvoided)
            .put("optimization_applied", optimizationApplied)
            .put(
                "candidate_applied",
                diagnostics.optBoolean("candidate_applied", true),
            )
            .put(
                "source_values_modified",
                diagnostics.optBoolean("source_values_modified", true),
            )
            .put(
                "creates_new_evidence",
                diagnostics.optBoolean("creates_new_evidence", true),
            )
            .put(
                "scientific_writeback_allowed",
                diagnostics.optBoolean("scientific_writeback_allowed", true),
            )
    }

    private fun base(
        telemetryOrigin: String,
        available: Boolean,
        bindingVerified: Boolean,
        routeAttribution: String,
        contradictions: JSONArray,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "DIAGNOSTIC_ROUTE_ATTRIBUTION")
            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("telemetry_available", available)
            .put("binding_verified", bindingVerified)
            .put("telemetry_origin", telemetryOrigin)
            .put("route_attribution", routeAttribution)
            .put(
                "route_attribution_contradiction",
                contradictions.length() > 0,
            )
            .put("contradictions", contradictions)
            .put("attribution_uses_source_read_count", false)
            .put("timing_is_scientific_evidence", false)
            .put("timing_may_change_scientific_authority", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

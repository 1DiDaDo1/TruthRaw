package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only aggregation of per-profile performance telemetry.
 *
 * This is deliberately outside scientific authority. It exists only so a
 * Foundation export can prove which runtime stages and shared-context cache
 * paths were exercised for each observation.
 */
object FreeWorldPerformanceDiagnosticsV01 {
    const val SCHEMA = "D.RAW/FreeWorldPerformanceDiagnostics/0.1"

    fun build(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        var telemetryProfiles = 0
        var totalElapsedMs = 0.0
        var sharedReportedAudits = 0
        var sharedHits = 0
        var sharedMisses = 0
        var releaseAttempted = 0
        var releaseSucceeded = 0
        var allSingleSourcePolicy = true
        var allCrossObservationReuseDisallowed = true

        for (profile in profiles) {
            val sourceSha = profile.optString("source_sha256")
            val cacheGeneration =
                profile.optString("profile_cache_generation")
            val diagnostics =
                profile.optJSONObject("performance_diagnostics_v0_1")
            if (diagnostics == null) {
                observations.put(
                    JSONObject()
                        .put("source_sha256", sourceSha)
                        .put(
                            "profile_cache_generation",
                            if (cacheGeneration.isBlank()) JSONObject.NULL else cacheGeneration,
                        )
                        .put("status", "UNKNOWN_NO_PROFILE_TELEMETRY"),
                )
                continue
            }

            telemetryProfiles++
            val elapsed =
                diagnostics.optDouble("profile_elapsed_ms", Double.NaN)
            if (elapsed.isFinite()) totalElapsedMs += elapsed

            val shared =
                diagnostics.optJSONObject(
                    "shared_scientific_preparation",
                ) ?: JSONObject()
            sharedReportedAudits +=
                shared.optInt("reported_audit_count", 0)
            sharedHits += shared.optInt("cache_hit_count", 0)
            sharedMisses += shared.optInt("cache_miss_count", 0)
            if (
                shared.optBoolean(
                    "cache_release_after_profile_attempted",
                    false,
                )
            ) {
                releaseAttempted++
            }
            if (
                shared.optBoolean(
                    "cache_release_after_profile_succeeded",
                    false,
                )
            ) {
                releaseSucceeded++
            }
            allSingleSourcePolicy =
                allSingleSourcePolicy &&
                    shared.optBoolean(
                        "single_source_context_policy",
                        false,
                    )
            allCrossObservationReuseDisallowed =
                allCrossObservationReuseDisallowed &&
                    !shared.optBoolean(
                        "cross_observation_context_reuse_allowed",
                        true,
                    )

            observations.put(
                JSONObject()
                    .put("source_sha256", sourceSha)
                    .put(
                        "profile_cache_generation",
                        if (cacheGeneration.isBlank()) JSONObject.NULL else cacheGeneration,
                    )
                    .put("status", "PROFILE_RUNTIME_TELEMETRY_AVAILABLE")
                    .put(
                        "profile_elapsed_ms",
                        if (elapsed.isFinite()) {
                            elapsed
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "stages",
                        diagnostics.optJSONArray("stages")
                            ?: JSONArray(),
                    )
                    .put(
                        "shared_scientific_preparation",
                        shared,
                    ),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "expected_profile_cache_generation",
                UniversalSourceProfiler.CACHE_GENERATION,
            )
            .put(
                "status",
                if (telemetryProfiles > 0) {
                    "PERFORMANCE_DIAGNOSTICS_AVAILABLE"
                } else {
                    "UNKNOWN_NO_PROFILE_TELEMETRY"
                },
            )
            .put("profile_count", profiles.size)
            .put("profile_telemetry_count", telemetryProfiles)
            .put(
                "aggregate_profile_elapsed_ms",
                if (telemetryProfiles > 0) {
                    totalElapsedMs
                } else {
                    JSONObject.NULL
                },
            )
            .put("observations", observations)
            .put(
                "shared_scientific_preparation_summary",
                JSONObject()
                    .put(
                        "reported_audit_count",
                        sharedReportedAudits,
                    )
                    .put("cache_hit_count", sharedHits)
                    .put("cache_miss_count", sharedMisses)
                    .put(
                        "profiles_cache_release_attempted",
                        releaseAttempted,
                    )
                    .put(
                        "profiles_cache_release_succeeded",
                        releaseSucceeded,
                    )
                    .put(
                        "all_reported_profiles_single_source_context_policy",
                        telemetryProfiles > 0 &&
                            allSingleSourcePolicy,
                    )
                    .put(
                        "all_reported_profiles_cross_observation_reuse_disallowed",
                        telemetryProfiles > 0 &&
                            allCrossObservationReuseDisallowed,
                    )
                    .put("max_process_local_contexts", 1),
            )
            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
            .put("timing_is_scientific_evidence", false)
            .put("performance_changes_measurement_authority", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }
}

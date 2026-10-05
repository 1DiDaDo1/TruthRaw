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

    private fun scientificMasterTileReadAttribution(
        bindProfile: JSONObject?,
        passArtifactAttribution: JSONObject?,
    ): JSONObject {
        val available = bindProfile?.optBoolean("available", false) == true
        val rawCalls = bindProfile?.optLong("source_read_raw_call_count", 0L) ?: 0L
        val reconstructionCalls =
            bindProfile?.optLong("reconstruction_call_count", 0L) ?: 0L
        val rawMs =
            bindProfile?.optDouble("source_read_raw_ms", Double.NaN)
                ?: Double.NaN
        val safeTwoPassCount =
            reconstructionCalls > 0L &&
                reconstructionCalls <= Long.MAX_VALUE / 2L
        val canonicalTwoPassRawCalls =
            if (safeTwoPassCount) reconstructionCalls * 2L else -1L

        val routeAttribution =
            passArtifactAttribution?.optString(
                "route_attribution",
                "UNKNOWN_FAIL_CLOSED",
            ) ?: "UNKNOWN_FAIL_CLOSED"
        val routeTelemetryUsable =
            passArtifactAttribution != null &&
                passArtifactAttribution.optString("schema") ==
                    "D.RAW/ScientificMasterPassArtifactAttribution/0.1" &&
                passArtifactAttribution.optString("status") ==
                    "DIAGNOSTIC_ROUTE_ATTRIBUTION" &&
                passArtifactAttribution.optString("authority") ==
                    "DIAGNOSTIC_RUNTIME_ONLY" &&
                passArtifactAttribution.optBoolean("telemetry_available", false) &&
                passArtifactAttribution.optBoolean("binding_verified", false) &&
                !passArtifactAttribution.optBoolean(
                    "route_attribution_contradiction",
                    true,
                ) &&
                !passArtifactAttribution.optBoolean("creates_new_evidence", true) &&
                !passArtifactAttribution.optBoolean(
                    "scientific_writeback_allowed",
                    true,
                )

        val exactGaugeRoute =
            routeTelemetryUsable &&
                routeAttribution == "EXACT_GAUGE_RETAINED_V0_3" &&
                passArtifactAttribution!!.optBoolean("optimization_applied", false) &&
                !passArtifactAttribution.optBoolean("candidate_applied", true) &&
                !passArtifactAttribution.optBoolean("source_values_modified", true) &&
                passArtifactAttribution.optLong(
                    "stage2_gauge_scan_passes_actually_used",
                    -1L,
                ) == 1L &&
                passArtifactAttribution.optLong(
                    "pass2_stage2_tile_reads_avoided",
                    -1L,
                ) == reconstructionCalls

        val canonicalFallbackRoute =
            routeTelemetryUsable &&
                routeAttribution == "CANONICAL_V0_2_FALLBACK" &&
                !passArtifactAttribution!!.optBoolean("optimization_applied", true) &&
                !passArtifactAttribution.optBoolean("candidate_applied", true) &&
                !passArtifactAttribution.optBoolean("source_values_modified", true) &&
                passArtifactAttribution.optLong(
                    "pass2_stage2_tile_reads_avoided",
                    -1L,
                ) == 0L

        val expectedActiveRouteRawCalls =
            when {
                exactGaugeRoute -> reconstructionCalls
                canonicalFallbackRoute && safeTwoPassCount -> canonicalTwoPassRawCalls
                else -> -1L
            }
        val reconciles =
            available &&
                expectedActiveRouteRawCalls > 0L &&
                rawCalls == expectedActiveRouteRawCalls

        val status =
            when {
                exactGaugeRoute && reconciles ->
                    "EXACT_GAUGE_V0_3_ONE_PASS_RECONCILED"
                canonicalFallbackRoute && reconciles ->
                    "CANONICAL_V0_2_TWO_PASS_RECONCILED"
                else -> "UNKNOWN_FAIL_CLOSED"
            }
        val attributionBasis =
            when {
                exactGaugeRoute ->
                    "EXPLICIT_PASS_ARTIFACT_DIAGNOSTICS_EXACT_GAUGE_V0_3"
                canonicalFallbackRoute ->
                    "EXPLICIT_PASS_ARTIFACT_DIAGNOSTICS_CANONICAL_V0_2_FALLBACK"
                else -> "UNKNOWN_FAIL_CLOSED"
            }
        val pass1RawCalls: Any =
            if (reconciles) reconstructionCalls else JSONObject.NULL
        val pass2RawCalls: Any =
            when {
                !reconciles -> JSONObject.NULL
                exactGaugeRoute -> 0L
                canonicalFallbackRoute -> reconstructionCalls
                else -> JSONObject.NULL
            }
        val pass2RawCallsAvoided: Any =
            when {
                !reconciles -> JSONObject.NULL
                exactGaugeRoute -> reconstructionCalls
                canonicalFallbackRoute -> 0L
                else -> JSONObject.NULL
            }
        val unattributedRawCalls =
            if (reconciles) 0L else rawCalls

        return JSONObject()
            .put(
                "schema",
                "D.RAW/ScientificMasterTileReadAttribution/0.1",
            )
            .put("status", status)
            .put("attribution_basis", attributionBasis)
            .put("route_attribution", routeAttribution)
            .put("route_attribution_verified", routeTelemetryUsable)
            .put(
                "pass_1_role",
                "SCIENTIFIC_MASTER_DIGEST_RECONSTRUCTION_AUTHORITY_AND_HIGH16_GAUGE",
            )
            .put(
                "pass_2_role",
                "EXACT_SELF_GAUGE_LOW16_RESOLUTION",
            )
            .put("aggregate_source_read_raw_call_count", rawCalls)
            .put(
                "expected_two_pass_raw_call_count",
                if (safeTwoPassCount) canonicalTwoPassRawCalls else JSONObject.NULL,
            )
            .put(
                "expected_active_route_raw_call_count",
                if (expectedActiveRouteRawCalls > 0L) {
                    expectedActiveRouteRawCalls
                } else {
                    JSONObject.NULL
                },
            )
            .put("pass_1_raw_call_count", pass1RawCalls)
            .put("pass_2_raw_call_count", pass2RawCalls)
            .put("pass_2_raw_calls_avoided", pass2RawCallsAvoided)
            .put("unattributed_raw_call_count", unattributedRawCalls)
            .put("raw_call_count_reconciles", reconciles)
            .put(
                "aggregate_source_read_raw_ms",
                if (rawMs.isFinite()) rawMs else JSONObject.NULL,
            )
            .put("per_pass_source_read_timing_available", false)
            .put("per_pass_timing_inferred", false)
            .put("optimization_applied", exactGaugeRoute && reconciles)
            .put("source_values_modified", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
    }

    fun build(profiles: List<JSONObject>): JSONObject {
        val observations = JSONArray()
        var telemetryProfiles = 0
        var totalElapsedMs = 0.0
        var sharedReportedAudits = 0
        var sharedHits = 0
        var sharedMisses = 0
        var currentRunDerivedStageHits = 0
        var currentRunDerivedStageMisses = 0
        var currentRunNativeAuditCompute = 0
        var cachedOriginNativeAuditTelemetry = 0
        var profilesWithCachedOriginComputeTelemetry = 0
        var releaseAttempted = 0
        var releaseSucceeded = 0
        var allSingleSourcePolicy = true
        var allCrossObservationReuseDisallowed = true

        for (profile in profiles) {
            val sourceSha = profile.optString("source_sha256")
            val t5CorridorAuditBinding =
                ResearchPerformanceT5CorridorBindingV01.forProfile(profile)
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
                        .put("status", "UNKNOWN_NO_PROFILE_TELEMETRY")
                        .put(
                            "t5_corridor_audit_binding_v0_1",
                            t5CorridorAuditBinding,
                        ),
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
            currentRunDerivedStageHits +=
                shared.optInt(
                    "current_run_derived_stage_cache_hit_count",
                    0,
                )
            currentRunDerivedStageMisses +=
                shared.optInt(
                    "current_run_derived_stage_cache_miss_count",
                    0,
                )
            currentRunNativeAuditCompute +=
                shared.optInt(
                    "current_run_native_audit_compute_count",
                    0,
                )
            val cachedOriginCount =
                shared.optInt(
                    "cached_origin_native_audit_telemetry_count",
                    0,
                )
            cachedOriginNativeAuditTelemetry += cachedOriginCount
            if (cachedOriginCount > 0) {
                profilesWithCachedOriginComputeTelemetry++
            }
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

            val n2LocalExecution =
                shared.optJSONObject("n2_local_execution") ?: JSONObject()
            val nativeTimingOrigin =
                n2LocalExecution.optString(
                    "native_phase_timing_origin",
                    "UNAVAILABLE",
                )
            val passArtifactAttribution =
                n2LocalExecution.optJSONObject(
                    "scientific_master_pass_artifact_attribution_v0_1",
                )
            val rawScientificMasterBindProfile =
                profile
                    .optJSONObject("n2_local_spatial_binding")
                    ?.optJSONObject("scientific_master_bind_profile_v0_1")
            val scientificMasterBindProfile =
                if (rawScientificMasterBindProfile != null) {
                    JSONObject(rawScientificMasterBindProfile.toString())
                        .put("timing_origin", nativeTimingOrigin)
                        .put(
                            "timing_represents_current_run",
                            nativeTimingOrigin == "CURRENT_PROFILE_RUN",
                        )
                        .put(
                            "timing_is_cached_origin_compute",
                            nativeTimingOrigin ==
                                "CACHED_DERIVED_STAGE_ORIGIN_COMPUTE",
                        )
                        .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
                } else {
                    JSONObject()
                        .put(
                            "schema",
                            "D.RAW/ScientificMasterBindProfile/0.1",
                        )
                        .put("available", false)
                        .put("timing_origin", nativeTimingOrigin)
                        .put("timing_represents_current_run", false)
                        .put(
                            "timing_is_cached_origin_compute",
                            nativeTimingOrigin ==
                                "CACHED_DERIVED_STAGE_ORIGIN_COMPUTE",
                        )
                        .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
                        .put("timing_is_scientific_evidence", false)
                        .put(
                            "timing_may_change_scientific_authority",
                            false,
                        )
                }
            scientificMasterBindProfile.put(
                "tile_read_attribution_v0_1",
                scientificMasterTileReadAttribution(
                    rawScientificMasterBindProfile,
                    passArtifactAttribution,
                ),
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
                        "profile_elapsed_scope",
                        diagnostics.optString(
                            "profile_elapsed_scope",
                            "CURRENT_PROFILE_RUN",
                        ),
                    )
                    .put(
                        "stage_elapsed_scope",
                        diagnostics.optString(
                            "stage_elapsed_scope",
                            "CURRENT_PROFILE_RUN",
                        ),
                    )
                    .put(
                        "timing_provenance_policy",
                        diagnostics.optString(
                            "timing_provenance_policy",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "contains_cached_origin_compute_telemetry",
                        diagnostics.optBoolean(
                            "contains_cached_origin_compute_telemetry",
                            false,
                        ),
                    )
                    .put(
                        "stages",
                        diagnostics.optJSONArray("stages")
                            ?: JSONArray(),
                    )
                    .put(
                        "shared_scientific_preparation",
                        shared,
                    )
                    .put(
                        "scientific_master_bind_profile_v0_1",
                        scientificMasterBindProfile,
                    )
                    .put(
                        "t5_corridor_audit_binding_v0_1",
                        t5CorridorAuditBinding,
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
            .put(
                "aggregate_profile_elapsed_scope",
                "CURRENT_PROFILE_RUN",
            )
            .put(
                "timing_provenance_policy",
                "CURRENT_RUN_SEPARATE_FROM_CACHED_ORIGIN_COMPUTE",
            )
            .put(
                "profiles_with_cached_origin_compute_telemetry",
                profilesWithCachedOriginComputeTelemetry,
            )
            .put("observations", observations)
            .put(
                "shared_scientific_preparation_summary",
                JSONObject()
                    .put(
                        "reported_audit_count",
                        sharedReportedAudits,
                    )
                    .put(
                        "native_shared_prepare_cache_count_scope",
                        "ORIGIN_NATIVE_EXECUTION",
                    )
                    .put("cache_hit_count", sharedHits)
                    .put("cache_miss_count", sharedMisses)
                    .put(
                        "current_run_derived_stage_cache_hit_count",
                        currentRunDerivedStageHits,
                    )
                    .put(
                        "current_run_derived_stage_cache_miss_count",
                        currentRunDerivedStageMisses,
                    )
                    .put(
                        "current_run_native_audit_compute_count",
                        currentRunNativeAuditCompute,
                    )
                    .put(
                        "cached_origin_native_audit_telemetry_count",
                        cachedOriginNativeAuditTelemetry,
                    )
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

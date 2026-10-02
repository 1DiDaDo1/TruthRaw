package com.truthraw.adaptiveui

import android.os.SystemClock
import org.json.JSONArray
import org.json.JSONObject

/**
 * Diagnostic-only runtime timing and shared scientific-preparation telemetry.
 *
 * This object never changes source samples, Scientific Master values, solver
 * decisions, promotion state or authority. Timings are monotonic wall durations
 * for performance analysis only.
 */
object ResearchPerformanceDiagnosticsV01 {
    const val SCHEMA = "D.RAW/ResearchPerformanceDiagnostics/0.1"

    class ProfileTrace {
        private val profileStartedNs = SystemClock.elapsedRealtimeNanos()
        private val stages = JSONArray()
        private var currentStage: String? = null
        private var currentStageStartedNs: Long = profileStartedNs
        private var currentDerivedStageCacheHit: Boolean = false

        fun onProgress(event: String) {
            val now = SystemClock.elapsedRealtimeNanos()
            closeCurrent(now)
            val cacheHit = event.endsWith("_CACHE_HIT")
            currentStage =
                if (cacheHit) {
                    event.removeSuffix("_CACHE_HIT")
                } else {
                    event
                }
            currentStageStartedNs = now
            currentDerivedStageCacheHit = cacheHit
        }

        fun attach(
            profile: JSONObject,
            sharedCacheReleaseAttempted: Boolean,
            sharedCacheReleaseSucceeded: Boolean,
        ) {
            val finishedNs = SystemClock.elapsedRealtimeNanos()
            closeCurrent(finishedNs)

            val shared =
                summarizeSharedScientificPreparation(
                    profile = profile,
                    cacheReleaseAttempted = sharedCacheReleaseAttempted,
                    cacheReleaseSucceeded = sharedCacheReleaseSucceeded,
                )

            profile.put(
                "performance_diagnostics_v0_1",
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("status", "RUNTIME_DIAGNOSTICS_AVAILABLE")
                    .put(
                        "profile_cache_generation",
                        UniversalSourceProfiler.CACHE_GENERATION,
                    )
                    .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
                    .put(
                        "source_sha256",
                        profile.optString("source_sha256"),
                    )
                    .put(
                        "profile_elapsed_ms",
                        nanosToMillis(finishedNs - profileStartedNs),
                    )
                    .put("stage_count", stages.length())
                    .put("stages", stages)
                    .put("shared_scientific_preparation", shared)
                    .put("timing_is_scientific_evidence", false)
                    .put("timing_may_change_scientific_authority", false)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false),
            )
        }

        private fun closeCurrent(nowNs: Long) {
            val stage = currentStage ?: return
            stages.put(
                JSONObject()
                    .put("stage", stage)
                    .put(
                        "elapsed_ms",
                        nanosToMillis(nowNs - currentStageStartedNs),
                    )
                    .put(
                        "derived_stage_cache_hit",
                        currentDerivedStageCacheHit,
                    )
                    .put("authority", "DIAGNOSTIC_RUNTIME_ONLY"),
            )
            currentStage = null
            currentDerivedStageCacheHit = false
        }
    }

    private fun summarizeSharedScientificPreparation(
        profile: JSONObject,
        cacheReleaseAttempted: Boolean,
        cacheReleaseSucceeded: Boolean,
    ): JSONObject {
        val auditSpecs =
            listOf(
                "N2_LOCAL_SPATIAL" to "n2_local_spatial_binding",
                "N2_STRUCTURE_SUPPORT" to "n2_structure_support_binding",
                "N2_SAMPLE_SUPPORT_DISTANCE" to "n2_sample_support_distance",
                "ANCHOR_LOCAL_RECONSTRUCTION" to
                    "anchor_constrained_local_reconstruction",
            )

        val audits = JSONArray()
        var reported = 0
        var hits = 0
        var misses = 0
        val reportedValues = mutableListOf<Boolean>()

        for ((auditId, profileKey) in auditSpecs) {
            val audit = profile.optJSONObject(profileKey)
            val hasTelemetry =
                audit?.has("shared_pipeline_prepare_cache_hit") == true
            val cacheHit =
                if (hasTelemetry) {
                    audit!!.optBoolean(
                        "shared_pipeline_prepare_cache_hit",
                        false,
                    )
                } else {
                    false
                }

            if (hasTelemetry) {
                reported++
                reportedValues += cacheHit
                if (cacheHit) hits++ else misses++
            }

            audits.put(
                JSONObject()
                    .put("audit", auditId)
                    .put(
                        "status",
                        audit?.optString("status", "UNAVAILABLE")
                            ?: "UNAVAILABLE",
                    )
                    .put("telemetry_reported", hasTelemetry)
                    .put(
                        "shared_pipeline_prepare_cache_hit",
                        if (hasTelemetry) cacheHit else JSONObject.NULL,
                    ),
            )
        }

        val first = reportedValues.firstOrNull()
        val later = reportedValues.drop(1)
        val n2Local =
            profile.optJSONObject("n2_local_spatial_binding")
        val sparseReferenceReported =
            n2Local?.has(
                "v01_sparse_reference_reuse_verified",
            ) == true
        val sparseReferenceVerified =
            n2Local?.optBoolean(
                "v01_sparse_reference_reuse_verified",
                false,
            ) ?: false
        val v01RerunPerformed =
            n2Local?.optBoolean(
                "v01_rerun_performed",
                true,
            ) ?: true
        val sparseReferenceIndexComplete =
            n2Local?.optBoolean(
                "v01_sparse_reference_index_complete",
                false,
            ) ?: false
        val rowBandReported =
            n2Local?.has("row_band_reuse_active") == true
        val rowBandActive =
            n2Local?.optBoolean(
                "row_band_reuse_active",
                false,
            ) ?: false

        return JSONObject()
            .put(
                "policy",
                "ONE_SOURCE_CONTEXT_MAX_FIRST_EXECUTED_NATIVE_AUDIT_PREPARES_LATER_SAME_RAW_AUDITS_REUSE",
            )
            .put("source_sha256", profile.optString("source_sha256"))
            .put("max_process_local_contexts", 1)
            .put("single_source_context_policy", true)
            .put("cross_observation_context_reuse_allowed", false)
            .put(
                "n2_local_execution",
                JSONObject()
                    .put(
                        "sparse_reference_telemetry_reported",
                        sparseReferenceReported,
                    )
                    .put(
                        "v01_sparse_reference_reuse_verified",
                        if (sparseReferenceReported) {
                            sparseReferenceVerified
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_rerun_performed",
                        if (sparseReferenceReported) {
                            v01RerunPerformed
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_sparse_reference_index_complete",
                        if (sparseReferenceReported) {
                            sparseReferenceIndexComplete
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_telemetry_reported",
                        rowBandReported,
                    )
                    .put(
                        "row_band_reuse_active",
                        if (rowBandReported) {
                            rowBandActive
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_row_band_fill_count",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "v01_row_band_fill_count",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_row_band_served_request_count",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "v01_row_band_served_request_count",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_row_band_cache_hit_request_count",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "v01_row_band_cache_hit_request_count",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "v01_row_band_fallback_request_count",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "v01_row_band_fallback_request_count",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_fill_count_total",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "row_band_fill_count_total",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_served_request_count_total",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "row_band_served_request_count_total",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_cache_hit_request_count_total",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "row_band_cache_hit_request_count_total",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_fallback_request_count_total",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "row_band_fallback_request_count_total",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_peak_cache_bytes",
                        if (rowBandReported) {
                            n2Local?.optLong(
                                "row_band_peak_cache_bytes",
                                0L,
                            ) ?: 0L
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_band_scientific_values_modified",
                        if (rowBandReported) {
                            n2Local?.optBoolean(
                                "row_band_scientific_values_modified",
                                true,
                            ) ?: true
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "native_phase_timing_available",
                        n2Local?.optBoolean(
                            "native_phase_timing_available",
                            false,
                        ) ?: false,
                    )
                    .put(
                        "phase_shared_acquire_ms",
                        n2Local?.optDouble(
                            "phase_shared_acquire_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_shared_context_lock_wait_ms",
                        n2Local?.optDouble(
                            "phase_shared_context_lock_wait_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_v01_cfa_audit_ms",
                        n2Local?.optDouble(
                            "phase_v01_cfa_audit_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_center_excluded_ms",
                        n2Local?.optDouble(
                            "phase_center_excluded_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_confidence_derive_ms",
                        n2Local?.optDouble(
                            "phase_confidence_derive_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_factored_derive_ms",
                        n2Local?.optDouble(
                            "phase_factored_derive_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_factored_encode_ms",
                        n2Local?.optDouble(
                            "phase_factored_encode_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_write_readback_reverify_ms",
                        n2Local?.optDouble(
                            "phase_write_readback_reverify_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_total_bridge_ms",
                        n2Local?.optDouble(
                            "phase_total_bridge_ms",
                            0.0,
                        ) ?: 0.0,
                    )
                    .put(
                        "phase_timing_is_scientific_evidence",
                        n2Local?.optBoolean(
                            "phase_timing_is_scientific_evidence",
                            true,
                        ) ?: true,
                    )
                    .put(
                        "phase_timing_may_change_scientific_authority",
                        n2Local?.optBoolean(
                            "phase_timing_may_change_scientific_authority",
                            true,
                        ) ?: true,
                    )
                    .put(
                        "subphase_timing",
                        JSONObject()
                            .put(
                                "shared_acquire_subphase_timing_available",
                                n2Local?.optBoolean(
                                    "shared_acquire_subphase_timing_available",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "shared_acquire_probe_seal_ms",
                                n2Local?.optDouble(
                                    "shared_acquire_probe_seal_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "shared_acquire_cache_lookup_ms",
                                n2Local?.optDouble(
                                    "shared_acquire_cache_lookup_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "shared_acquire_prepare_total_ms",
                                n2Local?.optDouble(
                                    "shared_acquire_prepare_total_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "prepare_seal_source_ms",
                                n2Local?.optDouble(
                                    "prepare_seal_source_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "prepare_pre_open_reverify_ms",
                                n2Local?.optDouble(
                                    "prepare_pre_open_reverify_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "prepare_open_dng_adapter_ms",
                                n2Local?.optDouble(
                                    "prepare_open_dng_adapter_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "prepare_bind_scientific_master_ms",
                                n2Local?.optDouble(
                                    "prepare_bind_scientific_master_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "prepare_summarize_authority_field_ms",
                                n2Local?.optDouble(
                                    "prepare_summarize_authority_field_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "authority_field_fused_into_scientific_master_pass",
                                n2Local?.optBoolean(
                                    "authority_field_fused_into_scientific_master_pass",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "authority_field_replay_pass_performed",
                                n2Local?.optBoolean(
                                    "authority_field_replay_pass_performed",
                                    true,
                                ) ?: true,
                            )
                            .put(
                                "authority_field_fused_finalize_ms",
                                n2Local?.optDouble(
                                    "authority_field_fused_finalize_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "authority_direct_record_streaming_active",
                                n2Local?.optBoolean(
                                    "authority_direct_record_streaming_active",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "authority_temporary_record_vector_used",
                                n2Local?.optBoolean(
                                    "authority_temporary_record_vector_used",
                                    true,
                                ) ?: true,
                            )
                            .put(
                                "authority_direct_byte_encoding_active",
                                n2Local?.optBoolean(
                                    "authority_direct_byte_encoding_active",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "authority_generic_record_validation_bypassed",
                                n2Local?.optBoolean(
                                    "authority_generic_record_validation_bypassed",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "authority_canonical_record_bytes",
                                n2Local?.optLong(
                                    "authority_canonical_record_bytes",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "authority_hash_batch_record_capacity",
                                n2Local?.optLong(
                                    "authority_hash_batch_record_capacity",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "authority_hash_batch_bytes",
                                n2Local?.optLong(
                                    "authority_hash_batch_bytes",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "authority_direct_record_stream_ms",
                                n2Local?.optDouble(
                                    "authority_direct_record_stream_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "authority_direct_record_stream_tile_count",
                                n2Local?.optLong(
                                    "authority_direct_record_stream_tile_count",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "authority_direct_record_stream_record_count",
                                n2Local?.optLong(
                                    "authority_direct_record_stream_record_count",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "authority_accumulator_resident_bytes_upper_bound",
                                n2Local?.optLong(
                                    "authority_accumulator_resident_bytes_upper_bound",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "prepare_total_instrumented_ms",
                                n2Local?.optDouble(
                                    "prepare_total_instrumented_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_subphase_timing_available",
                                n2Local?.optBoolean(
                                    "center_excluded_subphase_timing_available",
                                    false,
                                ) ?: false,
                            )
                            .put(
                                "center_excluded_fill_stage2_ms",
                                n2Local?.optDouble(
                                    "center_excluded_fill_stage2_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_candidate_loop_ms",
                                n2Local?.optDouble(
                                    "center_excluded_candidate_loop_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_predictor_estimate_ms",
                                n2Local?.optDouble(
                                    "center_excluded_predictor_estimate_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_final_hash_ms",
                                n2Local?.optDouble(
                                    "center_excluded_final_hash_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_total_instrumented_ms",
                                n2Local?.optDouble(
                                    "center_excluded_total_instrumented_ms",
                                    0.0,
                                ) ?: 0.0,
                            )
                            .put(
                                "center_excluded_tile_count",
                                n2Local?.optLong(
                                    "center_excluded_tile_count",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "center_excluded_candidate_tile_count",
                                n2Local?.optLong(
                                    "center_excluded_candidate_tile_count",
                                    0L,
                                ) ?: 0L,
                            )
                            .put(
                                "center_excluded_candidate_center_count",
                                n2Local?.optLong(
                                    "center_excluded_candidate_center_count",
                                    0L,
                                ) ?: 0L,
                            )
                            .put("authority", "DIAGNOSTIC_RUNTIME_ONLY")
                            .put("timing_is_scientific_evidence", false)
                            .put(
                                "timing_may_change_scientific_authority",
                                false,
                            ),
                    )
                    .put(
                        "bounded_index_fallback_policy",
                        "LEGACY_V0_2_1_RERUN_IF_SPARSE_INDEX_INCOMPLETE",
                    )
                    .put(
                        "scientific_output_contract",
                        "N2_CENTER_EXCLUDED_SPATIAL_AUDIT_V0_2_1_BYTE_IDENTICAL",
                    )
                    .put("authority", "DIAGNOSTIC_RUNTIME_ONLY"),
            )
            .put("audit_order", audits)
            .put("reported_audit_count", reported)
            .put("cache_hit_count", hits)
            .put("cache_miss_count", misses)
            .put(
                "first_reported_audit_cache_hit",
                first ?: JSONObject.NULL,
            )
            .put(
                "first_reported_audit_is_prepare_miss",
                first?.not() ?: false,
            )
            .put(
                "subsequent_reported_audits_all_cache_hit",
                later.isNotEmpty() && later.all { it },
            )
            .put(
                "cache_release_after_profile_attempted",
                cacheReleaseAttempted,
            )
            .put(
                "cache_release_after_profile_succeeded",
                cacheReleaseSucceeded,
            )
            .put("source_values_modified", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun nanosToMillis(nanos: Long): Double =
        nanos.coerceAtLeast(0L).toDouble() / 1_000_000.0
}

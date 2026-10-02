package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import kotlin.math.ceil
import kotlin.math.floor

/**
 * Same-observation spatial binding between Dark Chroma frontside tiles and the
 * existing N2 Factored Confidence v0.3.1 tile field.
 *
 * This is a binding/audit layer only. It does not promote N2, does not estimate
 * a replacement colour and does not alter Scientific Master/D.RAWnegative.
 */
object N2LocalSpatialBindingAudit {
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/N2LocalSpatialBinding/0.1")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "DIAGNOSTIC_BINDING_ONLY")
            .put("local_n2_backside_binding_available", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    fun analyze(
        resolver: ContentResolver,
        sourceUri: Uri,
        sourceSha256: String,
        cacheDir: File,
        frontsideV01: JSONObject?,
    ): JSONObject {
        if (frontsideV01 == null) {
            return unavailable(sourceSha256, "FRONTSIDE_V0_1_MISSING")
        }
        if (
            frontsideV01.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.1" ||
            frontsideV01.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "FRONTSIDE_V0_1_BINDING_MISMATCH")
        }

        val temp = runCatching {
            File.createTempFile("draw_n2_factored_binding_", ".json", cacheDir)
        }.getOrNull()
            ?: return unavailable(sourceSha256, "TEMP_FILE_CREATE_FAILED")

        try {
            val statusText = try {
                resolver.openFileDescriptor(sourceUri, "r")?.use { src ->
                    ParcelFileDescriptor.open(
                        temp,
                        ParcelFileDescriptor.MODE_CREATE or
                            ParcelFileDescriptor.MODE_TRUNCATE or
                            ParcelFileDescriptor.MODE_READ_WRITE,
                    ).use { dst ->
                        TruthNegativeN2FactoredConfidenceBridge.exportAndVerify(
                            src.fd,
                            dst.fd,
                            MAX_SOURCE_RESIDENT_BYTES,
                            MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            } catch (error: Throwable) {
                null
            } ?: return unavailable(sourceSha256, "N2_FACTORED_BRIDGE_FAILED")

            val status = runCatching { JSONObject(statusText) }.getOrNull()
                ?: return unavailable(sourceSha256, "N2_FACTORED_STATUS_INVALID")
            if (status.optInt("status", -999) != 0) {
                return unavailable(
                    sourceSha256,
                    "N2_FACTORED_STATUS_" + status.optInt("status", -999),
                )
            }
            if (
                !status.optBoolean("postWriteVerified", false) ||
                status.optString("sourceSha256") != sourceSha256 ||
                (
                    status.optBoolean("rowBandReuseActive", false) &&
                        status.optBoolean(
                            "rowBandScientificValuesModified",
                            true,
                        )
                    )
            ) {
                return unavailable(sourceSha256, "N2_FACTORED_STATUS_BINDING_MISMATCH")
            }

            val factoredText = runCatching { temp.readText() }.getOrNull()
                ?: return unavailable(sourceSha256, "N2_FACTORED_FILE_READ_FAILED")
            val factored = runCatching { JSONObject(factoredText) }.getOrNull()
                ?: return unavailable(sourceSha256, "N2_FACTORED_FILE_INVALID")

            if (
                factored.optString("schema") !=
                "D.RAW/TruthNegative/N2FactoredConfidenceState/0.3.1" ||
                factored.optString("source_sha256") != sourceSha256 ||
                !factored.optBoolean(
                    "exact_confidence_field_binding_verified",
                    false,
                ) ||
                factored.optBoolean("promotion_eligible", true) ||
                factored.optBoolean("candidate_applied", true) ||
                factored.optBoolean("scientific_writeback_allowed", true) ||
                factored.optBoolean("support_distance_admitted", true)
            ) {
                return unavailable(sourceSha256, "N2_FACTORED_FILE_CONTRACT_MISMATCH")
            }

            val sourceWidth = factored.optInt("source_width", 0)
            val sourceHeight = factored.optInt("source_height", 0)
            val n2TileEdge = factored.optInt("tile_edge", 0)
            val n2Tiles = factored.optJSONArray("tiles") ?: JSONArray()
            val analysisWidth = frontsideV01.optInt("analysis_width", 0)
            val analysisHeight = frontsideV01.optInt("analysis_height", 0)
            val frontTiles = frontsideV01.optJSONArray("tiles") ?: JSONArray()

            if (
                sourceWidth <= 0 ||
                sourceHeight <= 0 ||
                n2TileEdge <= 0 ||
                analysisWidth <= 0 ||
                analysisHeight <= 0 ||
                n2Tiles.length() <= 0 ||
                frontTiles.length() <= 0
            ) {
                return unavailable(sourceSha256, "SPATIAL_DIMENSION_CONTRACT_MISMATCH")
            }

            val n2ByGrid = HashMap<Long, JSONObject>()
            for (i in 0 until n2Tiles.length()) {
                val tile = n2Tiles.optJSONObject(i) ?: continue
                val x = tile.optInt("x", -1)
                val y = tile.optInt("y", -1)
                if (x < 0 || y < 0) continue
                n2ByGrid[gridKey(x / n2TileEdge, y / n2TileEdge)] = tile
            }

            val outTiles = JSONArray()
            var boundCount = 0L
            var visibleCandidateBound = 0L
            var visibleCandidateStructureBlocked = 0L
            var visibleCandidateCensorBlocked = 0L
            var visibleCandidateAllPredictable = 0L
            var visibleCandidateCenterOutlierFree = 0L
            var visibleCandidatePairFree = 0L
            var visibleCandidateScaleFree = 0L
            var visibleCandidateStrictVector = 0L

            for (i in 0 until frontTiles.length()) {
                val ft = frontTiles.optJSONObject(i) ?: continue
                val fx = ft.optInt("x")
                val fy = ft.optInt("y")
                val fw = ft.optInt("width")
                val fh = ft.optInt("height")
                val visibleCandidate =
                    ft.optBoolean("frontside_chroma_instability_candidate", false)

                val left = floor(
                    fx.toDouble() * sourceWidth.toDouble() / analysisWidth.toDouble(),
                ).toInt().coerceIn(0, sourceWidth - 1)
                val top = floor(
                    fy.toDouble() * sourceHeight.toDouble() / analysisHeight.toDouble(),
                ).toInt().coerceIn(0, sourceHeight - 1)
                val right = ceil(
                    (fx + fw).toDouble() * sourceWidth.toDouble() /
                        analysisWidth.toDouble(),
                ).toInt().coerceIn(left + 1, sourceWidth)
                val bottom = ceil(
                    (fy + fh).toDouble() * sourceHeight.toDouble() /
                        analysisHeight.toDouble(),
                ).toInt().coerceIn(top + 1, sourceHeight)

                val gx0 = left / n2TileEdge
                val gy0 = top / n2TileEdge
                val gx1 = (right - 1) / n2TileEdge
                val gy1 = (bottom - 1) / n2TileEdge

                val overlaps = ArrayList<JSONObject>()
                for (gy in gy0..gy1) {
                    for (gx in gx0..gx1) {
                        n2ByGrid[gridKey(gx, gy)]?.let(overlaps::add)
                    }
                }

                val expected = (gx1 - gx0 + 1) * (gy1 - gy0 + 1)
                val bindingVerified = overlaps.size == expected && expected > 0

                val allCandidatesPredictable =
                    bindingVerified && overlaps.all {
                        it.optBoolean("all_candidates_predictable", false)
                    }
                val centerOutlierFree =
                    bindingVerified && overlaps.all {
                        it.optBoolean("center_outlier_free", false)
                    }
                val pairRejectionFree =
                    bindingVerified && overlaps.all {
                        it.optBoolean("pair_rejection_free", false)
                    }
                val scaleRejectionFree =
                    bindingVerified && overlaps.all {
                        it.optBoolean("scale_rejection_free", false)
                    }
                val predictorVarianceSupported =
                    bindingVerified && overlaps.all {
                        it.optBoolean(
                            "max_predictor_variance_le_center_variance",
                            false,
                        )
                    }
                val structureProtection =
                    overlaps.any {
                        it.optBoolean("structure_protection_present", false)
                    }
                val censorProtection =
                    overlaps.any {
                        it.optBoolean("censor_protection_present", false) ||
                            it.optBoolean(
                                "censor_boundary_protection_present",
                                false,
                            )
                    }

                val strictVector =
                    bindingVerified &&
                        allCandidatesPredictable &&
                        centerOutlierFree &&
                        pairRejectionFree &&
                        scaleRejectionFree &&
                        predictorVarianceSupported &&
                        !structureProtection &&
                        !censorProtection

                if (bindingVerified) boundCount++
                if (visibleCandidate && bindingVerified) visibleCandidateBound++
                if (visibleCandidate && structureProtection) {
                    visibleCandidateStructureBlocked++
                }
                if (visibleCandidate && censorProtection) {
                    visibleCandidateCensorBlocked++
                }
                if (visibleCandidate && allCandidatesPredictable) {
                    visibleCandidateAllPredictable++
                }
                if (visibleCandidate && centerOutlierFree) {
                    visibleCandidateCenterOutlierFree++
                }
                if (visibleCandidate && pairRejectionFree) {
                    visibleCandidatePairFree++
                }
                if (visibleCandidate && scaleRejectionFree) {
                    visibleCandidateScaleFree++
                }
                if (visibleCandidate && strictVector) {
                    visibleCandidateStrictVector++
                }

                outTiles.put(
                    JSONObject()
                        .put("frontside_x", fx)
                        .put("frontside_y", fy)
                        .put("frontside_width", fw)
                        .put("frontside_height", fh)
                        .put(
                            "source_rect",
                            JSONArray()
                                .put(left)
                                .put(top)
                                .put(right)
                                .put(bottom),
                        )
                        .put("overlapping_n2_tiles", overlaps.size)
                        .put("expected_n2_tiles", expected)
                        .put("local_binding_verified", bindingVerified)
                        .put("visible_chroma_candidate", visibleCandidate)
                        .put(
                            "all_candidates_predictable",
                            allCandidatesPredictable,
                        )
                        .put("center_outlier_free", centerOutlierFree)
                        .put("pair_rejection_free", pairRejectionFree)
                        .put("scale_rejection_free", scaleRejectionFree)
                        .put(
                            "predictor_variance_supported",
                            predictorVarianceSupported,
                        )
                        .put(
                            "structure_protection_present",
                            structureProtection,
                        )
                        .put("censor_protection_present", censorProtection)
                        .put("strict_local_support_vector", strictVector)
                        .put(
                            "support_state",
                            when {
                                !bindingVerified ->
                                    "LOCAL_BINDING_INCOMPLETE"
                                structureProtection ->
                                    "LOCAL_N2_STRUCTURE_BLOCKED"
                                censorProtection ->
                                    "LOCAL_N2_CENSOR_BLOCKED"
                                strictVector ->
                                    "LOCAL_N2_STRICT_VECTOR_PRESENT_NOT_PROMOTED"
                                else ->
                                    "LOCAL_N2_FACTORS_BOUND_NOT_SUFFICIENT"
                            },
                        )
                        .put("chroma_correction_supported", false)
                        .put("candidate_applied", false),
                )
            }

            return JSONObject()
                .put("schema", "D.RAW/Frontside/N2LocalSpatialBinding/0.1")
                .put("status", "AUDIT_ONLY_BINDING_AVAILABLE")
                .put("source_sha256", sourceSha256)
                .put(
                    "shared_pipeline_prepare_cache_hit",
                    status.optBoolean("sharedPipelineCacheHit", false),
                )
                .put(
                    "v01_sparse_reference_reuse_verified",
                    status.optBoolean(
                        "v01SparseReferenceReuseVerified",
                        false,
                    ),
                )
                .put(
                    "v01_rerun_performed",
                    status.optBoolean("v01RerunPerformed", true),
                )
                .put(
                    "v01_sparse_reference_index_complete",
                    status.optBoolean(
                        "v01SparseReferenceIndexComplete",
                        false,
                    ),
                )
                .put(
                    "row_band_reuse_active",
                    status.optBoolean("rowBandReuseActive", false),
                )
                .put(
                    "v01_row_band_fill_count",
                    status.optLong("v01RowBandFillCount", 0L),
                )
                .put(
                    "v01_row_band_served_request_count",
                    status.optLong(
                        "v01RowBandServedRequestCount",
                        0L,
                    ),
                )
                .put(
                    "v01_row_band_cache_hit_request_count",
                    status.optLong(
                        "v01RowBandCacheHitRequestCount",
                        0L,
                    ),
                )
                .put(
                    "v01_row_band_fallback_request_count",
                    status.optLong(
                        "v01RowBandFallbackRequestCount",
                        0L,
                    ),
                )
                .put(
                    "row_band_fill_count_total",
                    status.optLong("rowBandFillCountTotal", 0L),
                )
                .put(
                    "row_band_served_request_count_total",
                    status.optLong(
                        "rowBandServedRequestCountTotal",
                        0L,
                    ),
                )
                .put(
                    "row_band_cache_hit_request_count_total",
                    status.optLong(
                        "rowBandCacheHitRequestCountTotal",
                        0L,
                    ),
                )
                .put(
                    "row_band_fallback_request_count_total",
                    status.optLong(
                        "rowBandFallbackRequestCountTotal",
                        0L,
                    ),
                )
                .put(
                    "row_band_peak_cache_bytes",
                    status.optLong("rowBandPeakCacheBytes", 0L),
                )
                .put(
                    "row_band_scientific_values_modified",
                    status.optBoolean(
                        "rowBandScientificValuesModified",
                        true,
                    ),
                )
                .put(
                    "native_phase_timing_available",
                    status.optBoolean(
                        "nativePhaseTimingAvailable",
                        false,
                    ),
                )
                .put(
                    "phase_shared_acquire_ms",
                    status.optDouble("phaseSharedAcquireMs", 0.0),
                )
                .put(
                    "phase_shared_context_lock_wait_ms",
                    status.optDouble(
                        "phaseSharedContextLockWaitMs",
                        0.0,
                    ),
                )
                .put(
                    "phase_v01_cfa_audit_ms",
                    status.optDouble("phaseV01CfaAuditMs", 0.0),
                )
                .put(
                    "phase_center_excluded_ms",
                    status.optDouble("phaseCenterExcludedMs", 0.0),
                )
                .put(
                    "phase_confidence_derive_ms",
                    status.optDouble(
                        "phaseConfidenceDeriveMs",
                        0.0,
                    ),
                )
                .put(
                    "phase_factored_derive_ms",
                    status.optDouble(
                        "phaseFactoredDeriveMs",
                        0.0,
                    ),
                )
                .put(
                    "phase_factored_encode_ms",
                    status.optDouble(
                        "phaseFactoredEncodeMs",
                        0.0,
                    ),
                )
                .put(
                    "phase_write_readback_reverify_ms",
                    status.optDouble(
                        "phaseWriteReadbackReverifyMs",
                        0.0,
                    ),
                )
                .put(
                    "phase_total_bridge_ms",
                    status.optDouble("phaseTotalBridgeMs", 0.0),
                )
                .put(
                    "phase_timing_is_scientific_evidence",
                    status.optBoolean(
                        "phaseTimingIsScientificEvidence",
                        true,
                    ),
                )
                .put(
                    "phase_timing_may_change_scientific_authority",
                    status.optBoolean(
                        "phaseTimingMayChangeScientificAuthority",
                        true,
                    ),
                )
                .put(
                    "shared_acquire_subphase_timing_available",
                    status.optBoolean(
                        "sharedAcquireSubphaseTimingAvailable",
                        false,
                    ),
                )
                .put(
                    "shared_acquire_probe_seal_ms",
                    status.optDouble("sharedAcquireProbeSealMs", 0.0),
                )
                .put(
                    "shared_acquire_cache_lookup_ms",
                    status.optDouble("sharedAcquireCacheLookupMs", 0.0),
                )
                .put(
                    "shared_acquire_prepare_total_ms",
                    status.optDouble("sharedAcquirePrepareTotalMs", 0.0),
                )
                .put(
                    "prepare_duplicate_and_byte_source_ms",
                    status.optDouble(
                        "prepareDuplicateAndByteSourceMs",
                        0.0,
                    ),
                )
                .put(
                    "prepare_seal_source_ms",
                    status.optDouble("prepareSealSourceMs", 0.0),
                )
                .put(
                    "prepare_color_binding_ms",
                    status.optDouble("prepareColorBindingMs", 0.0),
                )
                .put(
                    "prepare_color_source_ms",
                    status.optDouble("prepareColorSourceMs", 0.0),
                )
                .put(
                    "prepare_pre_open_reverify_ms",
                    status.optDouble("preparePreOpenReverifyMs", 0.0),
                )
                .put(
                    "prepare_open_dng_adapter_ms",
                    status.optDouble("prepareOpenDngAdapterMs", 0.0),
                )
                .put(
                    "prepare_bind_scientific_master_ms",
                    status.optDouble(
                        "prepareBindScientificMasterMs",
                        0.0,
                    ),
                )
                .put(
                    "prepare_finalize_phase2_ms",
                    status.optDouble("prepareFinalizePhase2Ms", 0.0),
                )
                .put(
                    "prepare_summarize_authority_field_ms",
                    status.optDouble(
                        "prepareSummarizeAuthorityFieldMs",
                        0.0,
                    ),
                )
                .put(
                    "authority_field_fused_into_scientific_master_pass",
                    status.optBoolean(
                        "authorityFieldFusedIntoScientificMasterPass",
                        false,
                    ),
                )
                .put(
                    "authority_field_replay_pass_performed",
                    status.optBoolean(
                        "authorityFieldReplayPassPerformed",
                        true,
                    ),
                )
                .put(
                    "authority_field_fused_finalize_ms",
                    status.optDouble(
                        "authorityFieldFusedFinalizeMs",
                        0.0,
                    ),
                )
                .put(
                    "authority_direct_record_streaming_active",
                    status.optBoolean(
                        "authorityDirectRecordStreamingActive",
                        false,
                    ),
                )
                .put(
                    "authority_temporary_record_vector_used",
                    status.optBoolean(
                        "authorityTemporaryRecordVectorUsed",
                        true,
                    ),
                )
                .put(
                    "authority_direct_byte_encoding_active",
                    status.optBoolean(
                        "authorityDirectByteEncodingActive",
                        false,
                    ),
                )
                .put(
                    "authority_generic_record_validation_bypassed",
                    status.optBoolean(
                        "authorityGenericRecordValidationBypassed",
                        false,
                    ),
                )
                .put(
                    "authority_pixel_triplet_encoding_active",
                    status.optBoolean(
                        "authorityPixelTripletEncodingActive",
                        false,
                    ),
                )
                .put(
                    "authority_canonical_record_bytes",
                    status.optLong(
                        "authorityCanonicalRecordBytes",
                        0L,
                    ),
                )
                .put(
                    "authority_canonical_pixel_triplet_bytes",
                    status.optLong(
                        "authorityCanonicalPixelTripletBytes",
                        0L,
                    ),
                )
                .put(
                    "authority_hash_batch_record_capacity",
                    status.optLong(
                        "authorityHashBatchRecordCapacity",
                        0L,
                    ),
                )
                .put(
                    "authority_hash_batch_bytes",
                    status.optLong(
                        "authorityHashBatchBytes",
                        0L,
                    ),
                )
                .put(
                    "authority_direct_byte_record_count",
                    status.optLong(
                        "authorityDirectByteRecordCount",
                        0L,
                    ),
                )
                .put(
                    "authority_generic_fallback_record_count",
                    status.optLong(
                        "authorityGenericFallbackRecordCount",
                        0L,
                    ),
                )
                .put(
                    "authority_direct_pixel_triplet_count",
                    status.optLong(
                        "authorityDirectPixelTripletCount",
                        0L,
                    ),
                )
                .put(
                    "authority_generic_fallback_pixel_count",
                    status.optLong(
                        "authorityGenericFallbackPixelCount",
                        0L,
                    ),
                )
                .put(
                    "authority_sha_direct_block_transport_active",
                    status.optBoolean(
                        "authorityShaDirectBlockTransportActive",
                        false,
                    ),
                )
                .put(
                    "authority_sha_direct_input_block_transform_count",
                    status.optLong(
                        "authorityShaDirectInputBlockTransformCount",
                        0L,
                    ),
                )
                .put(
                    "authority_sha_buffered_input_block_transform_count",
                    status.optLong(
                        "authorityShaBufferedInputBlockTransformCount",
                        0L,
                    ),
                )
                .put(
                    "authority_sha_direct_input_bytes",
                    status.optLong(
                        "authorityShaDirectInputBytes",
                        0L,
                    ),
                )
                .put(
                    "authority_direct_record_stream_ms",
                    status.optDouble(
                        "authorityDirectRecordStreamMs",
                        0.0,
                    ),
                )
                .put(
                    "authority_direct_record_stream_tile_count",
                    status.optLong(
                        "authorityDirectRecordStreamTileCount",
                        0L,
                    ),
                )
                .put(
                    "authority_direct_record_stream_record_count",
                    status.optLong(
                        "authorityDirectRecordStreamRecordCount",
                        0L,
                    ),
                )
                .put(
                    "authority_accumulator_resident_bytes_upper_bound",
                    status.optLong(
                        "authorityAccumulatorResidentBytesUpperBound",
                        0L,
                    ),
                )
                .put(
                    "prepare_finalize_truthnegative_ms",
                    status.optDouble(
                        "prepareFinalizeTruthNegativeMs",
                        0.0,
                    ),
                )
                .put(
                    "prepare_finalize_drawnegative_ms",
                    status.optDouble(
                        "prepareFinalizeDrawNegativeMs",
                        0.0,
                    ),
                )
                .put(
                    "prepare_total_instrumented_ms",
                    status.optDouble("prepareTotalInstrumentedMs", 0.0),
                )
                .put(
                    "center_excluded_subphase_timing_available",
                    status.optBoolean(
                        "centerExcludedSubphaseTimingAvailable",
                        false,
                    ),
                )
                .put(
                    "center_excluded_fill_stage2_ms",
                    status.optDouble("centerExcludedFillStage2Ms", 0.0),
                )
                .put(
                    "center_excluded_candidate_loop_ms",
                    status.optDouble(
                        "centerExcludedCandidateLoopMs",
                        0.0,
                    ),
                )
                .put(
                    "center_excluded_predictor_estimate_ms",
                    status.optDouble(
                        "centerExcludedPredictorEstimateMs",
                        0.0,
                    ),
                )
                .put(
                    "center_excluded_final_hash_ms",
                    status.optDouble("centerExcludedFinalHashMs", 0.0),
                )
                .put(
                    "center_excluded_total_instrumented_ms",
                    status.optDouble(
                        "centerExcludedTotalInstrumentedMs",
                        0.0,
                    ),
                )
                .put(
                    "center_excluded_tile_count",
                    status.optLong("centerExcludedTileCount", 0L),
                )
                .put(
                    "center_excluded_candidate_tile_count",
                    status.optLong(
                        "centerExcludedCandidateTileCount",
                        0L,
                    ),
                )
                .put(
                    "center_excluded_candidate_center_count",
                    status.optLong(
                        "centerExcludedCandidateCenterCount",
                        0L,
                    ),
                )
                .put("authority", "DIAGNOSTIC_BINDING_ONLY")
                .put("source_width", sourceWidth)
                .put("source_height", sourceHeight)
                .put("frontside_width", analysisWidth)
                .put("frontside_height", analysisHeight)
                .put("n2_tile_edge", n2TileEdge)
                .put(
                    "factored_state_sha256",
                    factored.optString("factored_state_sha256"),
                )
                .put(
                    "truthnegative_state_sha256",
                    factored.optString("truthnegative_state_sha256"),
                )
                .put(
                    "global",
                    JSONObject()
                        .put("frontside_tile_count", frontTiles.length())
                        .put("bound_frontside_tiles", boundCount)
                        .put(
                            "visible_candidate_bound_tiles",
                            visibleCandidateBound,
                        )
                        .put(
                            "visible_candidate_structure_blocked_tiles",
                            visibleCandidateStructureBlocked,
                        )
                        .put(
                            "visible_candidate_censor_blocked_tiles",
                            visibleCandidateCensorBlocked,
                        )
                        .put(
                            "visible_candidate_all_predictable_tiles",
                            visibleCandidateAllPredictable,
                        )
                        .put(
                            "visible_candidate_center_outlier_free_tiles",
                            visibleCandidateCenterOutlierFree,
                        )
                        .put(
                            "visible_candidate_pair_rejection_free_tiles",
                            visibleCandidatePairFree,
                        )
                        .put(
                            "visible_candidate_scale_rejection_free_tiles",
                            visibleCandidateScaleFree,
                        )
                        .put(
                            "visible_candidate_strict_local_support_vector_tiles",
                            visibleCandidateStrictVector,
                        ),
                )
                .put("tiles", outTiles)
                .put("local_n2_backside_binding_available", boundCount > 0L)
                .put("n2_promotion_eligible", false)
                .put("chroma_correction_supported", false)
                .put("private_ab_delta_allowed", false)
                .put("candidate_applied", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
        } finally {
            runCatching { temp.delete() }
        }
    }

    private fun gridKey(
        gx: Int,
        gy: Int,
    ): Long =
        (gy.toLong() shl 32) xor (gx.toLong() and 0xffffffffL)
}

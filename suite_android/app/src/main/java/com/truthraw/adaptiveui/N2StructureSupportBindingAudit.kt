package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.net.Uri
import android.os.ParcelFileDescriptor
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import kotlin.math.ceil
import kotlin.math.floor
import kotlin.math.max

object TruthNegativeN2StructureSupportBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun exportAndVerify(
        sourceFd: Int,
        destinationFd: Int,
        maxSourceResidentBytes: Int,
        maxLogicalResidentBytes: Int,
    ): String
}

/**
 * Audit-only fine structure-support binding.
 *
 * This reruns the existing N2 CFA preservation logic at a 32x32 source-tile
 * reporting resolution, with the same period=8 sample grid. It does not alter
 * the underlying structure gate, does not infer unsampled pixels and does not
 * weaken any v0.4 protection. It only measures how much sampled protection is
 * present around each frontside region.
 */
object N2StructureSupportBindingAudit {
    private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024
    private const val MAX_LOGICAL_RESIDENT_BYTES = 64 * 1024 * 1024
    private const val EXPECTED_TILE_EDGE = 32
    private const val EXPECTED_SAMPLING_PERIOD = 8

    fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/N2StructureSupportBinding/0.1")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "DIAGNOSTIC_SAMPLE_GRID_BINDING_ONLY")
            .put("fine_structure_binding_available", false)
            .put("can_reduce_protection", false)
            .put("can_enable_correction", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    fun skipped(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        unavailable(sourceSha256, reason)
            .put("status", "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE")

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
            return unavailable(
                sourceSha256,
                "FRONTSIDE_V0_1_BINDING_MISMATCH",
            )
        }

        val temp = runCatching {
            File.createTempFile(
                "draw_n2_structure_support_",
                ".json",
                cacheDir,
            )
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
                        TruthNegativeN2StructureSupportBridge.exportAndVerify(
                            src.fd,
                            dst.fd,
                            MAX_SOURCE_RESIDENT_BYTES,
                            MAX_LOGICAL_RESIDENT_BYTES,
                        )
                    }
                }
            } catch (error: Throwable) {
                null
            } ?: return unavailable(
                sourceSha256,
                "N2_STRUCTURE_SUPPORT_BRIDGE_FAILED",
            )

            val status = runCatching { JSONObject(statusText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_STATUS_INVALID",
                )
            if (status.optInt("status", -999) != 0) {
                return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_STATUS_" +
                        status.optInt("status", -999),
                )
            }
            if (
                !status.optBoolean("postWriteVerified", false) ||
                status.optString("sourceSha256") != sourceSha256 ||
                status.optBoolean("canReduceProtection", true) ||
                status.optBoolean("canEnableCorrection", true) ||
                status.optBoolean("candidateApplied", true) ||
                status.optBoolean("scientificWritebackAllowed", true) ||
                status.optBoolean("unsampledPixelsInferred", true) ||
                !status.optBoolean("sampleGridEvidenceOnly", false)
            ) {
                return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_STATUS_CONTRACT_MISMATCH",
                )
            }

            val rawText = runCatching { temp.readText() }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_FILE_READ_FAILED",
                )
            val field = runCatching { JSONObject(rawText) }.getOrNull()
                ?: return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_FILE_INVALID",
                )

            if (
                field.optString("schema") !=
                "D.RAW/TruthNegative/N2StructureSupportField/0.1" ||
                field.optString("source_sha256") != sourceSha256 ||
                field.optInt("tile_edge", 0) != EXPECTED_TILE_EDGE ||
                field.optInt("sampling_period", 0) !=
                EXPECTED_SAMPLING_PERIOD ||
                !field.optBoolean("sample_grid_evidence_only", false) ||
                field.optBoolean("unsampled_pixels_inferred", true) ||
                field.optBoolean("can_reduce_protection", true) ||
                field.optBoolean("can_enable_correction", true) ||
                field.optBoolean("candidate_applied", true) ||
                field.optBoolean("scientific_writeback_allowed", true)
            ) {
                return unavailable(
                    sourceSha256,
                    "N2_STRUCTURE_SUPPORT_FILE_CONTRACT_MISMATCH",
                )
            }

            val sourceWidth = field.optInt("source_width", 0)
            val sourceHeight = field.optInt("source_height", 0)
            val fineEdge = field.optInt("tile_edge", 0)
            val fineTiles = field.optJSONArray("tiles") ?: JSONArray()

            val analysisWidth = frontsideV01.optInt("analysis_width", 0)
            val analysisHeight = frontsideV01.optInt("analysis_height", 0)
            val frontTiles =
                frontsideV01.optJSONArray("tiles") ?: JSONArray()

            if (
                sourceWidth <= 0 ||
                sourceHeight <= 0 ||
                fineEdge <= 0 ||
                analysisWidth <= 0 ||
                analysisHeight <= 0 ||
                fineTiles.length() <= 0 ||
                frontTiles.length() <= 0
            ) {
                return unavailable(
                    sourceSha256,
                    "SPATIAL_DIMENSION_CONTRACT_MISMATCH",
                )
            }

            val fineByGrid = HashMap<Long, JSONObject>()
            for (i in 0 until fineTiles.length()) {
                val tile = fineTiles.optJSONObject(i) ?: continue
                val x = tile.optInt("x", -1)
                val y = tile.optInt("y", -1)
                if (x < 0 || y < 0) continue
                fineByGrid[gridKey(x / fineEdge, y / fineEdge)] = tile
            }

            val outTiles = JSONArray()
            var boundFrontsideTiles = 0L
            var visibleCandidateTiles = 0L
            var visibleCandidateBoundTiles = 0L
            var candidateOverlapSampled = 0L
            var candidateOverlapStructureProtected = 0L
            var candidateInteriorSampled = 0L
            var candidateInteriorStructureProtected = 0L
            var candidateFineTilesWithStructure = 0L
            var candidateFineTilesWithoutStructure = 0L
            var candidateMaxFineFraction = 0.0
            var candidateOverlapFractionSum = 0.0
            var candidateFractionCount = 0L

            for (i in 0 until frontTiles.length()) {
                val ft = frontTiles.optJSONObject(i) ?: continue
                val fx = ft.optInt("x")
                val fy = ft.optInt("y")
                val fw = ft.optInt("width")
                val fh = ft.optInt("height")
                val visibleCandidate =
                    ft.optBoolean(
                        "frontside_chroma_instability_candidate",
                        false,
                    )

                val left = floor(
                    fx.toDouble() * sourceWidth.toDouble() /
                        analysisWidth.toDouble(),
                ).toInt().coerceIn(0, sourceWidth - 1)
                val top = floor(
                    fy.toDouble() * sourceHeight.toDouble() /
                        analysisHeight.toDouble(),
                ).toInt().coerceIn(0, sourceHeight - 1)
                val right = ceil(
                    (fx + fw).toDouble() * sourceWidth.toDouble() /
                        analysisWidth.toDouble(),
                ).toInt().coerceIn(left + 1, sourceWidth)
                val bottom = ceil(
                    (fy + fh).toDouble() * sourceHeight.toDouble() /
                        analysisHeight.toDouble(),
                ).toInt().coerceIn(top + 1, sourceHeight)

                val gx0 = left / fineEdge
                val gy0 = top / fineEdge
                val gx1 = (right - 1) / fineEdge
                val gy1 = (bottom - 1) / fineEdge

                var overlapTiles = 0
                var expectedTiles = 0
                var interiorTiles = 0
                var boundaryTiles = 0
                var overlapSampled = 0L
                var overlapStructure = 0L
                var overlapCensor = 0L
                var overlapCensorBoundary = 0L
                var interiorSampled = 0L
                var interiorStructure = 0L
                var structurePresentFineTiles = 0L
                var structureFreeFineTiles = 0L
                var maxFineStructureFraction = 0.0

                for (gy in gy0..gy1) {
                    for (gx in gx0..gx1) {
                        expectedTiles++
                        val tile =
                            fineByGrid[gridKey(gx, gy)] ?: continue
                        overlapTiles++

                        val tx = tile.optInt("x")
                        val ty = tile.optInt("y")
                        val tw = tile.optInt("width")
                        val th = tile.optInt("height")
                        val tr = tx + tw
                        val tb = ty + th
                        val fullyContained =
                            tx >= left &&
                                ty >= top &&
                                tr <= right &&
                                tb <= bottom

                        val sampled = tile.optLong("sampled", 0L)
                        val structure =
                            tile.optLong("structure_protected", 0L)
                        val censor =
                            tile.optLong("censored_protected", 0L)
                        val censorBoundary =
                            tile.optLong(
                                "censor_boundary_protected",
                                0L,
                            )
                        val structureFraction =
                            tile.optDouble(
                                "structure_protection_fraction",
                                0.0,
                            )

                        overlapSampled += sampled
                        overlapStructure += structure
                        overlapCensor += censor
                        overlapCensorBoundary += censorBoundary

                        if (fullyContained) {
                            interiorTiles++
                            interiorSampled += sampled
                            interiorStructure += structure
                        } else {
                            boundaryTiles++
                        }

                        if (structure > 0L) {
                            structurePresentFineTiles++
                        } else {
                            structureFreeFineTiles++
                        }
                        maxFineStructureFraction =
                            max(
                                maxFineStructureFraction,
                                structureFraction,
                            )
                    }
                }

                val bindingVerified =
                    overlapTiles == expectedTiles && expectedTiles > 0
                val overlapStructureFraction =
                    fraction(overlapStructure, overlapSampled)
                val interiorStructureFraction =
                    fraction(interiorStructure, interiorSampled)

                if (bindingVerified) boundFrontsideTiles++
                if (visibleCandidate) visibleCandidateTiles++
                if (visibleCandidate && bindingVerified) {
                    visibleCandidateBoundTiles++
                    candidateOverlapSampled += overlapSampled
                    candidateOverlapStructureProtected += overlapStructure
                    candidateInteriorSampled += interiorSampled
                    candidateInteriorStructureProtected +=
                        interiorStructure
                    candidateFineTilesWithStructure +=
                        structurePresentFineTiles
                    candidateFineTilesWithoutStructure +=
                        structureFreeFineTiles
                    candidateMaxFineFraction =
                        max(
                            candidateMaxFineFraction,
                            maxFineStructureFraction,
                        )
                    candidateOverlapFractionSum +=
                        overlapStructureFraction
                    candidateFractionCount++
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
                        .put(
                            "visible_chroma_candidate",
                            visibleCandidate,
                        )
                        .put("fine_binding_verified", bindingVerified)
                        .put("overlapping_fine_tiles", overlapTiles)
                        .put("expected_fine_tiles", expectedTiles)
                        .put("fully_contained_fine_tiles", interiorTiles)
                        .put("boundary_fine_tiles", boundaryTiles)
                        .put("overlap_sampled", overlapSampled)
                        .put(
                            "overlap_structure_protected",
                            overlapStructure,
                        )
                        .put(
                            "overlap_structure_protection_fraction",
                            overlapStructureFraction,
                        )
                        .put(
                            "interior_sampled",
                            interiorSampled,
                        )
                        .put(
                            "interior_structure_protected",
                            interiorStructure,
                        )
                        .put(
                            "interior_structure_protection_fraction",
                            interiorStructureFraction,
                        )
                        .put(
                            "structure_present_fine_tiles",
                            structurePresentFineTiles,
                        )
                        .put(
                            "structure_free_fine_tiles",
                            structureFreeFineTiles,
                        )
                        .put(
                            "max_fine_tile_structure_fraction",
                            maxFineStructureFraction,
                        )
                        .put(
                            "overlap_censored_protected",
                            overlapCensor,
                        )
                        .put(
                            "overlap_censor_boundary_protected",
                            overlapCensorBoundary,
                        )
                        .put(
                            "coverage_semantics",
                            "WHOLE_FINE_TILE_OVERLAP_CONTEXT_PLUS_EXACT_FULLY_CONTAINED_TILE_COUNTS",
                        )
                        .put(
                            "unsampled_pixels_inferred",
                            false,
                        )
                        .put("can_reduce_protection", false)
                        .put("can_enable_correction", false)
                        .put(
                            "chroma_correction_supported",
                            false,
                        )
                        .put("candidate_applied", false),
                )
            }

            val fieldGlobal =
                field.optJSONObject("global") ?: JSONObject()

            return JSONObject()
                .put(
                    "schema",
                    "D.RAW/Frontside/N2StructureSupportBinding/0.1",
                )
                .put("status", "AUDIT_ONLY_FINE_BINDING_AVAILABLE")
                .put("source_sha256", sourceSha256)
                .put(
                    "shared_pipeline_prepare_cache_hit",
                    status.optBoolean("sharedPipelineCacheHit", false),
                )
                .put(
                    "authority",
                    "DIAGNOSTIC_SAMPLE_GRID_BINDING_ONLY",
                )
                .put("source_width", sourceWidth)
                .put("source_height", sourceHeight)
                .put("frontside_width", analysisWidth)
                .put("frontside_height", analysisHeight)
                .put("fine_tile_edge", fineEdge)
                .put(
                    "sampling_period",
                    field.optInt("sampling_period", 0),
                )
                .put(
                    "structure_field_sha256",
                    status.optString("jsonSha256"),
                )
                .put(
                    "candidate_sha256",
                    field.optString("candidate_sha256"),
                )
                .put(
                    "n2_audit_sha256",
                    field.optString("audit_sha256"),
                )
                .put(
                    "n2_spatial_sha256",
                    field.optString("spatial_sha256"),
                )
                .put(
                    "truthnegative_state_sha256",
                    field.optString("truthnegative_state_sha256"),
                )
                .put(
                    "field_global",
                    fieldGlobal,
                )
                .put(
                    "global",
                    JSONObject()
                        .put(
                            "frontside_tile_count",
                            frontTiles.length(),
                        )
                        .put(
                            "bound_frontside_tiles",
                            boundFrontsideTiles,
                        )
                        .put(
                            "visible_candidate_tiles",
                            visibleCandidateTiles,
                        )
                        .put(
                            "visible_candidate_bound_tiles",
                            visibleCandidateBoundTiles,
                        )
                        .put(
                            "visible_candidate_overlap_sampled",
                            candidateOverlapSampled,
                        )
                        .put(
                            "visible_candidate_overlap_structure_protected",
                            candidateOverlapStructureProtected,
                        )
                        .put(
                            "visible_candidate_overlap_structure_fraction",
                            fraction(
                                candidateOverlapStructureProtected,
                                candidateOverlapSampled,
                            ),
                        )
                        .put(
                            "visible_candidate_mean_region_structure_fraction",
                            if (candidateFractionCount > 0L) {
                                candidateOverlapFractionSum /
                                    candidateFractionCount.toDouble()
                            } else {
                                0.0
                            },
                        )
                        .put(
                            "visible_candidate_interior_sampled",
                            candidateInteriorSampled,
                        )
                        .put(
                            "visible_candidate_interior_structure_protected",
                            candidateInteriorStructureProtected,
                        )
                        .put(
                            "visible_candidate_interior_structure_fraction",
                            fraction(
                                candidateInteriorStructureProtected,
                                candidateInteriorSampled,
                            ),
                        )
                        .put(
                            "visible_candidate_fine_tiles_with_structure",
                            candidateFineTilesWithStructure,
                        )
                        .put(
                            "visible_candidate_fine_tiles_without_structure",
                            candidateFineTilesWithoutStructure,
                        )
                        .put(
                            "visible_candidate_max_fine_tile_structure_fraction",
                            candidateMaxFineFraction,
                        ),
                )
                .put("tiles", outTiles)
                .put("fine_structure_binding_available", boundFrontsideTiles > 0L)
                .put("sample_grid_evidence_only", true)
                .put("unsampled_pixels_inferred", false)
                .put("can_reduce_protection", false)
                .put("can_enable_correction", false)
                .put("chroma_correction_supported", false)
                .put("private_ab_delta_allowed", false)
                .put("candidate_applied", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)
        } finally {
            runCatching { temp.delete() }
        }
    }

    private fun fraction(
        numerator: Long,
        denominator: Long,
    ): Double =
        if (denominator > 0L) {
            numerator.toDouble() / denominator.toDouble()
        } else {
            0.0
        }

    private fun gridKey(
        gx: Int,
        gy: Int,
    ): Long =
        (gy.toLong() shl 32) xor (gx.toLong() and 0xffffffffL)
}

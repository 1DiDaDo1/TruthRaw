package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Dark Chroma Stability v0.5.
 *
 * v0.5 does not replace v0.4. It keeps the proven v0.4 source/local-N2
 * binding and adds a finer 32x32 N2 structure-support measurement so the
 * project can study whether the coarse 64x64 any-structure-present veto is
 * spatially over-broad.
 *
 * This is still audit-only. Fine structure measurements cannot reduce
 * protection, cannot enable chroma correction and cannot override
 * DARK_UNINFORMATIVE.
 */
object DarkChromaStabilityV05Audit {
    fun analyze(
        sourceSha256: String,
        v03: JSONObject?,
        v04: JSONObject?,
        fineStructure: JSONObject?,
    ): JSONObject {
        if (
            v03 == null ||
            v03.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.3" ||
            v03.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "V0_3_BINDING_MISMATCH")
        }
        if (
            v04 == null ||
            v04.optString("schema") !=
            "D.RAW/Frontside/DarkChromaStability/0.4" ||
            v04.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "V0_4_BINDING_MISMATCH")
        }

        val globalStateV03 =
            v03.optString("global_information_state", "UNKNOWN")
        val globallyDarkUninformative =
            globalStateV03.startsWith("DARK_UNINFORMATIVE")

        val fineStatus = fineStructure?.optString("status", "UNKNOWN")
            ?: "UNKNOWN"
        val fineAvailable =
            fineStructure != null &&
                fineStatus == "AUDIT_ONLY_FINE_BINDING_AVAILABLE" &&
                fineStructure.optString("source_sha256") == sourceSha256 &&
                fineStructure.optBoolean(
                    "fine_structure_binding_available",
                    false,
                ) &&
                !fineStructure.optBoolean("can_reduce_protection", true) &&
                !fineStructure.optBoolean("can_enable_correction", true) &&
                !fineStructure.optBoolean("unsampled_pixels_inferred", true)

        val fineSkipped =
            fineStatus == "NOT_REQUIRED_BY_CURRENT_FRONT_SIDE_STATE"

        val v04Tiles = v04.optJSONArray("tiles") ?: JSONArray()
        val fineTiles =
            fineStructure?.optJSONArray("tiles") ?: JSONArray()
        val fineByXY = HashMap<Long, JSONObject>()
        for (i in 0 until fineTiles.length()) {
            val t = fineTiles.optJSONObject(i) ?: continue
            fineByXY[key(t.optInt("frontside_x"), t.optInt("frontside_y"))] = t
        }

        val outTiles = JSONArray()
        var visibleCandidates = 0L
        var fineBoundCandidates = 0L
        var legacyCoarseProtectionBlocked = 0L
        var fineMeasuredCandidates = 0L
        var fineInteriorEvidenceCandidates = 0L
        var fineZeroInteriorStructureCandidates = 0L
        var correctionSupported = 0L

        for (i in 0 until v04Tiles.length()) {
            val t4 = v04Tiles.optJSONObject(i) ?: continue
            val x = t4.optInt("x")
            val y = t4.optInt("y")
            val visible =
                t4.optBoolean("visible_chroma_candidate", false)
            val v04State = t4.optString("v0_4_state", "UNKNOWN")
            val legacyProtectionBlocked =
                v04State == "LOCAL_N2_PROTECTION_BLOCK"
            val localFine = fineByXY[key(x, y)]
            val fineBound =
                localFine?.optBoolean("fine_binding_verified", false) == true
            val overlapStructureFraction =
                localFine?.optDouble(
                    "overlap_structure_protection_fraction",
                    Double.NaN,
                ) ?: Double.NaN
            val interiorSampled =
                localFine?.optLong("interior_sampled", 0L) ?: 0L
            val interiorStructure =
                localFine?.optLong(
                    "interior_structure_protected",
                    0L,
                ) ?: 0L
            val interiorStructureFraction =
                localFine?.optDouble(
                    "interior_structure_protection_fraction",
                    Double.NaN,
                ) ?: Double.NaN
            val maxFineFraction =
                localFine?.optDouble(
                    "max_fine_tile_structure_fraction",
                    Double.NaN,
                ) ?: Double.NaN
            val structurePresentFineTiles =
                localFine?.optLong(
                    "structure_present_fine_tiles",
                    0L,
                ) ?: 0L
            val structureFreeFineTiles =
                localFine?.optLong(
                    "structure_free_fine_tiles",
                    0L,
                ) ?: 0L

            if (visible) visibleCandidates++
            if (visible && fineBound) fineBoundCandidates++
            if (visible && legacyProtectionBlocked) {
                legacyCoarseProtectionBlocked++
            }
            if (visible && fineBound) fineMeasuredCandidates++
            if (visible && fineBound && interiorSampled > 0L) {
                fineInteriorEvidenceCandidates++
            }
            if (
                visible &&
                fineBound &&
                interiorSampled > 0L &&
                interiorStructure == 0L
            ) {
                fineZeroInteriorStructureCandidates++
            }

            // v0.5 is measurement/refinement only.
            val tileCorrectionSupported = false
            if (tileCorrectionSupported) correctionSupported++

            outTiles.put(
                JSONObject()
                    .put("x", x)
                    .put("y", y)
                    .put("visible_chroma_candidate", visible)
                    .put("v0_4_state", v04State)
                    .put(
                        "legacy_v0_4_coarse_structure_block",
                        legacyProtectionBlocked,
                    )
                    .put("fine_structure_binding_verified", fineBound)
                    .put(
                        "fine_overlap_structure_fraction",
                        valueOrNull(overlapStructureFraction),
                    )
                    .put(
                        "fine_interior_sampled",
                        interiorSampled,
                    )
                    .put(
                        "fine_interior_structure_protected",
                        interiorStructure,
                    )
                    .put(
                        "fine_interior_structure_fraction",
                        valueOrNull(interiorStructureFraction),
                    )
                    .put(
                        "fine_max_tile_structure_fraction",
                        valueOrNull(maxFineFraction),
                    )
                    .put(
                        "fine_structure_present_tiles",
                        structurePresentFineTiles,
                    )
                    .put(
                        "fine_structure_free_tiles",
                        structureFreeFineTiles,
                    )
                    .put(
                        "v0_5_state",
                        when {
                            globallyDarkUninformative ->
                                "BLOCKED_DARK_UNINFORMATIVE"
                            !visible ->
                                "NO_CHROMA_INSTABILITY"
                            !fineAvailable ->
                                "FINE_STRUCTURE_SUPPORT_UNAVAILABLE"
                            !fineBound ->
                                "FINE_STRUCTURE_BINDING_INCOMPLETE"
                            else ->
                                "FINE_STRUCTURE_SUPPORT_MEASURED_AUDIT_ONLY"
                        },
                    )
                    .put("can_reduce_protection", false)
                    .put("can_enable_correction", false)
                    .put(
                        "chroma_correction_supported",
                        tileCorrectionSupported,
                    )
                    .put("candidate_applied", false),
            )
        }

        val fineGlobal =
            fineStructure?.optJSONObject("global") ?: JSONObject()

        return JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.5")
            .put(
                "status",
                if (fineAvailable || fineSkipped) {
                    "AUDIT_ONLY_STRUCTURE_REFINEMENT_AVAILABLE"
                } else {
                    "AUDIT_ONLY_STRUCTURE_REFINEMENT_FAIL_CLOSED"
                },
            )
            .put("source_sha256", sourceSha256)
            .put(
                "authority",
                "APPEARANCE_DERIVED_PLUS_FINE_N2_STRUCTURE_DIAGNOSTIC_ONLY",
            )
            .put("v0_3_global_information_state", globalStateV03)
            .put(
                "global_dark_uninformative_preserved",
                globallyDarkUninformative,
            )
            .put("fine_structure_binding_available", fineAvailable)
            .put("fine_structure_binding_skipped", fineSkipped)
            .put(
                "fine_structure_skip_reason",
                if (fineSkipped) {
                    fineStructure?.optString("reason", "") ?: ""
                } else {
                    ""
                },
            )
            .put(
                "fine_structure_field_sha256",
                fineStructure?.optString(
                    "structure_field_sha256",
                    "",
                ) ?: "",
            )
            .put(
                "global",
                JSONObject()
                    .put("visible_candidate_tiles", visibleCandidates)
                    .put(
                        "fine_bound_visible_candidate_tiles",
                        fineBoundCandidates,
                    )
                    .put(
                        "legacy_coarse_protection_blocked_visible_tiles",
                        legacyCoarseProtectionBlocked,
                    )
                    .put(
                        "fine_measured_visible_candidate_tiles",
                        fineMeasuredCandidates,
                    )
                    .put(
                        "fine_interior_evidence_visible_candidate_tiles",
                        fineInteriorEvidenceCandidates,
                    )
                    .put(
                        "fine_zero_interior_structure_visible_candidate_tiles",
                        fineZeroInteriorStructureCandidates,
                    )
                    .put(
                        "visible_candidate_overlap_structure_fraction",
                        fineGlobal.optDouble(
                            "visible_candidate_overlap_structure_fraction",
                            0.0,
                        ),
                    )
                    .put(
                        "visible_candidate_interior_structure_fraction",
                        fineGlobal.optDouble(
                            "visible_candidate_interior_structure_fraction",
                            0.0,
                        ),
                    )
                    .put(
                        "visible_candidate_max_fine_tile_structure_fraction",
                        fineGlobal.optDouble(
                            "visible_candidate_max_fine_tile_structure_fraction",
                            0.0,
                        ),
                    )
                    .put(
                        "chroma_correction_supported_tiles",
                        correctionSupported,
                    ),
            )
            .put(
                "refinement_contract",
                JSONObject()
                    .put(
                        "legacy_v0_4_protection_semantics_reduced",
                        false,
                    )
                    .put("fine_field_tile_edge", 32)
                    .put("fine_field_sampling_period", 8)
                    .put("sample_grid_evidence_only", true)
                    .put("unsampled_pixels_inferred", false)
                    .put("fine_metrics_are_probability", false)
                    .put("fine_metrics_can_reduce_protection", false)
                    .put("fine_metrics_can_enable_correction", false)
                    .put(
                        "dark_uninformative_can_be_overridden",
                        false,
                    )
                    .put("chroma_correction_supported", false)
                    .put("private_ab_delta_allowed", false)
                    .put(
                        "next_required_gate",
                        "DEVICE_VALIDATE_FINE_STRUCTURE_SUPPORT_ON_SELECTIVE_MAIN_WIDE",
                    ),
            )
            .put("tiles", outTiles)
            .put(
                "single_observation_contract",
                JSONObject()
                    .put("source_observation_count", 1)
                    .put("selected_source_sha256", sourceSha256)
                    .put("other_physical_lenses_used", false)
                    .put("temporal_frames_used", false)
                    .put("burst_used", false)
                    .put("multi_observation_fusion_allowed", false),
            )
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("replacement_colour_estimated", false)
            .put("pixel_value_replacement_proposed", false)
    }

    private fun unavailable(
        sourceSha256: String,
        reason: String,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.5")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "UNKNOWN")
            .put("fine_structure_binding_available", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun valueOrNull(value: Double): Any =
        if (value.isFinite()) value else JSONObject.NULL

    private fun key(
        x: Int,
        y: Int,
    ): Long =
        (y.toLong() shl 32) xor (x.toLong() and 0xffffffffL)
}

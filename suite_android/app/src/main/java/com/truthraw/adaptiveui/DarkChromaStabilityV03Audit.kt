package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Dark Chroma Stability v0.3.
 *
 * v0.3 is a successor, not a rewrite of v0.1/v0.2. It adds:
 * 1) a frontside degeneracy gate that does not hinge on one entropy cutoff;
 * 2) a measured backside signal-support blocker from the selected DNG CFA payload.
 *
 * Neither gate can enable a correction. They can only keep colour reconstruction
 * UNKNOWN / blocked. Local N2/backside binding is still required before any
 * future private A/B/Delta chroma candidate may exist.
 */
object DarkChromaStabilityV03Audit {
    private const val DEGENERATE_DARK_FRACTION_MIN = 0.98
    private const val DEGENERATE_CANDIDATE_FRACTION_MIN = 0.98
    private const val DEGENERATE_STRUCTURE_FRACTION_MAX = 0.02
    private const val DEGENERATE_EDGE_DENSITY_MAX = 0.010

    fun unavailable(
        sourceSha256: String,
        v01: JSONObject?,
        v02: JSONObject?,
        backsideSignal: JSONObject?,
    ): JSONObject =
        base(
            sourceSha256 = sourceSha256,
            status = "FRONTSIDE_PREVIEW_UNAVAILABLE",
            v01 = v01,
            v02 = v02,
            backsideSignal = backsideSignal,
        )
            .put("global_information_state", "UNKNOWN")
            .put("frontside_degenerate", false)
            .put("backside_near_black_dominated", false)
            .put("tiles", JSONArray())

    fun analyze(
        sourceSha256: String,
        v01: JSONObject,
        v02: JSONObject,
        backsideSignal: JSONObject?,
    ): JSONObject {
        val v01SourceOk =
            v01.optString("source_sha256") == sourceSha256 &&
                v01.optString("schema") == "D.RAW/Frontside/DarkChromaStability/0.1"
        val v02SourceOk =
            v02.optString("source_sha256") == sourceSha256 &&
                v02.optString("schema") == "D.RAW/Frontside/DarkChromaStability/0.2"

        val g1 = v01.optJSONObject("global") ?: JSONObject()
        val tileCount = g1.optLong("tile_count", 0L)
        val darkTiles = g1.optLong("dark_tile_count", 0L)
        val candidateTiles =
            g1.optLong("frontside_chroma_instability_candidate_tiles", 0L)
        val structureTiles = g1.optLong("structure_protected_tiles", 0L)

        val darkFraction = fraction(darkTiles, tileCount)
        val candidateFraction = fraction(candidateTiles, tileCount)
        val structureFraction = fraction(structureTiles, tileCount)
        val edgeDensity = v02.optDouble("global_edge_density", Double.NaN)
        val entropy = v02.optDouble("global_luma_entropy_bits_32_bin", Double.NaN)

        val frontsideDegenerate =
            v01SourceOk &&
                v02SourceOk &&
                tileCount > 0L &&
                darkFraction >= DEGENERATE_DARK_FRACTION_MIN &&
                candidateFraction >= DEGENERATE_CANDIDATE_FRACTION_MIN &&
                structureFraction <= DEGENERATE_STRUCTURE_FRACTION_MAX &&
                edgeDensity.isFinite() &&
                edgeDensity <= DEGENERATE_EDGE_DENSITY_MAX

        val backsideSourceOk =
            backsideSignal?.optString("source_sha256") == sourceSha256
        val backsideMeasured =
            backsideSourceOk &&
                backsideSignal?.optString("status") ==
                "MEASURED_SOURCE_PAYLOAD_SAMPLE_AVAILABLE"
        val backsideNearBlack =
            backsideMeasured &&
                backsideSignal?.optString("signal_support_state") ==
                "NEAR_BLACK_DOMINATED"

        val globalState = when {
            frontsideDegenerate -> "DARK_UNINFORMATIVE_BY_DEGENERACY"
            backsideNearBlack -> "DARK_UNINFORMATIVE_BY_BACKSIDE_SIGNAL"
            else -> "FRONTSIDE_INFORMATION_PRESENT"
        }

        val v01Tiles = v01.optJSONArray("tiles") ?: JSONArray()
        val v02Tiles = v02.optJSONArray("tiles") ?: JSONArray()
        val outTiles = JSONArray()
        var structureProtectedCount = 0L
        var darkUninformativeCount = 0L
        var visibleInstabilityCount = 0L
        var backsidePendingCount = 0L
        var correctionSupportedCount = 0L

        val n = minOf(v01Tiles.length(), v02Tiles.length())
        for (i in 0 until n) {
            val t1 = v01Tiles.optJSONObject(i) ?: continue
            val t2 = v02Tiles.optJSONObject(i) ?: continue

            val structureProtected = t1.optBoolean("structure_protected", false)
            val visibleCandidate =
                t1.optBoolean("frontside_chroma_instability_candidate", false)
            val v02DarkUninformative =
                t2.optBoolean("dark_uninformative", false)

            val globallyBlocked =
                visibleCandidate &&
                    (frontsideDegenerate || backsideNearBlack)
            val darkUninformative =
                !structureProtected &&
                    (globallyBlocked || v02DarkUninformative)

            val state = when {
                structureProtected -> "STRUCTURE_PROTECTED"
                darkUninformative && frontsideDegenerate ->
                    "DARK_UNINFORMATIVE_BY_GLOBAL_DEGENERACY"
                darkUninformative && backsideNearBlack ->
                    "DARK_UNINFORMATIVE_BY_BACKSIDE_SIGNAL"
                darkUninformative -> "DARK_UNINFORMATIVE"
                visibleCandidate -> "CHROMA_INSTABILITY_VISIBLE"
                else -> "NO_CHROMA_INSTABILITY"
            }

            val localBacksideN2Bound = false
            val correctionSupported = false

            if (structureProtected) structureProtectedCount++
            if (visibleCandidate) visibleInstabilityCount++
            if (darkUninformative) darkUninformativeCount++
            if (
                visibleCandidate &&
                !structureProtected &&
                !darkUninformative &&
                !localBacksideN2Bound
            ) {
                backsidePendingCount++
            }
            if (correctionSupported) correctionSupportedCount++

            outTiles.put(
                JSONObject()
                    .put("x", t1.optInt("x"))
                    .put("y", t1.optInt("y"))
                    .put("width", t1.optInt("width"))
                    .put("height", t1.optInt("height"))
                    .put("v0_1_visible_candidate", visibleCandidate)
                    .put("structure_protected", structureProtected)
                    .put("information_state", state)
                    .put("dark_uninformative", darkUninformative)
                    .put("local_backside_n2_bound", localBacksideN2Bound)
                    .put(
                        "correction_support_state",
                        when {
                            structureProtected -> "BLOCKED_BY_STRUCTURE"
                            darkUninformative -> "BLOCKED_DARK_UNINFORMATIVE"
                            !visibleCandidate -> "NOT_REQUESTED"
                            else -> "LOCAL_BACKSIDE_N2_CONFIRMATION_PENDING"
                        },
                    )
                    .put("chroma_correction_supported", correctionSupported)
                    .put("candidate_applied", false),
            )
        }

        return base(
            sourceSha256 = sourceSha256,
            status =
                if (v01SourceOk && v02SourceOk) {
                    "AUDIT_ONLY_AVAILABLE"
                } else {
                    "FAIL_CLOSED_PREDECESSOR_BINDING_MISMATCH"
                },
            v01 = v01,
            v02 = v02,
            backsideSignal = backsideSignal,
        )
            .put("exact_v0_1_binding_verified", v01SourceOk)
            .put("exact_v0_2_binding_verified", v02SourceOk)
            .put("global_information_state", globalState)
            .put("frontside_degenerate", frontsideDegenerate)
            .put("backside_near_black_dominated", backsideNearBlack)
            .put(
                "degeneracy_factors",
                JSONObject()
                    .put("dark_tile_fraction", darkFraction)
                    .put("visible_candidate_fraction", candidateFraction)
                    .put("structure_protected_fraction", structureFraction)
                    .put("edge_density", edgeDensity)
                    .put("entropy_diagnostic_only", entropy)
                    .put("entropy_is_hard_gate", false),
            )
            .put(
                "degeneracy_thresholds",
                JSONObject()
                    .put("authority", "CONSERVATIVE_BLOCKING_HEURISTIC_ONLY")
                    .put("dark_fraction_min", DEGENERATE_DARK_FRACTION_MIN)
                    .put(
                        "visible_candidate_fraction_min",
                        DEGENERATE_CANDIDATE_FRACTION_MIN,
                    )
                    .put(
                        "structure_protected_fraction_max",
                        DEGENERATE_STRUCTURE_FRACTION_MAX,
                    )
                    .put("edge_density_max", DEGENERATE_EDGE_DENSITY_MAX),
            )
            .put(
                "global",
                JSONObject()
                    .put("tile_count", tileCount)
                    .put("visible_chroma_instability_tiles", visibleInstabilityCount)
                    .put("structure_protected_tiles", structureProtectedCount)
                    .put("dark_uninformative_tiles", darkUninformativeCount)
                    .put("backside_confirmation_pending_tiles", backsidePendingCount)
                    .put(
                        "chroma_correction_supported_tiles",
                        correctionSupportedCount,
                    ),
            )
            .put(
                "correction_contract",
                JSONObject()
                    .put(
                        "required_intersection",
                        "VISIBLE_CHROMA_INSTABILITY_AND_INFORMATION_SUPPORT_AND_NO_STRUCTURE_VETO_AND_LOCAL_BACKSIDE_N2_SUPPORT",
                    )
                    .put("global_backside_signal_can_block", true)
                    .put("global_backside_signal_can_enable", false)
                    .put("frontside_degeneracy_can_block", true)
                    .put("frontside_degeneracy_can_enable", false)
                    .put("local_backside_n2_support_bound", false)
                    .put("chroma_correction_supported", false)
                    .put("private_ab_delta_allowed", false),
            )
            .put("tiles", outTiles)
    }

    private fun base(
        sourceSha256: String,
        status: String,
        v01: JSONObject?,
        v02: JSONObject?,
        backsideSignal: JSONObject?,
    ): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.3")
            .put("status", status)
            .put("source_sha256", sourceSha256)
            .put("authority", "APPEARANCE_DERIVED_PLUS_SOURCE_PAYLOAD_BLOCKING_ONLY")
            .put(
                "predecessor_binding",
                JSONObject()
                    .put("v0_1_schema", v01?.optString("schema", "UNKNOWN") ?: "UNKNOWN")
                    .put(
                        "v0_1_audit_sha256",
                        v01?.optString("audit_sha256", "") ?: "",
                    )
                    .put("v0_2_schema", v02?.optString("schema", "UNKNOWN") ?: "UNKNOWN")
                    .put(
                        "v0_2_audit_sha256",
                        v02?.optString("audit_sha256", "") ?: "",
                    ),
            )
            .put(
                "backside_signal_binding",
                JSONObject()
                    .put(
                        "schema",
                        backsideSignal?.optString("schema", "UNKNOWN") ?: "UNKNOWN",
                    )
                    .put(
                        "source_sha256",
                        backsideSignal?.optString("source_sha256", "") ?: "",
                    )
                    .put(
                        "status",
                        backsideSignal?.optString("status", "UNKNOWN") ?: "UNKNOWN",
                    )
                    .put(
                        "signal_support_state",
                        backsideSignal?.optString(
                            "signal_support_state",
                            "UNKNOWN",
                        ) ?: "UNKNOWN",
                    )
                    .put(
                        "authority",
                        backsideSignal?.optString("authority", "UNKNOWN") ?: "UNKNOWN",
                    ),
            )
            .put(
                "single_observation_contract",
                JSONObject()
                    .put("source_observation_count", 1)
                    .put("selected_source_sha256", sourceSha256)
                    .put("other_physical_lenses_used", false)
                    .put("temporal_frames_used", false)
                    .put("burst_used", false)
                    .put("multi_observation_fusion_allowed", false)
                    .put("drawnegative_target_count", 1),
            )
            .put("uses_ai_or_learned_model", false)
            .put("audit_only", true)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("replacement_colour_estimated", false)
            .put("pixel_value_replacement_proposed", false)

    private fun fraction(
        numerator: Long,
        denominator: Long,
    ): Double =
        if (denominator > 0L) {
            numerator.toDouble() / denominator.toDouble()
        } else {
            0.0
        }
}

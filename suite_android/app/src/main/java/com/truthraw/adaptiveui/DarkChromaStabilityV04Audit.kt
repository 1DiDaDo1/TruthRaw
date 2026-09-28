package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Dark Chroma Stability v0.4.
 *
 * v0.4 proves same-observation spatial binding between the frontside Dark Chroma
 * field and the local N2 Factored Confidence field. The binding is diagnostic
 * only. It never overrides a DARK_UNINFORMATIVE block and it still cannot
 * promote chroma correction or private A/B/Delta.
 */
object DarkChromaStabilityV04Audit {
    fun analyze(
        sourceSha256: String,
        v03: JSONObject?,
        localN2: JSONObject?,
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
            localN2 == null ||
            localN2.optString("schema") !=
            "D.RAW/Frontside/N2LocalSpatialBinding/0.1" ||
            localN2.optString("source_sha256") != sourceSha256
        ) {
            return unavailable(sourceSha256, "LOCAL_N2_BINDING_MISMATCH")
        }

        val v03GlobalState =
            v03.optString("global_information_state", "UNKNOWN")
        val globallyDarkUninformative =
            v03GlobalState.startsWith("DARK_UNINFORMATIVE")
        val localStatus = localN2.optString("status", "UNKNOWN")
        val localAvailable =
            localStatus == "AUDIT_ONLY_BINDING_AVAILABLE" &&
                localN2.optBoolean(
                    "local_n2_backside_binding_available",
                    false,
                )

        val v03Tiles = v03.optJSONArray("tiles") ?: JSONArray()
        val localTiles = localN2.optJSONArray("tiles") ?: JSONArray()
        val localByXY = HashMap<Long, JSONObject>()
        for (i in 0 until localTiles.length()) {
            val t = localTiles.optJSONObject(i) ?: continue
            localByXY[key(t.optInt("frontside_x"), t.optInt("frontside_y"))] = t
        }

        val outTiles = JSONArray()
        var visibleCandidates = 0L
        var locallyBoundVisibleCandidates = 0L
        var blockedByDarkUninformative = 0L
        var blockedByStructureOrCensor = 0L
        var localStrictVectorPresent = 0L
        var localFactorsBoundNotSufficient = 0L
        var correctionSupported = 0L

        for (i in 0 until v03Tiles.length()) {
            val t3 = v03Tiles.optJSONObject(i) ?: continue
            val x = t3.optInt("x")
            val y = t3.optInt("y")
            val visible =
                t3.optBoolean("v0_1_visible_candidate", false)
            val v03Dark =
                t3.optBoolean("dark_uninformative", false)
            val local = localByXY[key(x, y)]
            val bound =
                local?.optBoolean("local_binding_verified", false) == true
            val structure =
                local?.optBoolean(
                    "structure_protection_present",
                    false,
                ) == true
            val censor =
                local?.optBoolean(
                    "censor_protection_present",
                    false,
                ) == true
            val strict =
                local?.optBoolean(
                    "strict_local_support_vector",
                    false,
                ) == true

            if (visible) visibleCandidates++
            if (visible && bound) locallyBoundVisibleCandidates++
            if (visible && (globallyDarkUninformative || v03Dark)) {
                blockedByDarkUninformative++
            }
            if (visible && (structure || censor)) {
                blockedByStructureOrCensor++
            }
            if (
                visible &&
                bound &&
                !globallyDarkUninformative &&
                !v03Dark &&
                !structure &&
                !censor &&
                strict
            ) {
                localStrictVectorPresent++
            }
            if (
                visible &&
                bound &&
                !globallyDarkUninformative &&
                !v03Dark &&
                !structure &&
                !censor &&
                !strict
            ) {
                localFactorsBoundNotSufficient++
            }

            // v0.4 deliberately does not promote any tile.
            val tileCorrectionSupported = false
            if (tileCorrectionSupported) correctionSupported++

            outTiles.put(
                JSONObject()
                    .put("x", x)
                    .put("y", y)
                    .put("visible_chroma_candidate", visible)
                    .put("v0_3_dark_uninformative", v03Dark)
                    .put("local_n2_binding_verified", bound)
                    .put("local_n2_structure_protection", structure)
                    .put("local_n2_censor_protection", censor)
                    .put("local_n2_strict_vector_present", strict)
                    .put(
                        "v0_4_state",
                        when {
                            globallyDarkUninformative || v03Dark ->
                                "BLOCKED_DARK_UNINFORMATIVE"
                            !visible ->
                                "NO_CHROMA_INSTABILITY"
                            !bound ->
                                "LOCAL_N2_BINDING_PENDING"
                            structure || censor ->
                                "LOCAL_N2_PROTECTION_BLOCK"
                            strict ->
                                "LOCAL_N2_STRICT_VECTOR_BOUND_NOT_PROMOTED"
                            else ->
                                "LOCAL_N2_FACTORS_BOUND_NOT_SUFFICIENT"
                        },
                    )
                    .put(
                        "chroma_correction_supported",
                        tileCorrectionSupported,
                    )
                    .put("candidate_applied", false),
            )
        }

        return JSONObject()
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.4")
            .put("status", "AUDIT_ONLY_LOCAL_BINDING_AVAILABLE")
            .put("source_sha256", sourceSha256)
            .put(
                "authority",
                "APPEARANCE_DERIVED_PLUS_LOCAL_N2_DIAGNOSTIC_BINDING_ONLY",
            )
            .put("v0_3_global_information_state", v03GlobalState)
            .put("global_dark_uninformative_preserved", globallyDarkUninformative)
            .put("local_n2_binding_available", localAvailable)
            .put(
                "factored_state_sha256",
                localN2.optString("factored_state_sha256"),
            )
            .put(
                "truthnegative_state_sha256",
                localN2.optString("truthnegative_state_sha256"),
            )
            .put(
                "global",
                JSONObject()
                    .put("visible_candidate_tiles", visibleCandidates)
                    .put(
                        "locally_bound_visible_candidate_tiles",
                        locallyBoundVisibleCandidates,
                    )
                    .put(
                        "dark_uninformative_blocked_tiles",
                        blockedByDarkUninformative,
                    )
                    .put(
                        "structure_or_censor_blocked_tiles",
                        blockedByStructureOrCensor,
                    )
                    .put(
                        "local_strict_vector_present_tiles",
                        localStrictVectorPresent,
                    )
                    .put(
                        "local_factors_bound_not_sufficient_tiles",
                        localFactorsBoundNotSufficient,
                    )
                    .put(
                        "chroma_correction_supported_tiles",
                        correctionSupported,
                    ),
            )
            .put(
                "promotion_contract",
                JSONObject()
                    .put("spatial_binding_proven", localAvailable)
                    .put(
                        "dark_uninformative_can_be_overridden_by_local_n2",
                        false,
                    )
                    .put("local_n2_factors_are_probability", false)
                    .put("local_n2_factors_can_enable_correction", false)
                    .put("chroma_correction_supported", false)
                    .put("private_ab_delta_allowed", false)
                    .put(
                        "next_required_gate",
                        "DEVICE_VALIDATE_LOCAL_BINDING_THEN_DEFINE_PRIVATE_CHROMA_CANDIDATE_GATE",
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
            .put("schema", "D.RAW/Frontside/DarkChromaStability/0.4")
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256", sourceSha256)
            .put("authority", "UNKNOWN")
            .put("local_n2_binding_available", false)
            .put("chroma_correction_supported", false)
            .put("private_ab_delta_allowed", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun key(
        x: Int,
        y: Int,
    ): Long =
        (y.toLong() shl 32) xor (x.toLong() and 0xffffffffL)
}

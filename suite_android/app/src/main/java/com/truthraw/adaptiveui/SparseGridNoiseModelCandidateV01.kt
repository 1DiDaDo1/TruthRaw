package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Builds a signal-dependent noise candidate directly from the already computed
 * repeated sparse-grid relation sets.
 *
 * This is still a research candidate. DARK/FLAT/REPEATED_SCENE sets remain
 * labelled separately and no component is promoted merely because a fit exists.
 */
object SparseGridNoiseModelCandidateV01 {
    const val SCHEMA = "D.RAW/SparseGridNoiseModelCandidate/0.1"

    fun evaluate(
        repeatedSparseGrid: JSONObject,
    ): JSONObject {
        if (
            repeatedSparseGrid.optString("status") !=
            "REPEATED_SPARSE_GRID_NOISE_CANDIDATES_AVAILABLE"
        ) {
            return unavailable("REPEATED_SPARSE_GRID_CANDIDATES_UNAVAILABLE")
        }

        val sets = repeatedSparseGrid.optJSONArray("sets")
            ?: return unavailable("REPEATED_SPARSE_GRID_SETS_MISSING")

        val trainSignal = ArrayList<Double>()
        val trainVariance = ArrayList<Double>()
        val heldSignal = ArrayList<Double>()
        val heldVariance = ArrayList<Double>()
        val fixedSpatial = ArrayList<Double>()
        val rowVariance = ArrayList<Double>()
        val columnVariance = ArrayList<Double>()
        val phaseVariance = ArrayList<Double>()
        val roots = linkedSetOf<String>()
        var darkSetCount = 0
        var flatSetCount = 0
        var sceneSetCount = 0

        for (i in 0 until sets.length()) {
            val set = sets.optJSONObject(i) ?: continue
            val kind = set.optString("observation_kind").uppercase()
            when (kind) {
                "DARK" -> darkSetCount++
                "FLAT" -> flatSetCount++
                "REPEATED_SCENE" -> sceneSetCount++
            }

            val sourceRoots = set.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (j in 0 until sourceRoots.length()) {
                sourceRoots.optString(j)
                    .takeIf(String::isNotBlank)
                    ?.let(roots::add)
            }

            val mean =
                set.optDouble(
                    "global_mean_normalized_above_black",
                    Double.NaN,
                )
            val variance =
                set.optDouble(
                    "median_temporal_variance",
                    Double.NaN,
                )
            if (
                mean.isFinite() &&
                variance.isFinite() &&
                variance >= 0.0
            ) {
                // Relation sets do not currently carry a dedicated held-out
                // flag, so all remain TRAIN candidates. Promotion still
                // requires a separately preserved held-out gate.
                trainSignal += mean.coerceAtLeast(0.0)
                trainVariance += variance
            }

            fun addOptional(key: String, out: MutableList<Double>) {
                val v = set.optDouble(key, Double.NaN)
                if (v.isFinite() && v >= 0.0) out += v
            }
            addOptional("spatial_fixed_variance_candidate", fixedSpatial)
            addOptional("row_level_variance_candidate", rowVariance)
            addOptional("column_level_variance_candidate", columnVariance)
            addOptional("cfa_phase_level_variance_candidate", phaseVariance)
        }

        if (trainSignal.size < 3) {
            return unavailable("AT_LEAST_THREE_SIGNAL_LEVEL_RELATION_SETS_REQUIRED")
                .put("available_signal_level_count", trainSignal.size)
                .put("source_sha256_roots", JSONArray(roots.toList()))
        }

        val fit =
            ResearchMathV01.linearFit(trainSignal, trainVariance)
                ?: return unavailable("SPARSE_GRID_SIGNAL_VARIANCE_FIT_FAILED")
                    .put("source_sha256_roots", JSONArray(roots.toList()))

        val shotSlope =
            fit.slope.takeIf { it.isFinite() && it >= 0.0 }
        val readVariance =
            fit.intercept.takeIf { it.isFinite() && it >= 0.0 }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SPARSE_GRID_NOISE_MODEL_CANDIDATE_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("signal_level_count", trainSignal.size)
            .put("dark_relation_set_count", darkSetCount)
            .put("flat_relation_set_count", flatSetCount)
            .put("repeated_scene_relation_set_count", sceneSetCount)
            .put(
                "signal_dependent_temporal_variance_candidate",
                JSONObject()
                    .put("model", "VARIANCE=SLOPE*SIGNAL+INTERCEPT")
                    .put(
                        "shot_slope_candidate",
                        shotSlope ?: JSONObject.NULL,
                    )
                    .put(
                        "read_plus_dark_variance_intercept_candidate",
                        readVariance ?: JSONObject.NULL,
                    )
                    .put("training_rmse", fit.rmse)
                    .put("held_out_rmse", JSONObject.NULL),
            )
            .put(
                "fixed_pattern_candidate_summary",
                JSONObject()
                    .put(
                        "spatial_fixed_variance_median",
                        ResearchMathV01.median(fixedSpatial)
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "row_variance_median",
                        ResearchMathV01.median(rowVariance)
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "column_variance_median",
                        ResearchMathV01.median(columnVariance)
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "cfa_phase_variance_median",
                        ResearchMathV01.median(phaseVariance)
                            ?: JSONObject.NULL,
                    ),
            )
            .put(
                "held_out_validation_available",
                heldSignal.isNotEmpty() && heldVariance.isNotEmpty(),
            )
            .put("dark_current_component_separated", false)
            .put("dsnu_component_separated", false)
            .put("prnu_component_separated", false)
            .put("row_column_component_separated", false)
            .put("unknown_residual_preserved", true)
            .put("noise_component_decomposition_performed", false)
            .put("noise_component_calibration_promoted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun unavailable(reason: String): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("noise_component_decomposition_performed", false)
            .put("noise_component_calibration_promoted", false)
            .put("noise_reduction_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
}

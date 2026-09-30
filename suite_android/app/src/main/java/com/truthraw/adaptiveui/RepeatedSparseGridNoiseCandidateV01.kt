package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Data-driven repeated-observation candidate over the exact sparse CFA grid
 * exported by BacksideSignalSupportAudit.
 *
 * Controlled DARK and FLAT relations are treated separately. No camera/lens or
 * RAW identity is used as a scientific key.
 */
object RepeatedSparseGridNoiseCandidateV01 {
    const val SCHEMA = "D.RAW/RepeatedSparseGridNoiseCandidate/0.1"

    private data class Point(
        val x: Int,
        val y: Int,
        val phase: Int,
        val value: Double,
    )

    fun evaluate(
        profiles: List<JSONObject>,
        records: List<JSONObject>,
    ): JSONObject {
        val bySha =
            profiles.mapNotNull { p ->
                p.optString("source_sha256")
                    .takeIf(String::isNotBlank)
                    ?.let { it to p }
            }.toMap()

        val sets = JSONArray()
        var usableSetCount = 0

        for (record in records) {
            val axis = record.optString("axis_scope")
            if (
                axis != "DARK_NOISE_OFFSET" &&
                axis != "NOISE_COMPONENT_SEPARATION"
            ) {
                continue
            }
            val validation =
                CalibrationObservationRecordValidatorV01.validate(record)
            if (
                validation.optString("status") !=
                "CALIBRATION_OBSERVATION_RECORD_VALID"
            ) {
                continue
            }

            val payload = record.optJSONObject("axis_payload") ?: continue
            val kind = payload.optString("observation_kind").uppercase()
            if (kind !in setOf("DARK", "FLAT", "REPEATED_SCENE")) continue

            val roots = record.optJSONArray("source_sha256_roots") ?: continue
            val sourceRoots = ArrayList<String>()
            for (i in 0 until roots.length()) {
                roots.optString(i).takeIf(String::isNotBlank)?.let(sourceRoots::add)
            }
            if (sourceRoots.size < 2) continue

            val frames = linkedMapOf<String, Map<String, Point>>()
            for (sha in sourceRoots) {
                val profile = bySha[sha] ?: continue
                val backside =
                    profile.optJSONObject("backside_signal_support")
                        ?: continue
                val grid =
                    backside.optJSONObject("sparse_measured_sample_grid")
                        ?: continue
                if (
                    grid.optString("status") !=
                    "MEASURED_SPARSE_SOURCE_GRID_AVAILABLE"
                ) {
                    continue
                }
                val points = grid.optJSONArray("points") ?: continue
                val map = linkedMapOf<String, Point>()
                for (i in 0 until points.length()) {
                    val p = points.optJSONObject(i) ?: continue
                    val x = p.optInt("source_x", Int.MIN_VALUE)
                    val y = p.optInt("source_y", Int.MIN_VALUE)
                    val phase = p.optInt("cfa_phase", -1)
                    val value =
                        p.optDouble("normalized_above_black", Double.NaN)
                    if (
                        x == Int.MIN_VALUE ||
                        y == Int.MIN_VALUE ||
                        phase !in 0..3 ||
                        !value.isFinite()
                    ) {
                        continue
                    }
                    map[key(x, y, phase)] =
                        Point(x, y, phase, value)
                }
                if (map.isNotEmpty()) frames[sha] = map
            }

            if (frames.size < 2) continue

            val commonKeys =
                frames.values
                    .map { it.keys.toSet() }
                    .reduce { a, b -> a.intersect(b) }

            if (commonKeys.size < 64) continue

            val temporalVariancePerCell = ArrayList<Double>()
            val cellMeans = ArrayList<Double>()
            val rowMeans = linkedMapOf<Int, MutableList<Double>>()
            val columnMeans = linkedMapOf<Int, MutableList<Double>>()
            val phaseMeans = Array(4) { ArrayList<Double>() }

            for (k in commonKeys) {
                val points =
                    frames.values.mapNotNull { it[k] }
                if (points.size != frames.size) continue
                val values = points.map { it.value }
                val mean = ResearchMathV01.mean(values) ?: continue
                val variance =
                    ResearchMathV01.sampleVariance(values) ?: continue
                temporalVariancePerCell += variance
                cellMeans += mean

                val p = points.first()
                rowMeans.getOrPut(p.y) { ArrayList() }.add(mean)
                columnMeans.getOrPut(p.x) { ArrayList() }.add(mean)
                phaseMeans[p.phase].add(mean)
            }

            if (cellMeans.size < 64) continue

            val globalMean =
                ResearchMathV01.mean(cellMeans) ?: continue
            val temporalVarianceMedian =
                ResearchMathV01.median(temporalVariancePerCell) ?: continue
            val cellMeanVariance =
                ResearchMathV01.sampleVariance(cellMeans)

            val rowLevel =
                rowMeans.values.mapNotNull(ResearchMathV01::mean)
            val columnLevel =
                columnMeans.values.mapNotNull(ResearchMathV01::mean)
            val phaseLevel =
                phaseMeans.mapNotNull(ResearchMathV01::mean)

            val spatialFixedVarianceCandidate =
                if (
                    cellMeanVariance != null &&
                    cellMeanVariance >= temporalVarianceMedian
                ) {
                    cellMeanVariance - temporalVarianceMedian
                } else {
                    0.0
                }

            val relativeFixedPatternRms =
                if (globalMean != 0.0 && spatialFixedVarianceCandidate >= 0.0) {
                    kotlin.math.sqrt(spatialFixedVarianceCandidate) /
                        kotlin.math.abs(globalMean)
                } else {
                    Double.NaN
                }

            val result =
                JSONObject()
                    .put("observation_kind", kind)
                    .put("source_sha256_roots", JSONArray(frames.keys.toList()))
                    .put("frame_count", frames.size)
                    .put("common_sparse_cell_count", cellMeans.size)
                    .put("global_mean_normalized_above_black", globalMean)
                    .put(
                        "median_temporal_variance",
                        temporalVarianceMedian,
                    )
                    .put(
                        "cell_mean_spatial_variance",
                        cellMeanVariance ?: JSONObject.NULL,
                    )
                    .put(
                        "spatial_fixed_variance_candidate",
                        spatialFixedVarianceCandidate,
                    )
                    .put(
                        "relative_fixed_pattern_rms_candidate",
                        if (relativeFixedPatternRms.isFinite()) {
                            relativeFixedPatternRms
                        } else {
                            JSONObject.NULL
                        },
                    )
                    .put(
                        "row_level_variance_candidate",
                        ResearchMathV01.sampleVariance(rowLevel)
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "column_level_variance_candidate",
                        ResearchMathV01.sampleVariance(columnLevel)
                            ?: JSONObject.NULL,
                    )
                    .put(
                        "cfa_phase_level_variance_candidate",
                        ResearchMathV01.sampleVariance(phaseLevel)
                            ?: JSONObject.NULL,
                    )
                    .put("component_interpretation_promoted", false)
                    .put("candidate_applied", false)
                    .put("noise_reduction_applied", false)

            if (kind == "DARK") {
                result
                    .put("dark_offset_candidate_available", true)
                    .put("dsnu_candidate_available", true)
                    .put("read_plus_dark_temporal_candidate_available", true)
                    .put("prnu_candidate_available", false)
            } else if (kind == "FLAT") {
                result
                    .put("dark_offset_candidate_available", false)
                    .put("dsnu_candidate_available", false)
                    .put("read_plus_dark_temporal_candidate_available", true)
                    .put(
                        "prnu_like_relative_fixed_pattern_candidate_available",
                        payload.optBoolean("flat_field_relation_admitted", false),
                    )
            }

            sets.put(result)
            usableSetCount++
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (usableSetCount > 0) {
                    "REPEATED_SPARSE_GRID_NOISE_CANDIDATES_AVAILABLE"
                } else {
                    "UNKNOWN_FAIL_CLOSED"
                },
            )
            .put("usable_relation_set_count", usableSetCount)
            .put("sets", sets)
            .put("source_grid_interpolation_performed", false)
            .put("camera_lens_vendor_or_raw_identity_used_as_key", false)
            .put("noise_component_calibration_promoted", false)
            .put("noise_reduction_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun key(
        x: Int,
        y: Int,
        phase: Int,
    ): String = "$x:$y:$phase"
}

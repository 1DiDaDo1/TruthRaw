package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.sqrt

/**
 * Candidate decomposition after explicit world/source and radiometric relations.
 *
 * The record must carry a world_point_id and sensor_cell_id for each scalar
 * sample. Those IDs are relation evidence, not camera/lens identity.
 */
object WorldSpaceResidualCandidateSolverV01 {
    const val SCHEMA = "D.RAW/WorldSpaceResidualCandidate/0.1"

    private data class Sample(
        val worldPointId: String,
        val sensorCellId: String,
        val sourceSha256: String,
        val value: Double,
        val role: String,
    )

    fun evaluate(
        records: List<JSONObject>,
        worldSourceBridge: JSONObject =
            FreeWorldSourceLatticeBridgeContractV01.describe(),
        promotionState: JSONObject =
            ScientificPromotionStateV01.blocked(),
    ): JSONObject {
        val promotedBridge =
            worldSourceBridge.optBoolean(
                "world_to_source_bridge_admitted",
                false,
            ) &&
                ScientificPromotionStateV01.decision(
                    promotionState,
                    "world_to_source_bridge_promoted",
                )
        val promotedRadiometry =
            ScientificPromotionStateV01.decision(
                promotionState,
                "radiometric_calibration_promoted",
            )

        val roots = linkedSetOf<String>()
        val samples = ArrayList<Sample>()
        var relationRecordCount = 0

        for (record in records) {
            if (record.optString("axis_scope") != "WORLD_SPACE_RESIDUAL") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "WORLD_SPACE_RESIDUAL",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            val explicitCandidateRelation =
                record.optString("session_binding_status") ==
                    "BOUND_TO_ACTIVE_OBSERVATION_SET" &&
                    record.optString("relation_evidence_class") in
                    setOf(
                        "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
                        "SEALED_CAPTURE_SESSION_PROVENANCE",
                    ) &&
                    payload.optBoolean(
                        "world_to_source_relation_admitted",
                        false,
                    ) &&
                    payload.optBoolean(
                        "radiometric_relation_admitted",
                        false,
                    )
            if (!explicitCandidateRelation && !promotedBridge) continue
            relationRecordCount++
            val rs =
                record.optJSONArray("source_sha256_roots") ?: JSONArray()
            val processingRs =
                record.optJSONArray(
                    "session_processing_source_sha256_roots",
                ) ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i)
                    .trim()
                    .lowercase()
                    .takeIf(String::isNotBlank)
                    ?.let(roots::add)
            }
            for (i in 0 until processingRs.length()) {
                processingRs.optString(i)
                    .trim()
                    .lowercase()
                    .takeIf(String::isNotBlank)
                    ?.let(roots::add)
            }
            val arr = payload.optJSONArray("samples") ?: continue
            for (i in 0 until arr.length()) {
                val s = arr.optJSONObject(i) ?: continue
                if (s.optBoolean("motion_or_occlusion", false)) continue
                val world = s.optString("world_point_id")
                val sensor = s.optString("sensor_cell_id")
                val sha = s.optString("source_sha256")
                val value = s.optDouble("value", Double.NaN)
                val normalizedSha = sha.trim().lowercase()
                if (
                    world.isBlank() || sensor.isBlank() ||
                    normalizedSha !in roots.map(String::lowercase).toSet() ||
                    !value.isFinite()
                ) {
                    continue
                }
                samples += Sample(
                    worldPointId = world,
                    sensorCellId = sensor,
                    sourceSha256 = normalizedSha,
                    value = value,
                    role = s.optString("role", "TRAIN"),
                )
            }
        }

        val train = samples.filter { it.role == "TRAIN" }
        val held = samples.filter { it.role == "HELD_OUT" }
        if (relationRecordCount == 0) {
            return unavailable("NO_ADMITTED_WORLD_RADIOMETRIC_RELATION", roots)
        }
        if (train.size < 4) {
            return unavailable("INSUFFICIENT_WORLD_SPACE_SAMPLES", roots)
        }

        val worldMedian = linkedMapOf<String, Double>()
        for ((id, group) in train.groupBy { it.worldPointId }) {
            ResearchMathV01.median(group.map { it.value })?.let {
                worldMedian[id] = it
            }
        }
        if (worldMedian.isEmpty()) {
            return unavailable("NO_WORLD_FIXED_CANDIDATES", roots)
        }

        val sensorResidualGroups = linkedMapOf<String, MutableList<Double>>()
        for (s in train) {
            val w = worldMedian[s.worldPointId] ?: continue
            sensorResidualGroups
                .getOrPut(s.sensorCellId) { ArrayList() }
                .add(s.value - w)
        }
        val sensorMedian = linkedMapOf<String, Double>()
        for ((id, values) in sensorResidualGroups) {
            ResearchMathV01.median(values)?.let { sensorMedian[id] = it }
        }

        val unexplained = ArrayList<Double>()
        for (s in train) {
            val w = worldMedian[s.worldPointId] ?: continue
            val sensor = sensorMedian[s.sensorCellId] ?: 0.0
            unexplained += s.value - w - sensor
        }
        val temporalResidualRms =
            if (unexplained.isNotEmpty()) {
                sqrt(unexplained.sumOf { it * it } / unexplained.size.toDouble())
                    .takeIf(Double::isFinite)
            } else {
                null
            }

        val heldActual = ArrayList<Double>()
        val heldPred = ArrayList<Double>()
        for (s in held) {
            val w = worldMedian[s.worldPointId] ?: continue
            val sensor = sensorMedian[s.sensorCellId] ?: 0.0
            heldActual += s.value
            heldPred += w + sensor
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "WORLD_SPACE_RESIDUAL_CANDIDATES_AVAILABLE")
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("training_sample_count", train.size)
            .put("held_out_sample_count", heldActual.size)
            .put("world_point_candidate_count", worldMedian.size)
            .put("sensor_cell_candidate_count", sensorMedian.size)
            .put("world_fixed_candidates", mapJson(worldMedian))
            .put("sensor_fixed_residual_candidates", mapJson(sensorMedian))
            .put(
                "temporal_or_unexplained_residual_rms",
                temporalResidualRms ?: JSONObject.NULL,
            )
            .put(
                "held_out_reconstruction_rmse",
                ResearchMathV01.rmse(heldActual, heldPred)
                    ?: JSONObject.NULL,
            )
            .put(
                "world_to_source_relation_authority",
                if (promotedBridge) {
                    "PROMOTED_WORLD_TO_SOURCE_BRIDGE"
                } else {
                    "EXPLICIT_SESSION_BOUND_RECORD_CANDIDATE_ONLY"
                },
            )
            .put(
                "radiometric_relation_authority",
                if (promotedRadiometry) {
                    "PROMOTED_RADIOMETRIC_CALIBRATION"
                } else {
                    "EXPLICIT_SESSION_BOUND_RECORD_CANDIDATE_ONLY"
                },
            )
            .put("view_dependent_component_estimated", false)
            .put("motion_occlusion_component_estimated", false)
            .put("unknown_residual_preserved", true)
            .put("world_fixed_signal_estimated", false)
            .put("sensor_fixed_pattern_estimated", false)
            .put("temporal_random_residual_estimated", false)
            .put("world_space_noise_separation_promoted", false)
            .put("world_space_denoise_applied", false)
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun mapJson(values: Map<String, Double>): JSONObject {
        val out = JSONObject()
        for ((k, v) in values) out.put(k, v)
        return out
    }

    private fun unavailable(
        reason: String,
        roots: Set<String>,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("source_sha256_roots", JSONArray(roots.toList()))
            .put("world_space_noise_separation_promoted", false)
            .put("world_space_denoise_applied", false)
            .put("scientific_writeback_allowed", false)
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Deterministic additive world-vs-sensor field candidate from explicit
 * controlled-rotation relation records.
 *
 * Model: observed_relative_ev ~= world_cell + sensor_cell.
 * Alternating medians are used only as a robust candidate decomposition.
 * Lens-only vignetting is never claimed.
 *
 * The normal public entry point validates/admit records before solving.
 * A separate derived-diagnostic entry point exists only for already-admitted
 * read-only dry-runs that change derived cell labels in memory. That path does
 * not create, validate, bind, or promote a replacement calibration record.
 */
object FieldResponseRotationSeparationCandidateV01 {
    const val SCHEMA = "D.RAW/FieldResponseRotationSeparationCandidate/0.1"

    private data class Sample(
        val worldCellId: String,
        val sensorCellId: String,
        val valueEv: Double,
        val role: String,
    )

    fun evaluate(records: List<JSONObject>): JSONObject {
        val roots = linkedSetOf<String>()
        val samples = ArrayList<Sample>()
        var admittedRecordCount = 0

        for (record in records) {
            if (record.optString("axis_scope") != "FIELD_RESPONSE") continue
            val admission =
                CalibrationObservationAdmissionV01.admitForNumericCandidate(
                    record = record,
                    axis = "FIELD_RESPONSE",
                )
            if (
                admission.optString("status") !=
                "NUMERIC_CANDIDATE_RELATION_ADMITTED"
            ) {
                continue
            }
            val payload = record.optJSONObject("axis_payload") ?: continue
            if (!payload.optBoolean("controlled_rotation_relation", false)) continue
            admittedRecordCount++
            val rs = record.optJSONArray("source_sha256_roots") ?: JSONArray()
            for (i in 0 until rs.length()) {
                rs.optString(i).takeIf(String::isNotBlank)?.let(roots::add)
            }
            parseSamples(
                array = payload.optJSONArray("samples") ?: JSONArray(),
                out = samples,
            )
        }

        if (admittedRecordCount == 0) {
            return unavailable("NO_CONTROLLED_ROTATION_RELATION_RECORD")
        }

        return solve(
            samples = samples,
            roots = roots,
            inputAuthority =
                "ADMITTED_CALIBRATION_OBSERVATION_RECORD_SAMPLES",
            derivedDiagnosticInput = false,
        )
    }

    /**
     * Solve an in-memory derived diagnostic sample set without pretending that
     * the relabelled samples form a new CalibrationObservationRecord.
     *
     * Callers must first admit the unchanged source record through
     * CalibrationObservationAdmissionV01. This function does not validate or
     * bind a record and therefore cannot establish relation authority.
     */
    fun evaluateDerivedDiagnosticSamples(
        samples: JSONArray,
        sourceSha256Roots: JSONArray,
    ): JSONObject {
        val parsed = ArrayList<Sample>()
        parseSamples(
            array = samples,
            out = parsed,
        )
        val roots = linkedSetOf<String>()
        for (i in 0 until sourceSha256Roots.length()) {
            sourceSha256Roots
                .optString(i)
                .takeIf(String::isNotBlank)
                ?.lowercase()
                ?.let(roots::add)
        }

        return solve(
            samples = parsed,
            roots = roots,
            inputAuthority =
                "DERIVED_IN_MEMORY_DIAGNOSTIC_SAMPLES_FROM_ALREADY_ADMITTED_RECORD",
            derivedDiagnosticInput = true,
        )
    }

    private fun parseSamples(
        array: JSONArray,
        out: MutableList<Sample>,
    ) {
        for (i in 0 until array.length()) {
            val s = array.optJSONObject(i) ?: continue
            val world = s.optString("world_cell_id")
            val sensor = s.optString("sensor_cell_id")
            val value = s.optDouble("relative_signal_ev", Double.NaN)
            if (
                world.isBlank() ||
                sensor.isBlank() ||
                !value.isFinite()
            ) {
                continue
            }
            out +=
                Sample(
                    worldCellId = world,
                    sensorCellId = sensor,
                    valueEv = value,
                    role = s.optString("role", "TRAIN"),
                )
        }
    }

    private fun solve(
        samples: List<Sample>,
        roots: Set<String>,
        inputAuthority: String,
        derivedDiagnosticInput: Boolean,
    ): JSONObject {
        val train = samples.filter { it.role == "TRAIN" }
        val held = samples.filter { it.role == "HELD_OUT" }
        if (train.size < 6) {
            return unavailable(
                reason = "INSUFFICIENT_FIELD_SEPARATION_SAMPLES",
                inputAuthority = inputAuthority,
                derivedDiagnosticInput =
                    derivedDiagnosticInput,
            )
        }
        if (
            train.map { it.worldCellId }
                .toSet()
                .size < 2
        ) {
            return unavailable(
                reason = "WORLD_CELL_DIVERSITY_REQUIRED",
                inputAuthority = inputAuthority,
                derivedDiagnosticInput =
                    derivedDiagnosticInput,
            )
        }
        if (
            train.map { it.sensorCellId }
                .toSet()
                .size < 2
        ) {
            return unavailable(
                reason = "SENSOR_CELL_DIVERSITY_REQUIRED",
                inputAuthority = inputAuthority,
                derivedDiagnosticInput =
                    derivedDiagnosticInput,
            )
        }

        val world = linkedMapOf<String, Double>()
        val sensor = linkedMapOf<String, Double>()

        for ((id, group) in train.groupBy { it.worldCellId }) {
            ResearchMathV01.median(
                group.map { it.valueEv },
            )?.let {
                world[id] = it
            }
        }

        repeat(4) {
            sensor.clear()
            for ((id, group) in train.groupBy { it.sensorCellId }) {
                val residuals =
                    group.mapNotNull { s ->
                        world[s.worldCellId]
                            ?.let { s.valueEv - it }
                    }
                ResearchMathV01.median(residuals)
                    ?.let {
                        sensor[id] = it
                    }
            }

            // Fix the additive gauge by forcing median sensor effect to zero.
            val sensorGauge =
                ResearchMathV01.median(
                    sensor.values.toList(),
                ) ?: 0.0
            if (sensorGauge != 0.0) {
                for (key in sensor.keys.toList()) {
                    sensor[key] =
                        (sensor[key] ?: 0.0) -
                            sensorGauge
                }
            }

            world.clear()
            for ((id, group) in train.groupBy { it.worldCellId }) {
                val corrected =
                    group.mapNotNull { s ->
                        sensor[s.sensorCellId]
                            ?.let { s.valueEv - it }
                    }
                ResearchMathV01.median(corrected)
                    ?.let {
                        world[id] = it
                    }
            }
        }

        val trainActual = ArrayList<Double>()
        val trainPred = ArrayList<Double>()
        for (s in train) {
            val w = world[s.worldCellId] ?: continue
            val q = sensor[s.sensorCellId] ?: continue
            trainActual += s.valueEv
            trainPred += w + q
        }

        val heldActual = ArrayList<Double>()
        val heldPred = ArrayList<Double>()
        for (s in held) {
            val w = world[s.worldCellId] ?: continue
            val q = sensor[s.sensorCellId] ?: continue
            heldActual += s.valueEv
            heldPred += w + q
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "WORLD_SENSOR_FIELD_SEPARATION_CANDIDATE_AVAILABLE",
            )
            .put(
                "source_sha256_roots",
                JSONArray(roots.toList()),
            )
            .put("input_authority", inputAuthority)
            .put(
                "derived_diagnostic_input",
                derivedDiagnosticInput,
            )
            .put(
                "derived_diagnostic_input_is_calibration_record",
                false,
            )
            .put(
                "derived_diagnostic_input_creates_relation_authority",
                false,
            )
            .put(
                "training_sample_count",
                trainActual.size,
            )
            .put(
                "held_out_sample_count",
                heldActual.size,
            )
            .put(
                "world_component_candidates_ev",
                mapJson(world),
            )
            .put(
                "sensor_component_candidates_ev",
                mapJson(sensor),
            )
            .put(
                "training_rmse_ev",
                ResearchMathV01.rmse(
                    trainActual,
                    trainPred,
                ) ?: JSONObject.NULL,
            )
            .put(
                "held_out_rmse_ev",
                ResearchMathV01.rmse(
                    heldActual,
                    heldPred,
                ) ?: JSONObject.NULL,
            )
            .put(
                "candidate_separation_computed",
                true,
            )
            .put(
                "world_fixed_component_estimated_candidate",
                true,
            )
            .put(
                "sensor_fixed_component_estimated_candidate",
                true,
            )
            .put(
                "world_fixed_component_estimated",
                false,
            )
            .put(
                "sensor_fixed_component_estimated",
                false,
            )
            .put("scene_illumination_separated", false)
            .put("lens_only_vignetting_proven", false)
            .put("correction_gain_allowed", false)
            .put(
                "field_response_calibration_promoted",
                false,
            )
            .put("candidate_applied", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun mapJson(
        values: Map<String, Double>,
    ): JSONObject {
        val out = JSONObject()
        for ((k, v) in values) {
            out.put(k, v)
        }
        return out
    }

    private fun unavailable(
        reason: String,
        inputAuthority: String =
            "NONE",
        derivedDiagnosticInput: Boolean =
            false,
    ): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put("status", "UNKNOWN_FAIL_CLOSED")
            .put("reason", reason)
            .put("input_authority", inputAuthority)
            .put(
                "derived_diagnostic_input",
                derivedDiagnosticInput,
            )
            .put(
                "derived_diagnostic_input_is_calibration_record",
                false,
            )
            .put(
                "derived_diagnostic_input_creates_relation_authority",
                false,
            )
            .put(
                "candidate_separation_computed",
                false,
            )
            .put(
                "world_fixed_component_estimated",
                false,
            )
            .put(
                "sensor_fixed_component_estimated",
                false,
            )
            .put("lens_only_vignetting_proven", false)
            .put("correction_gain_allowed", false)
            .put("scientific_writeback_allowed", false)
}

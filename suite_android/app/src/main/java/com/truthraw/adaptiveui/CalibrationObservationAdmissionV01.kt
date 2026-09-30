package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Central fail-closed admission gate for numeric research candidates.
 *
 * Record validity is intentionally weaker than numerical relation admission:
 * USER_GROUPING_HINT_ONLY and NONE may be stored as records but may never drive
 * a scientific candidate solver.
 */
object CalibrationObservationAdmissionV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationAdmission/0.1"

    private val numericRelationClasses =
        setOf(
            "SEALED_CAPTURE_SESSION_PROVENANCE",
            "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
            "OBSERVATION_REPEATABILITY_CANDIDATE",
        )

    private val numericValidationStatuses =
        setOf(
            "RELATION_RECORDED",
            "MEASURED",
            "VALIDATED",
            "HELD_OUT_VALIDATED",
            "PASS",
            "PASS_HELD_OUT",
        )

    fun admitForNumericCandidate(
        record: JSONObject,
        axis: String,
    ): JSONObject {
        val validation =
            CalibrationObservationRecordValidatorV01.validate(record)
        val issues = JSONArray()

        if (
            validation.optString("status") !=
            "CALIBRATION_OBSERVATION_RECORD_VALID"
        ) {
            issues.put("RECORD_VALIDATION_FAILED")
        }
        if (record.optString("axis_scope") != axis) {
            issues.put("AXIS_SCOPE_MISMATCH")
        }
        val relation = record.optString("relation_evidence_class")
        if (relation !in numericRelationClasses) {
            issues.put("RELATION_EVIDENCE_INSUFFICIENT_FOR_NUMERIC_CANDIDATE")
        }
        val validationStatus =
            record.optString("validation_status").uppercase()
        if (validationStatus !in numericValidationStatuses) {
            issues.put("RECORD_VALIDATION_STATUS_NOT_NUMERICALLY_ADMITTED")
        }

        if (
            record.optString("session_binding_status").isNotBlank() &&
            record.optString("session_binding_status") !=
                "BOUND_TO_ACTIVE_OBSERVATION_SET"
        ) {
            issues.put("RECORD_NOT_BOUND_TO_ACTIVE_SESSION")
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (issues.length() == 0) {
                    "NUMERIC_CANDIDATE_RELATION_ADMITTED"
                } else {
                    "NUMERIC_CANDIDATE_RELATION_BLOCKED"
                },
            )
            .put("axis_scope", axis)
            .put("relation_evidence_class", relation)
            .put("issues", issues)
            .put("candidate_computation_allowed", issues.length() == 0)
            .put("record_validation_promotes_calibration", false)
            .put("calibration_promoted", false)
            .put("scientific_writeback_allowed", false)
    }
}

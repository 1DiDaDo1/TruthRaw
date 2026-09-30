package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Versioned relation-based calibration observation record contract.
 *
 * Calibration observations are optional extra evidence. They are never keyed
 * by a camera/lens product name and never required at normal intake.
 */
object CalibrationObservationRecordV01 {
    const val SCHEMA = "D.RAW/CalibrationObservationRecord/0.1"

    fun describeContract(): JSONObject =
        JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                "CALIBRATION_OBSERVATION_RECORD_CONTRACT_AVAILABLE",
            )
            .put(
                "supported_axes",
                JSONArray()
                    .put("FIELD_RESPONSE")
                    .put("COLOUR_RELATION")
                    .put("OPTICAL_SUPPORT")
                    .put("DARK_NOISE_OFFSET")
                    .put("TEMPORAL_FOOTPRINT"),
            )
            .put(
                "required_record_fields",
                JSONArray()
                    .put("RECORD_VERSION")
                    .put("AXIS_SCOPE")
                    .put("SOURCE_SHA256_ROOTS")
                    .put("OBSERVATION_ROLES")
                    .put("SETUP_DESCRIPTION")
                    .put("RELATION_EVIDENCE_CLASS")
                    .put("UNCERTAINTY")
                    .put("VALIDATION_STATUS"),
            )
            .put(
                "identity_policy",
                JSONObject()
                    .put("camera_model_name_is_key", false)
                    .put("lens_model_name_is_key", false)
                    .put("vendor_name_is_key", false)
                    .put("source_sha256_roots_are_primary", true),
            )
            .put(
                "normal_use_policy",
                JSONObject()
                    .put("required_for_raw_intake", false)
                    .put("required_for_scientific_master", false)
                    .put("optional_extra_observation_evidence", true),
            )
            .put("record_attached", false)
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("creates_new_evidence_without_physical_observation", false)
            .put("scientific_writeback_allowed", false)
}

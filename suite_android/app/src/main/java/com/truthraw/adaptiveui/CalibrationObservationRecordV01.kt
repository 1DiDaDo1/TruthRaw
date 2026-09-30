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
                    .put("RADIOMETRIC_RESPONSE")
                    .put("FIELD_RESPONSE")
                    .put("COLOUR_RELATION")
                    .put("OPTICAL_SUPPORT")
                    .put("DARK_NOISE_OFFSET")
                    .put("NOISE_COMPONENT_SEPARATION")
                    .put("GEOMETRY_DEPTH_VISIBILITY")
                    .put("TEMPORAL_FOOTPRINT")
                    .put("WORLD_SPACE_RESIDUAL"),
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
                "candidate_runtime_payload",
                JSONObject()
                    .put("field_name", "AXIS_PAYLOAD")
                    .put("optional_for_record_validity", true)
                    .put("required_for_numeric_candidate_solver", true)
                    .put("camera_lens_vendor_or_raw_identity_may_replace_payload_evidence", false),
            )
            .put(
                "identity_policy",
                JSONObject()
                    .put("camera_model_name_is_key", false)
                    .put("lens_model_name_is_key", false)
                    .put("vendor_name_is_key", false)
                    .put("source_sha256_roots_are_primary", true)
                    .put(
                        "canonical_record_identity",
                        "CalibrationObservationRecordIdentityV01",
                    )
                    .put(
                        "object_key_order_changes_identity",
                        false,
                    )
                    .put("sha256_hex_normalized_lowercase", true),
            )
            .put(
                "session_binding_policy",
                JSONObject()
                    .put(
                        "runtime",
                        "CalibrationObservationSessionBindingV01",
                    )
                    .put(
                        "record_roots_must_belong_to_active_session",
                        true,
                    )
                    .put(
                        "camera_upstream_rawsensor_alias_supported",
                        true,
                    )
                    .put("cross_session_record_use_allowed", false),
            )
            .put(
                "bundle_ingest",
                JSONObject()
                    .put(
                        "runtime",
                        "CalibrationObservationRecordBundleV01",
                    )
                    .put(
                        "foundation_entry_point",
                        "FreeWorldObservationGeometryFoundationV01.buildWithCalibrationBundle",
                    )
                    .put("invalid_records_fail_closed", true)
                    .put("canonical_deduplication_enabled", true)
                    .put(
                        "numeric_candidate_validation_status_required",
                        true,
                    ),
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

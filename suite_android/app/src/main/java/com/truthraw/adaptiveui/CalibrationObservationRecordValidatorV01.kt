package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Fail-closed validator for optional calibration observation records.
 *
 * A valid record is still only a record. Validation never promotes the
 * calibration or authorizes correction.
 */
object CalibrationObservationRecordValidatorV01 {
    const val SCHEMA =
        "D.RAW/CalibrationObservationRecordValidation/0.1"

    private val allowedAxes =
        setOf(
            "RADIOMETRIC_RESPONSE",
            "FIELD_RESPONSE",
            "COLOUR_RELATION",
            "OPTICAL_SUPPORT",
            "DARK_NOISE_OFFSET",
            "NOISE_COMPONENT_SEPARATION",
            "GEOMETRY_DEPTH_VISIBILITY",
            "TEMPORAL_FOOTPRINT",
            "WORLD_SPACE_RESIDUAL",
        )

    private val allowedRelationClasses =
        setOf(
            "SEALED_CAPTURE_SESSION_PROVENANCE",
            "EXPLICIT_CALIBRATION_CAPTURE_RECORD",
            "OBSERVATION_REPEATABILITY_CANDIDATE",
            "USER_GROUPING_HINT_ONLY",
            "NONE",
        )

    fun validate(record: JSONObject): JSONObject {
        val issues = JSONArray()

        val version =
            record.optString("record_version")
        if (version.isBlank()) {
            issues.put("MISSING_RECORD_VERSION")
        }

        val axis = record.optString("axis_scope")
        if (axis !in allowedAxes) {
            issues.put("UNSUPPORTED_AXIS_SCOPE")
        }

        val roots =
            record.optJSONArray("source_sha256_roots")
        val uniqueRoots = linkedSetOf<String>()
        if (roots == null || roots.length() == 0) {
            issues.put("SOURCE_SHA256_ROOTS_REQUIRED")
        } else {
            for (i in 0 until roots.length()) {
                val sha = roots.optString(i)
                if (
                    sha.length != 64 ||
                    sha.any {
                        !it.isDigit() &&
                            it.lowercaseChar() !in 'a'..'f'
                    }
                ) {
                    issues.put(
                        "INVALID_SOURCE_SHA256_AT_INDEX_$i",
                    )
                } else {
                    uniqueRoots += sha.lowercase()
                }
            }
            if (uniqueRoots.size != roots.length()) {
                issues.put("DUPLICATE_SOURCE_SHA256_ROOT")
            }
        }

        val roles =
            record.optJSONArray("observation_roles")
        if (
            roles == null ||
            roots == null ||
            roles.length() != roots.length()
        ) {
            issues.put(
                "OBSERVATION_ROLES_MUST_MATCH_SOURCE_ROOT_COUNT",
            )
        }

        if (
            record.optString("setup_description").isBlank()
        ) {
            issues.put("SETUP_DESCRIPTION_REQUIRED")
        }

        val relation =
            record.optString("relation_evidence_class")
        if (relation !in allowedRelationClasses) {
            issues.put("UNSUPPORTED_RELATION_EVIDENCE_CLASS")
        }

        if (!record.has("uncertainty")) {
            issues.put("UNCERTAINTY_REQUIRED")
        }

        if (
            record.optString("validation_status").isBlank()
        ) {
            issues.put("VALIDATION_STATUS_REQUIRED")
        }

        if (
            record.has("axis_payload") &&
            record.opt("axis_payload") !is JSONObject
        ) {
            issues.put("AXIS_PAYLOAD_MUST_BE_OBJECT")
        }

        val forbiddenIdentityKeys =
            setOf(
                "camera_model_key",
                "lens_model_key",
                "vendor_key",
                "device_profile_key",
                "raw_format_key",
                "container_format_key",
                "decoder_route_key",
                "physical_camera_id_key",
            )
        scanForbiddenIdentityKeys(
            value = record,
            path = "$",
            forbidden = forbiddenIdentityKeys,
            issues = issues,
        )

        return JSONObject()
            .put("schema", SCHEMA)
            .put(
                "status",
                if (issues.length() == 0) {
                    "CALIBRATION_OBSERVATION_RECORD_VALID"
                } else {
                    "CALIBRATION_OBSERVATION_RECORD_INVALID"
                },
            )
            .put("issues", issues)
            .put(
                "validated_source_root_count",
                uniqueRoots.size,
            )
            .put(
                "record_validation_promotes_calibration",
                false,
            )
            .put("calibration_promoted", false)
            .put("correction_authorized", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun scanForbiddenIdentityKeys(
        value: Any?,
        path: String,
        forbidden: Set<String>,
        issues: JSONArray,
    ) {
        when (value) {
            is JSONObject -> {
                val keys = value.keys()
                while (keys.hasNext()) {
                    val key = keys.next()
                    val child = value.opt(key)
                    val childPath = "$path.$key"
                    if (key in forbidden) {
                        issues.put(
                            "FORBIDDEN_IDENTITY_KEY_AT_$childPath",
                        )
                    }
                    scanForbiddenIdentityKeys(
                        child,
                        childPath,
                        forbidden,
                        issues,
                    )
                }
            }
            is JSONArray -> {
                for (i in 0 until value.length()) {
                    scanForbiddenIdentityKeys(
                        value.opt(i),
                        "$path[$i]",
                        forbidden,
                        issues,
                    )
                }
            }
        }
    }
}

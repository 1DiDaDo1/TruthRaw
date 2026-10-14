package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only real-device diagnostic projection of the already-created Source
 * Capability runtime envelope.
 *
 * This object MUST NOT parse a source, hash a source, construct a second
 * Observation Evidence Ledger, evaluate image samples, or invoke radiometric
 * validators. It only projects facts from the existing runtime envelope and
 * its already-composed raw-to-linear contract into a deterministic diagnostic
 * record suitable for real-device inspection.
 */
object SourceCapabilityRealDeviceDiagnosticV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRealDeviceDiagnostic/0.1"
    const val STATUS = "SHADOW_ONLY_ZERO_PIXEL_CHANGE"
    const val BUILD_PROVENANCE_AUTHORITY = "PROVENANCE_ONLY_NOT_SCIENTIFIC_AUTHORITY"

    private val rawToLinearSafeguardKeys =
        listOf(
            "table_application_to_image_samples_validated",
            "full_raw_to_linear_mapping_validated",
            "image_sample_payload_read",
            "linearization_applied_to_image_samples",
            "black_subtraction_applied_to_image_samples",
            "normalization_applied_to_image_samples",
            "clipping_applied_to_image_samples",
            "observed_sample_censoring_validated",
            "physical_sensor_saturation_calibrated",
            "scene_radiance_above_censor_recovered",
            "record_lifecycle_effect",
            "scientific_admission_effect",
            "downstream_scientific_consumer_enabled",
            "pixel_route_effect",
            "scientific_master_pixel_effect",
            "appearance_pixel_effect",
            "source_values_modified",
            "creates_new_evidence",
            "scientific_writeback_allowed",
            "parser_tag_rename_performed",
            "observation_evidence_ledger_identity_changed",
        )

    fun describe(
        runtimeEnvelope: JSONObject,
        buildGitSha: String? = null,
    ): JSONObject {
        val sourceSha256 = runtimeEnvelope.optString("source_sha256", "").lowercase()
        val rawToLinear =
            runtimeEnvelope.optJSONObject("radiometric_raw_to_linear_contract_v0_1")
        val sourceBindingStatus =
            when {
                !sourceSha256.matches(Regex("[0-9a-f]{64}")) ->
                    "UNRESOLVED_RUNTIME_ENVELOPE_SOURCE_SHA256_FAIL_CLOSED"
                rawToLinear == null ->
                    "UNRESOLVED_RAW_TO_LINEAR_CONTRACT_MISSING_FAIL_CLOSED"
                rawToLinear.optString("source_sha256", "").lowercase() != sourceSha256 ->
                    "CONTRADICTED_RAW_TO_LINEAR_SOURCE_BINDING_FAIL_CLOSED"
                else -> "VALIDATED_EXISTING_RUNTIME_ENVELOPE_SOURCE_BINDING"
            }

        val safeguards = JSONObject()
        var allSafeguardsExplicitlyFalse = rawToLinear != null
        for (key in rawToLinearSafeguardKeys) {
            val explicitlyFalse =
                rawToLinear?.has(key) == true &&
                    rawToLinear.opt(key) is Boolean &&
                    !rawToLinear.optBoolean(key, true)
            safeguards.put(key, if (rawToLinear?.has(key) == true) rawToLinear.opt(key) else JSONObject.NULL)
            if (!explicitlyFalse) allSafeguardsExplicitlyFalse = false
        }

        val ifdReasons = JSONArray()
        val sourceIfds = rawToLinear?.optJSONArray("ifds") ?: JSONArray()
        val canonicalIfdReasons = mutableListOf<String>()
        for (index in 0 until sourceIfds.length()) {
            val ifd = sourceIfds.optJSONObject(index) ?: continue
            val projected =
                JSONObject()
                    .put("ifd_identity", ifd.optString("ifd_identity", "UNKNOWN_IFD_IDENTITY"))
                    .put("ifd_path", ifd.optString("ifd_path", "UNKNOWN_IFD"))
                    .put("ifd_offset", ifd.optLong("ifd_offset", -1L))
                    .put(
                        "raw_to_linear_composition_status",
                        ifd.optString("raw_to_linear_composition_status", "UNRESOLVED_FAIL_CLOSED"),
                    )
                    .put("reason", ifd.optString("reason", "UNRESOLVED_REASON_NOT_EXPOSED_FAIL_CLOSED"))
            canonicalIfdReasons += canonicalJson(projected)
        }
        canonicalIfdReasons.sorted().forEach { encoded -> ifdReasons.put(JSONObject(encoded)) }

        val buildSha = buildGitSha?.trim()?.lowercase()?.takeIf { it.matches(Regex("[0-9a-f]{40}")) }
        val buildProvenance =
            JSONObject()
                .put("authority", BUILD_PROVENANCE_AUTHORITY)
                .put("git_sha", buildSha ?: JSONObject.NULL)
                .put("availability", if (buildSha == null) "UNKNOWN_NOT_SUPPLIED" else "AVAILABLE")
                .put("scientific_authority", false)
                .put("scientific_admission_effect", false)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.ifBlank { JSONObject.NULL })
            .put("source_byte_length", runtimeEnvelope.opt("source_byte_length") ?: JSONObject.NULL)
            .put("runtime_envelope_schema", runtimeEnvelope.optString("schema", "UNKNOWN"))
            .put("runtime_envelope_status", runtimeEnvelope.optString("status", "UNKNOWN_FAIL_CLOSED"))
            .put("runtime_envelope_parser_id", runtimeEnvelope.optString("parser_id", "UNKNOWN"))
            .put("ledger_schema", runtimeEnvelope.optString("ledger_schema", "UNKNOWN"))
            .put("ledger_digest_sha256", runtimeEnvelope.opt("ledger_digest_sha256") ?: JSONObject.NULL)
            .put(
                "observation_evidence_digest_sha256",
                runtimeEnvelope.opt("observation_evidence_digest_sha256") ?: JSONObject.NULL,
            )
            .put("source_binding_status", sourceBindingStatus)
            .put("raw_to_linear_contract_present", rawToLinear != null)
            .put("raw_to_linear_contract_schema", rawToLinear?.optString("schema", "UNKNOWN") ?: "UNKNOWN")
            .put("raw_to_linear_contract_status", rawToLinear?.optString("status", "UNKNOWN") ?: "UNKNOWN")
            .put(
                "raw_to_linear_composition_status",
                rawToLinear?.optString("raw_to_linear_composition_status", "UNRESOLVED_FAIL_CLOSED")
                    ?: "UNRESOLVED_RAW_TO_LINEAR_CONTRACT_MISSING_FAIL_CLOSED",
            )
            .put(
                "dependency_ifd_identity_integrity_status",
                rawToLinear?.optString(
                    "dependency_ifd_identity_integrity_status",
                    "UNRESOLVED_FAIL_CLOSED",
                ) ?: "UNRESOLVED_RAW_TO_LINEAR_CONTRACT_MISSING_FAIL_CLOSED",
            )
            .put(
                "linearization_report_binding_status",
                bindingStatus(rawToLinear, "linearization_report_binding"),
            )
            .put(
                "effective_black_normalization_report_binding_status",
                bindingStatus(rawToLinear, "effective_black_normalization_report_binding"),
            )
            .put(
                "clipping_rule_report_binding_status",
                bindingStatus(rawToLinear, "clipping_rule_report_binding"),
            )
            .put(
                "metadata_ordering_composition_contract_validated",
                rawToLinear?.optBoolean("metadata_ordering_composition_contract_validated", false) ?: false,
            )
            .put("full_raw_to_linear_mapping_validated", false)
            .put("upstream_hard_false_safeguards", safeguards)
            .put("all_upstream_hard_false_safeguards_explicitly_false", allSafeguardsExplicitlyFalse)
            .put("ifd_fail_closed_reasons", ifdReasons)
            .put("build_provenance", buildProvenance)
            .put("image_sample_payload_read", false)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("downstream_scientific_consumer_enabled", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("observation_evidence_ledger_identity_changed", false)
    }

    /** Stable serialization for export/log comparison across repeated runs. */
    fun deterministicJson(
        runtimeEnvelope: JSONObject,
        buildGitSha: String? = null,
    ): String = canonicalJson(describe(runtimeEnvelope, buildGitSha))

    private fun bindingStatus(
        report: JSONObject?,
        key: String,
    ): String =
        report?.optJSONObject(key)?.optString("binding_status", "UNRESOLVED_FAIL_CLOSED")
            ?: "UNRESOLVED_RAW_TO_LINEAR_CONTRACT_MISSING_FAIL_CLOSED"

    private fun canonicalJson(value: Any?): String =
        when (value) {
            null, JSONObject.NULL -> "null"
            is JSONObject -> {
                val keys = mutableListOf<String>()
                val iterator = value.keys()
                while (iterator.hasNext()) keys += iterator.next()
                keys.sorted().joinToString(prefix = "{", postfix = "}", separator = ",") { key ->
                    JSONObject.quote(key) + ":" + canonicalJson(value.opt(key))
                }
            }
            is JSONArray ->
                (0 until value.length()).joinToString(prefix = "[", postfix = "]", separator = ",") { index ->
                    canonicalJson(value.opt(index))
                }
            is String -> JSONObject.quote(value)
            is Boolean -> value.toString()
            is Number -> value.toString()
            else -> JSONObject.quote(value.toString())
        }
}

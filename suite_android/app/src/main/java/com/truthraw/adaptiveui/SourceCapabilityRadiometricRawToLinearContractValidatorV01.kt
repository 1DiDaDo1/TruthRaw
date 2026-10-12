package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only Source Capability raw-to-linear metadata composition contract v0.1.
 *
 * This validator composes already-existing source-bound validator reports only.
 * It never receives parsed source metadata, never reads image sample payload,
 * never applies radiometric operations to pixels, and never changes evidence
 * lifecycle, scientific admission, Scientific Master, Appearance, or writeback.
 *
 * Current v0.1 can validate only the existing metadata-order prefix:
 * Linearization -> Black Subtraction -> Rescaling / Normalization.
 * A separate Source Capability clipping contract does not yet exist, so the
 * complete raw-to-linear composition remains fail-closed and unresolved.
 */
object SourceCapabilityRadiometricRawToLinearContractValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricRawToLinearContract/0.1"
    const val STATUS = "SHADOW_ONLY_RAW_TO_LINEAR_ORDERING_COMPOSITION_CONTRACT"

    const val CONTRADICTED = "CONTRADICTED_RAW_TO_LINEAR_METADATA_COMPOSITION_CONTRACT_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_RAW_TO_LINEAR_METADATA_COMPOSITION_CONTRACT_FAIL_CLOSED"
    const val PREFIX_VALIDATED = "VALIDATED_LINEARIZATION_BLACK_NORMALIZATION_PREFIX_SHADOW_ONLY"
    const val PREFIX_UNRESOLVED = "UNRESOLVED_LINEARIZATION_BLACK_NORMALIZATION_PREFIX_FAIL_CLOSED"
    const val PREFIX_CONTRADICTED = "CONTRADICTED_LINEARIZATION_BLACK_NORMALIZATION_PREFIX_FAIL_CLOSED"
    const val CLIPPING_UNRESOLVED = "UNRESOLVED_CLIPPING_COMPONENT_NOT_VALIDATED"

    const val COMPONENT_BOUND = "VALIDATED_EXISTING_COMPONENT_REPORT_BINDING"
    const val COMPONENT_MISSING = "UNRESOLVED_COMPONENT_REPORT_MISSING_FAIL_CLOSED"
    const val COMPONENT_SCHEMA_UNRESOLVED = "UNRESOLVED_COMPONENT_REPORT_SCHEMA_OR_STATUS_FAIL_CLOSED"
    const val COMPONENT_SOURCE_CONTRADICTED = "CONTRADICTED_COMPONENT_SOURCE_BINDING_FAIL_CLOSED"

    private data class ComponentIndex(
        val byIdentity: Map<String, JSONObject>,
        val duplicateIdentities: Set<String>,
        val malformedIdentityCount: Int,
    )

    fun describe(
        sourceSha256: String,
        linearizationReport: JSONObject?,
        effectiveBlackNormalizationReport: JSONObject?,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }
        val source = sourceSha256.lowercase()

        val linearizationBinding =
            componentBinding(
                source = source,
                report = linearizationReport,
                expectedSchema = SourceCapabilityRadiometricLinearizationTableValidatorV01.SCHEMA,
                expectedStatus = SourceCapabilityRadiometricLinearizationTableValidatorV01.STATUS,
            )
        val normalizationBinding =
            componentBinding(
                source = source,
                report = effectiveBlackNormalizationReport,
                expectedSchema = SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA,
                expectedStatus = SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS,
            )

        val linearizationIndex = componentIndex(linearizationReport, linearizationBinding)
        val normalizationIndex = componentIndex(effectiveBlackNormalizationReport, normalizationBinding)
        val identities =
            (linearizationIndex.byIdentity.keys + normalizationIndex.byIdentity.keys)
                .toSortedSet()

        val ifdResults = JSONArray()
        var contradictedCount = 0
        var unresolvedCount = 0
        var prefixValidatedCount = 0

        for (identity in identities) {
            val linearization = linearizationIndex.byIdentity[identity]
            val normalization = normalizationIndex.byIdentity[identity]
            val path = linearization?.optString("ifd_path") ?: normalization?.optString("ifd_path") ?: "UNKNOWN_IFD"
            val offset = linearization?.optLong("ifd_offset", -1L) ?: normalization?.optLong("ifd_offset", -1L) ?: -1L

            val linearizationStatus =
                linearization?.optString("linearization_table_status")
                    ?: "MISSING_LINEARIZATION_COMPONENT_FOR_IFD"
            val normalizationStatus =
                normalization?.optString("normalization_domain_status")
                    ?: "MISSING_EFFECTIVE_BLACK_NORMALIZATION_COMPONENT_FOR_IFD"

            val prefixStatus =
                when {
                    linearizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                        normalizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED -> PREFIX_CONTRADICTED
                    identity in linearizationIndex.duplicateIdentities || identity in normalizationIndex.duplicateIdentities -> PREFIX_CONTRADICTED
                    linearizationStatus == SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED ||
                        normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.CONTRADICTED -> PREFIX_CONTRADICTED
                    linearizationStatus == SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED &&
                        normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED -> PREFIX_VALIDATED
                    else -> PREFIX_UNRESOLVED
                }

            val compositionStatus = if (prefixStatus == PREFIX_CONTRADICTED) CONTRADICTED else UNRESOLVED
            val reason =
                when (prefixStatus) {
                    PREFIX_CONTRADICTED -> "EXISTING_RAW_TO_LINEAR_PREFIX_DEPENDENCY_CONTRADICTED"
                    PREFIX_VALIDATED -> "CLIPPING_COMPONENT_NOT_SEPARATELY_VALIDATED"
                    else -> "EXISTING_RAW_TO_LINEAR_PREFIX_DEPENDENCY_UNRESOLVED"
                }

            if (prefixStatus == PREFIX_VALIDATED) prefixValidatedCount += 1
            if (compositionStatus == CONTRADICTED) contradictedCount += 1 else unresolvedCount += 1

            val stages =
                JSONArray()
                    .put(
                        JSONObject()
                            .put("stage", "LINEARIZATION")
                            .put("status", linearizationStatus)
                            .put("source", "SourceCapabilityRadiometricLinearizationTableValidatorV01"),
                    )
                    .put(
                        JSONObject()
                            .put("stage", "BLACK_SUBTRACTION")
                            .put("status", normalizationStatus)
                            .put("source", "SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01"),
                    )
                    .put(
                        JSONObject()
                            .put("stage", "RESCALING_NORMALIZATION")
                            .put("status", normalizationStatus)
                            .put("source", "SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01"),
                    )
                    .put(
                        JSONObject()
                            .put("stage", "CLIPPING")
                            .put("status", CLIPPING_UNRESOLVED)
                            .put("source", "NO_SEPARATE_SOURCE_CAPABILITY_CLIPPING_VALIDATOR_V01"),
                    )

            ifdResults.put(
                JSONObject()
                    .put("ifd_identity", identity)
                    .put("ifd_path", path)
                    .put("ifd_offset", offset)
                    .put("linearization_component_status", linearizationStatus)
                    .put("effective_black_normalization_component_status", normalizationStatus)
                    .put("linearization_black_normalization_prefix_status", prefixStatus)
                    .put("clipping_component_status", CLIPPING_UNRESOLVED)
                    .put("raw_to_linear_composition_status", compositionStatus)
                    .put("reason", reason)
                    .put("ordered_stages", stages)
                    .put("linearization_identity_default_applied", linearization?.optBoolean("identity_default_applied", false) ?: false)
                    .put(
                        "linearization_identity_default_mints_source_occurrence",
                        linearization?.optBoolean("identity_default_mints_source_occurrence", false) ?: false,
                    )
                    .put("metadata_ordering_prefix_validated", prefixStatus == PREFIX_VALIDATED)
                    .put("metadata_ordering_composition_contract_validated", false)
                    .put("full_raw_to_linear_mapping_validated", false)
                    .put("image_sample_payload_read", false)
                    .put("linearization_applied_to_image_samples", false)
                    .put("black_subtraction_applied_to_image_samples", false)
                    .put("normalization_applied_to_image_samples", false)
                    .put("clipping_applied_to_image_samples", false)
                    .put("record_lifecycle_effect", false)
                    .put("scientific_admission_effect", false)
                    .put("downstream_scientific_consumer_enabled", false)
                    .put("pixel_route_effect", false)
                    .put("scientific_master_pixel_effect", false)
                    .put("appearance_pixel_effect", false)
                    .put("source_values_modified", false)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false),
            )
        }

        val envelopeContradicted =
            linearizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                normalizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                linearizationIndex.duplicateIdentities.isNotEmpty() ||
                normalizationIndex.duplicateIdentities.isNotEmpty()
        val reportStatus = if (envelopeContradicted || contradictedCount > 0) CONTRADICTED else UNRESOLVED

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", source)
            .put("specification_anchor", "DNG_1_7_1_0_RAW_TO_LINEAR_ORDERING")
            .put(
                "ordered_stages",
                JSONArray()
                    .put("LINEARIZATION")
                    .put("BLACK_SUBTRACTION")
                    .put("RESCALING_NORMALIZATION")
                    .put("CLIPPING"),
            )
            .put("linearization_report_binding", linearizationBinding)
            .put("effective_black_normalization_report_binding", normalizationBinding)
            .put("clipping_component_validator_present", false)
            .put("clipping_component_status", CLIPPING_UNRESOLVED)
            .put("raw_to_linear_composition_status", reportStatus)
            .put("prefix_validated_ifd_count", prefixValidatedCount)
            .put("contradicted_ifd_count", contradictedCount)
            .put("unresolved_ifd_count", unresolvedCount)
            .put("ifds", ifdResults)
            .put("linearization_duplicate_ifd_identity_count", linearizationIndex.duplicateIdentities.size)
            .put("normalization_duplicate_ifd_identity_count", normalizationIndex.duplicateIdentities.size)
            .put("linearization_malformed_ifd_identity_count", linearizationIndex.malformedIdentityCount)
            .put("normalization_malformed_ifd_identity_count", normalizationIndex.malformedIdentityCount)
            .put(
                "all_existing_dependency_reports_bind_same_source",
                linearizationBinding.getString("binding_status") == COMPONENT_BOUND &&
                    normalizationBinding.getString("binding_status") == COMPONENT_BOUND,
            )
            .put("metadata_ordering_composition_contract_validated", false)
            .put("table_application_to_image_samples_validated", false)
            .put("full_raw_to_linear_mapping_validated", false)
            .put("image_sample_payload_read", false)
            .put("linearization_applied_to_image_samples", false)
            .put("black_subtraction_applied_to_image_samples", false)
            .put("normalization_applied_to_image_samples", false)
            .put("clipping_applied_to_image_samples", false)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("downstream_scientific_consumer_enabled", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
            .put("parser_tag_rename_performed", false)
            .put("observation_evidence_ledger_identity_changed", false)
    }

    private fun componentBinding(
        source: String,
        report: JSONObject?,
        expectedSchema: String,
        expectedStatus: String,
    ): JSONObject {
        if (report == null) {
            return JSONObject()
                .put("binding_status", COMPONENT_MISSING)
                .put("expected_schema", expectedSchema)
                .put("expected_status", expectedStatus)
        }
        val reportSource = report.optString("source_sha256", "").lowercase()
        if (reportSource != source) {
            return JSONObject()
                .put("binding_status", COMPONENT_SOURCE_CONTRADICTED)
                .put("expected_source_sha256", source)
                .put("observed_source_sha256", reportSource)
                .put("expected_schema", expectedSchema)
                .put("observed_schema", report.optString("schema", "MISSING"))
        }
        if (report.optString("schema", "") != expectedSchema || report.optString("status", "") != expectedStatus) {
            return JSONObject()
                .put("binding_status", COMPONENT_SCHEMA_UNRESOLVED)
                .put("expected_schema", expectedSchema)
                .put("observed_schema", report.optString("schema", "MISSING"))
                .put("expected_status", expectedStatus)
                .put("observed_status", report.optString("status", "MISSING"))
        }
        return JSONObject()
            .put("binding_status", COMPONENT_BOUND)
            .put("schema", expectedSchema)
            .put("status", expectedStatus)
            .put("source_sha256", source)
    }

    private fun componentIndex(
        report: JSONObject?,
        binding: JSONObject,
    ): ComponentIndex {
        if (report == null || binding.optString("binding_status") != COMPONENT_BOUND) {
            return ComponentIndex(emptyMap(), emptySet(), 0)
        }
        val array = report.optJSONArray("ifds") ?: return ComponentIndex(emptyMap(), emptySet(), 0)
        val out = linkedMapOf<String, JSONObject>()
        val duplicates = linkedSetOf<String>()
        var malformed = 0
        for (i in 0 until array.length()) {
            val item = array.optJSONObject(i)
            if (item == null) {
                malformed += 1
                continue
            }
            val path = item.optString("ifd_path", "")
            val offset = item.optLong("ifd_offset", -1L)
            if (path.isBlank() || offset < 0L) {
                malformed += 1
                continue
            }
            val identity = "$path@$offset"
            if (out.containsKey(identity)) {
                duplicates += identity
            } else {
                out[identity] = item
            }
        }
        return ComponentIndex(out, duplicates, malformed)
    }
}

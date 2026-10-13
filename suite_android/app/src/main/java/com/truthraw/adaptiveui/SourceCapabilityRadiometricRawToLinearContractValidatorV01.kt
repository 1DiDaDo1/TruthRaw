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
 * The metadata-order contract follows the DNG dependency order:
 * Linearization -> Black Subtraction -> Rescaling / Normalization -> Clipping.
 * A validated metadata ordering composition is not a validated sample-domain
 * raw-to-linear mapping; every sample-application/writeback authority remains
 * explicitly false.
 */
object SourceCapabilityRadiometricRawToLinearContractValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricRawToLinearContract/0.1"
    const val STATUS = "SHADOW_ONLY_RAW_TO_LINEAR_ORDERING_COMPOSITION_CONTRACT"

    const val VALIDATED = "VALIDATED_RAW_TO_LINEAR_METADATA_ORDERING_COMPOSITION_SHADOW_ONLY"
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

    const val IFD_IDENTITY_INTEGRITY_VALIDATED = "VALIDATED_COMPONENT_IFD_IDENTITY_INTEGRITY"
    const val IFD_IDENTITY_INTEGRITY_UNRESOLVED = "UNRESOLVED_MALFORMED_COMPONENT_IFD_IDENTITY_FAIL_CLOSED"
    const val IFD_IDENTITY_INTEGRITY_CONTRADICTED = "CONTRADICTED_DUPLICATE_COMPONENT_IFD_IDENTITY_FAIL_CLOSED"

    private data class ComponentIndex(
        val byIdentity: Map<String, JSONObject>,
        val duplicateIdentities: Set<String>,
        val malformedIdentityCount: Int,
    )

    fun describe(
        sourceSha256: String,
        linearizationReport: JSONObject?,
        effectiveBlackNormalizationReport: JSONObject?,
        clippingRuleReport: JSONObject? = null,
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
        val clippingBinding =
            componentBinding(
                source = source,
                report = clippingRuleReport,
                expectedSchema = SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.SCHEMA,
                expectedStatus = SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.STATUS,
            )

        val linearizationIndex = componentIndex(linearizationReport, linearizationBinding)
        val normalizationIndex = componentIndex(effectiveBlackNormalizationReport, normalizationBinding)
        val clippingIndex = componentIndex(clippingRuleReport, clippingBinding)
        val duplicateDependencyIdentityCount =
            linearizationIndex.duplicateIdentities.size +
                normalizationIndex.duplicateIdentities.size +
                clippingIndex.duplicateIdentities.size
        val malformedDependencyIdentityCount =
            linearizationIndex.malformedIdentityCount +
                normalizationIndex.malformedIdentityCount +
                clippingIndex.malformedIdentityCount
        val dependencyIfdIdentityIntegrityStatus =
            when {
                duplicateDependencyIdentityCount > 0 -> IFD_IDENTITY_INTEGRITY_CONTRADICTED
                malformedDependencyIdentityCount > 0 -> IFD_IDENTITY_INTEGRITY_UNRESOLVED
                else -> IFD_IDENTITY_INTEGRITY_VALIDATED
            }
        val identities =
            (linearizationIndex.byIdentity.keys + normalizationIndex.byIdentity.keys + clippingIndex.byIdentity.keys)
                .toSortedSet()

        val ifdResults = JSONArray()
        var contradictedCount = 0
        var unresolvedCount = 0
        var validatedCount = 0
        var prefixValidatedCount = 0

        for (identity in identities) {
            val linearization = linearizationIndex.byIdentity[identity]
            val normalization = normalizationIndex.byIdentity[identity]
            val clipping = clippingIndex.byIdentity[identity]
            val path =
                linearization?.optString("ifd_path")
                    ?: normalization?.optString("ifd_path")
                    ?: clipping?.optString("ifd_path")
                    ?: "UNKNOWN_IFD"
            val offset =
                linearization?.optLong("ifd_offset", -1L)
                    ?: normalization?.optLong("ifd_offset", -1L)
                    ?: clipping?.optLong("ifd_offset", -1L)
                    ?: -1L

            val linearizationStatus =
                linearization?.optString("linearization_table_status")
                    ?: "MISSING_LINEARIZATION_COMPONENT_FOR_IFD"
            val normalizationStatus =
                normalization?.optString("normalization_domain_status")
                    ?: "MISSING_EFFECTIVE_BLACK_NORMALIZATION_COMPONENT_FOR_IFD"
            val clippingStatus =
                clipping?.optString("clipping_rule_status")
                    ?: CLIPPING_UNRESOLVED

            val prefixStatus =
                when {
                    linearizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                        normalizationBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED -> PREFIX_CONTRADICTED
                    identity in linearizationIndex.duplicateIdentities ||
                        identity in normalizationIndex.duplicateIdentities -> PREFIX_CONTRADICTED
                    linearizationStatus == SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED ||
                        normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.CONTRADICTED -> PREFIX_CONTRADICTED
                    linearizationStatus == SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED &&
                        normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED -> PREFIX_VALIDATED
                    else -> PREFIX_UNRESOLVED
                }

            val compositionStatus =
                when {
                    prefixStatus == PREFIX_CONTRADICTED ||
                        clippingBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                        identity in clippingIndex.duplicateIdentities ||
                        clippingStatus == SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.CONTRADICTED -> CONTRADICTED
                    prefixStatus == PREFIX_VALIDATED &&
                        clippingStatus == SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.VALIDATED -> VALIDATED
                    else -> UNRESOLVED
                }
            val reason =
                when (compositionStatus) {
                    CONTRADICTED -> "RAW_TO_LINEAR_METADATA_DEPENDENCY_CONTRADICTED"
                    VALIDATED -> "ALL_FOUR_DNG_METADATA_ORDERING_DEPENDENCIES_VALIDATED_SHADOW_ONLY"
                    else ->
                        if (prefixStatus == PREFIX_VALIDATED) {
                            "CLIPPING_RULE_DEPENDENCY_UNRESOLVED"
                        } else {
                            "EXISTING_RAW_TO_LINEAR_PREFIX_DEPENDENCY_UNRESOLVED"
                        }
                }

            if (prefixStatus == PREFIX_VALIDATED) prefixValidatedCount += 1
            when (compositionStatus) {
                VALIDATED -> validatedCount += 1
                CONTRADICTED -> contradictedCount += 1
                else -> unresolvedCount += 1
            }

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
                            .put("status", clippingStatus)
                            .put("source", "SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01"),
                    )

            ifdResults.put(
                JSONObject()
                    .put("ifd_identity", identity)
                    .put("ifd_path", path)
                    .put("ifd_offset", offset)
                    .put("linearization_component_status", linearizationStatus)
                    .put("effective_black_normalization_component_status", normalizationStatus)
                    .put("linearization_black_normalization_prefix_status", prefixStatus)
                    .put("clipping_component_status", clippingStatus)
                    .put("raw_to_linear_composition_status", compositionStatus)
                    .put("reason", reason)
                    .put("ordered_stages", stages)
                    .put("linearization_identity_default_applied", linearization?.optBoolean("identity_default_applied", false) ?: false)
                    .put(
                        "linearization_identity_default_mints_source_occurrence",
                        linearization?.optBoolean("identity_default_mints_source_occurrence", false) ?: false,
                    )
                    .put("metadata_ordering_prefix_validated", prefixStatus == PREFIX_VALIDATED)
                    .put("metadata_ordering_composition_contract_validated", compositionStatus == VALIDATED)
                    .put("table_application_to_image_samples_validated", false)
                    .put("full_raw_to_linear_mapping_validated", false)
                    .put("image_sample_payload_read", false)
                    .put("linearization_applied_to_image_samples", false)
                    .put("black_subtraction_applied_to_image_samples", false)
                    .put("normalization_applied_to_image_samples", false)
                    .put("clipping_applied_to_image_samples", false)
                    .put("observed_sample_censoring_validated", false)
                    .put("physical_sensor_saturation_calibrated", false)
                    .put("scene_radiance_above_censor_recovered", false)
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
                clippingBinding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                duplicateDependencyIdentityCount > 0
        val allBindingsBound =
            linearizationBinding.getString("binding_status") == COMPONENT_BOUND &&
                normalizationBinding.getString("binding_status") == COMPONENT_BOUND &&
                clippingBinding.getString("binding_status") == COMPONENT_BOUND
        val reportStatus =
            when {
                envelopeContradicted || contradictedCount > 0 -> CONTRADICTED
                !allBindingsBound || malformedDependencyIdentityCount > 0 || unresolvedCount > 0 || validatedCount == 0 -> UNRESOLVED
                else -> VALIDATED
            }

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
            .put("clipping_rule_report_binding", clippingBinding)
            .put("dependency_ifd_identity_integrity_status", dependencyIfdIdentityIntegrityStatus)
            .put("all_existing_dependency_ifd_identities_well_formed", malformedDependencyIdentityCount == 0)
            .put("all_existing_dependency_ifd_identities_unique", duplicateDependencyIdentityCount == 0)
            .put("clipping_component_validator_present", clippingRuleReport != null)
            .put(
                "clipping_component_status",
                clippingRuleReport?.optString("clipping_rule_contract_status", CLIPPING_UNRESOLVED)
                    ?: CLIPPING_UNRESOLVED,
            )
            .put("raw_to_linear_composition_status", reportStatus)
            .put("prefix_validated_ifd_count", prefixValidatedCount)
            .put("validated_ifd_count", validatedCount)
            .put("contradicted_ifd_count", contradictedCount)
            .put("unresolved_ifd_count", unresolvedCount)
            .put("ifds", ifdResults)
            .put("linearization_duplicate_ifd_identity_count", linearizationIndex.duplicateIdentities.size)
            .put("normalization_duplicate_ifd_identity_count", normalizationIndex.duplicateIdentities.size)
            .put("clipping_duplicate_ifd_identity_count", clippingIndex.duplicateIdentities.size)
            .put("linearization_malformed_ifd_identity_count", linearizationIndex.malformedIdentityCount)
            .put("normalization_malformed_ifd_identity_count", normalizationIndex.malformedIdentityCount)
            .put("clipping_malformed_ifd_identity_count", clippingIndex.malformedIdentityCount)
            .put("all_existing_dependency_reports_bind_same_source", allBindingsBound)
            .put("metadata_ordering_composition_contract_validated", reportStatus == VALIDATED)
            .put("table_application_to_image_samples_validated", false)
            .put("full_raw_to_linear_mapping_validated", false)
            .put("image_sample_payload_read", false)
            .put("linearization_applied_to_image_samples", false)
            .put("black_subtraction_applied_to_image_samples", false)
            .put("normalization_applied_to_image_samples", false)
            .put("clipping_applied_to_image_samples", false)
            .put("observed_sample_censoring_validated", false)
            .put("physical_sensor_saturation_calibrated", false)
            .put("scene_radiance_above_censor_recovered", false)
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

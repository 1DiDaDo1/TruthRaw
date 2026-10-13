package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only DNG post-normalization clipping-rule dependency v0.1.
 *
 * This contract binds the already-existing Effective Black / Normalization report
 * to the DNG 1.7.1.0 processing rule that rescaled linear-reference values above
 * 1.0 are clipped to 1.0. It validates only that metadata processing-rule
 * dependency. It does not read RAW sample payload, detect which samples are
 * clipped, calibrate physical sensor full-well capacity, recover scene radiance
 * above a censor boundary, or apply clipping to any image sample.
 *
 * WhiteLevel is treated here only in its DNG processing role: the source-declared
 * (or specification-default) fully saturated encoding boundary used by the
 * existing normalization contract. That is not an independently calibrated
 * physical sensor-saturation/electron/radiance claim.
 */
object SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricRawToLinearClippingRule/0.1"
    const val STATUS = "SHADOW_ONLY_DNG_POST_NORMALIZATION_CLIPPING_RULE_CONTRACT"

    const val VALIDATED = "VALIDATED_DNG_POST_NORMALIZATION_CLIPPING_RULE_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_DNG_POST_NORMALIZATION_CLIPPING_RULE_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_DNG_POST_NORMALIZATION_CLIPPING_RULE_FAIL_CLOSED"

    const val COMPONENT_BOUND = "VALIDATED_EXISTING_NORMALIZATION_REPORT_BINDING"
    const val COMPONENT_MISSING = "UNRESOLVED_NORMALIZATION_REPORT_MISSING_FAIL_CLOSED"
    const val COMPONENT_SCHEMA_UNRESOLVED = "UNRESOLVED_NORMALIZATION_REPORT_SCHEMA_OR_STATUS_FAIL_CLOSED"
    const val COMPONENT_SOURCE_CONTRADICTED = "CONTRADICTED_NORMALIZATION_REPORT_SOURCE_BINDING_FAIL_CLOSED"

    const val IFD_IDENTITY_INTEGRITY_VALIDATED = "VALIDATED_NORMALIZATION_IFD_IDENTITY_INTEGRITY"
    const val IFD_IDENTITY_INTEGRITY_UNRESOLVED = "UNRESOLVED_MALFORMED_NORMALIZATION_IFD_IDENTITY_FAIL_CLOSED"
    const val IFD_IDENTITY_INTEGRITY_CONTRADICTED = "CONTRADICTED_DUPLICATE_NORMALIZATION_IFD_IDENTITY_FAIL_CLOSED"

    private data class ComponentIndex(
        val byIdentity: Map<String, JSONObject>,
        val duplicateIdentities: Set<String>,
        val malformedIdentityCount: Int,
    )

    fun describe(
        sourceSha256: String,
        effectiveBlackNormalizationReport: JSONObject?,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }
        val source = sourceSha256.lowercase()
        val binding = componentBinding(source, effectiveBlackNormalizationReport)
        val index = componentIndex(effectiveBlackNormalizationReport, binding)
        val identityIntegrityStatus =
            when {
                index.duplicateIdentities.isNotEmpty() -> IFD_IDENTITY_INTEGRITY_CONTRADICTED
                index.malformedIdentityCount > 0 -> IFD_IDENTITY_INTEGRITY_UNRESOLVED
                else -> IFD_IDENTITY_INTEGRITY_VALIDATED
            }

        val ifds = JSONArray()
        var validatedCount = 0
        var contradictedCount = 0
        var unresolvedCount = 0

        for ((identity, normalization) in index.byIdentity.toSortedMap()) {
            val normalizationStatus = normalization.optString("normalization_domain_status", "MISSING_NORMALIZATION_DOMAIN_STATUS")
            val clippingRuleStatus =
                when {
                    binding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED -> CONTRADICTED
                    identity in index.duplicateIdentities -> CONTRADICTED
                    normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.CONTRADICTED -> CONTRADICTED
                    normalizationStatus == SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED -> VALIDATED
                    else -> UNRESOLVED
                }
            when (clippingRuleStatus) {
                VALIDATED -> validatedCount += 1
                CONTRADICTED -> contradictedCount += 1
                else -> unresolvedCount += 1
            }

            ifds.put(
                JSONObject()
                    .put("ifd_identity", identity)
                    .put("ifd_path", normalization.optString("ifd_path", "UNKNOWN_IFD"))
                    .put("ifd_offset", normalization.optLong("ifd_offset", -1L))
                    .put("normalization_component_status", normalizationStatus)
                    .put("clipping_rule_status", clippingRuleStatus)
                    .put("normalized_upper_clip_threshold", 1.0)
                    .put("above_upper_threshold_action", "CLIP_TO_1_0_IN_DNG_LINEAR_REFERENCE_PROCESSING")
                    .put("negative_value_policy", "LOWER_CLIPPING_OPTIONAL_PRESERVE_NEGATIVE_VALUES_EARLY_RECOMMENDED")
                    .put("white_level_semantics", "DNG_FULLY_SATURATED_ENCODING_LEVEL_PROCESSING_BOUNDARY")
                    .put("white_level_occurrence_or_default_binding", "DELEGATED_TO_EXISTING_VALIDATED_NORMALIZATION_DEPENDENCY")
                    .put("source_occurrence_minted", false)
                    .put("observed_clipped_sample_pattern_validated", false)
                    .put("observed_clipped_sample_count_validated", false)
                    .put("physical_sensor_saturation_calibrated", false)
                    .put("physical_full_well_capacity_calibrated", false)
                    .put("absolute_scene_radiance_boundary_calibrated", false)
                    .put("scene_radiance_above_censor_recovered", false)
                    .put("truthrange_censor_authority_created", false)
                    .put("image_sample_payload_read", false)
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

        val aggregateStatus =
            when {
                binding.getString("binding_status") == COMPONENT_SOURCE_CONTRADICTED ||
                    index.duplicateIdentities.isNotEmpty() ||
                    contradictedCount > 0 -> CONTRADICTED
                binding.getString("binding_status") != COMPONENT_BOUND ||
                    index.malformedIdentityCount > 0 ||
                    unresolvedCount > 0 ||
                    validatedCount == 0 -> UNRESOLVED
                else -> VALIDATED
            }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", source)
            .put("specification_anchor", "DNG_1_7_1_0_CHAPTER_5_POST_NORMALIZATION_CLIPPING")
            .put("effective_black_normalization_report_binding", binding)
            .put("dependency_ifd_identity_integrity_status", identityIntegrityStatus)
            .put("all_dependency_ifd_identities_well_formed", index.malformedIdentityCount == 0)
            .put("all_dependency_ifd_identities_unique", index.duplicateIdentities.isEmpty())
            .put("clipping_rule_contract_status", aggregateStatus)
            .put("validated_ifd_count", validatedCount)
            .put("contradicted_ifd_count", contradictedCount)
            .put("unresolved_ifd_count", unresolvedCount)
            .put("malformed_ifd_identity_count", index.malformedIdentityCount)
            .put("duplicate_ifd_identity_count", index.duplicateIdentities.size)
            .put("ifds", ifds)
            .put("normalized_upper_clip_threshold", 1.0)
            .put("upper_clipping_rule", "NORMALIZED_LINEAR_REFERENCE_VALUE_GT_1_CLIP_TO_1")
            .put("negative_value_early_preservation_recommended", true)
            .put("lower_clipping_required_by_this_contract", false)
            .put("white_level_semantics", "DNG_FULLY_SATURATED_ENCODING_LEVEL_PROCESSING_BOUNDARY")
            .put("white_level_is_independent_physical_sensor_calibration", false)
            .put("white_level_is_absolute_scene_radiance_calibration", false)
            .put("source_occurrence_minted", false)
            .put("observed_sample_censoring_validated", false)
            .put("observed_clipped_sample_count_validated", false)
            .put("physical_sensor_saturation_calibrated", false)
            .put("scene_radiance_above_censor_recovered", false)
            .put("truthrange_censor_authority_created", false)
            .put("image_sample_payload_read", false)
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
    }

    private fun componentBinding(
        source: String,
        report: JSONObject?,
    ): JSONObject {
        if (report == null) {
            return JSONObject()
                .put("binding_status", COMPONENT_MISSING)
                .put("expected_schema", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA)
                .put("expected_status", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS)
        }
        val reportSource = report.optString("source_sha256", "").lowercase()
        if (reportSource != source) {
            return JSONObject()
                .put("binding_status", COMPONENT_SOURCE_CONTRADICTED)
                .put("expected_source_sha256", source)
                .put("observed_source_sha256", reportSource)
        }
        if (
            report.optString("schema", "") != SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA ||
                report.optString("status", "") != SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS
        ) {
            return JSONObject()
                .put("binding_status", COMPONENT_SCHEMA_UNRESOLVED)
                .put("expected_schema", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA)
                .put("observed_schema", report.optString("schema", "MISSING"))
                .put("expected_status", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS)
                .put("observed_status", report.optString("status", "MISSING"))
        }
        return JSONObject()
            .put("binding_status", COMPONENT_BOUND)
            .put("schema", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA)
            .put("status", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS)
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

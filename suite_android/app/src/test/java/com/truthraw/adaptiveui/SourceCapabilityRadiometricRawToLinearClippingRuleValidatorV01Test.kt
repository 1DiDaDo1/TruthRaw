package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01Test {
    private val sha = "c".repeat(64)

    @Test
    fun validatedNormalizationBindsOnlyTheDngProcessingRule() {
        val report = describe(normalization())
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.VALIDATED,
            report.getString("clipping_rule_contract_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.VALIDATED,
            ifd.getString("clipping_rule_status"),
        )
        assertEquals(1.0, report.getDouble("normalized_upper_clip_threshold"), 0.0)
        assertEquals(
            "NORMALIZED_LINEAR_REFERENCE_VALUE_GT_1_CLIP_TO_1",
            report.getString("upper_clipping_rule"),
        )
        assertTrue(report.getBoolean("negative_value_early_preservation_recommended"))
        assertFalse(report.getBoolean("lower_clipping_required_by_this_contract"))
        assertEquals(
            "DNG_FULLY_SATURATED_ENCODING_LEVEL_PROCESSING_BOUNDARY",
            report.getString("white_level_semantics"),
        )
        assertFalse(report.getBoolean("white_level_is_independent_physical_sensor_calibration"))
        assertFalse(report.getBoolean("white_level_is_absolute_scene_radiance_calibration"))
        assertFalse(report.getBoolean("observed_sample_censoring_validated"))
        assertFalse(report.getBoolean("observed_clipped_sample_count_validated"))
        assertFalse(report.getBoolean("physical_sensor_saturation_calibrated"))
        assertFalse(report.getBoolean("scene_radiance_above_censor_recovered"))
        assertFalse(report.getBoolean("truthrange_censor_authority_created"))
    }

    @Test
    fun unresolvedNormalizationKeepsClippingRuleUnresolved() {
        val report =
            describe(
                normalization(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.UNRESOLVED),
            )

        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.UNRESOLVED,
            report.getString("clipping_rule_contract_status"),
        )
    }

    @Test
    fun contradictedNormalizationContradictsRuleFailClosed() {
        val report =
            describe(
                normalization(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.CONTRADICTED),
            )

        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.CONTRADICTED,
            report.getString("clipping_rule_contract_status"),
        )
    }

    @Test
    fun sourceMismatchContradictsBinding() {
        val report = describe(normalization(sourceSha = "b".repeat(64)))

        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.COMPONENT_SOURCE_CONTRADICTED,
            report.getJSONObject("effective_black_normalization_report_binding").getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.CONTRADICTED,
            report.getString("clipping_rule_contract_status"),
        )
    }

    @Test
    fun malformedIdentityRemainsUnresolved() {
        val normalization = normalization()
        normalization
            .getJSONArray("ifds")
            .put(
                JSONObject()
                    .put("ifd_offset", 16L)
                    .put(
                        "normalization_domain_status",
                        SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED,
                    ),
            )

        val report = describe(normalization)

        assertEquals(1, report.getInt("malformed_ifd_identity_count"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.IFD_IDENTITY_INTEGRITY_UNRESOLVED,
            report.getString("dependency_ifd_identity_integrity_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.UNRESOLVED,
            report.getString("clipping_rule_contract_status"),
        )
    }

    @Test
    fun duplicateIdentityContradictsRatherThanSelectingSilently() {
        val normalization = normalization()
        normalization
            .getJSONArray("ifds")
            .put(JSONObject(normalization.getJSONArray("ifds").getJSONObject(0).toString()))

        val report = describe(normalization)

        assertEquals(1, report.getInt("duplicate_ifd_identity_count"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.IFD_IDENTITY_INTEGRITY_CONTRADICTED,
            report.getString("dependency_ifd_identity_integrity_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.CONTRADICTED,
            report.getString("clipping_rule_contract_status"),
        )
    }

    @Test
    fun ruleContractNeverCreatesSampleOrScientificAuthority() {
        val report = describe(normalization())

        listOf(
            "source_occurrence_minted",
            "observed_sample_censoring_validated",
            "observed_clipped_sample_count_validated",
            "physical_sensor_saturation_calibrated",
            "scene_radiance_above_censor_recovered",
            "truthrange_censor_authority_created",
            "image_sample_payload_read",
            "clipping_applied_to_image_samples",
            "record_lifecycle_effect",
            "scientific_admission_effect",
            "downstream_scientific_consumer_enabled",
            "pixel_route_effect",
            "scientific_master_pixel_effect",
            "appearance_pixel_effect",
            "source_values_modified",
            "creates_new_evidence",
            "scientific_writeback_allowed",
        ).forEach { key -> assertFalse("Expected $key=false", report.getBoolean(key)) }
    }

    private fun describe(normalization: JSONObject?): JSONObject =
        SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.describe(
            sourceSha256 = sha,
            effectiveBlackNormalizationReport = normalization,
        )

    private fun normalization(
        status: String = SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED,
        sourceSha: String = sha,
    ): JSONObject =
        JSONObject()
            .put("schema", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA)
            .put("status", SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.STATUS)
            .put("source_sha256", sourceSha)
            .put(
                "ifds",
                JSONArray().put(
                    JSONObject()
                        .put("ifd_path", "IFD0")
                        .put("ifd_offset", 8L)
                        .put("normalization_domain_status", status),
                ),
            )
}

package com.truthraw.adaptiveui

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRawToLinearRuntimeExposureV01Test {
    private val sourceSha256 = "a".repeat(64)

    @Test
    fun existingRuntimeEnvelopeExposesSingleReadOnlyRawToLinearComposition() {
        val envelope =
            SourceCapabilityEnvelopeV02.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = JSONObject(),
            )

        val linearization = envelope.optJSONObject("radiometric_linearization_table_v0_1")
        val normalization = envelope.optJSONObject("effective_black_normalization_v0_1")
        val rawToLinear = envelope.optJSONObject("radiometric_raw_to_linear_contract_v0_1")

        assertNotNull(linearization)
        assertNotNull(normalization)
        assertNotNull(rawToLinear)

        requireNotNull(linearization)
        requireNotNull(normalization)
        requireNotNull(rawToLinear)

        assertEquals(
            SourceCapabilityRadiometricLinearizationTableValidatorV01.SCHEMA,
            linearization.getString("schema"),
        )
        assertEquals(
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.SCHEMA,
            normalization.getString("schema"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.SCHEMA,
            rawToLinear.getString("schema"),
        )
        assertEquals(sourceSha256, rawToLinear.getString("source_sha256"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_BOUND,
            rawToLinear.getJSONObject("linearization_report_binding").getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_BOUND,
            rawToLinear
                .getJSONObject("effective_black_normalization_report_binding")
                .getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_BOUND,
            rawToLinear.getJSONObject("clipping_rule_report_binding").getString("binding_status"),
        )
        assertTrue(rawToLinear.getBoolean("clipping_component_validator_present"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearClippingRuleValidatorV01.UNRESOLVED,
            rawToLinear.getString("clipping_component_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            rawToLinear.getString("raw_to_linear_composition_status"),
        )
        assertFalse(rawToLinear.getBoolean("metadata_ordering_composition_contract_validated"))
        assertFalse(rawToLinear.getBoolean("table_application_to_image_samples_validated"))
        assertFalse(rawToLinear.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(rawToLinear.getBoolean("image_sample_payload_read"))
        assertFalse(rawToLinear.getBoolean("linearization_applied_to_image_samples"))
        assertFalse(rawToLinear.getBoolean("black_subtraction_applied_to_image_samples"))
        assertFalse(rawToLinear.getBoolean("normalization_applied_to_image_samples"))
        assertFalse(rawToLinear.getBoolean("clipping_applied_to_image_samples"))
        assertFalse(rawToLinear.getBoolean("observed_sample_censoring_validated"))
        assertFalse(rawToLinear.getBoolean("physical_sensor_saturation_calibrated"))
        assertFalse(rawToLinear.getBoolean("scene_radiance_above_censor_recovered"))
        assertFalse(rawToLinear.getBoolean("record_lifecycle_effect"))
        assertFalse(rawToLinear.getBoolean("scientific_admission_effect"))
        assertFalse(rawToLinear.getBoolean("downstream_scientific_consumer_enabled"))
        assertFalse(rawToLinear.getBoolean("pixel_route_effect"))
        assertFalse(rawToLinear.getBoolean("scientific_master_pixel_effect"))
        assertFalse(rawToLinear.getBoolean("appearance_pixel_effect"))
        assertFalse(rawToLinear.getBoolean("source_values_modified"))
        assertFalse(rawToLinear.getBoolean("creates_new_evidence"))
        assertFalse(rawToLinear.getBoolean("scientific_writeback_allowed"))
        assertFalse(rawToLinear.getBoolean("parser_tag_rename_performed"))
        assertFalse(rawToLinear.getBoolean("observation_evidence_ledger_identity_changed"))

        assertFalse(envelope.getBoolean("downstream_consumers_enabled"))
        assertFalse(envelope.getBoolean("pixel_route_effect"))
        assertFalse(envelope.getBoolean("scientific_master_pixel_effect"))
        assertFalse(envelope.getBoolean("appearance_pixel_effect"))
        assertFalse(envelope.getBoolean("source_values_modified"))
        assertFalse(envelope.getBoolean("creates_new_evidence"))
        assertFalse(envelope.getBoolean("scientific_writeback_allowed"))
    }
}

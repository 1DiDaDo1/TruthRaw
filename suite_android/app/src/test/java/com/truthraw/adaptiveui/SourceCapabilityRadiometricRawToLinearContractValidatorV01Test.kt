package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricRawToLinearContractValidatorV01Test {
    private val sha = "d".repeat(64)

    @Test
    fun validatedExistingPrefixStillFailsClosedBecauseClippingIsNotValidated() {
        val report = compose(linearization(), normalization())
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            report.getString("raw_to_linear_composition_status"),
        )
        assertEquals(1, report.getInt("prefix_validated_ifd_count"))
        assertFalse(report.getBoolean("clipping_component_validator_present"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.CLIPPING_UNRESOLVED,
            report.getString("clipping_component_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.PREFIX_VALIDATED,
            ifd.getString("linearization_black_normalization_prefix_status"),
        )
        assertTrue(ifd.getBoolean("metadata_ordering_prefix_validated"))
        assertEquals("CLIPPING_COMPONENT_NOT_SEPARATELY_VALIDATED", ifd.getString("reason"))
        assertFalse(ifd.getBoolean("metadata_ordering_composition_contract_validated"))
        assertFalse(ifd.getBoolean("full_raw_to_linear_mapping_validated"))
    }

    @Test
    fun missingLinearizationReportRemainsUnresolvedWithoutInventingIdentity() {
        val report = compose(null, normalization())

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_MISSING,
            report.getJSONObject("linearization_report_binding").getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            report.getString("raw_to_linear_composition_status"),
        )
        assertFalse(report.getBoolean("metadata_ordering_composition_contract_validated"))
        assertFalse(report.getBoolean("full_raw_to_linear_mapping_validated"))
    }

    @Test
    fun missingNormalizationReportRemainsUnresolved() {
        val report = compose(linearization(), null)
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_MISSING,
            report.getJSONObject("effective_black_normalization_report_binding").getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.PREFIX_UNRESOLVED,
            ifd.getString("linearization_black_normalization_prefix_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            report.getString("raw_to_linear_composition_status"),
        )
    }

    @Test
    fun contradictedExistingComponentContradictsCompositionFailClosed() {
        val report =
            compose(
                linearization(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED),
                normalization(),
            )
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.PREFIX_CONTRADICTED,
            ifd.getString("linearization_black_normalization_prefix_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.CONTRADICTED,
            report.getString("raw_to_linear_composition_status"),
        )
        assertFalse(report.getBoolean("full_raw_to_linear_mapping_validated"))
    }

    @Test
    fun unresolvedExistingComponentRemainsUnresolved() {
        val report =
            compose(
                linearization(),
                normalization(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.UNRESOLVED),
            )
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.PREFIX_UNRESOLVED,
            ifd.getString("linearization_black_normalization_prefix_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            report.getString("raw_to_linear_composition_status"),
        )
    }

    @Test
    fun sourceShaMismatchContradictsBindingFailClosed() {
        val otherSha = "e".repeat(64)
        val report = compose(linearization(sourceSha = otherSha), normalization())

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_SOURCE_CONTRADICTED,
            report.getJSONObject("linearization_report_binding").getString("binding_status"),
        )
        assertFalse(report.getBoolean("all_existing_dependency_reports_bind_same_source"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.CONTRADICTED,
            report.getString("raw_to_linear_composition_status"),
        )
    }

    @Test
    fun identityDefaultRemainsAValueDependencyAndDoesNotMintOccurrence() {
        val report = compose(linearization(identityDefault = true), normalization())
        val ifd = report.getJSONArray("ifds").getJSONObject(0)

        assertTrue(ifd.getBoolean("linearization_identity_default_applied"))
        assertFalse(ifd.getBoolean("linearization_identity_default_mints_source_occurrence"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.PREFIX_VALIDATED,
            ifd.getString("linearization_black_normalization_prefix_status"),
        )
        assertFalse(report.getBoolean("creates_new_evidence"))
    }

    @Test
    fun duplicateIfdIdentityContradictsRatherThanSelectingSilently() {
        val linearization = linearization()
        val duplicated = JSONObject(linearization.toString())
        duplicated.getJSONArray("ifds").put(JSONObject(duplicated.getJSONArray("ifds").getJSONObject(0).toString()))

        val report = compose(duplicated, normalization())

        assertEquals(1, report.getInt("linearization_duplicate_ifd_identity_count"))
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.CONTRADICTED,
            report.getString("raw_to_linear_composition_status"),
        )
        assertFalse(report.getBoolean("full_raw_to_linear_mapping_validated"))
    }

    @Test
    fun schemaMismatchDoesNotBecomeValidatedComposition() {
        val bad = linearization().put("schema", "D.RAW/Unexpected/9.9")
        val report = compose(bad, normalization())

        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.COMPONENT_SCHEMA_UNRESOLVED,
            report.getJSONObject("linearization_report_binding").getString("binding_status"),
        )
        assertEquals(
            SourceCapabilityRadiometricRawToLinearContractValidatorV01.UNRESOLVED,
            report.getString("raw_to_linear_composition_status"),
        )
    }

    @Test
    fun allScientificPixelAndAuthorityFirewallsRemainClosed() {
        val report = compose(linearization(), normalization())

        listOf(
            "metadata_ordering_composition_contract_validated",
            "table_application_to_image_samples_validated",
            "full_raw_to_linear_mapping_validated",
            "image_sample_payload_read",
            "linearization_applied_to_image_samples",
            "black_subtraction_applied_to_image_samples",
            "normalization_applied_to_image_samples",
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
            "parser_tag_rename_performed",
            "observation_evidence_ledger_identity_changed",
        ).forEach { key -> assertFalse("Expected $key=false", report.getBoolean(key)) }

        assertTrue(report.getBoolean("all_existing_dependency_reports_bind_same_source"))
        val stages = report.getJSONArray("ordered_stages")
        assertEquals("LINEARIZATION", stages.getString(0))
        assertEquals("BLACK_SUBTRACTION", stages.getString(1))
        assertEquals("RESCALING_NORMALIZATION", stages.getString(2))
        assertEquals("CLIPPING", stages.getString(3))
    }

    private fun compose(
        linearization: JSONObject?,
        normalization: JSONObject?,
    ): JSONObject =
        SourceCapabilityRadiometricRawToLinearContractValidatorV01.describe(
            sourceSha256 = sha,
            linearizationReport = linearization,
            effectiveBlackNormalizationReport = normalization,
        )

    private fun linearization(
        status: String = SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED,
        sourceSha: String = sha,
        identityDefault: Boolean = false,
    ): JSONObject =
        JSONObject()
            .put("schema", SourceCapabilityRadiometricLinearizationTableValidatorV01.SCHEMA)
            .put("status", SourceCapabilityRadiometricLinearizationTableValidatorV01.STATUS)
            .put("source_sha256", sourceSha)
            .put(
                "ifds",
                JSONArray().put(
                    JSONObject()
                        .put("ifd_path", "IFD0")
                        .put("ifd_offset", 8L)
                        .put("linearization_table_status", status)
                        .put("identity_default_applied", identityDefault)
                        .put("identity_default_mints_source_occurrence", false),
                ),
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

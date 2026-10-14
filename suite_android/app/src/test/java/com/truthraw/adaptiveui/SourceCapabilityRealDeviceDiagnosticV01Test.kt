package com.truthraw.adaptiveui

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRealDeviceDiagnosticV01Test {
    private val sourceSha256 = "a".repeat(64)
    private val buildGitSha = "b".repeat(40)

    @Test
    fun deterministicDiagnosticPreservesExistingObservationIdentityAndZeroPixelChangeBoundary() {
        val envelope =
            SourceCapabilityEnvelopeV02.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = JSONObject(),
            ).put("source_byte_length", 123456L)

        val first = SourceCapabilityRealDeviceDiagnosticV01.describe(envelope, buildGitSha)
        val second = SourceCapabilityRealDeviceDiagnosticV01.describe(envelope, buildGitSha)
        val firstSerialized = SourceCapabilityRealDeviceDiagnosticV01.deterministicJson(envelope, buildGitSha)
        val secondSerialized = SourceCapabilityRealDeviceDiagnosticV01.deterministicJson(envelope, buildGitSha)

        assertEquals(SourceCapabilityRealDeviceDiagnosticV01.SCHEMA, first.getString("schema"))
        assertEquals(SourceCapabilityRealDeviceDiagnosticV01.STATUS, first.getString("status"))
        assertEquals(firstSerialized, secondSerialized)
        assertEquals(first.toString().length, second.toString().length)

        assertEquals(sourceSha256, first.getString("source_sha256"))
        assertEquals(123456L, first.getLong("source_byte_length"))
        assertEquals(envelope.getString("ledger_schema"), first.getString("ledger_schema"))
        assertEquals(envelope.getString("ledger_digest_sha256"), first.getString("ledger_digest_sha256"))
        assertEquals(
            envelope.getString("observation_evidence_digest_sha256"),
            first.getString("observation_evidence_digest_sha256"),
        )
        assertEquals(
            "VALIDATED_EXISTING_RUNTIME_ENVELOPE_SOURCE_BINDING",
            first.getString("source_binding_status"),
        )

        val rawToLinear = envelope.getJSONObject("radiometric_raw_to_linear_contract_v0_1")
        assertEquals(
            rawToLinear.getString("raw_to_linear_composition_status"),
            first.getString("raw_to_linear_composition_status"),
        )
        assertEquals(
            rawToLinear.getBoolean("metadata_ordering_composition_contract_validated"),
            first.getBoolean("metadata_ordering_composition_contract_validated"),
        )
        assertFalse(first.getBoolean("full_raw_to_linear_mapping_validated"))
        assertTrue(first.getBoolean("all_upstream_hard_false_safeguards_explicitly_false"))

        val safeguards = first.getJSONObject("upstream_hard_false_safeguards")
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
        ).forEach { key -> assertFalse("Expected upstream safeguard $key=false", safeguards.getBoolean(key)) }

        assertFalse(first.getBoolean("image_sample_payload_read"))
        assertFalse(first.getBoolean("record_lifecycle_effect"))
        assertFalse(first.getBoolean("scientific_admission_effect"))
        assertFalse(first.getBoolean("downstream_scientific_consumer_enabled"))
        assertFalse(first.getBoolean("pixel_route_effect"))
        assertFalse(first.getBoolean("scientific_master_pixel_effect"))
        assertFalse(first.getBoolean("appearance_pixel_effect"))
        assertFalse(first.getBoolean("source_values_modified"))
        assertFalse(first.getBoolean("creates_new_evidence"))
        assertFalse(first.getBoolean("scientific_writeback_allowed"))
        assertFalse(first.getBoolean("observation_evidence_ledger_identity_changed"))

        val build = first.getJSONObject("build_provenance")
        assertEquals(
            SourceCapabilityRealDeviceDiagnosticV01.BUILD_PROVENANCE_AUTHORITY,
            build.getString("authority"),
        )
        assertEquals(buildGitSha, build.getString("git_sha"))
        assertEquals("AVAILABLE", build.getString("availability"))
        assertFalse(build.getBoolean("scientific_authority"))
        assertFalse(build.getBoolean("scientific_admission_effect"))
    }

    @Test
    fun missingBuildShaStaysUnknownAndCannotBecomeScientificAuthority() {
        val envelope =
            SourceCapabilityEnvelopeV02.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = JSONObject(),
            )

        val diagnostic = SourceCapabilityRealDeviceDiagnosticV01.describe(envelope)
        val build = diagnostic.getJSONObject("build_provenance")

        assertEquals("UNKNOWN_NOT_SUPPLIED", build.getString("availability"))
        assertTrue(build.isNull("git_sha"))
        assertEquals(
            SourceCapabilityRealDeviceDiagnosticV01.BUILD_PROVENANCE_AUTHORITY,
            build.getString("authority"),
        )
        assertFalse(build.getBoolean("scientific_authority"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
        assertFalse(diagnostic.getBoolean("creates_new_evidence"))
    }

    @Test
    fun diagnosticFailsClosedWhenExistingContractSourceBindingIsContradicted() {
        val envelope =
            SourceCapabilityEnvelopeV02.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = JSONObject(),
            )
        envelope
            .getJSONObject("radiometric_raw_to_linear_contract_v0_1")
            .put("source_sha256", "c".repeat(64))

        val diagnostic = SourceCapabilityRealDeviceDiagnosticV01.describe(envelope)

        assertEquals(
            "CONTRADICTED_RAW_TO_LINEAR_SOURCE_BINDING_FAIL_CLOSED",
            diagnostic.getString("source_binding_status"),
        )
        assertFalse(diagnostic.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(diagnostic.getBoolean("scientific_admission_effect"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
    }
}

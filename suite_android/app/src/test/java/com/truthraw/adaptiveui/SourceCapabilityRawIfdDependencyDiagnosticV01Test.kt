package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class SourceCapabilityRawIfdDependencyDiagnosticV01Test {
    private val source = "a".repeat(64)

    @Test
    fun reportsDependenciesOnlyInsideUniqueValidatedRawIfd() {
        val profile =
            profile(
                offsets = listOf(128L),
                containerStructureComplete = true,
                records =
                    JSONArray()
                        .put(record(50712, 128L, "TAG_50712"))
                        .put(record(50716, 128L, "BlackLevelDeltaV"))
                        .put(record(50715, 512L, "BlackLevelDeltaH")),
            )

        val diagnostic =
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(profile)

        assertEquals(
            "PRESENT_SOURCE_BOUND_VALIDATED_IDENTITY_ONLY",
            diagnostic.getString("raw_ifd_authority_status"),
        )
        assertEquals("IFD0", diagnostic.getString("raw_ifd_path"))
        assertEquals(128L, diagnostic.getLong("raw_ifd_offset"))
        assertEquals(
            "PRESENT_IN_VALIDATED_RAW_IFD_LEDGER",
            dependency(diagnostic, 50712).getString("status"),
        )
        assertEquals(
            "ABSENT_FROM_COMPLETE_PARSED_VALIDATED_RAW_IFD_LEDGER",
            dependency(diagnostic, 50715).getString("status"),
        )
        assertEquals(
            "PRESENT_IN_VALIDATED_RAW_IFD_LEDGER",
            dependency(diagnostic, 50716).getString("status"),
        )
        assertFalse(diagnostic.getBoolean("native_decoder_open_observed"))
        assertFirewallsFalse(diagnostic)
        assertEquals(
            diagnostic.toString(),
            profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY).toString(),
        )
    }

    @Test
    fun doesNotTurnIncompleteTraversalIntoAbsence() {
        val diagnostic =
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile(
                    offsets = listOf(128L),
                    containerStructureComplete = false,
                    records = JSONArray(),
                ),
            )

        assertEquals(
            "UNKNOWN_CONTAINER_TRAVERSAL_INCOMPLETE",
            dependency(diagnostic, 50712).getString("status"),
        )
        assertFirewallsFalse(diagnostic)
    }

    @Test
    fun ambiguousRawIfdAuthorityStaysUnknownInsteadOfGuessing() {
        val diagnostic =
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile(
                    offsets = listOf(128L, 256L),
                    containerStructureComplete = true,
                    records =
                        JSONArray()
                            .put(record(50712, 128L, "TAG_50712")),
                ),
            )

        assertEquals(
            "UNKNOWN_OR_AMBIGUOUS_FAIL_CLOSED",
            diagnostic.getString("raw_ifd_authority_status"),
        )
        for (tag in listOf(50712, 50715, 50716)) {
            assertEquals(
                "UNKNOWN_NO_UNIQUE_VALIDATED_RAW_IFD",
                dependency(diagnostic, tag).getString("status"),
            )
        }
        assertFirewallsFalse(diagnostic)
    }

    private fun dependency(
        diagnostic: JSONObject,
        tag: Int,
    ): JSONObject {
        val dependencies = diagnostic.getJSONArray("dependencies")
        for (index in 0 until dependencies.length()) {
            val item = dependencies.getJSONObject(index)
            if (item.getInt("tag") == tag) return item
        }
        error("missing dependency tag=$tag")
    }

    private fun record(
        tag: Int,
        offset: Long,
        semanticKey: String,
    ): JSONObject =
        JSONObject()
            .put("tag", tag)
            .put("semantic_key", semanticKey)
            .put("ifd_offset", offset)
            .put("lifecycle", "PARSED")
            .put("admission", "UNSUPPORTED_SEMANTICS")
            .put("authority", "UNKNOWN_SEMANTICS")

    private fun assertFirewallsFalse(diagnostic: JSONObject) {
        listOf(
            "image_sample_payload_read",
            "linearization_applied_to_image_samples",
            "black_subtraction_applied_to_image_samples",
            "normalization_applied_to_image_samples",
            "clipping_applied_to_image_samples",
            "table_application_to_image_samples_validated",
            "full_raw_to_linear_mapping_validated",
            "record_lifecycle_effect",
            "scientific_admission_effect",
            "downstream_scientific_consumer_enabled",
            "scientific_master_pixel_effect",
            "appearance_pixel_effect",
            "source_values_modified",
            "creates_new_evidence",
            "scientific_writeback_allowed",
        ).forEach { key ->
            assertFalse("$key must remain false", diagnostic.getBoolean(key))
        }
    }

    private fun profile(
        offsets: List<Long>,
        containerStructureComplete: Boolean,
        records: JSONArray,
    ): JSONObject {
        val rawDomainIfds = JSONArray()
        val contractIfds = JSONArray()
        offsets.forEachIndexed { index, offset ->
            val path = if (index == 0) "IFD0" else "IFD$index"
            rawDomainIfds.put(
                JSONObject()
                    .put("ifd_path", path)
                    .put("ifd_offset", offset)
                    .put(
                        "domain_status",
                        SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN,
                    ),
            )
            contractIfds.put(
                JSONObject()
                    .put("ifd_identity", "$path@$offset")
                    .put("ifd_path", path)
                    .put("ifd_offset", offset)
                    .put(
                        "raw_to_linear_composition_status",
                        SourceCapabilityRadiometricRawToLinearContractValidatorV01.VALIDATED,
                    )
                    .put("metadata_ordering_composition_contract_validated", true)
                    .put("full_raw_to_linear_mapping_validated", false),
            )
        }

        val rawDomain =
            JSONObject()
                .put("schema", SourceCapabilityRawIfdDomainValidatorV01.SCHEMA)
                .put("status", SourceCapabilityRawIfdDomainValidatorV01.STATUS)
                .put("source_sha256", source)
                .put("ifds", rawDomainIfds)

        val contract =
            JSONObject()
                .put("schema", SourceCapabilityRadiometricRawToLinearContractValidatorV01.SCHEMA)
                .put("status", SourceCapabilityRadiometricRawToLinearContractValidatorV01.STATUS)
                .put("source_sha256", source)
                .put(
                    "dependency_ifd_identity_integrity_status",
                    SourceCapabilityRadiometricRawToLinearContractValidatorV01.IFD_IDENTITY_INTEGRITY_VALIDATED,
                )
                .put(
                    "raw_to_linear_composition_status",
                    SourceCapabilityRadiometricRawToLinearContractValidatorV01.VALIDATED,
                )
                .put("all_existing_dependency_reports_bind_same_source", true)
                .put("metadata_ordering_composition_contract_validated", true)
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
                .put("ifds", contractIfds)

        val envelope =
            JSONObject()
                .put("source_sha256", source)
                .put("container_structure_complete", containerStructureComplete)
                .put("records", records)
                .put("raw_ifd_domain_validation_v0_1", rawDomain)
                .put("radiometric_raw_to_linear_contract_v0_1", contract)

        return JSONObject()
            .put("source_sha256", source)
            .put("source_capability_envelope_v0_2", envelope)
    }
}

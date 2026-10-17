package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRawIfdRuntimeAuthorityV01Test {
    private val source = "a".repeat(64)

    @Test
    fun acceptsExactlyOneSourceBoundValidatedIfdWithoutPromotingSampleMapping() {
        val authority =
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.extract(
                profile(offsets = listOf(25373814L)),
            )

        assertNotNull(authority)
        requireNotNull(authority)
        assertEquals(source, authority.sourceSha256)
        assertEquals("IFD0", authority.ifdPath)
        assertEquals(25373814L, authority.ifdOffset)
    }

    @Test
    fun rejectsCrossSourceContract() {
        val profile = profile(offsets = listOf(25373814L))
        profile
            .getJSONObject("source_capability_envelope_v0_2")
            .getJSONObject("radiometric_raw_to_linear_contract_v0_1")
            .put("source_sha256", "b".repeat(64))

        assertNull(SourceCapabilityRawIfdRuntimeAuthorityBinderV01.extract(profile))
    }

    @Test
    fun rejectsMultipleValidatedRawIfdsInsteadOfGuessing() {
        assertNull(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.extract(
                profile(offsets = listOf(100L, 200L)),
            ),
        )
    }

    @Test
    fun rejectsAnyAccidentalSampleDomainPromotion() {
        val profile = profile(offsets = listOf(25373814L))
        profile
            .getJSONObject("source_capability_envelope_v0_2")
            .getJSONObject("radiometric_raw_to_linear_contract_v0_1")
            .put("full_raw_to_linear_mapping_validated", true)

        assertNull(SourceCapabilityRawIfdRuntimeAuthorityBinderV01.extract(profile))
    }

    @Test
    fun bindRegistersAuthorityBeforeReadingRuntimeObservation() {
        val profile = profile(offsets = listOf(25373814L))
        val port =
            RecordingPort(
                runtimeObservation = runtimeObservation(),
            )

        assertTrue(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.bindIfAvailable(
                profile = profile,
                port = port,
            ),
        )

        assertEquals(listOf("register", "read"), port.calls)
        val diagnostic =
            profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertTrue(diagnostic.getBoolean("runtime_open_observation_available"))
        assertEquals("SUCCESS", diagnostic.getString("source_open_result"))
        assertEquals(25373814L, diagnostic.getLong("validated_raw_ifd_offset"))
        assertEquals(25373814L, diagnostic.getLong("requested_raw_ifd_offset"))
        assertEquals(25373814L, diagnostic.getLong("decoder_source_audit_raw_ifd_offset"))
        assertTrue(diagnostic.getBoolean("raw_ifd_authority_match"))
        assertFalse(diagnostic.getBoolean("image_sample_payload_read"))
        assertFalse(diagnostic.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(diagnostic.getBoolean("scientific_master_pixel_effect"))
        assertFalse(diagnostic.getBoolean("creates_new_evidence"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun bindFailsClosedWithoutReadingObservationWhenRegistrationFails() {
        val profile = profile(offsets = listOf(25373814L))
        val port =
            RecordingPort(
                registerResult = false,
                runtimeObservation = runtimeObservation(),
            )

        assertFalse(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.bindIfAvailable(
                profile = profile,
                port = port,
            ),
        )

        assertEquals(listOf("register"), port.calls)
        val diagnostic =
            profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertFalse(diagnostic.getBoolean("runtime_open_observation_available"))
        assertEquals("NOT_OBSERVED", diagnostic.getString("source_open_result"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun refreshReadsPassiveObservationWithoutRegisteringAgain() {
        val profile = profile(offsets = listOf(25373814L))
        val port =
            RecordingPort(
                runtimeObservation = runtimeObservation(),
            )

        assertTrue(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.refreshRuntimeDiagnosticIfAvailable(
                profile = profile,
                port = port,
            ),
        )

        assertEquals(listOf("read"), port.calls)
        val diagnostic =
            profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertTrue(diagnostic.getBoolean("runtime_open_observation_available"))
        assertEquals("SUCCESS", diagnostic.getString("source_open_result"))
        assertFalse(diagnostic.getBoolean("source_values_modified"))
    }

    @Test
    fun postNativeOpenRefreshReplacesStaleNotObservedDiagnosticInSameProfile() {
        val profile = profile(offsets = listOf(25373814L))
        val port = RecordingPort()

        // A pre-open cache snapshot cannot claim to have seen the decoder.
        assertTrue(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.bindIfAvailable(
                profile = profile,
                port = port,
            ),
        )
        val before = profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertEquals("NOT_OBSERVED", before.getString("source_open_result"))
        assertFalse(before.getBoolean("runtime_open_observation_available"))

        // The same source-bound native registry now reports the completed open.
        port.runtimeObservation = runtimeObservation()
        assertTrue(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.refreshRuntimeDiagnosticIfAvailable(
                profile = profile,
                port = port,
            ),
        )
        val after = profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertEquals(listOf("register", "read", "read"), port.calls)
        assertEquals("SUCCESS", after.getString("source_open_result"))
        assertEquals(25373814L, after.getLong("decoder_source_audit_raw_ifd_offset"))
        assertTrue(after.getBoolean("raw_ifd_authority_match"))
        assertTrue(after.getBoolean("runtime_open_observation_available"))
        assertFalse(after.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(after.getBoolean("scientific_master_pixel_effect"))
        assertFalse(after.getBoolean("creates_new_evidence"))
        assertFalse(after.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun postNativeOpenRefreshFailsClosedOnDifferentValidatedIfdOffset() {
        val profile = profile(offsets = listOf(25373814L))
        val port = RecordingPort(
            runtimeObservation = runtimeObservation().copy(
                validatedRawIfdOffset = 128L,
                decoderSourceAuditRawIfdOffset = 128L,
            ),
        )
        assertFalse(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.refreshRuntimeDiagnosticIfAvailable(
                profile = profile,
                port = port,
            ),
        )
        val diagnostic = profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertEquals("NOT_OBSERVED", diagnostic.getString("source_open_result"))
        assertFalse(diagnostic.getBoolean("runtime_open_observation_available"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun refreshRejectsCrossSourceObservationInsteadOfCrossBinding() {
        val profile = profile(offsets = listOf(25373814L))
        val port =
            RecordingPort(
                runtimeObservation =
                    runtimeObservation(
                        sourceSha256 = "b".repeat(64),
                    ),
            )

        assertFalse(
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.refreshRuntimeDiagnosticIfAvailable(
                profile = profile,
                port = port,
            ),
        )

        assertEquals(listOf("read"), port.calls)
        val diagnostic =
            profile.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
        assertFalse(diagnostic.getBoolean("runtime_open_observation_available"))
        assertEquals("NOT_OBSERVED", diagnostic.getString("source_open_result"))
        assertTrue(diagnostic.isNull("decoder_source_audit_raw_ifd_offset"))
        assertFalse(diagnostic.getBoolean("scientific_writeback_allowed"))
    }

    private fun runtimeObservation(
        sourceSha256: String = source,
    ): RawIfdRuntimeObservationV01 =
        RawIfdRuntimeObservationV01(
            sourceSha256 = sourceSha256,
            validatedRawIfdOffset = 25373814L,
            requestedRawIfdOffset = 25373814L,
            decoderSourceAuditRawIfdOffset = 25373814L,
            authorityMatch = true,
            decoderOpenObserved = true,
            sourceOpenSucceeded = true,
            sourceOpenResultCode = 0,
        )

    private class RecordingPort(
        private val registerResult: Boolean = true,
        var runtimeObservation: RawIfdRuntimeObservationV01? = null,
    ) : RawIfdRuntimeAuthorityPortV01 {
        val calls = mutableListOf<String>()

        override fun registerValidatedRawIfdAuthority(
            sourceSha256: String,
            rawIfdOffset: Long,
        ): Boolean {
            calls += "register"
            return registerResult
        }

        override fun readRawIfdRuntimeObservation(
            sourceSha256: String,
        ): RawIfdRuntimeObservationV01? {
            calls += "read"
            return runtimeObservation
        }
    }

    private fun profile(offsets: List<Long>): JSONObject {
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
                .put("raw_ifd_domain_validation_v0_1", rawDomain)
                .put("radiometric_raw_to_linear_contract_v0_1", contract)

        return JSONObject()
            .put("source_sha256", source)
            .put("source_capability_envelope_v0_2", envelope)
    }
}

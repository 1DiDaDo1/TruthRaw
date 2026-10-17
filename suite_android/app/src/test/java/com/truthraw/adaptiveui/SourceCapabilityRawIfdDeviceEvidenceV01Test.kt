package com.truthraw.adaptiveui

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRawIfdDeviceEvidenceV01Test {
    private val source = "a".repeat(64)
    private val forbidden = listOf(
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
    )

    @Test
    fun exportsMatchedNativeObservationWithoutPromotingScientificAuthority() {
        val profile = profile("SUCCESS")
        val before = profile.toString()
        val result = SourceCapabilityRawIfdDeviceEvidenceV01.build(profile)
        requireNotNull(result)

        assertEquals(before, profile.toString())
        assertEquals(source, result.getString("source_sha256"))
        val diagnostic = result.getJSONObject("raw_ifd_dependency_diagnostic_v0_1")
        assertEquals("SUCCESS", diagnostic.getString("source_open_result"))
        assertEquals(128L, diagnostic.getLong("validated_raw_ifd_offset"))
        assertEquals(128L, diagnostic.getLong("decoder_source_audit_raw_ifd_offset"))
        assertTrue(diagnostic.getBoolean("raw_ifd_authority_match"))
        assertTrue(diagnostic.getBoolean("runtime_open_observation_available"))
        assertFalse(result.getBoolean("creates_new_evidence"))
        assertFalse(result.getBoolean("scientific_writeback_allowed"))
        forbidden.forEach { flag -> assertFalse(diagnostic.getBoolean(flag)) }
    }

    @Test
    fun preservesNotObservedFailClosedInsteadOfInventingRuntimeProof() {
        val result = SourceCapabilityRawIfdDeviceEvidenceV01.build(profile("NOT_OBSERVED"))
        requireNotNull(result)
        val diagnostic = result.getJSONObject("raw_ifd_dependency_diagnostic_v0_1")
        assertEquals("NOT_OBSERVED", diagnostic.getString("source_open_result"))
        assertFalse(diagnostic.getBoolean("runtime_open_observation_available"))
        assertFalse(result.getBoolean("export_is_sealed_source_evidence"))
    }

    @Test
    fun rejectsCrossSourceOrMissingAuthorityAndAnyForbiddenPromotion() {
        val crossSource = profile("SUCCESS")
        crossSource.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
            .put("source_sha256", "b".repeat(64))
        assertNull(SourceCapabilityRawIfdDeviceEvidenceV01.build(crossSource))

        val forbiddenPromotion = profile("SUCCESS")
        forbiddenPromotion.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
            .put("full_raw_to_linear_mapping_validated", true)
        assertNull(SourceCapabilityRawIfdDeviceEvidenceV01.build(forbiddenPromotion))

        val missingFlag = profile("SUCCESS")
        missingFlag.getJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
            .remove("creates_new_evidence")
        assertNull(SourceCapabilityRawIfdDeviceEvidenceV01.build(missingFlag))

        val invalidSource = profile("SUCCESS").put("source_sha256", "not-a-sha")
        assertNull(SourceCapabilityRawIfdDeviceEvidenceV01.build(invalidSource))
    }

    @Test
    fun lastAdapterOpenForAnotherSourceNeverResolvesNotObserved() {
        val otherSource = "b".repeat(64)
        val packet =
            "1|1|128|0|1|$otherSource|0|0|8|1|0|1|-1"
        val probe =
            RawIfdNativeRegistryProbeV01.parse(source, packet)
        requireNotNull(probe)
        assertTrue(probe.authorityRegisteredForRequestedSource)
        assertFalse(probe.exactSourceObservationPresent)
        assertEquals(otherSource, probe.lastAdapterOpenSourceSha256)

        val exported =
            SourceCapabilityRawIfdDeviceEvidenceV01.build(
                profile("NOT_OBSERVED"),
                nativeRegistryProbe = probe,
            )
        requireNotNull(exported)
        val exact =
            exported.getJSONObject("raw_ifd_dependency_diagnostic_v0_1")
        val processMemory = exported.getJSONObject("native_registry_probe_v0_1")
        assertEquals("NOT_OBSERVED", exact.getString("source_open_result"))
        assertFalse(exact.getBoolean("runtime_open_observation_available"))
        assertEquals(otherSource, processMemory.getString("last_adapter_open_source_sha256"))
        assertFalse(processMemory.getBoolean("last_adapter_open_source_matches_requested"))
        assertFalse(processMemory.getBoolean("last_adapter_is_source_evidence"))
        assertFalse(exported.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun nativeRegistryProbeSeparatesRegisteredSourceFromNoRecordedOpen() {
        val probe = RawIfdNativeRegistryProbeV01.parse(
            source, "1|1|128|0|0|-|0|0|0|0|-1|0|-1",
        )
        requireNotNull(probe)
        assertTrue(probe.authorityRegisteredForRequestedSource)
        assertEquals(128L, probe.registeredRawIfdOffset)
        assertFalse(probe.exactSourceObservationPresent)
        assertFalse(probe.lastAdapterOpenRecorded)
        val report = probe.toJson()
        assertTrue(report.isNull("last_adapter_open_source_sha256"))
        assertTrue(report.isNull("last_adapter_open_succeeded"))
        assertFalse(report.getBoolean("creates_new_evidence"))
    }

    @Test
    fun registryProbeRejectsMalformedOrCrossBoundPackets() {
        assertNull(RawIfdNativeRegistryProbeV01.parse(source, "1|1|128"))
        assertNull(RawIfdNativeRegistryProbeV01.parse(source, "1|1|128|0|1|-|0|0|0|0|-1|0|-1"))
        assertNull(RawIfdNativeRegistryProbeV01.parse(source, "1|0|128|0|0|-|0|0|0|0|-1|0|-1"))
        assertNull(RawIfdNativeRegistryProbeV01.parse("not-sha", "1|1|128|0|0|-|0|0|0|0|-1|0|-1"))

        val otherRequested = "b".repeat(64)
        val otherProbe = RawIfdNativeRegistryProbeV01.parse(
            otherRequested, "1|1|128|0|0|-|0|0|0|0|-1|0|-1",
        )
        requireNotNull(otherProbe)
        assertNull(
            SourceCapabilityRawIfdDeviceEvidenceV01.build(
                profile("NOT_OBSERVED"),
                nativeRegistryProbe = otherProbe,
            ),
        )
    }

    private fun profile(sourceOpenResult: String): JSONObject {
        val observed = sourceOpenResult == "SUCCESS"
        val diagnostic =
            JSONObject()
                .put("schema", SourceCapabilityRawIfdDependencyDiagnosticV01.SCHEMA)
                .put("source_sha256", source)
                .put("source_open_result", sourceOpenResult)
                .put("validated_raw_ifd_offset", 128L)
                .put(
                    "decoder_source_audit_raw_ifd_offset",
                    if (observed) 128L else JSONObject.NULL,
                )
                .put(
                    "raw_ifd_authority_match",
                    if (observed) true else JSONObject.NULL,
                )
                .put("runtime_open_observation_available", observed)
        forbidden.forEach { diagnostic.put(it, false) }
        return JSONObject()
            .put("source_sha256", source)
            .put(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY, diagnostic)
    }
}

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

class SourceCapabilityRuntimeEvidenceEnvelopeV01Test {
    private val sha = "c".repeat(64)

    @Test
    fun runtimeSchemaIsDistinctFromCanonicalPhysicalKnowledgeMapWithoutChangingLedgerAuthority() {
        val parsed =
            JSONObject()
                .put("parser", "truthraw.test.classic-tiff")
                .put("fileSizeBytes", 4096L)
                .put("containerTraversalStatus", "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY")
                .put("containerTraversalAnomalies", JSONArray())
                .put("ifds", JSONArray())

        val report = SourceCapabilityEnvelopeV02.describe(sha, parsed)

        assertEquals("D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1", report.getString("schema"))
        assertEquals("D.RAW/ObservationEvidenceLedger/0.1", report.getString("ledger_schema"))
        assertEquals(
            "D.RAW/SourceCapabilityEnvelope/0.2",
            report.getString("canonical_physical_knowledge_map_schema"),
        )
        assertEquals(
            "D.RAW/SourceCapabilityEnvelope/0.2",
            report.getString("legacy_runtime_schema_label"),
        )
        assertFalse(report.getBoolean("canonical_physical_knowledge_map_serialization_equivalent"))
        assertFalse(report.getBoolean("legacy_runtime_schema_label_is_authoritative"))
        assertFalse(report.getBoolean("schema_migration_changes_scientific_authority"))
        assertFalse(report.getBoolean("schema_migration_changes_pixel_route"))
        assertEquals(0, report.getInt("record_count"))
        assertEquals(0, report.getJSONArray("admitted_capabilities").length())
        assertEquals(0, report.getJSONArray("used_capabilities").length())
        assertEquals(SourceCapabilityEnvelopeV02.ledgerDigest(emptyList()), report.getString("ledger_digest_sha256"))
        assertFalse(report.getBoolean("downstream_consumers_enabled"))
        assertFalse(report.getBoolean("pixel_route_effect"))
        assertFalse(report.getBoolean("scientific_master_pixel_effect"))
        assertFalse(report.getBoolean("appearance_pixel_effect"))
        assertFalse(report.getBoolean("creates_new_evidence"))
        assertFalse(report.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun unavailableRuntimeEnvelopeUsesNewSchemaAndStaysFailClosed() {
        val report = SourceCapabilityEnvelopeV02.unavailable(sha, "TEST_PARSE_FAILURE")

        assertEquals("D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1", report.getString("schema"))
        assertEquals("D.RAW/SourceCapabilityEnvelope/0.2", report.getString("canonical_physical_knowledge_map_schema"))
        assertFalse(report.getBoolean("canonical_physical_knowledge_map_serialization_equivalent"))
        assertEquals("UNKNOWN_FAIL_CLOSED", report.getString("status"))
        assertEquals(0, report.getInt("record_count"))
        assertEquals(0, report.getJSONArray("admitted_capabilities").length())
        assertEquals(0, report.getJSONArray("used_capabilities").length())
        assertFalse(report.getBoolean("creates_new_evidence"))
        assertFalse(report.getBoolean("scientific_writeback_allowed"))
    }
}

#!/usr/bin/env python3
from pathlib import Path

SOURCE = Path('suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt')
TEST = Path('suite_android/app/src/test/java/com/truthraw/adaptiveui/SourceCapabilityRuntimeEvidenceEnvelopeV01Test.kt')

s = SOURCE.read_text()

old_header = """/**
 * Passive Source Capability Envelope v0.2 / Observation Evidence Ledger v0.1.
 *
 * This layer is intentionally read-only and shadow-only. It describes facts
 * that the existing DNG/TIFF parser already exposed. It does not parse image
 * samples, choose a scientific model, alter Scientific Master pixels, or feed
 * any current reconstruction/Appearance operator.
 *
 * Evidence lifecycle is explicit and monotonic:
 * PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED.
 *
 * v0.2 starts conservatively: parser facts are PARSED unless an independent
 * validator later upgrades them. Identity/private/unknown semantics remain
 * provenance-only or unsupported and can never become scientific USED merely
 * because a vendor/camera/lens name is recognizable.
 */"""
new_header = """/**
 * Runtime Source Capability Evidence Envelope v0.1 / Observation Evidence Ledger v0.1.
 *
 * The Kotlin object name is retained for source compatibility, but this runtime
 * serialization is deliberately distinct from the canonical historical
 * D.RAW/SourceCapabilityEnvelope/0.2 Physical Knowledge Map.
 *
 * This layer is intentionally read-only and shadow-only. It describes facts
 * that the existing DNG/TIFF parser already exposed. It does not parse image
 * samples, choose a scientific model, alter Scientific Master pixels, or feed
 * any current reconstruction/Appearance operator.
 *
 * Evidence lifecycle is explicit and monotonic:
 * PRESENT -> PARSED -> VALIDATED -> ADMITTED -> USED.
 *
 * Parser facts remain PARSED unless an independent validator upgrades them.
 * Identity/private/unknown semantics remain provenance-only or unsupported and
 * can never become scientific USED merely because a vendor/camera/lens name is
 * recognizable.
 */"""
if old_header not in s:
    raise SystemExit('expected SourceCapabilityEnvelopeV02 header not found exactly')
s = s.replace(old_header, new_header, 1)

old_constants = """object SourceCapabilityEnvelopeV02 {
    const val SCHEMA = \"D.RAW/SourceCapabilityEnvelope/0.2\"
    const val LEDGER_SCHEMA = \"D.RAW/ObservationEvidenceLedger/0.1\"
    const val STATUS = \"SHADOW_READ_ONLY_ZERO_PIXEL_CHANGE\"
"""
new_constants = """object SourceCapabilityEnvelopeV02 {
    const val SCHEMA = \"D.RAW/SourceCapabilityRuntimeEvidenceEnvelope/0.1\"
    const val CANONICAL_PHYSICAL_KNOWLEDGE_MAP_SCHEMA = \"D.RAW/SourceCapabilityEnvelope/0.2\"
    const val LEGACY_RUNTIME_SCHEMA_LABEL = \"D.RAW/SourceCapabilityEnvelope/0.2\"
    const val LEDGER_SCHEMA = \"D.RAW/ObservationEvidenceLedger/0.1\"
    const val STATUS = \"SHADOW_READ_ONLY_ZERO_PIXEL_CHANGE\"
"""
if old_constants not in s:
    raise SystemExit('expected SourceCapabilityEnvelopeV02 constants not found exactly')
s = s.replace(old_constants, new_constants, 1)

old_base = """        JSONObject()
            .put(\"schema\", SCHEMA)
            .put(\"source_sha256\", sourceSha256.lowercase())"""
new_base = """        JSONObject()
            .put(\"schema\", SCHEMA)
            .put(\"canonical_physical_knowledge_map_schema\", CANONICAL_PHYSICAL_KNOWLEDGE_MAP_SCHEMA)
            .put(\"canonical_physical_knowledge_map_serialization_equivalent\", false)
            .put(\"legacy_runtime_schema_label\", LEGACY_RUNTIME_SCHEMA_LABEL)
            .put(\"legacy_runtime_schema_label_is_authoritative\", false)
            .put(\"schema_migration_changes_scientific_authority\", false)
            .put(\"schema_migration_changes_pixel_route\", false)
            .put(\"source_sha256\", sourceSha256.lowercase())"""
if old_base not in s:
    raise SystemExit('expected SourceCapabilityEnvelopeV02 base() prefix not found exactly')
s = s.replace(old_base, new_base, 1)
SOURCE.write_text(s)

TEST.write_text(r'''package com.truthraw.adaptiveui

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
''')

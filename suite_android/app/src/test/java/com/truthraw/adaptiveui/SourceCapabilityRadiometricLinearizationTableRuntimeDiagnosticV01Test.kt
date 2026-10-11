package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricLinearizationTableRuntimeDiagnosticV01Test {
    private val sha = "d".repeat(64)

    @Test
    fun runtimeExposurePreservesExactPhysicalLedgerAndObservationEvidenceIdentity() {
        val parsed = parsedWithExplicitLinearizationTable()
        val parsedBefore = parsed.toString()

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val report = envelope.getJSONObject("radiometric_linearization_table_v0_1")

        assertEquals(
            "6740e6e04c18797e8b57604f30c1884c6e8428e9a469e53cb139c26abe59afb3",
            envelope.getString("ledger_digest_sha256"),
        )
        assertEquals(
            "2bdaa44fe3435929310b79fa38d6ce3b0aa0cc05ef68f9522b4554c6862b38e6",
            envelope.getString("observation_evidence_digest_sha256"),
        )
        assertEquals(parsedBefore, parsed.toString())
        assertEquals(5, envelope.getInt("record_count"))
        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())

        val tag50712 =
            (0 until envelope.getJSONArray("records").length())
                .map { envelope.getJSONArray("records").getJSONObject(it) }
                .single { it.getInt("tag") == 50712 }
        assertEquals("TAG_50712", tag50712.getString("semantic_key"))
        assertEquals("PARSED", tag50712.getString("lifecycle"))
        assertEquals("UNSUPPORTED_SEMANTICS", tag50712.getString("admission"))
        assertEquals("UNKNOWN_SEMANTICS", tag50712.getString("authority"))
        assertEquals(0, tag50712.getJSONArray("consumers").length())

        assertEquals(
            SourceCapabilityRadiometricLinearizationTableValidatorV01.SCHEMA,
            report.getString("schema"),
        )
        assertEquals(
            SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED,
            report.getJSONArray("ifds").getJSONObject(0).getString("linearization_table_status"),
        )
        assertFalse(report.getBoolean("parser_tag_rename_performed"))
        assertFalse(report.getBoolean("observation_evidence_ledger_identity_changed"))
        assertFalse(report.getBoolean("table_application_to_image_samples_validated"))
        assertFalse(report.getBoolean("linearization_applied_to_image_samples"))
        assertFalse(report.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(report.getBoolean("image_sample_payload_read"))
        assertFalse(report.getBoolean("record_lifecycle_effect"))
        assertFalse(report.getBoolean("scientific_admission_effect"))
        assertFalse(report.getBoolean("pixel_route_effect"))
        assertFalse(report.getBoolean("scientific_master_pixel_effect"))
        assertFalse(report.getBoolean("appearance_pixel_effect"))
        assertFalse(report.getBoolean("source_values_modified"))
        assertFalse(report.getBoolean("creates_new_evidence"))
        assertFalse(report.getBoolean("scientific_writeback_allowed"))

        assertFalse(envelope.getBoolean("downstream_consumers_enabled"))
        assertFalse(envelope.getBoolean("pixel_route_effect"))
        assertFalse(envelope.getBoolean("scientific_master_pixel_effect"))
        assertFalse(envelope.getBoolean("appearance_pixel_effect"))
        assertFalse(envelope.getBoolean("creates_new_evidence"))
        assertFalse(envelope.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun identityDefaultIsVisibleAsDiagnosticButMintsNoPhysicalTag50712Record() {
        val parsed = parsedWithoutLinearizationTable()
        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val report = envelope.getJSONObject("radiometric_linearization_table_v0_1")
        val result = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(
            SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED,
            result.getString("linearization_table_status"),
        )
        assertEquals("DNG_SPEC_IDENTITY_TABLE_DEFAULT", result.getString("reason"))
        assertTrue(result.getBoolean("identity_default_applied"))
        assertFalse(result.getBoolean("identity_default_mints_source_occurrence"))

        val tags =
            (0 until envelope.getJSONArray("records").length())
                .map { envelope.getJSONArray("records").getJSONObject(it).getInt("tag") }
        assertFalse(50712 in tags)
        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())
    }

    private fun parsedWithExplicitLinearizationTable(): JSONObject =
        parsed(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
            entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
            entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
            entry(50712, "TAG_50712", 3, "SHORT", 4, JSONArray().put(0).put(100).put(500).put(4095), 58),
        )

    private fun parsedWithoutLinearizationTable(): JSONObject =
        parsed(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
            entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
            entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
        )

    private fun parsed(vararg entries: JSONObject): JSONObject {
        val array = JSONArray()
        entries.forEach(array::put)
        return JSONObject()
            .put("parser", "truthraw.test.classic-tiff")
            .put("fileSizeBytes", 4096L)
            .put("containerTraversalStatus", "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY")
            .put("containerTraversalAnomalies", JSONArray())
            .put(
                "ifds",
                JSONArray().put(
                    JSONObject()
                        .put("path", "IFD0")
                        .put("offset", 8L)
                        .put("entries", array),
                ),
            )
    }

    private fun entry(
        tag: Int,
        name: String,
        type: Int,
        typeName: String,
        count: Long,
        value: Any,
        entryOffset: Long,
    ): JSONObject =
        JSONObject()
            .put("tag", tag)
            .put("name", name)
            .put("type", type)
            .put("typeName", typeName)
            .put("count", count)
            .put("value", value)
            .put("entryOffset", entryOffset)
            .put("valueFieldOffset", entryOffset + 8L)
            .put("decodeStatus", "DECODED")
            .put("storageKind", "EXTERNAL")
            .put("locatorStatus", "LOCATED")
            .put("parseStatus", "PARSED_CLASSIC_TIFF_ENTRY")
}

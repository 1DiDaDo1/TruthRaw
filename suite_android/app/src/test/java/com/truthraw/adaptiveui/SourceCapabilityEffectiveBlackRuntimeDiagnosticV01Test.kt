package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityEffectiveBlackRuntimeDiagnosticV01Test {
    private val sha = "d".repeat(64)

    @Test
    fun runtimeEnvelopeMirrorsStandaloneEffectiveBlackDiagnosticWithoutMutatingEvidenceState() {
        val parsedForEnvelope =
            parsed(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 3, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 70),
                ),
            )
        val parsedBefore = parsedForEnvelope.toString()
        val parsedForStandalone = JSONObject(parsedBefore)

        val standalone =
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(
                sha,
                parsedForStandalone,
            )
        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsedForEnvelope)
        val attached = envelope.getJSONObject("effective_black_normalization_v0_1")

        assertEquals(parsedBefore, parsedForEnvelope.toString())
        assertEquals(standalone.toString(), attached.toString())
        assertEquals(
            "D.RAW/SourceCapabilityRadiometricEffectiveBlackNormalization/0.1",
            attached.getString("schema"),
        )
        assertEquals(sha, attached.getString("source_sha256"))

        val ifd = attached.getJSONArray("ifds").getJSONObject(0)
        val plane = ifd.getJSONArray("sample_planes").getJSONObject(0)
        assertEquals(
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED,
            ifd.getString("normalization_domain_status"),
        )
        assertEquals(4095.0, plane.getDouble("normalization_denominator"), 0.0)
        assertTrue(plane.getBoolean("denominator_strictly_positive"))

        val records = envelope.getJSONArray("records")
        assertEquals(envelope.getInt("record_count"), records.length())
        val physicalTags =
            (0 until records.length())
                .map { records.getJSONObject(it).optInt("tag", -1) }
                .toSet()
        assertFalse(50714 in physicalTags)
        assertFalse(50715 in physicalTags)
        assertFalse(50716 in physicalTags)
        assertFalse(50717 in physicalTags)

        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())
        assertFalse(envelope.getBoolean("downstream_consumers_enabled"))
        assertFalse(envelope.getBoolean("pixel_route_effect"))
        assertFalse(envelope.getBoolean("scientific_master_pixel_effect"))
        assertFalse(envelope.getBoolean("appearance_pixel_effect"))
        assertFalse(envelope.getBoolean("creates_new_evidence"))
        assertFalse(envelope.getBoolean("scientific_writeback_allowed"))

        assertFalse(attached.getBoolean("linearization_table_processing_validated"))
        assertFalse(attached.getBoolean("full_raw_to_linear_mapping_validated"))
        assertFalse(attached.getBoolean("image_sample_payload_read"))
        assertFalse(attached.getBoolean("record_lifecycle_effect"))
        assertFalse(attached.getBoolean("scientific_admission_effect"))
        assertFalse(attached.getBoolean("pixel_route_effect"))
        assertFalse(attached.getBoolean("scientific_master_pixel_effect"))
        assertFalse(attached.getBoolean("appearance_pixel_effect"))
        assertFalse(attached.getBoolean("source_values_modified"))
        assertFalse(attached.getBoolean("creates_new_evidence"))
        assertFalse(attached.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun unavailableEnvelopeDoesNotInventEffectiveBlackDiagnostic() {
        val envelope = SourceCapabilityEnvelopeV02.unavailable(sha, "TEST_PARSE_FAILURE")

        assertFalse(envelope.has("effective_black_normalization_v0_1"))
        assertEquals("UNKNOWN_FAIL_CLOSED", envelope.getString("status"))
        assertEquals(0, envelope.getInt("record_count"))
        assertEquals(0, envelope.getJSONArray("admitted_capabilities").length())
        assertEquals(0, envelope.getJSONArray("used_capabilities").length())
        assertFalse(envelope.getBoolean("creates_new_evidence"))
        assertFalse(envelope.getBoolean("scientific_writeback_allowed"))
    }

    private fun parsed(vararg ifds: JSONObject): JSONObject {
        val array = JSONArray()
        ifds.forEach(array::put)
        return JSONObject()
            .put("parser", "truthraw.test.classic-tiff")
            .put("fileSizeBytes", 4096L)
            .put("containerTraversalStatus", "COMPLETE_NO_REPORTED_TRAVERSAL_ANOMALY")
            .put("containerTraversalAnomalies", JSONArray())
            .put("ifds", array)
    }

    private fun cfaIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
            entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
            entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            *extra,
        )

    private fun ifd(vararg entries: JSONObject): JSONObject {
        val array = JSONArray()
        entries.forEach(array::put)
        return JSONObject()
            .put("path", "IFD0")
            .put("offset", 8L)
            .put("entries", array)
    }

    private fun dimensionEntry(tag: Int, name: String, value: Int, entryOffset: Long): JSONObject =
        entry(tag, name, 4, "LONG", 1, value, entryOffset)

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
            .put("storageKind", "INLINE")
            .put("locatorStatus", "LOCATED")
            .put("parseStatus", "PARSED_CLASSIC_TIFF_ENTRY")
}

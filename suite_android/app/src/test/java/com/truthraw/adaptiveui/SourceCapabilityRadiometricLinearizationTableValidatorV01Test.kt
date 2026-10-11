package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricLinearizationTableValidatorV01Test {
    private val sha = "c".repeat(64)

    @Test
    fun absentTagUsesIdentityDefaultWithoutMintingPhysicalOccurrence() {
        val parsed =
            parsed(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                ),
            )
        val report = SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(sha, parsed)
        val result = report.getJSONArray("ifds").getJSONObject(0)
        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED, result.getString("linearization_table_status"))
        assertEquals("DNG_SPEC_IDENTITY_TABLE_DEFAULT", result.getString("reason"))
        assertEquals("IDENTITY_TABLE_SPEC_DEFAULT", result.getString("effective_mapping"))
        assertEquals(0, result.getInt("occurrence_count"))
        assertTrue(result.getBoolean("identity_default_applied"))
        assertFalse(result.getBoolean("identity_default_mints_source_occurrence"))

        val records = envelope.getJSONArray("records")
        val tags = (0 until records.length()).map { records.getJSONObject(it).optInt("tag", -1) }
        assertFalse(50712 in tags)
        assertFalse(report.getBoolean("creates_new_evidence"))
    }

    @Test
    fun explicitShortTableMayBeShorterThanTwoPowBitsPerSample() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    linearizationEntry(
                        count = 4,
                        value = JSONArray().put(0).put(100).put(500).put(4095),
                        entryOffset = 58,
                    ),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED, result.getString("linearization_table_status"))
        assertEquals(4, result.getInt("table_value_count"))
        assertEquals(0L, result.getLong("table_first_value"))
        assertEquals(4095L, result.getLong("table_last_value"))
        assertFalse(result.getBoolean("table_count_equal_two_pow_bits_per_sample_required"))
        assertEquals("MAP_TO_LAST_TABLE_ENTRY_PER_DNG_PROCESSING_MODEL", result.getString("out_of_table_range_behavior"))
        assertFalse(result.getBoolean("explicit_table_applied_to_pixels"))
    }

    @Test
    fun duplicatePhysicalOccurrencesRemainUnresolvedWithoutSilentSelection() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    linearizationEntry(2, JSONArray().put(0).put(100), 58),
                    linearizationEntry(2, JSONArray().put(0).put(200), 70),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.UNRESOLVED, result.getString("linearization_table_status"))
        assertEquals("DUPLICATE_TAG_50712_NO_SILENT_SELECTION", result.getString("reason"))
        assertEquals(2, result.getInt("occurrence_count"))
    }

    @Test
    fun wrongTiffTypeContradictsFailClosed() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(50712, "TAG_50712", 4, "LONG", 2, JSONArray().put(0).put(4095), 58),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED, result.getString("linearization_table_status"))
        assertEquals("EXPLICIT_LINEARIZATION_TABLE_TYPE_MUST_BE_SHORT", result.getString("reason"))
    }

    @Test
    fun tableCountOutsideSdkRangeContradictsFailClosed() {
        val tooShort =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    linearizationEntry(1, 0, 58),
                ),
            )
        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED, tooShort.getString("linearization_table_status"))
        assertEquals("EXPLICIT_LINEARIZATION_TABLE_COUNT_OUTSIDE_2_TO_65536", tooShort.getString("reason"))

        val tooLong =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(50712, "TAG_50712", 3, "SHORT", 65537, JSONArray().put(0).put(1), 58),
                ),
            )
        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED, tooLong.getString("linearization_table_status"))
        assertEquals("EXPLICIT_LINEARIZATION_TABLE_COUNT_OUTSIDE_2_TO_65536", tooLong.getString("reason"))
    }

    @Test
    fun floatingPointRawWithExplicitTableContradictsFailClosed() {
        val result =
            result(
                linearRawIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 16, 34),
                    entry(339, "SampleFormat", 3, "SHORT", 1, 3, 46),
                    linearizationEntry(2, JSONArray().put(0).put(65535), 58),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED, result.getString("linearization_table_status"))
        assertEquals("LINEARIZATION_TABLE_NOT_ALLOWED_FOR_FLOATING_POINT_RAW", result.getString("reason"))
    }

    @Test
    fun unsignedRawAboveSixteenBitsWithExplicitTableContradictsFailClosed() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 32, 46),
                    linearizationEntry(2, JSONArray().put(0).put(65535), 58),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.CONTRADICTED, result.getString("linearization_table_status"))
        assertEquals("LINEARIZATION_TABLE_NOT_ALLOWED_ABOVE_16_BITS_PER_SAMPLE", result.getString("reason"))
    }

    @Test
    fun oneTableAppliesToAllSamplePlanesWithoutPerPlaneDuplication() {
        val result =
            result(
                linearRawIfd(
                    entry(277, "SamplesPerPixel", 3, "SHORT", 1, 3, 34),
                    entry(258, "BitsPerSample", 3, "SHORT", 3, JSONArray().put(12).put(12).put(12), 46),
                    entry(339, "SampleFormat", 3, "SHORT", 3, JSONArray().put(1).put(1).put(1), 58),
                    linearizationEntry(4, JSONArray().put(0).put(200).put(1000).put(4095), 70),
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.VALIDATED, result.getString("linearization_table_status"))
        assertEquals(1, result.getInt("occurrence_count"))
        assertEquals(4, result.getInt("table_value_count"))
    }

    @Test
    fun undecodedExplicitOccurrenceRemainsUnresolved() {
        val entry = linearizationEntry(2, JSONArray().put(0).put(4095), 58)
        entry.put("decodeStatus", "EXTERNAL_RANGE_OUT_OF_BOUNDS")
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry,
                ),
            )

        assertEquals(SourceCapabilityRadiometricLinearizationTableValidatorV01.UNRESOLVED, result.getString("linearization_table_status"))
        assertEquals("EXPLICIT_LINEARIZATION_TABLE_NOT_DECODED", result.getString("reason"))
    }

    @Test
    fun standaloneValidationDoesNotRenameLedgerRecordOrChangeEvidenceIdentity() {
        val parsed =
            parsed(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    linearizationEntry(4, JSONArray().put(0).put(100).put(500).put(4095), 58),
                ),
            )

        val before = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val beforeLedger = before.getString("ledger_digest_sha256")
        val beforeObservation = before.getString("observation_evidence_digest_sha256")

        val report = SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(sha, parsed)
        val after = SourceCapabilityEnvelopeV02.describe(sha, parsed)

        assertEquals(beforeLedger, after.getString("ledger_digest_sha256"))
        assertEquals(beforeObservation, after.getString("observation_evidence_digest_sha256"))
        assertFalse(report.getBoolean("observation_evidence_ledger_identity_changed"))
        assertFalse(report.getBoolean("parser_tag_rename_performed"))

        val record =
            (0 until after.getJSONArray("records").length())
                .map { after.getJSONArray("records").getJSONObject(it) }
                .single { it.getInt("tag") == 50712 }
        assertEquals("TAG_50712", record.getString("semantic_key"))
        assertEquals("PARSED", record.getString("lifecycle"))
        assertEquals("UNSUPPORTED_SEMANTICS", record.getString("admission"))
        assertEquals(0, record.getJSONArray("consumers").length())
    }

    @Test
    fun allScientificAndPixelFirewallsRemainClosed() {
        val report =
            SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(
                sha,
                parsed(
                    cfaIfd(
                        entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                        linearizationEntry(4, JSONArray().put(0).put(100).put(500).put(4095), 58),
                    ),
                ),
            )

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
        assertTrue(report.getBoolean("single_table_applies_to_all_sample_planes"))
        assertFalse(report.getBoolean("table_count_must_equal_two_pow_bits_per_sample"))
    }

    private fun result(ifd: JSONObject): JSONObject =
        SourceCapabilityRadiometricLinearizationTableValidatorV01.describe(sha, parsed(ifd))
            .getJSONArray("ifds")
            .getJSONObject(0)

    private fun cfaIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
            entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
            entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            *extra,
        )

    private fun linearRawIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 10),
            *extra,
        )

    private fun linearizationEntry(count: Long, value: Any, entryOffset: Long): JSONObject =
        entry(50712, "TAG_50712", 3, "SHORT", count, value, entryOffset)

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

    private fun ifd(vararg entries: JSONObject): JSONObject {
        val array = JSONArray()
        entries.forEach(array::put)
        return JSONObject()
            .put("path", "IFD0")
            .put("offset", 8L)
            .put("entries", array)
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

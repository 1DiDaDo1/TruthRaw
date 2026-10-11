package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricEncodingConsistencyValidatorV01Test {
    private val sha = "f".repeat(64)

    @Test
    fun unsignedTwelveBitRawIfdValidatesExplicitRadiometricBoundsShadowOnly() {
        val envelope =
            envelope(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(50714, "BlackLevel", 3, "SHORT", 1, 64, 58),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4095, 70),
                ),
            )
        val consistency = envelope.getJSONObject("radiometric_encoding_consistency_v0_1")
        assertEquals(1, consistency.getInt("validated_ifd_count"))
        assertEquals(0, consistency.getInt("contradicted_ifd_count"))
        assertFalse(consistency.getBoolean("defaults_mint_source_occurrence"))
        assertFalse(consistency.getBoolean("scientific_admission_effect"))
        assertFalse(consistency.getBoolean("image_sample_payload_read"))
        assertFalse(consistency.getBoolean("scientific_writeback_allowed"))

        val result = consistency.getJSONArray("ifds").getJSONObject(0)
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED, result.getString("consistency_status"))
        assertEquals(
            "RESOLVED_TIFF_DEFAULT_UNSIGNED_INTEGER_1",
            result.getJSONObject("sample_format_dependency").getString("status"),
        )
        assertTrue(result.getJSONObject("black_level").getBoolean("integer_encoding_max_applied"))
        assertTrue(result.getJSONObject("white_level").getBoolean("integer_encoding_max_applied"))
        assertFalse(result.getBoolean("black_white_ordering_validated"))

        val records = envelope.getJSONArray("records")
        assertFalse((0 until records.length()).map { records.getJSONObject(it) }.any { it.optInt("tag", -1) == 339 })
        for (i in 0 until records.length()) {
            val record = records.getJSONObject(i)
            if (record.optInt("tag", -1) == 50714 || record.optInt("tag", -1) == 50717) {
                assertEquals("PARSED", record.getString("lifecycle"))
                assertEquals("PRESENT_UNVALIDATED", record.getString("admission"))
            }
        }
    }

    @Test
    fun unsignedWhiteLevelAboveBitEncodingRangeIsContradictedFailClosed() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4096, 58),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, result.getString("consistency_status"))
        assertEquals(
            SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED,
            result.getJSONObject("white_level").getString("validation_status"),
        )
    }

    @Test
    fun floatingLinearRawUsesFloatWhiteDefaultWithoutIntegerMaxRule() {
        val result =
            result(
                linearRawIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 16, 22),
                    entry(339, "SampleFormat", 3, "SHORT", 1, 3, 34),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED, result.getString("consistency_status"))
        val white = result.getJSONObject("white_level")
        assertEquals("ABSENT_SPEC_DEFAULT_NO_SOURCE_OCCURRENCE", white.getString("selection_status"))
        assertEquals(1.0, white.getJSONArray("effective_defaults").getDouble(0), 0.0)
        assertFalse(white.getBoolean("integer_encoding_max_applied"))
        assertFalse(white.getBoolean("default_mints_source_occurrence"))
    }

    @Test
    fun floatingTwentyBitEncodingIsContradictedByDngFloatDepthContract() {
        val result =
            result(
                linearRawIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 20, 22),
                    entry(339, "SampleFormat", 3, "SHORT", 1, 3, 34),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, result.getString("consistency_status"))
        assertEquals(
            SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED,
            result.getString("encoding_contract_outcome"),
        )
    }

    @Test
    fun duplicateBitsPerSampleNeverSelectsWinner() {
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 14, 58),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4095, 70),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.UNRESOLVED, result.getString("consistency_status"))
        assertEquals(
            "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
            result.getJSONObject("bits_per_sample_dependency").getString("status"),
        )
    }

    @Test
    fun absentBitsPerSampleUsesTiffDefaultButContradictsDngEightToThirtyTwoRange() {
        val result = result(cfaIfd())
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, result.getString("consistency_status"))
        val bits = result.getJSONObject("bits_per_sample_dependency")
        assertEquals("RESOLVED_TIFF_DEFAULT_1_OUTSIDE_DNG_SUPPORTED_RANGE", bits.getString("status"))
        assertEquals(0, bits.getInt("occurrence_count"))
    }

    @Test
    fun multiSampleLinearRawRequiresSameDngBitDepthAndValidatesMatchingVectors() {
        val valid =
            result(
                linearRawIfd(
                    entry(277, "SamplesPerPixel", 3, "SHORT", 1, 3, 22),
                    entry(258, "BitsPerSample", 3, "SHORT", 3, JSONArray().put(16).put(16).put(16), 34),
                    entry(339, "SampleFormat", 3, "SHORT", 3, JSONArray().put(1).put(1).put(1), 46),
                    entry(50717, "WhiteLevel", 4, "LONG", 3, JSONArray().put(65535).put(65535).put(65535), 58),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED, valid.getString("consistency_status"))

        val invalid =
            result(
                linearRawIfd(
                    entry(277, "SamplesPerPixel", 3, "SHORT", 1, 3, 22),
                    entry(258, "BitsPerSample", 3, "SHORT", 3, JSONArray().put(12).put(14).put(12), 34),
                    entry(339, "SampleFormat", 3, "SHORT", 3, JSONArray().put(1).put(1).put(1), 46),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, invalid.getString("consistency_status"))
        assertEquals(
            "CONTRADICTED_DNG_MULTI_SAMPLE_BIT_DEPTH_MUST_MATCH",
            invalid.getJSONObject("bits_per_sample_dependency").getString("status"),
        )
    }

    @Test
    fun rationalBlackLevelOutsideUnsignedEncodingRangeIsContradicted() {
        val rational = JSONObject().put("numerator", 5000).put("denominator", 1).put("value", 5000.0)
        val result =
            result(
                cfaIfd(
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 46),
                    entry(50714, "BlackLevel", 5, "RATIONAL", 1, rational, 58),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4095, 70),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, result.getString("consistency_status"))
        assertEquals(
            SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED,
            result.getJSONObject("black_level").getString("validation_status"),
        )
    }

    @Test
    fun reducedLinearRawRadiometricMetadataRemainsContradictedWithoutAdmission() {
        val parsed =
            parsed(
                ifd(
                    entry(254, "NewSubfileType", 4, "LONG", 1, 1, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 16, 34),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 65535, 46),
                ),
            )
        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val result =
            envelope.getJSONObject("radiometric_encoding_consistency_v0_1")
                .getJSONArray("ifds").getJSONObject(0)
        assertEquals(SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED, result.getString("consistency_status"))
        assertFalse(result.getBoolean("domain_validation_is_scientific_admission"))

        val records = envelope.getJSONArray("records")
        val white = (0 until records.length()).map { records.getJSONObject(it) }.first { it.optInt("tag", -1) == 50717 }
        assertEquals("PARSED", white.getString("lifecycle"))
        assertEquals("PRESENT_UNVALIDATED", white.getString("admission"))
        assertFalse(white.getBoolean("scientific_writeback_allowed"))
    }

    private fun envelope(vararg ifds: JSONObject): JSONObject =
        SourceCapabilityEnvelopeV02.describe(sha, parsed(*ifds))

    private fun result(ifd: JSONObject): JSONObject =
        envelope(ifd)
            .getJSONObject("radiometric_encoding_consistency_v0_1")
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
            .put("storageKind", "INLINE")
            .put("locatorStatus", "LOCATED")
            .put("parseStatus", "PARSED_CLASSIC_TIFF_ENTRY")
}

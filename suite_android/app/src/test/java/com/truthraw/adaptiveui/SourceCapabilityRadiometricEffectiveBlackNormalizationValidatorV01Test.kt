package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01Test {
    private val sha = "b".repeat(64)

    @Test
    fun explicitPatternAndSpatialDeltasProduceExactPlaneExtremaAndPositiveDenominator() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 4, 46),
                    dimensionEntry(257, "ImageLength", 4, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 10, 70),
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 82),
                    entry(50714, "BlackLevel", 3, "SHORT", 4, JSONArray().put(64).put(65).put(66).put(67), 94),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 4, srationals(0, 1, 2, 3), 106),
                    entry(50716, "BlackLevelDeltaV", 10, "SRATIONAL", 4, srationals(0, -1, 4, 5), 118),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 1023, 130),
                ),
            )

        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED, result.getString("normalization_domain_status"))
        val plane = result.getJSONArray("sample_planes").getJSONObject(0)
        assertEquals(64.0, plane.getDouble("minimum_effective_black_level"), 0.0)
        assertEquals(75.0, plane.getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(948.0, plane.getDouble("normalization_denominator"), 0.0)
        assertEquals(1.0 / 948.0, plane.getDouble("normalization_scale_factor"), 0.0)
        assertTrue(plane.getBoolean("denominator_strictly_positive"))
        assertTrue(result.getBoolean("normalization_denominator_validated"))
        assertFalse(result.getBoolean("image_sample_payload_read"))
        assertFalse(result.getBoolean("normalization_applied_to_pixels"))
        assertFalse(result.getBoolean("domain_validation_is_scientific_admission"))
        assertFalse(result.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun specificationDefaultsValidateWithoutMintingOccurrences() {
        val parsed =
            parsed(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 3, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 70),
                ),
            )
        val report = SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(sha, parsed)
        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val result = report.getJSONArray("ifds").getJSONObject(0)
        val plane = result.getJSONArray("sample_planes").getJSONObject(0)

        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED, result.getString("normalization_domain_status"))
        assertEquals(0.0, plane.getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(4095.0, plane.getDouble("white_level"), 0.0)
        assertEquals(4095.0, plane.getDouble("normalization_denominator"), 0.0)
        assertTrue(result.getBoolean("black_level_delta_h_default_applied"))
        assertTrue(result.getBoolean("black_level_delta_v_default_applied"))

        val records = envelope.getJSONArray("records")
        val tags = (0 until records.length()).map { records.getJSONObject(it).optInt("tag", -1) }
        assertFalse(50714 in tags)
        assertFalse(50715 in tags)
        assertFalse(50716 in tags)
        assertFalse(50717 in tags)
        assertFalse(report.getBoolean("creates_new_evidence"))
    }

    @Test
    fun whiteLevelNotAboveMaximumEffectiveBlackContradictsFailClosed() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 8, 70),
                    entry(50714, "BlackLevel", 3, "SHORT", 1, 250, 82),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(5, 5), 94),
                    entry(50717, "WhiteLevel", 3, "SHORT", 1, 255, 106),
                ),
            )
        val plane = result.getJSONArray("sample_planes").getJSONObject(0)
        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.CONTRADICTED, result.getString("normalization_domain_status"))
        assertEquals(255.0, plane.getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(0.0, plane.getDouble("normalization_denominator"), 0.0)
        assertFalse(plane.getBoolean("denominator_strictly_positive"))
        assertFalse(result.getBoolean("normalization_denominator_validated"))
    }

    @Test
    fun multiSampleLinearRawValidatesEachPlaneIndependently() {
        val result =
            result(
                linearRawIfd(
                    dimensionEntry(256, "ImageWidth", 2, 22),
                    dimensionEntry(257, "ImageLength", 2, 34),
                    entry(277, "SamplesPerPixel", 3, "SHORT", 1, 3, 46),
                    entry(258, "BitsPerSample", 3, "SHORT", 3, JSONArray().put(16).put(16).put(16), 58),
                    entry(339, "SampleFormat", 3, "SHORT", 3, JSONArray().put(1).put(1).put(1), 70),
                    entry(50714, "BlackLevel", 3, "SHORT", 3, JSONArray().put(10).put(20).put(30), 82),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(0, 2), 94),
                    entry(50716, "BlackLevelDeltaV", 10, "SRATIONAL", 2, srationals(0, 3), 106),
                    entry(50717, "WhiteLevel", 4, "LONG", 3, JSONArray().put(100).put(110).put(120), 118),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED, result.getString("normalization_domain_status"))
        val planes = result.getJSONArray("sample_planes")
        assertEquals(15.0, planes.getJSONObject(0).getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(25.0, planes.getJSONObject(1).getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(35.0, planes.getJSONObject(2).getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(85.0, planes.getJSONObject(0).getDouble("normalization_denominator"), 0.0)
        assertEquals(85.0, planes.getJSONObject(1).getDouble("normalization_denominator"), 0.0)
        assertEquals(85.0, planes.getJSONObject(2).getDouble("normalization_denominator"), 0.0)
    }

    @Test
    fun repeatPatternCellsOutsideSmallActiveAreaDoNotPolluteMaximum() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 1, 46),
                    dimensionEntry(257, "ImageLength", 1, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 70),
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 82),
                    entry(50714, "BlackLevel", 3, "SHORT", 4, JSONArray().put(10).put(4000).put(4000).put(4000), 94),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 100, 106),
                ),
            )
        val plane = result.getJSONArray("sample_planes").getJSONObject(0)
        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.VALIDATED, result.getString("normalization_domain_status"))
        assertEquals(10.0, plane.getDouble("maximum_effective_black_level"), 0.0)
        assertEquals(90.0, plane.getDouble("normalization_denominator"), 0.0)
    }

    @Test
    fun ambiguousSpatialDependencyRemainsUnresolvedAndNeverNormalizesPixels() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 70),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(0, 0), 82),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(1, 1), 94),
                ),
            )
        assertEquals(SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.UNRESOLVED, result.getString("normalization_domain_status"))
        assertFalse(result.getBoolean("effective_black_values_computed"))
        assertFalse(result.getBoolean("normalization_denominator_validated"))
        assertFalse(result.getBoolean("normalization_applied_to_pixels"))
    }

    @Test
    fun topLevelFirewallsStayClosedAndLinearizationRemainsExplicitlyOutOfScope() {
        val report =
            SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(
                sha,
                parsed(
                    cfaIfd(
                        dimensionEntry(256, "ImageWidth", 2, 46),
                        dimensionEntry(257, "ImageLength", 2, 58),
                        entry(258, "BitsPerSample", 3, "SHORT", 1, 12, 70),
                    ),
                ),
            )
        assertFalse(report.getBoolean("linearization_table_processing_validated"))
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
    }

    private fun envelope(vararg ifds: JSONObject): JSONObject =
        SourceCapabilityEnvelopeV02.describe(sha, parsed(*ifds))

    private fun result(ifd: JSONObject): JSONObject =
        SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01.describe(sha, parsed(ifd))
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

    private fun dimensionEntry(tag: Int, name: String, value: Int, entryOffset: Long): JSONObject =
        entry(tag, name, 4, "LONG", 1, value, entryOffset)

    private fun srationals(vararg values: Int): JSONArray {
        val out = JSONArray()
        values.forEach { value ->
            out.put(
                JSONObject()
                    .put("numerator", value)
                    .put("denominator", 1)
                    .put("value", value.toDouble()),
            )
        }
        return out
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

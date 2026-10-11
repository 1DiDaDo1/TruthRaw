package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricSpatialBlackLevelValidatorV01Test {
    private val sha = "a".repeat(64)

    @Test
    fun explicitActiveAreaAndSrationalDeltasValidateShadowOnly() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 8, 46),
                    dimensionEntry(257, "ImageLength", 6, 58),
                    entry(
                        50829,
                        "ActiveArea",
                        4,
                        "LONG",
                        4,
                        JSONArray().put(1).put(2).put(4).put(6),
                        70,
                    ),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 4, srationals(0, 1, -1, 2), 82),
                    entry(50716, "BlackLevelDeltaV", 10, "SRATIONAL", 3, srationals(0, 1, -1), 94),
                ),
            )

        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.VALIDATED, result.getString("spatial_black_level_status"))
        assertEquals(4L, result.getJSONObject("active_area_dependency").getLong("active_area_width"))
        assertEquals(3L, result.getJSONObject("active_area_dependency").getLong("active_area_length"))
        assertEquals(4L, result.getJSONObject("black_level_delta_h").getLong("expected_count"))
        assertEquals(3L, result.getJSONObject("black_level_delta_v").getLong("expected_count"))
        assertTrue(result.getBoolean("effective_black_dependency_complete"))
        assertFalse(result.getBoolean("effective_black_values_computed"))
        assertFalse(result.getBoolean("normalization_denominator_validated"))
    }

    @Test
    fun absentActiveAreaAndDeltasUseDngDefaultsWithoutMintingOccurrences() {
        val parsed =
            parsed(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 8, 46),
                    dimensionEntry(257, "ImageLength", 6, 58),
                ),
            )
        val report = SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.describe(sha, parsed)
        val result = report.getJSONArray("ifds").getJSONObject(0)

        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.VALIDATED, result.getString("spatial_black_level_status"))
        val active = result.getJSONObject("active_area_dependency")
        assertEquals("RESOLVED_DNG_DEFAULT_FULL_IMAGE", active.getString("status"))
        assertEquals(8L, active.getLong("active_area_width"))
        assertEquals(6L, active.getLong("active_area_length"))
        assertEquals(0, active.getInt("occurrence_count"))
        assertTrue(active.getBoolean("default_applied"))

        val deltaH = result.getJSONObject("black_level_delta_h")
        val deltaV = result.getJSONObject("black_level_delta_v")
        assertEquals("RESOLVED_DNG_DEFAULT_ALL_ZERO", deltaH.getString("status"))
        assertEquals("RESOLVED_DNG_DEFAULT_ALL_ZERO", deltaV.getString("status"))
        assertEquals(0, deltaH.getInt("occurrence_count"))
        assertEquals(0, deltaV.getInt("occurrence_count"))
        assertFalse(report.getBoolean("defaults_mint_source_occurrence"))

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val records = envelope.getJSONArray("records")
        assertFalse(
            (0 until records.length())
                .map { records.getJSONObject(it) }
                .any { it.optInt("tag", -1) == 50715 || it.optInt("tag", -1) == 50716 },
        )
    }

    @Test
    fun wrongDeltaTypeIsContradictedFailClosed() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(50715, "BlackLevelDeltaH", 5, "RATIONAL", 2, rationals(0, 0), 70),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED, result.getString("spatial_black_level_status"))
        assertEquals(
            "CONTRADICTED_BLACK_LEVEL_DELTA_H_TYPE_CONTRACT",
            result.getJSONObject("black_level_delta_h").getString("status"),
        )
    }

    @Test
    fun deltaCountsMustMatchActiveAreaAxes() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 8, 46),
                    dimensionEntry(257, "ImageLength", 6, 58),
                    entry(
                        50829,
                        "ActiveArea",
                        4,
                        "LONG",
                        4,
                        JSONArray().put(1).put(2).put(4).put(6),
                        70,
                    ),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 3, srationals(0, 0, 0), 82),
                    entry(50716, "BlackLevelDeltaV", 10, "SRATIONAL", 4, srationals(0, 0, 0, 0), 94),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED, result.getString("spatial_black_level_status"))
        assertEquals(
            "CONTRADICTED_BLACK_LEVEL_DELTA_H_COUNT_CONTRACT",
            result.getJSONObject("black_level_delta_h").getString("status"),
        )
        assertEquals(
            "CONTRADICTED_BLACK_LEVEL_DELTA_V_COUNT_CONTRACT",
            result.getJSONObject("black_level_delta_v").getString("status"),
        )
    }

    @Test
    fun duplicateDeltaNeverSelectsWinner() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(0, 0), 70),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(1, 1), 82),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.UNRESOLVED, result.getString("spatial_black_level_status"))
        assertEquals(
            "AMBIGUOUS_MULTIPLE_BLACK_LEVEL_DELTA_H_OCCURRENCES_FAIL_CLOSED",
            result.getJSONObject("black_level_delta_h").getString("status"),
        )
    }

    @Test
    fun duplicateActiveAreaNeverSelectsWinner() {
        val active = JSONArray().put(0).put(0).put(2).put(2)
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(50829, "ActiveArea", 4, "LONG", 4, active, 70),
                    entry(50829, "ActiveArea", 4, "LONG", 4, JSONArray().put(0).put(0).put(2).put(2), 82),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.UNRESOLVED, result.getString("spatial_black_level_status"))
        assertEquals(
            "AMBIGUOUS_MULTIPLE_ACTIVE_AREA_OCCURRENCES_FAIL_CLOSED",
            result.getJSONObject("active_area_dependency").getString("status"),
        )
    }

    @Test
    fun activeAreaOutsideImageBoundsIsContradicted() {
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 8, 46),
                    dimensionEntry(257, "ImageLength", 6, 58),
                    entry(
                        50829,
                        "ActiveArea",
                        4,
                        "LONG",
                        4,
                        JSONArray().put(0).put(0).put(7).put(8),
                        70,
                    ),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED, result.getString("spatial_black_level_status"))
        assertEquals(
            "CONTRADICTED_ACTIVE_AREA_OUTSIDE_IMAGE_BOUNDS",
            result.getJSONObject("active_area_dependency").getString("status"),
        )
    }

    @Test
    fun reducedResolutionIfdWithSpatialBlackMetadataIsContradictedNonRaw() {
        val result =
            result(
                ifd(
                    entry(254, "NewSubfileType", 4, "LONG", 1, 1, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                    dimensionEntry(256, "ImageWidth", 2, 34),
                    dimensionEntry(257, "ImageLength", 2, 46),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, srationals(0, 0), 58),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED, result.getString("spatial_black_level_status"))
        assertFalse(result.getBoolean("domain_validation_is_scientific_admission"))
        assertFalse(result.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun missingImageDimensionsLeavesDefaultActiveAreaUnresolvedFailClosed() {
        val result = result(cfaIfd())
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.UNRESOLVED, result.getString("spatial_black_level_status"))
        assertEquals(
            "UNRESOLVED_ACTIVE_AREA_DEFAULT_DIMENSION_DEPENDENCY_FAIL_CLOSED",
            result.getJSONObject("active_area_dependency").getString("status"),
        )
        assertFalse(result.getBoolean("effective_black_dependency_complete"))
    }

    @Test
    fun decodedSrationalWithZeroDenominatorIsContradicted() {
        val invalid =
            JSONArray()
                .put(JSONObject().put("numerator", 1).put("denominator", 0).put("value", JSONObject.NULL))
                .put(JSONObject().put("numerator", 0).put("denominator", 1).put("value", 0.0))
        val result =
            result(
                cfaIfd(
                    dimensionEntry(256, "ImageWidth", 2, 46),
                    dimensionEntry(257, "ImageLength", 2, 58),
                    entry(50715, "BlackLevelDeltaH", 10, "SRATIONAL", 2, invalid, 70),
                ),
            )
        assertEquals(SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED, result.getString("spatial_black_level_status"))
        assertEquals(
            "CONTRADICTED_BLACK_LEVEL_DELTA_H_SRATIONAL_VALUE_CONTRACT",
            result.getJSONObject("black_level_delta_h").getString("status"),
        )
    }

    @Test
    fun topLevelFirewallsStayClosed() {
        val report =
            SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.describe(
                sha,
                parsed(
                    cfaIfd(
                        dimensionEntry(256, "ImageWidth", 2, 46),
                        dimensionEntry(257, "ImageLength", 2, 58),
                    ),
                ),
            )
        assertFalse(report.getBoolean("record_lifecycle_effect"))
        assertFalse(report.getBoolean("scientific_admission_effect"))
        assertFalse(report.getBoolean("image_sample_payload_read"))
        assertFalse(report.getBoolean("pixel_route_effect"))
        assertFalse(report.getBoolean("scientific_master_pixel_effect"))
        assertFalse(report.getBoolean("appearance_pixel_effect"))
        assertFalse(report.getBoolean("creates_new_evidence"))
        assertFalse(report.getBoolean("scientific_writeback_allowed"))
        assertFalse(report.getBoolean("duplicate_occurrence_selection_allowed"))
    }

    private fun result(ifd: JSONObject): JSONObject =
        SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.describe(sha, parsed(ifd))
            .getJSONArray("ifds")
            .getJSONObject(0)

    private fun cfaIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
            entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
            entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            *extra,
        )

    private fun dimensionEntry(tag: Int, name: String, value: Long, offset: Long): JSONObject =
        entry(tag, name, 4, "LONG", 1, value, offset)

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

    private fun srationals(vararg numerators: Int): JSONArray {
        val out = JSONArray()
        numerators.forEach { numerator ->
            out.put(
                JSONObject()
                    .put("numerator", numerator)
                    .put("denominator", 1)
                    .put("value", numerator.toDouble()),
            )
        }
        return out
    }

    private fun rationals(vararg numerators: Int): JSONArray = srationals(*numerators)
}

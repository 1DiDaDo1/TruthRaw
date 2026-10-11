package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRadiometricSemanticValidatorV01Test {
    private val sha = "d".repeat(64)

    @Test
    fun specDefaultsValidateCountsWithoutMintingSourceOccurrencesOrAdmission() {
        val parsed =
            parsed(
                rawCfaIfd(
                    entry(50714, "BlackLevel", 3, "SHORT", 1, 64, 10),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 1023, 22),
                ),
            )

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val semantic = envelope.getJSONObject("radiometric_semantic_validation_v0_1")
        assertEquals(
            "D.RAW/SourceCapabilityRadiometricSemanticValidation/0.1",
            semantic.getString("schema"),
        )
        assertEquals(2, semantic.getInt("validated_occurrence_count"))
        assertEquals(0, semantic.getInt("contradicted_occurrence_count"))
        assertEquals(0, semantic.getInt("dependency_unresolved_occurrence_count"))
        assertFalse(semantic.getBoolean("defaults_mint_source_occurrence"))
        assertFalse(semantic.getBoolean("scientific_admission_effect"))
        assertFalse(semantic.getBoolean("scientific_writeback_allowed"))

        val ifdResult = semantic.getJSONArray("ifds").getJSONObject(0)
        assertEquals("VALIDATED_SHADOW_ONLY", ifdResult.getString("family_status"))
        assertEquals(
            "VALIDATED_RAW_IFD_FORMAT_USAGE_SHADOW_ONLY",
            ifdResult.getString("raw_ifd_usage_status"),
        )
        assertEquals(
            "RESOLVED_SPEC_DEFAULT_1",
            ifdResult.getJSONObject("samples_per_pixel_dependency").getString("status"),
        )
        assertEquals(
            "RESOLVED_SPEC_DEFAULT_1_X_1",
            ifdResult.getJSONObject("black_level_repeat_dim_dependency").getString("status"),
        )
        assertEquals("ABSENT_NO_SOURCE_OCCURRENCE", ifdResult.getString("black_level_repeat_dim_selection"))

        val records = envelope.getJSONArray("records")
        for (i in 0 until records.length()) {
            val record = records.getJSONObject(i)
            if (record.getInt("tag") == 50714 || record.getInt("tag") == 50717) {
                assertEquals("PARSED", record.getString("lifecycle"))
                assertEquals("PRESENT_UNVALIDATED", record.getString("admission"))
                assertFalse(record.getBoolean("creates_new_evidence"))
                assertFalse(record.getBoolean("scientific_writeback_allowed"))
            }
        }
    }

    @Test
    fun explicitRepeatDimAndSamplesPerPixelValidateBlackAndWhiteCountContracts() {
        val repeat = JSONArray().put(2).put(2)
        val parsed =
            parsed(
                rawCfaIfd(
                    entry(277, "SamplesPerPixel", 3, "SHORT", 1, 1, 10),
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, repeat, 22),
                    entry(50714, "BlackLevel", 5, "RATIONAL", 4, JSONArray(), 34),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4095, 46),
                ),
            )

        val semantic =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("radiometric_semantic_validation_v0_1")
        assertEquals(3, semantic.getInt("validated_occurrence_count"))
        assertEquals(0, semantic.getInt("contradicted_occurrence_count"))
        assertEquals(0, semantic.getInt("dependency_unresolved_occurrence_count"))

        val ifdResult = semantic.getJSONArray("ifds").getJSONObject(0)
        assertEquals("VALIDATED_SHADOW_ONLY", ifdResult.getString("family_status"))
        assertEquals(
            "VALIDATED_RAW_IFD_FORMAT_USAGE_SHADOW_ONLY",
            ifdResult.getString("raw_ifd_usage_status"),
        )
        assertEquals(
            "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
            ifdResult.getJSONObject("samples_per_pixel_dependency").getString("status"),
        )
        assertEquals(
            "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
            ifdResult.getJSONObject("black_level_repeat_dim_dependency").getString("status"),
        )
        assertEquals(4L, occurrence(ifdResult, 50714).getLong("expected_count"))
        assertTrue(occurrence(ifdResult, 50714).getBoolean("count_contract_valid"))
        assertEquals(1L, occurrence(ifdResult, 50717).getLong("expected_count"))
    }

    @Test
    fun blackLevelCountMismatchIsContradictedFailClosed() {
        val parsed =
            parsed(
                rawCfaIfd(
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 10),
                    entry(50714, "BlackLevel", 3, "SHORT", 1, 64, 22),
                ),
            )

        val semantic =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("radiometric_semantic_validation_v0_1")
        assertEquals(1, semantic.getInt("contradicted_occurrence_count"))
        val ifdResult = semantic.getJSONArray("ifds").getJSONObject(0)
        assertEquals("CONTRADICTED_FAIL_CLOSED", ifdResult.getString("family_status"))
        val black = occurrence(ifdResult, 50714)
        assertEquals("CONTRADICTED_FORMAT_SEMANTICS_FAIL_CLOSED", black.getString("validation_status"))
        assertEquals(4L, black.getLong("expected_count"))
        assertFalse(black.getBoolean("count_contract_valid"))
        assertFalse(black.getBoolean("admission_effect"))
    }

    @Test
    fun whiteLevelRationalTypeIsContradictedByDngContract() {
        val parsed =
            parsed(
                rawCfaIfd(
                    entry(50717, "WhiteLevel", 5, "RATIONAL", 1, JSONObject().put("numerator", 1).put("denominator", 1), 10),
                ),
            )

        val semantic =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("radiometric_semantic_validation_v0_1")
        val white = occurrence(semantic.getJSONArray("ifds").getJSONObject(0), 50717)
        assertEquals("CONTRADICTED_FORMAT_SEMANTICS_FAIL_CLOSED", white.getString("validation_status"))
        assertFalse(white.getBoolean("type_contract_valid"))
        assertEquals(1, semantic.getInt("contradicted_occurrence_count"))
    }

    @Test
    fun duplicateRepeatDimNeverSelectsWinnerAndLeavesBlackCountUnresolved() {
        val parsed =
            parsed(
                rawCfaIfd(
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 10),
                    entry(50713, "BlackLevelRepeatDim", 3, "SHORT", 2, JSONArray().put(1).put(1), 22),
                    entry(50714, "BlackLevel", 3, "SHORT", 4, JSONArray().put(1).put(1).put(1).put(1), 34),
                ),
            )

        val semantic =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("radiometric_semantic_validation_v0_1")
        val ifdResult = semantic.getJSONArray("ifds").getJSONObject(0)
        assertEquals(
            "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
            ifdResult.getJSONObject("black_level_repeat_dim_dependency").getString("status"),
        )
        assertEquals(
            "MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED",
            ifdResult.getString("black_level_repeat_dim_selection"),
        )
        assertEquals("UNRESOLVED_FAIL_CLOSED", ifdResult.getString("family_status"))
        assertEquals(
            "DEPENDENCY_UNRESOLVED_FAIL_CLOSED",
            occurrence(ifdResult, 50714).getString("validation_status"),
        )
        assertEquals(1, semantic.getInt("dependency_unresolved_occurrence_count"))
        assertTrue(semantic.getInt("ambiguous_selection_count") >= 1)
        assertFalse(semantic.getBoolean("duplicate_occurrence_selection_allowed"))
    }

    private fun occurrence(ifdResult: JSONObject, tag: Int): JSONObject {
        val occurrences = ifdResult.getJSONArray("occurrences")
        for (i in 0 until occurrences.length()) {
            val value = occurrences.getJSONObject(i)
            if (value.getInt("tag") == tag) return value
        }
        error("Missing occurrence for tag $tag")
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

    private fun rawCfaIfd(vararg radiometricEntries: JSONObject): JSONObject {
        val array = JSONArray()
        array.put(entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 1000))
        array.put(entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 1012))
        array.put(entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 1024))
        radiometricEntries.forEach(array::put)
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

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityRawIfdDomainValidatorV01Test {
    private val sha = "e".repeat(64)

    @Test
    fun cfaRawDomainUsesTiffDefaultZeroWithoutMintingOccurrence() {
        val parsed =
            parsed(
                ifd(
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
                    entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
                    entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 34),
                ),
            )

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val domain = envelope.getJSONObject("raw_ifd_domain_validation_v0_1")
        assertEquals(1, domain.getInt("validated_raw_ifd_count"))
        assertEquals(0, domain.getInt("unresolved_ifd_count"))
        assertFalse(domain.getBoolean("defaults_mint_source_occurrence"))
        assertFalse(domain.getBoolean("scientific_admission_effect"))
        assertFalse(domain.getBoolean("scientific_writeback_allowed"))

        val result = domain.getJSONArray("ifds").getJSONObject(0)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals(
            "RESOLVED_TIFF_DEFAULT_0",
            result.getJSONObject("new_subfile_type_dependency").getString("status"),
        )
        assertEquals(0, result.getJSONObject("new_subfile_type_dependency").getInt("occurrence_count"))
        assertEquals(
            "PRESENT_UNIQUE_SAME_IFD_NOT_SEMANTICALLY_VALIDATED",
            result.getString("cfa_same_ifd_dependency_status"),
        )

        val records = envelope.getJSONArray("records")
        assertEquals(3, records.length())
        assertFalse((0 until records.length()).map { records.getJSONObject(it) }.any { it.optInt("tag", -1) == 254 })
    }

    @Test
    fun fullResolutionLinearRawDomainValidatesShadowOnly() {
        val parsed =
            parsed(
                ifd(
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 10),
                ),
            )

        val domain =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("raw_ifd_domain_validation_v0_1")
        val result = domain.getJSONArray("ifds").getJSONObject(0)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals("NOT_APPLICABLE", result.getString("cfa_same_ifd_dependency_status"))
        assertFalse(result.getBoolean("domain_validation_is_scientific_admission"))
    }

    @Test
    fun reducedLinearRawIsContradictedNotRawDomain() {
        val parsed =
            parsed(
                ifd(
                    entry(254, "NewSubfileType", 4, "LONG", 1, 1, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                ),
            )

        val result = result(parsed)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals("NEW_SUBFILE_TYPE_IS_NOT_FULL_RESOLUTION_RAW", result.getString("reason"))
    }

    @Test
    fun enhancedLinearRawIsContradictedNotRawDomain() {
        val parsed =
            parsed(
                ifd(
                    entry(254, "NewSubfileType", 4, "LONG", 1, 16, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                ),
            )

        val result = result(parsed)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals("NEW_SUBFILE_TYPE_IS_NOT_FULL_RESOLUTION_RAW", result.getString("reason"))
    }

    @Test
    fun missingPhotometricIsUnresolvedFailClosed() {
        val result = result(parsed(ifd()))
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.UNRESOLVED_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals(
            "MISSING_REQUIRED_DOMAIN_DEPENDENCY_FAIL_CLOSED",
            result.getJSONObject("photometric_interpretation_dependency").getString("status"),
        )
    }

    @Test
    fun duplicatePhotometricNeverSelectsWinner() {
        val parsed =
            parsed(
                ifd(
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                    entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 34),
                    entry(33422, "CFAPattern", 1, "BYTE", 4, JSONArray().put(0).put(1).put(1).put(2), 46),
                ),
            )

        val domain =
            SourceCapabilityEnvelopeV02.describe(sha, parsed)
                .getJSONObject("raw_ifd_domain_validation_v0_1")
        val result = domain.getJSONArray("ifds").getJSONObject(0)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.UNRESOLVED_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals(
            "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
            result.getJSONObject("photometric_interpretation_dependency").getString("status"),
        )
        assertFalse(domain.getBoolean("duplicate_occurrence_selection_allowed"))
    }

    @Test
    fun cfaWithoutRequiredSameIfdPatternIsContradicted() {
        val parsed =
            parsed(
                ifd(
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 32803, 10),
                    entry(33421, "CFARepeatPatternDim", 3, "SHORT", 2, JSONArray().put(2).put(2), 22),
                ),
            )

        val result = result(parsed)
        assertEquals(
            SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN,
            result.getString("domain_status"),
        )
        assertEquals("MISSING_REQUIRED_CFA_DEPENDENCY_FAIL_CLOSED", result.getString("cfa_same_ifd_dependency_status"))
    }

    @Test
    fun radiometricMetadataInReducedIfdFailsClosedWithoutChangingLedgerAdmission() {
        val parsed =
            parsed(
                ifd(
                    entry(254, "NewSubfileType", 4, "LONG", 1, 1, 10),
                    entry(262, "PhotometricInterpretation", 3, "SHORT", 1, 34892, 22),
                    entry(50714, "BlackLevel", 3, "SHORT", 1, 64, 34),
                    entry(50717, "WhiteLevel", 4, "LONG", 1, 4095, 46),
                ),
            )

        val envelope = SourceCapabilityEnvelopeV02.describe(sha, parsed)
        val radiometric = envelope.getJSONObject("radiometric_semantic_validation_v0_1")
        val result = radiometric.getJSONArray("ifds").getJSONObject(0)
        assertEquals("CONTRADICTED_FAIL_CLOSED", result.getString("family_status"))
        assertEquals(
            "CONTRADICTED_NON_RAW_IFD_USAGE_FAIL_CLOSED",
            result.getString("raw_ifd_usage_status"),
        )

        val records = envelope.getJSONArray("records")
        for (i in 0 until records.length()) {
            val record = records.getJSONObject(i)
            if (record.optInt("tag", -1) == 50714 || record.optInt("tag", -1) == 50717) {
                assertEquals("PARSED", record.getString("lifecycle"))
                assertEquals("PRESENT_UNVALIDATED", record.getString("admission"))
                assertFalse(record.getBoolean("creates_new_evidence"))
                assertFalse(record.getBoolean("scientific_writeback_allowed"))
            }
        }
    }

    private fun result(parsed: JSONObject): JSONObject =
        SourceCapabilityEnvelopeV02.describe(sha, parsed)
            .getJSONObject("raw_ifd_domain_validation_v0_1")
            .getJSONArray("ifds")
            .getJSONObject(0)

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

package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SourceCapabilityDngVersionCompatibilityValidatorV01Test {
    private val sha = "a".repeat(64)

    @Test
    fun unsignedRawOnDng13ValidatesScopedVersionAndUsesBackwardDefaultWithoutOccurrence() {
        val envelope = envelope(unsignedCfaIfd(version(1, 3, 0, 0)))
        val v = envelope.getJSONObject("dng_version_compatibility_v0_1")
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.VALIDATED, v.getString("compatibility_status"))
        assertEquals("1.3.0.0", v.getJSONObject("dng_version").getString("version_text"))
        assertEquals(
            "RESOLVED_SPEC_DEFAULT_FROM_DNG_VERSION_ZERO_TAIL",
            v.getJSONObject("dng_backward_version").getString("status"),
        )
        assertEquals(0, v.getJSONObject("dng_backward_version").getInt("occurrence_count"))
        assertFalse(v.getBoolean("defaults_mint_source_occurrence"))
        assertFalse(v.getBoolean("scientific_admission_effect"))
        assertFalse(v.getBoolean("image_sample_payload_read"))
        assertFalse(v.getBoolean("whole_file_dng_conformance_validated"))
        assertFalse(v.getBoolean("scientific_writeback_allowed"))
    }

    @Test
    fun floatingRawInIfd0AtDng14AndBackward14ValidatesFeatureFloor() {
        val v =
            versionResult(
                floatingLinearIfd(
                    version(1, 4, 0, 0),
                    backwardVersion(1, 4, 0, 0),
                ),
            )
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.VALIDATED, v.getString("compatibility_status"))
        val feature = v.getJSONArray("scoped_features").getJSONObject(0)
        assertEquals("FLOATING_POINT_RAW_IMAGE_DATA", feature.getString("feature"))
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.VALIDATED, feature.getString("declared_dng_version_floor_status"))
        assertTrue(feature.getBoolean("main_image_relation_proven"))
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.VALIDATED, feature.getString("backward_version_floor_status"))
    }

    @Test
    fun floatingRawWithDeclaredDng13IsContradicted() {
        val v = versionResult(floatingLinearIfd(version(1, 3, 0, 0)))
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, v.getString("compatibility_status"))
        val feature = v.getJSONArray("scoped_features").getJSONObject(0)
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, feature.getString("declared_dng_version_floor_status"))
    }

    @Test
    fun floatingMainIfdWithBackward13IsContradicted() {
        val v =
            versionResult(
                floatingLinearIfd(
                    version(1, 4, 0, 0),
                    backwardVersion(1, 3, 0, 0),
                ),
            )
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, v.getString("compatibility_status"))
        assertEquals(
            SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED,
            v.getJSONArray("scoped_features").getJSONObject(0).getString("backward_version_floor_status"),
        )
    }

    @Test
    fun floatingRawSubIfdDoesNotGuessMainImageRelationForBackwardFloor() {
        val ifd0 = ifd("IFD0", version(1, 4, 0, 0), backwardVersion(1, 3, 0, 0))
        val raw =
            ifd(
                "IFD0/SubIFD[0]",
                entry(262, 3, 1, 34892, 10),
                entry(258, 3, 1, 16, 22),
                entry(339, 3, 1, 3, 34),
            )
        val v = versionResult(ifd0, raw)
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.UNRESOLVED, v.getString("compatibility_status"))
        val feature = v.getJSONArray("scoped_features").getJSONObject(0)
        assertFalse(feature.getBoolean("main_image_relation_proven"))
        assertEquals("UNRESOLVED_MAIN_IMAGE_RELATION_NOT_PROVEN", feature.getString("backward_version_floor_status"))
        assertEquals("NOT_PROVEN_NO_FIRST_RAW_IFD_HEURISTIC", feature.getString("main_image_relation_basis"))
        assertFalse(v.getBoolean("main_image_ifd_inferred_by_first_raw_candidate"))
    }

    @Test
    fun missingRequiredDngVersionIsContradictedFailClosed() {
        val v = versionResult(unsignedCfaIfd())
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, v.getString("compatibility_status"))
        assertEquals("MISSING_REQUIRED_DNG_VERSION_FAIL_CLOSED", v.getJSONObject("dng_version").getString("status"))
    }

    @Test
    fun duplicateDngVersionNeverSelectsWinner() {
        val v = versionResult(unsignedCfaIfd(version(1, 3, 0, 0), version(1, 4, 0, 0)))
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.UNRESOLVED, v.getString("compatibility_status"))
        assertEquals(
            "AMBIGUOUS_MULTIPLE_DNG_VERSION_OCCURRENCES_FAIL_CLOSED",
            v.getJSONObject("dng_version").getString("status"),
        )
        assertFalse(v.getBoolean("duplicate_occurrence_selection_allowed"))
    }

    @Test
    fun backwardVersionHigherThanReaderSpecFailsClosedWithoutWholeFileConformanceClaim() {
        val v =
            versionResult(
                unsignedCfaIfd(
                    version(1, 8, 0, 0),
                    backwardVersion(1, 8, 0, 0),
                ),
            )
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, v.getString("compatibility_status"))
        assertEquals(SourceCapabilityDngVersionCompatibilityValidatorV01.CONTRADICTED, v.getString("reader_backward_compatibility_status"))
        assertEquals("1.7.1.0", v.getString("reader_spec_version"))
        assertFalse(v.getBoolean("whole_file_dng_conformance_validated"))
        assertFalse(v.getBoolean("future_unknown_feature_set_exhaustively_validated"))
    }

    @Test
    fun backwardDefaultZerosPatchAndBuildBytesWithoutMintingOccurrence() {
        val v = versionResult(unsignedCfaIfd(version(1, 4, 2, 3)))
        val backward = v.getJSONObject("dng_backward_version")
        assertEquals("1.4.0.0", backward.getString("version_text"))
        assertEquals(0, backward.getInt("occurrence_count"))
        assertEquals("DNG_SPEC_DEFAULT_NOT_SOURCE_OCCURRENCE", backward.getString("basis"))
    }

    private fun versionResult(vararg ifds: JSONObject): JSONObject =
        envelope(*ifds).getJSONObject("dng_version_compatibility_v0_1")

    private fun envelope(vararg ifds: JSONObject): JSONObject =
        SourceCapabilityEnvelopeV02.describe(sha, parsed(*ifds))

    private fun unsignedCfaIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            "IFD0",
            entry(262, 3, 1, 32803, 10),
            entry(33421, 3, 2, JSONArray().put(2).put(2), 22),
            entry(33422, 1, 4, JSONArray().put(0).put(1).put(1).put(2), 34),
            entry(258, 3, 1, 12, 46),
            *extra,
        )

    private fun floatingLinearIfd(vararg extra: JSONObject): JSONObject =
        ifd(
            "IFD0",
            entry(262, 3, 1, 34892, 10),
            entry(258, 3, 1, 16, 22),
            entry(339, 3, 1, 3, 34),
            *extra,
        )

    private fun version(a: Int, b: Int, c: Int, d: Int): JSONObject =
        entry(50706, 1, 4, JSONArray().put(a).put(b).put(c).put(d), 100 + a * 12 + b)

    private fun backwardVersion(a: Int, b: Int, c: Int, d: Int): JSONObject =
        entry(50707, 1, 4, JSONArray().put(a).put(b).put(c).put(d), 160 + a * 12 + b)

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

    private fun ifd(path: String, vararg entries: JSONObject): JSONObject {
        val array = JSONArray()
        entries.forEach(array::put)
        return JSONObject()
            .put("path", path)
            .put("offset", if (path == "IFD0") 8L else 512L)
            .put("entries", array)
    }

    private fun entry(tag: Int, type: Int, count: Long, value: Any, entryOffset: Int): JSONObject =
        JSONObject()
            .put("tag", tag)
            .put("type", type)
            .put("count", count)
            .put("value", value)
            .put("entryOffset", entryOffset.toLong())
            .put("valueFieldOffset", entryOffset.toLong() + 8L)
            .put("decodeStatus", "DECODED")
            .put("storageKind", "INLINE")
            .put("locatorStatus", "LOCATED")
            .put("parseStatus", "PARSED_CLASSIC_TIFF_ENTRY")
}

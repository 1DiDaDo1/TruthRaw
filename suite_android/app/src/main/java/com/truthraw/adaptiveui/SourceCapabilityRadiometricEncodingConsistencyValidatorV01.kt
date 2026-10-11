package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only DNG Raw-IFD radiometric encoding-consistency validator v0.1.
 *
 * This layer uses only metadata occurrences already decoded by the existing
 * classic-TIFF/DNG parser. It never reads image payload bytes, never changes
 * source samples, never admits scientific authority, and never writes to the
 * Scientific Master or Appearance.
 *
 * Scope:
 * - BitsPerSample / SamplesPerPixel encoding contract;
 * - SampleFormat (TIFF default unsigned integer, DNG floating-point allowance);
 * - numeric representability of explicit BlackLevel / WhiteLevel metadata;
 * - DNG defaults for absent BlackLevel and WhiteLevel without minting source evidence.
 *
 * Deliberately out of scope in v0.1:
 * - declared DNG-version compatibility;
 * - BlackLevelDeltaH / BlackLevelDeltaV and the effective normalization denominator;
 * - image sample payload validation.
 */
object SourceCapabilityRadiometricEncodingConsistencyValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricEncodingConsistency/0.1"
    const val STATUS = "SHADOW_ONLY_RADIOMETRIC_ENCODING_CONSISTENCY"

    const val VALIDATED = "VALIDATED_RADIOMETRIC_ENCODING_CONSISTENCY_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_RADIOMETRIC_ENCODING_CONSISTENCY_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_RADIOMETRIC_ENCODING_CONSISTENCY_FAIL_CLOSED"

    private const val TAG_BITS_PER_SAMPLE = 258
    private const val TAG_SAMPLES_PER_PIXEL = 277
    private const val TAG_SAMPLE_FORMAT = 339
    private const val TAG_BLACK_LEVEL = 50714
    private const val TAG_WHITE_LEVEL = 50717

    private const val TIFF_TYPE_SHORT = 3
    private const val SAMPLE_FORMAT_UNSIGNED = 1L
    private const val SAMPLE_FORMAT_FLOAT = 3L

    private data class VectorDependency(
        val status: String,
        val values: List<Long>?,
        val occurrenceCount: Int,
        val basis: String,
        val outcome: String,
    ) {
        fun toJson(): JSONObject =
            JSONObject()
                .put("status", status)
                .put("values", values?.let(::longArrayJson) ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
                .put("contract_outcome", outcome)
    }

    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }

        val results = JSONArray()
        var validated = 0
        var contradicted = 0
        var unresolved = 0
        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()

        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val entries = ifd.optJSONArray("entries") ?: JSONArray()
            val rawDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            val rawStatus = rawDomain.getString("domain_status")
            val hasRadiometricOccurrence =
                entriesByTags(entries, setOf(TAG_BLACK_LEVEL, TAG_WHITE_LEVEL)).isNotEmpty()

            if (rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN &&
                !hasRadiometricOccurrence
            ) {
                continue
            }

            val result = classifyIfd(ifd)
            results.put(result)
            when (result.getString("consistency_status")) {
                VALIDATED -> validated += 1
                CONTRADICTED -> contradicted += 1
                UNRESOLVED -> unresolved += 1
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "DNG_1_7_1_0_PLUS_TIFF_6_0")
            .put("validated_ifd_count", validated)
            .put("contradicted_ifd_count", contradicted)
            .put("unresolved_ifd_count", unresolved)
            .put("ifds", results)
            .put("defaults_mint_source_occurrence", false)
            .put("duplicate_occurrence_selection_allowed", false)
            .put("declared_dng_version_compatibility_validated", false)
            .put("black_level_delta_h_v_included", false)
            .put("effective_normalization_denominator_validated", false)
            .put("image_sample_payload_read", false)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    internal fun classifyIfd(ifd: JSONObject): JSONObject {
        val entries = ifd.optJSONArray("entries") ?: JSONArray()
        val rawDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
        val rawStatus = rawDomain.getString("domain_status")
        val samplesPerPixel = resolveSamplesPerPixel(entries)
        val bitsPerSample = resolveBitsPerSample(entries, samplesPerPixel)
        val sampleFormat = resolveSampleFormat(entries, samplesPerPixel)
        val encodingOutcome = encodingOutcome(samplesPerPixel, bitsPerSample, sampleFormat)
        val black = validateBlackLevel(entries, samplesPerPixel, bitsPerSample, sampleFormat, encodingOutcome)
        val white = validateWhiteLevel(entries, samplesPerPixel, bitsPerSample, sampleFormat, encodingOutcome)

        val consistencyStatus =
            when {
                rawStatus == SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN -> CONTRADICTED
                rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN -> UNRESOLVED
                encodingOutcome == CONTRADICTED -> CONTRADICTED
                encodingOutcome == UNRESOLVED -> UNRESOLVED
                black.getString("validation_status") == CONTRADICTED ||
                    white.getString("validation_status") == CONTRADICTED -> CONTRADICTED
                black.getString("validation_status") == UNRESOLVED ||
                    white.getString("validation_status") == UNRESOLVED -> UNRESOLVED
                else -> VALIDATED
            }

        return JSONObject()
            .put("ifd_path", ifd.optString("path", "UNKNOWN_IFD"))
            .put("ifd_offset", ifd.optLong("offset", -1L))
            .put("consistency_status", consistencyStatus)
            .put("raw_ifd_domain", rawDomain)
            .put("samples_per_pixel_dependency", samplesPerPixel.toJson())
            .put("bits_per_sample_dependency", bitsPerSample.toJson())
            .put("sample_format_dependency", sampleFormat.toJson())
            .put("encoding_contract_outcome", encodingOutcome)
            .put("black_level", black)
            .put("white_level", white)
            .put("black_white_ordering_validated", false)
            .put("black_level_delta_h_v_included", false)
            .put("source_values_modified", false)
            .put("domain_validation_is_scientific_admission", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun resolveSamplesPerPixel(entries: JSONArray): VectorDependency {
        val occurrences = entriesByTag(entries, TAG_SAMPLES_PER_PIXEL)
        if (occurrences.isEmpty()) {
            return VectorDependency(
                status = "RESOLVED_TIFF_DEFAULT_1",
                values = listOf(1L),
                occurrenceCount = 0,
                basis = "TIFF_6_0_DEFAULT_NOT_SOURCE_OCCURRENCE",
                outcome = VALIDATED,
            )
        }
        if (occurrences.size != 1) {
            return VectorDependency(
                status = "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
                values = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                outcome = UNRESOLVED,
            )
        }
        val entry = occurrences.single()
        val value = (entry.opt("value") as? Number)?.toLong()
        val valid =
            entry.optInt("type", -1) == TIFF_TYPE_SHORT &&
                entry.optLong("count", -1L) == 1L &&
                entry.optString("decodeStatus", "") == "DECODED" &&
                value != null && value > 0L
        return if (valid) {
            VectorDependency(
                status = "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
                values = listOf(value!!),
                occurrenceCount = 1,
                basis = "EXACT_IFD_OCCURRENCE",
                outcome = VALIDATED,
            )
        } else {
            VectorDependency(
                status = "INVALID_EXPLICIT_DEPENDENCY_FAIL_CLOSED",
                values = null,
                occurrenceCount = 1,
                basis = "TYPE_COUNT_DECODE_OR_VALUE_CONTRACT_FAILED",
                outcome = UNRESOLVED,
            )
        }
    }

    private fun resolveBitsPerSample(
        entries: JSONArray,
        samplesPerPixel: VectorDependency,
    ): VectorDependency {
        val spp = samplesPerPixel.values?.singleOrNull()?.toInt()
            ?: return unresolvedDependency("SAMPLES_PER_PIXEL_UNRESOLVED")
        val occurrences = entriesByTag(entries, TAG_BITS_PER_SAMPLE)
        if (occurrences.isEmpty()) {
            return VectorDependency(
                status = "RESOLVED_TIFF_DEFAULT_1_OUTSIDE_DNG_SUPPORTED_RANGE",
                values = List(spp) { 1L },
                occurrenceCount = 0,
                basis = "TIFF_6_0_DEFAULT_NOT_SOURCE_OCCURRENCE_DNG_REQUIRES_8_TO_32",
                outcome = CONTRADICTED,
            )
        }
        if (occurrences.size != 1) {
            return VectorDependency(
                status = "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
                values = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                outcome = UNRESOLVED,
            )
        }
        val entry = occurrences.single()
        val values = longValues(entry)
        val structuralValid =
            entry.optInt("type", -1) == TIFF_TYPE_SHORT &&
                entry.optLong("count", -1L) == spp.toLong() &&
                entry.optString("decodeStatus", "") == "DECODED" &&
                values?.size == spp
        if (!structuralValid) {
            return VectorDependency(
                status = "CONTRADICTED_EXPLICIT_BITS_PER_SAMPLE_FORMAT_CONTRACT",
                values = values,
                occurrenceCount = 1,
                basis = "DNG_PLUS_TIFF_TYPE_COUNT_DECODE_CONTRACT",
                outcome = CONTRADICTED,
            )
        }
        val rangeValid = values!!.all { it in 8L..32L }
        val equalDepthValid = spp == 1 || values.distinct().size == 1
        val outcome = if (rangeValid && equalDepthValid) VALIDATED else CONTRADICTED
        val status =
            when {
                !rangeValid -> "CONTRADICTED_DNG_BITS_PER_SAMPLE_RANGE_8_TO_32"
                !equalDepthValid -> "CONTRADICTED_DNG_MULTI_SAMPLE_BIT_DEPTH_MUST_MATCH"
                else -> "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE"
            }
        return VectorDependency(status, values, 1, "EXACT_IFD_OCCURRENCE", outcome)
    }

    private fun resolveSampleFormat(
        entries: JSONArray,
        samplesPerPixel: VectorDependency,
    ): VectorDependency {
        val spp = samplesPerPixel.values?.singleOrNull()?.toInt()
            ?: return unresolvedDependency("SAMPLES_PER_PIXEL_UNRESOLVED")
        val occurrences = entriesByTag(entries, TAG_SAMPLE_FORMAT)
        if (occurrences.isEmpty()) {
            return VectorDependency(
                status = "RESOLVED_TIFF_DEFAULT_UNSIGNED_INTEGER_1",
                values = List(spp) { SAMPLE_FORMAT_UNSIGNED },
                occurrenceCount = 0,
                basis = "TIFF_6_0_DEFAULT_NOT_SOURCE_OCCURRENCE",
                outcome = VALIDATED,
            )
        }
        if (occurrences.size != 1) {
            return VectorDependency(
                status = "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
                values = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                outcome = UNRESOLVED,
            )
        }
        val entry = occurrences.single()
        val values = longValues(entry)
        val structuralValid =
            entry.optInt("type", -1) == TIFF_TYPE_SHORT &&
                entry.optLong("count", -1L) == spp.toLong() &&
                entry.optString("decodeStatus", "") == "DECODED" &&
                values?.size == spp
        if (!structuralValid) {
            return VectorDependency(
                status = "CONTRADICTED_EXPLICIT_SAMPLE_FORMAT_CONTRACT",
                values = values,
                occurrenceCount = 1,
                basis = "DNG_PLUS_TIFF_TYPE_COUNT_DECODE_CONTRACT",
                outcome = CONTRADICTED,
            )
        }
        val allowed = values!!.all { it == SAMPLE_FORMAT_UNSIGNED || it == SAMPLE_FORMAT_FLOAT }
        return VectorDependency(
            status = if (allowed) "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE" else "CONTRADICTED_DNG_SAMPLE_FORMAT_NOT_UNSIGNED_OR_FLOAT",
            values = values,
            occurrenceCount = 1,
            basis = "EXACT_IFD_OCCURRENCE",
            outcome = if (allowed) VALIDATED else CONTRADICTED,
        )
    }

    private fun encodingOutcome(
        samplesPerPixel: VectorDependency,
        bitsPerSample: VectorDependency,
        sampleFormat: VectorDependency,
    ): String {
        val dependencyOutcomes = listOf(samplesPerPixel.outcome, bitsPerSample.outcome, sampleFormat.outcome)
        if (CONTRADICTED in dependencyOutcomes) return CONTRADICTED
        if (UNRESOLVED in dependencyOutcomes) return UNRESOLVED

        val bits = bitsPerSample.values ?: return UNRESOLVED
        val formats = sampleFormat.values ?: return UNRESOLVED
        if (bits.size != formats.size) return UNRESOLVED
        for (i in bits.indices) {
            when (formats[i]) {
                SAMPLE_FORMAT_UNSIGNED -> if (bits[i] !in 8L..32L) return CONTRADICTED
                SAMPLE_FORMAT_FLOAT -> if (bits[i] !in setOf(16L, 24L, 32L)) return CONTRADICTED
                else -> return CONTRADICTED
            }
        }
        return VALIDATED
    }

    private fun validateBlackLevel(
        entries: JSONArray,
        samplesPerPixel: VectorDependency,
        bitsPerSample: VectorDependency,
        sampleFormat: VectorDependency,
        encodingOutcome: String,
    ): JSONObject {
        val occurrences = entriesByTag(entries, TAG_BLACK_LEVEL)
        if (occurrences.isEmpty()) {
            return JSONObject()
                .put("validation_status", VALIDATED)
                .put("selection_status", "ABSENT_SPEC_DEFAULT_ZERO_NO_SOURCE_OCCURRENCE")
                .put("occurrence_count", 0)
                .put("effective_default", 0.0)
                .put("default_mints_source_occurrence", false)
                .put("integer_encoding_max_applied", false)
        }
        if (occurrences.size != 1) {
            return unresolvedRadiometric("MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED", occurrences.size)
        }
        if (encodingOutcome != VALIDATED) {
            return unresolvedRadiometric("ENCODING_DEPENDENCY_NOT_VALIDATED", 1)
        }
        val values = numericValues(occurrences.single())
            ?: return unresolvedRadiometric("NUMERIC_VALUE_DECODE_UNRESOLVED_FAIL_CLOSED", 1)
        return validateExplicitRadiometricValues(
            values = values,
            samplesPerPixel = samplesPerPixel,
            bitsPerSample = bitsPerSample,
            sampleFormat = sampleFormat,
            selectionStatus = "UNIQUE_PHYSICAL_OCCURRENCE",
            occurrenceCount = 1,
        )
    }

    private fun validateWhiteLevel(
        entries: JSONArray,
        samplesPerPixel: VectorDependency,
        bitsPerSample: VectorDependency,
        sampleFormat: VectorDependency,
        encodingOutcome: String,
    ): JSONObject {
        val occurrences = entriesByTag(entries, TAG_WHITE_LEVEL)
        if (occurrences.isEmpty()) {
            if (encodingOutcome != VALIDATED) {
                return unresolvedRadiometric("ENCODING_DEPENDENCY_NOT_VALIDATED_FOR_DEFAULT", 0)
            }
            val bits = bitsPerSample.values ?: return unresolvedRadiometric("BITS_PER_SAMPLE_UNRESOLVED", 0)
            val formats = sampleFormat.values ?: return unresolvedRadiometric("SAMPLE_FORMAT_UNRESOLVED", 0)
            val defaults = ArrayList<Double>()
            for (i in bits.indices) {
                defaults +=
                    if (formats[i] == SAMPLE_FORMAT_FLOAT) 1.0
                    else unsignedMax(bits[i]).toDouble()
            }
            return JSONObject()
                .put("validation_status", VALIDATED)
                .put("selection_status", "ABSENT_SPEC_DEFAULT_NO_SOURCE_OCCURRENCE")
                .put("occurrence_count", 0)
                .put("effective_defaults", doubleArrayJson(defaults))
                .put("default_basis", "DNG_UNSIGNED_2_POW_BITS_MINUS_1_OR_FLOAT_1_0_PER_SAMPLE")
                .put("default_mints_source_occurrence", false)
                .put("integer_encoding_max_applied", formats.any { it == SAMPLE_FORMAT_UNSIGNED })
        }
        if (occurrences.size != 1) {
            return unresolvedRadiometric("MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED", occurrences.size)
        }
        if (encodingOutcome != VALIDATED) {
            return unresolvedRadiometric("ENCODING_DEPENDENCY_NOT_VALIDATED", 1)
        }
        val values = numericValues(occurrences.single())
            ?: return unresolvedRadiometric("NUMERIC_VALUE_DECODE_UNRESOLVED_FAIL_CLOSED", 1)
        return validateExplicitRadiometricValues(
            values = values,
            samplesPerPixel = samplesPerPixel,
            bitsPerSample = bitsPerSample,
            sampleFormat = sampleFormat,
            selectionStatus = "UNIQUE_PHYSICAL_OCCURRENCE",
            occurrenceCount = 1,
        )
    }

    private fun validateExplicitRadiometricValues(
        values: List<Double>,
        samplesPerPixel: VectorDependency,
        bitsPerSample: VectorDependency,
        sampleFormat: VectorDependency,
        selectionStatus: String,
        occurrenceCount: Int,
    ): JSONObject {
        val spp = samplesPerPixel.values?.singleOrNull()?.toInt()
            ?: return unresolvedRadiometric("SAMPLES_PER_PIXEL_UNRESOLVED", occurrenceCount)
        val bits = bitsPerSample.values ?: return unresolvedRadiometric("BITS_PER_SAMPLE_UNRESOLVED", occurrenceCount)
        val formats = sampleFormat.values ?: return unresolvedRadiometric("SAMPLE_FORMAT_UNRESOLVED", occurrenceCount)
        if (bits.size != spp || formats.size != spp) {
            return unresolvedRadiometric("ENCODING_VECTOR_LENGTH_MISMATCH", occurrenceCount)
        }

        var integerMaxApplied = false
        var contradicted = false
        for (i in values.indices) {
            val value = values[i]
            if (!value.isFinite() || value < 0.0) {
                contradicted = true
                break
            }
            val sample = i % spp
            if (formats[sample] == SAMPLE_FORMAT_UNSIGNED) {
                integerMaxApplied = true
                if (value > unsignedMax(bits[sample]).toDouble()) {
                    contradicted = true
                    break
                }
            }
        }

        return JSONObject()
            .put("validation_status", if (contradicted) CONTRADICTED else VALIDATED)
            .put("selection_status", selectionStatus)
            .put("occurrence_count", occurrenceCount)
            .put("decoded_values", doubleArrayJson(values))
            .put("integer_encoding_max_applied", integerMaxApplied)
            .put("floating_point_values_not_bounded_by_integer_bit_max", true)
            .put("source_values_modified", false)
            .put("admission_effect", false)
    }

    private fun unresolvedDependency(reason: String): VectorDependency =
        VectorDependency(
            status = "DEPENDENCY_UNRESOLVED_FAIL_CLOSED",
            values = null,
            occurrenceCount = 0,
            basis = reason,
            outcome = UNRESOLVED,
        )

    private fun unresolvedRadiometric(reason: String, occurrenceCount: Int): JSONObject =
        JSONObject()
            .put("validation_status", UNRESOLVED)
            .put("selection_status", reason)
            .put("occurrence_count", occurrenceCount)
            .put("source_values_modified", false)
            .put("admission_effect", false)

    private fun unsignedMax(bits: Long): Long =
        if (bits == 32L) 0xffff_ffffL else (1L shl bits.toInt()) - 1L

    private fun longValues(entry: JSONObject): List<Long>? {
        if (entry.optString("decodeStatus", "") != "DECODED") return null
        val expected = entry.optLong("count", -1L)
        if (expected < 0L || expected > Int.MAX_VALUE.toLong()) return null
        val raw = entry.opt("value")
        val items =
            if (raw is JSONArray) {
                (0 until raw.length()).map { raw.opt(it) }
            } else {
                listOf(raw)
            }
        if (items.size.toLong() != expected) return null
        val out = ArrayList<Long>(items.size)
        for (item in items) {
            val number = item as? Number ?: return null
            out += number.toLong()
        }
        return out
    }

    private fun numericValues(entry: JSONObject): List<Double>? {
        if (entry.optString("decodeStatus", "") != "DECODED") return null
        val expected = entry.optLong("count", -1L)
        if (expected < 0L || expected > Int.MAX_VALUE.toLong()) return null
        val raw = entry.opt("value")
        val items =
            if (raw is JSONArray) {
                (0 until raw.length()).map { raw.opt(it) }
            } else {
                listOf(raw)
            }
        if (items.size.toLong() != expected) return null
        val out = ArrayList<Double>(items.size)
        for (item in items) {
            val value =
                when (item) {
                    is Number -> item.toDouble()
                    is JSONObject -> (item.opt("value") as? Number)?.toDouble()
                    else -> null
                } ?: return null
            if (!value.isFinite()) return null
            out += value
        }
        return out
    }

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }

    private fun entriesByTags(entries: JSONArray, tags: Set<Int>): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) in tags }

    private fun longArrayJson(values: List<Long>): JSONArray {
        val out = JSONArray()
        values.forEach { out.put(it) }
        return out
    }

    private fun doubleArrayJson(values: List<Double>): JSONArray {
        val out = JSONArray()
        values.forEach { out.put(it) }
        return out
    }
}

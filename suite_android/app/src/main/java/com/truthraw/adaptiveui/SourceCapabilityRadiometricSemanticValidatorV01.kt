package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only DNG radiometric-boundary semantic validator v0.1.
 *
 * Scope is deliberately limited to the file-format contracts for:
 * - BlackLevelRepeatDim (50713)
 * - BlackLevel (50714)
 * - WhiteLevel (50717)
 * and the local SamplesPerPixel dependency required for count validation.
 *
 * This validator does not read pixels, does not choose a scientific calibration,
 * does not change Evidence Ledger lifecycle/admission, and does not write to the
 * Scientific Master or Appearance. TIFF/DNG specification defaults may be used
 * as validation dependencies, but defaults never mint a physical source occurrence.
 */
object SourceCapabilityRadiometricSemanticValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricSemanticValidation/0.1"
    const val STATUS = "SHADOW_ONLY_SPECIFICATION_SEMANTIC_VALIDATION"

    private const val TAG_SAMPLES_PER_PIXEL = 277
    private const val TAG_BLACK_LEVEL_REPEAT_DIM = 50713
    private const val TAG_BLACK_LEVEL = 50714
    private const val TAG_WHITE_LEVEL = 50717

    private const val TIFF_TYPE_SHORT = 3
    private const val TIFF_TYPE_LONG = 4
    private const val TIFF_TYPE_RATIONAL = 5

    private const val VALIDATED = "VALIDATED_FORMAT_SEMANTICS_SHADOW_ONLY"
    private const val CONTRADICTED = "CONTRADICTED_FORMAT_SEMANTICS_FAIL_CLOSED"
    private const val DEPENDENCY_UNRESOLVED = "DEPENDENCY_UNRESOLVED_FAIL_CLOSED"

    private data class ScalarDependency(
        val status: String,
        val value: Long?,
        val occurrenceCount: Int,
        val basis: String,
    ) {
        val resolved: Boolean get() = value != null

        fun toJson(): JSONObject =
            JSONObject()
                .put("status", status)
                .put("value", value ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
    }

    private data class RepeatDependency(
        val status: String,
        val rows: Long?,
        val cols: Long?,
        val occurrenceCount: Int,
        val basis: String,
    ) {
        val resolved: Boolean get() = rows != null && cols != null

        fun toJson(): JSONObject =
            JSONObject()
                .put("status", status)
                .put("rows", rows ?: JSONObject.NULL)
                .put("cols", cols ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
    }

    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }

        val ifdResults = JSONArray()
        var validatedCount = 0
        var contradictedCount = 0
        var unresolvedCount = 0
        var ambiguousSelectionCount = 0

        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()
        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val entries = ifd.optJSONArray("entries") ?: JSONArray()

            val familyOccurrences =
                entriesByTags(
                    entries,
                    setOf(TAG_BLACK_LEVEL_REPEAT_DIM, TAG_BLACK_LEVEL, TAG_WHITE_LEVEL),
                )
            if (familyOccurrences.isEmpty()) continue

            val samplesPerPixel = resolveSamplesPerPixel(entries)
            val repeatDim = resolveBlackLevelRepeatDim(entries)
            val rawIfdDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            val occurrenceReports = JSONArray()

            familyOccurrences.forEach { entry ->
                val report =
                    when (entry.optInt("tag", -1)) {
                        TAG_BLACK_LEVEL_REPEAT_DIM -> validateRepeatDimOccurrence(entry)
                        TAG_BLACK_LEVEL -> validateBlackLevelOccurrence(entry, repeatDim, samplesPerPixel)
                        TAG_WHITE_LEVEL -> validateWhiteLevelOccurrence(entry, samplesPerPixel)
                        else -> error("Unexpected radiometric semantic tag")
                    }
                occurrenceReports.put(report)
                when (report.getString("validation_status")) {
                    VALIDATED -> validatedCount += 1
                    CONTRADICTED -> contradictedCount += 1
                    DEPENDENCY_UNRESOLVED -> unresolvedCount += 1
                }
            }

            val repeatSelection = selectionStatus(entries, TAG_BLACK_LEVEL_REPEAT_DIM)
            val blackSelection = selectionStatus(entries, TAG_BLACK_LEVEL)
            val whiteSelection = selectionStatus(entries, TAG_WHITE_LEVEL)
            ambiguousSelectionCount +=
                listOf(repeatSelection, blackSelection, whiteSelection)
                    .count { it == "MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED" }

            val rawIfdDomainStatus = rawIfdDomain.getString("domain_status")
            val familyStatus =
                when {
                    occurrenceReportsHasStatus(occurrenceReports, CONTRADICTED) ||
                        rawIfdDomainStatus == SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN ->
                        "CONTRADICTED_FAIL_CLOSED"
                    occurrenceReportsHasStatus(occurrenceReports, DEPENDENCY_UNRESOLVED) ||
                        rawIfdDomainStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN ||
                        repeatSelection.startsWith("MULTIPLE_") ||
                        blackSelection.startsWith("MULTIPLE_") ||
                        whiteSelection.startsWith("MULTIPLE_") ->
                        "UNRESOLVED_FAIL_CLOSED"
                    else -> "VALIDATED_SHADOW_ONLY"
                }

            ifdResults.put(
                JSONObject()
                    .put("ifd_path", ifd.optString("path", "IFD[$i]"))
                    .put("ifd_offset", ifd.optLong("offset", -1L))
                    .put("family_status", familyStatus)
                    .put("raw_ifd_usage_status", rawIfdUsageStatus(rawIfdDomainStatus))
                    .put("raw_ifd_domain", rawIfdDomain)
                    .put("samples_per_pixel_dependency", samplesPerPixel.toJson())
                    .put("black_level_repeat_dim_dependency", repeatDim.toJson())
                    .put("black_level_repeat_dim_selection", repeatSelection)
                    .put("black_level_selection", blackSelection)
                    .put("white_level_selection", whiteSelection)
                    .put("occurrences", occurrenceReports),
            )
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "DNG_1_7_1_0_PLUS_TIFF_6_0")
            .put(
                "validated_contract_scope",
                JSONArray()
                    .put("BlackLevelRepeatDim:type=SHORT,count=2,default=1x1")
                    .put("BlackLevel:type=SHORT|LONG|RATIONAL,count=repeatRows*repeatCols*SamplesPerPixel")
                    .put("WhiteLevel:type=SHORT|LONG,count=SamplesPerPixel")
                    .put("SamplesPerPixel:type=SHORT,count=1,default=1 dependency only"),
            )
            .put("defaults_mint_source_occurrence", false)
            .put("duplicate_occurrence_selection_allowed", false)
            .put("validated_occurrence_count", validatedCount)
            .put("contradicted_occurrence_count", contradictedCount)
            .put("dependency_unresolved_occurrence_count", unresolvedCount)
            .put("ambiguous_selection_count", ambiguousSelectionCount)
            .put("ifds", ifdResults)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun validateRepeatDimOccurrence(entry: JSONObject): JSONObject {
        val typeOk = entry.optInt("type", -1) == TIFF_TYPE_SHORT
        val countOk = entry.optLong("count", -1L) == 2L
        val status = if (typeOk && countOk) VALIDATED else CONTRADICTED
        return baseOccurrence(entry, status)
            .put("expected_types", JSONArray().put("SHORT"))
            .put("expected_count", 2)
            .put("type_contract_valid", typeOk)
            .put("count_contract_valid", countOk)
    }

    private fun validateBlackLevelOccurrence(
        entry: JSONObject,
        repeatDim: RepeatDependency,
        samplesPerPixel: ScalarDependency,
    ): JSONObject {
        val type = entry.optInt("type", -1)
        val typeOk = type == TIFF_TYPE_SHORT || type == TIFF_TYPE_LONG || type == TIFF_TYPE_RATIONAL
        val expectedCount =
            if (repeatDim.resolved && samplesPerPixel.resolved) {
                safeMul3(repeatDim.rows!!, repeatDim.cols!!, samplesPerPixel.value!!)
            } else {
                null
            }
        val count = entry.optLong("count", -1L)
        val status =
            when {
                !typeOk -> CONTRADICTED
                expectedCount == null -> DEPENDENCY_UNRESOLVED
                count != expectedCount -> CONTRADICTED
                else -> VALIDATED
            }
        return baseOccurrence(entry, status)
            .put("expected_types", JSONArray().put("SHORT").put("LONG").put("RATIONAL"))
            .put("expected_count", expectedCount ?: JSONObject.NULL)
            .put("type_contract_valid", typeOk)
            .put("count_contract_valid", expectedCount?.let { count == it } ?: JSONObject.NULL)
            .put("repeat_dim_dependency_status", repeatDim.status)
            .put("samples_per_pixel_dependency_status", samplesPerPixel.status)
    }

    private fun validateWhiteLevelOccurrence(
        entry: JSONObject,
        samplesPerPixel: ScalarDependency,
    ): JSONObject {
        val type = entry.optInt("type", -1)
        val typeOk = type == TIFF_TYPE_SHORT || type == TIFF_TYPE_LONG
        val expectedCount = samplesPerPixel.value
        val count = entry.optLong("count", -1L)
        val status =
            when {
                !typeOk -> CONTRADICTED
                expectedCount == null -> DEPENDENCY_UNRESOLVED
                count != expectedCount -> CONTRADICTED
                else -> VALIDATED
            }
        return baseOccurrence(entry, status)
            .put("expected_types", JSONArray().put("SHORT").put("LONG"))
            .put("expected_count", expectedCount ?: JSONObject.NULL)
            .put("type_contract_valid", typeOk)
            .put("count_contract_valid", expectedCount?.let { count == it } ?: JSONObject.NULL)
            .put("samples_per_pixel_dependency_status", samplesPerPixel.status)
    }

    private fun baseOccurrence(
        entry: JSONObject,
        validationStatus: String,
    ): JSONObject =
        JSONObject()
            .put("tag", entry.optInt("tag", -1))
            .put("entry_offset", entry.optLong("entryOffset", -1L))
            .put("value_field_offset", entry.optLong("valueFieldOffset", -1L))
            .put("parser_type", entry.optString("typeName", "UNKNOWN_TYPE"))
            .put("item_count", entry.optLong("count", -1L))
            .put("decode_status", entry.optString("decodeStatus", "UNKNOWN_NOT_REPORTED"))
            .put("validation_status", validationStatus)
            .put("lifecycle_effect", false)
            .put("admission_effect", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

    private fun resolveSamplesPerPixel(entries: JSONArray): ScalarDependency {
        val occurrences = entriesByTag(entries, TAG_SAMPLES_PER_PIXEL)
        if (occurrences.isEmpty()) {
            return ScalarDependency(
                status = "RESOLVED_SPEC_DEFAULT_1",
                value = 1L,
                occurrenceCount = 0,
                basis = "TIFF_6_0_DEFAULT_NOT_SOURCE_OCCURRENCE",
            )
        }
        if (occurrences.size != 1) {
            return ScalarDependency(
                status = "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
                value = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
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
            ScalarDependency(
                status = "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
                value = value,
                occurrenceCount = 1,
                basis = "EXACT_IFD_OCCURRENCE",
            )
        } else {
            ScalarDependency(
                status = "INVALID_EXPLICIT_DEPENDENCY_FAIL_CLOSED",
                value = null,
                occurrenceCount = 1,
                basis = "TYPE_COUNT_DECODE_OR_VALUE_CONTRACT_FAILED",
            )
        }
    }

    private fun resolveBlackLevelRepeatDim(entries: JSONArray): RepeatDependency {
        val occurrences = entriesByTag(entries, TAG_BLACK_LEVEL_REPEAT_DIM)
        if (occurrences.isEmpty()) {
            return RepeatDependency(
                status = "RESOLVED_SPEC_DEFAULT_1_X_1",
                rows = 1L,
                cols = 1L,
                occurrenceCount = 0,
                basis = "DNG_1_7_1_0_DEFAULT_NOT_SOURCE_OCCURRENCE",
            )
        }
        if (occurrences.size != 1) {
            return RepeatDependency(
                status = "AMBIGUOUS_MULTIPLE_OCCURRENCES_FAIL_CLOSED",
                rows = null,
                cols = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
            )
        }
        val entry = occurrences.single()
        val value = entry.optJSONArray("value")
        val rows = (value?.opt(0) as? Number)?.toLong()
        val cols = (value?.opt(1) as? Number)?.toLong()
        val valid =
            entry.optInt("type", -1) == TIFF_TYPE_SHORT &&
                entry.optLong("count", -1L) == 2L &&
                entry.optString("decodeStatus", "") == "DECODED" &&
                value?.length() == 2 &&
                rows != null && rows > 0L &&
                cols != null && cols > 0L
        return if (valid) {
            RepeatDependency(
                status = "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
                rows = rows,
                cols = cols,
                occurrenceCount = 1,
                basis = "EXACT_IFD_OCCURRENCE",
            )
        } else {
            RepeatDependency(
                status = "INVALID_EXPLICIT_DEPENDENCY_FAIL_CLOSED",
                rows = null,
                cols = null,
                occurrenceCount = 1,
                basis = "TYPE_COUNT_DECODE_OR_VALUE_CONTRACT_FAILED",
            )
        }
    }

    private fun selectionStatus(entries: JSONArray, tag: Int): String =
        when (entriesByTag(entries, tag).size) {
            0 -> "ABSENT_NO_SOURCE_OCCURRENCE"
            1 -> "UNIQUE_PHYSICAL_OCCURRENCE"
            else -> "MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED"
        }

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }

    private fun entriesByTags(entries: JSONArray, tags: Set<Int>): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) in tags }

    private fun rawIfdUsageStatus(domainStatus: String): String =
        when (domainStatus) {
            SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN ->
                "VALIDATED_RAW_IFD_FORMAT_USAGE_SHADOW_ONLY"
            SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN ->
                "CONTRADICTED_NON_RAW_IFD_USAGE_FAIL_CLOSED"
            else -> "UNRESOLVED_RAW_IFD_USAGE_FAIL_CLOSED"
        }

    private fun occurrenceReportsHasStatus(reports: JSONArray, status: String): Boolean =
        (0 until reports.length())
            .mapNotNull { reports.optJSONObject(it) }
            .any { it.optString("validation_status") == status }

    private fun safeMul3(a: Long, b: Long, c: Long): Long? {
        if (a <= 0L || b <= 0L || c <= 0L) return null
        if (a > Long.MAX_VALUE / b) return null
        val ab = a * b
        if (ab > Long.MAX_VALUE / c) return null
        return ab * c
    }
}

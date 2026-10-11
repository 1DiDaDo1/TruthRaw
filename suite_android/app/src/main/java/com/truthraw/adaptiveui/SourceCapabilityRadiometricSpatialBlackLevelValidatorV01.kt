package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only DNG spatial black-level dependency validator v0.1.
 *
 * This layer validates only metadata contracts needed to describe the spatial
 * zero-light encoding domain of a Raw IFD:
 * - ImageWidth / ImageLength
 * - ActiveArea
 * - BlackLevelDeltaH / BlackLevelDeltaV
 *
 * It deliberately does NOT evaluate WhiteLevel - max(BlackLevel), does not read
 * image sample payload, does not modify Evidence Ledger lifecycle/admission, and
 * does not write to Scientific Master or Appearance. Specification defaults are
 * dependency facts only and never mint physical source occurrences.
 */
object SourceCapabilityRadiometricSpatialBlackLevelValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricSpatialBlackLevel/0.1"
    const val STATUS = "SHADOW_ONLY_RADIOMETRIC_SPATIAL_BLACK_LEVEL"

    const val VALIDATED = "VALIDATED_RADIOMETRIC_SPATIAL_BLACK_LEVEL_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_RADIOMETRIC_SPATIAL_BLACK_LEVEL_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_RADIOMETRIC_SPATIAL_BLACK_LEVEL_FAIL_CLOSED"

    private const val TAG_IMAGE_WIDTH = 256
    private const val TAG_IMAGE_LENGTH = 257
    private const val TAG_BLACK_LEVEL_DELTA_H = 50715
    private const val TAG_BLACK_LEVEL_DELTA_V = 50716
    private const val TAG_ACTIVE_AREA = 50829

    private const val TIFF_TYPE_SHORT = 3
    private const val TIFF_TYPE_LONG = 4
    private const val TIFF_TYPE_SRATIONAL = 10

    private data class DimensionDependency(
        val state: String,
        val status: String,
        val value: Long?,
        val occurrenceCount: Int,
        val basis: String,
    ) {
        val resolved: Boolean get() = state == VALIDATED && value != null

        fun toJson(): JSONObject =
            JSONObject()
                .put("validation_state", state)
                .put("status", status)
                .put("value", value ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
    }

    private data class ActiveAreaDependency(
        val state: String,
        val status: String,
        val top: Long?,
        val left: Long?,
        val bottom: Long?,
        val right: Long?,
        val occurrenceCount: Int,
        val basis: String,
        val defaultApplied: Boolean,
    ) {
        val resolved: Boolean
            get() =
                state == VALIDATED && top != null && left != null &&
                    bottom != null && right != null
        val width: Long? get() = if (resolved) right!! - left!! else null
        val length: Long? get() = if (resolved) bottom!! - top!! else null

        fun toJson(): JSONObject =
            JSONObject()
                .put("validation_state", state)
                .put("status", status)
                .put("top", top ?: JSONObject.NULL)
                .put("left", left ?: JSONObject.NULL)
                .put("bottom", bottom ?: JSONObject.NULL)
                .put("right", right ?: JSONObject.NULL)
                .put("active_area_width", width ?: JSONObject.NULL)
                .put("active_area_length", length ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
                .put("default_applied", defaultApplied)
                .put("default_mints_source_occurrence", false)
    }

    private data class DeltaDependency(
        val state: String,
        val status: String,
        val occurrenceCount: Int,
        val expectedCount: Long?,
        val observedCount: Long?,
        val defaultApplied: Boolean,
        val minimum: Double?,
        val maximum: Double?,
    ) {
        fun toJson(): JSONObject =
            JSONObject()
                .put("validation_state", state)
                .put("status", status)
                .put("occurrence_count", occurrenceCount)
                .put("expected_count", expectedCount ?: JSONObject.NULL)
                .put("observed_count", observedCount ?: JSONObject.NULL)
                .put("expected_type", "SRATIONAL")
                .put("default_applied", defaultApplied)
                .put("default_value", if (defaultApplied) 0.0 else JSONObject.NULL)
                .put("minimum_decoded_delta", minimum ?: JSONObject.NULL)
                .put("maximum_decoded_delta", maximum ?: JSONObject.NULL)
                .put("default_mints_source_occurrence", false)
    }

    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }

        val results = JSONArray()
        var validatedCount = 0
        var contradictedCount = 0
        var unresolvedCount = 0

        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()
        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val entries = ifd.optJSONArray("entries") ?: JSONArray()
            val rawIfdDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            val rawDomainStatus = rawIfdDomain.getString("domain_status")
            val hasSpatialBlackMetadata =
                entriesByTag(entries, TAG_BLACK_LEVEL_DELTA_H).isNotEmpty() ||
                    entriesByTag(entries, TAG_BLACK_LEVEL_DELTA_V).isNotEmpty() ||
                    entriesByTag(entries, TAG_ACTIVE_AREA).isNotEmpty()

            // Do not flood diagnostics with unrelated EXIF/preview IFDs. A validated
            // Raw IFD is always evaluated; any IFD carrying this tag family is also
            // evaluated so misplaced metadata fails closed visibly.
            if (
                rawDomainStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN &&
                !hasSpatialBlackMetadata
            ) {
                continue
            }

            val imageWidth = resolveDimension(entries, TAG_IMAGE_WIDTH, "ImageWidth")
            val imageLength = resolveDimension(entries, TAG_IMAGE_LENGTH, "ImageLength")
            val activeArea = resolveActiveArea(entries, imageWidth, imageLength)
            val deltaH =
                resolveDelta(
                    entries = entries,
                    tag = TAG_BLACK_LEVEL_DELTA_H,
                    expectedCount = activeArea.width,
                    axisName = "H",
                )
            val deltaV =
                resolveDelta(
                    entries = entries,
                    tag = TAG_BLACK_LEVEL_DELTA_V,
                    expectedCount = activeArea.length,
                    axisName = "V",
                )

            val states =
                listOf(
                    imageWidth.state,
                    imageLength.state,
                    activeArea.state,
                    deltaH.state,
                    deltaV.state,
                )
            val resultStatus =
                when {
                    rawDomainStatus == SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN ->
                        CONTRADICTED
                    states.any { it == CONTRADICTED } -> CONTRADICTED
                    rawDomainStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN ->
                        UNRESOLVED
                    states.any { it == UNRESOLVED } -> UNRESOLVED
                    else -> VALIDATED
                }

            when (resultStatus) {
                VALIDATED -> validatedCount += 1
                CONTRADICTED -> contradictedCount += 1
                UNRESOLVED -> unresolvedCount += 1
            }

            results.put(
                JSONObject()
                    .put("ifd_path", ifd.optString("path", "IFD[$i]"))
                    .put("ifd_offset", ifd.optLong("offset", -1L))
                    .put("spatial_black_level_status", resultStatus)
                    .put("raw_ifd_domain", rawIfdDomain)
                    .put("image_width_dependency", imageWidth.toJson())
                    .put("image_length_dependency", imageLength.toJson())
                    .put("active_area_dependency", activeArea.toJson())
                    .put("black_level_delta_h", deltaH.toJson())
                    .put("black_level_delta_v", deltaV.toJson())
                    .put("effective_black_dependency_complete", resultStatus == VALIDATED)
                    .put("effective_black_values_computed", false)
                    .put("normalization_denominator_validated", false)
                    .put("domain_validation_is_scientific_admission", false)
                    .put("source_values_modified", false)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false),
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
                    .put("ActiveArea:type=SHORT|LONG,count=4,default=[0,0,ImageLength,ImageWidth]")
                    .put("BlackLevelDeltaH:type=SRATIONAL,count=ActiveAreaWidth,default=all-zero")
                    .put("BlackLevelDeltaV:type=SRATIONAL,count=ActiveAreaLength,default=all-zero")
                    .put("ImageWidth/ImageLength:type=SHORT|LONG,count=1,positive dependency"),
            )
            .put(
                "explicitly_out_of_scope",
                JSONArray()
                    .put("WhiteLevel-minus-max-effective-BlackLevel denominator validation")
                    .put("image sample payload validation")
                    .put("Scientific Master or Appearance modification"),
            )
            .put("validated_ifd_count", validatedCount)
            .put("contradicted_ifd_count", contradictedCount)
            .put("unresolved_ifd_count", unresolvedCount)
            .put("ifds", results)
            .put("defaults_mint_source_occurrence", false)
            .put("duplicate_occurrence_selection_allowed", false)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("image_sample_payload_read", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun resolveDimension(
        entries: JSONArray,
        tag: Int,
        label: String,
    ): DimensionDependency {
        val occurrences = entriesByTag(entries, tag)
        if (occurrences.isEmpty()) {
            return DimensionDependency(
                state = UNRESOLVED,
                status = "MISSING_REQUIRED_${label.uppercase()}_DEPENDENCY_FAIL_CLOSED",
                value = null,
                occurrenceCount = 0,
                basis = "NO_DIMENSION_DEFAULT_APPLIED",
            )
        }
        if (occurrences.size != 1) {
            return DimensionDependency(
                state = UNRESOLVED,
                status = "AMBIGUOUS_MULTIPLE_${label.uppercase()}_OCCURRENCES_FAIL_CLOSED",
                value = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
            )
        }

        val entry = occurrences.single()
        val type = entry.optInt("type", -1)
        val count = entry.optLong("count", -1L)
        if (type != TIFF_TYPE_SHORT && type != TIFF_TYPE_LONG) {
            return DimensionDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_${label.uppercase()}_TYPE_CONTRACT",
                value = null,
                occurrenceCount = 1,
                basis = "EXPECTED_SHORT_OR_LONG",
            )
        }
        if (count != 1L) {
            return DimensionDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_${label.uppercase()}_COUNT_CONTRACT",
                value = null,
                occurrenceCount = 1,
                basis = "EXPECTED_COUNT_1",
            )
        }
        if (entry.optString("decodeStatus", "") != "DECODED") {
            return DimensionDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_${label.uppercase()}_DECODE_FAIL_CLOSED",
                value = null,
                occurrenceCount = 1,
                basis = "SOURCE_OCCURRENCE_NOT_DECODED",
            )
        }
        val value = (entry.opt("value") as? Number)?.toLong()
        if (value == null || value <= 0L) {
            return DimensionDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_${label.uppercase()}_VALUE_CONTRACT",
                value = null,
                occurrenceCount = 1,
                basis = "DIMENSION_MUST_BE_POSITIVE",
            )
        }
        return DimensionDependency(
            state = VALIDATED,
            status = "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
            value = value,
            occurrenceCount = 1,
            basis = "EXACT_IFD_OCCURRENCE",
        )
    }

    private fun resolveActiveArea(
        entries: JSONArray,
        imageWidth: DimensionDependency,
        imageLength: DimensionDependency,
    ): ActiveAreaDependency {
        val occurrences = entriesByTag(entries, TAG_ACTIVE_AREA)
        if (occurrences.isEmpty()) {
            if (imageWidth.resolved && imageLength.resolved) {
                return ActiveAreaDependency(
                    state = VALIDATED,
                    status = "RESOLVED_DNG_DEFAULT_FULL_IMAGE",
                    top = 0L,
                    left = 0L,
                    bottom = imageLength.value,
                    right = imageWidth.value,
                    occurrenceCount = 0,
                    basis = "DNG_1_7_1_0_DEFAULT_NOT_SOURCE_OCCURRENCE",
                    defaultApplied = true,
                )
            }
            return ActiveAreaDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_ACTIVE_AREA_DEFAULT_DIMENSION_DEPENDENCY_FAIL_CLOSED",
                top = null,
                left = null,
                bottom = null,
                right = null,
                occurrenceCount = 0,
                basis = "DEFAULT_REQUIRES_RESOLVED_IMAGE_LENGTH_AND_IMAGE_WIDTH",
                defaultApplied = false,
            )
        }
        if (occurrences.size != 1) {
            return ActiveAreaDependency(
                state = UNRESOLVED,
                status = "AMBIGUOUS_MULTIPLE_ACTIVE_AREA_OCCURRENCES_FAIL_CLOSED",
                top = null,
                left = null,
                bottom = null,
                right = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                defaultApplied = false,
            )
        }

        val entry = occurrences.single()
        val type = entry.optInt("type", -1)
        if (type != TIFF_TYPE_SHORT && type != TIFF_TYPE_LONG) {
            return invalidActiveArea("CONTRADICTED_ACTIVE_AREA_TYPE_CONTRACT", "EXPECTED_SHORT_OR_LONG")
        }
        if (entry.optLong("count", -1L) != 4L) {
            return invalidActiveArea("CONTRADICTED_ACTIVE_AREA_COUNT_CONTRACT", "EXPECTED_COUNT_4")
        }
        if (entry.optString("decodeStatus", "") != "DECODED") {
            return ActiveAreaDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_ACTIVE_AREA_DECODE_FAIL_CLOSED",
                top = null,
                left = null,
                bottom = null,
                right = null,
                occurrenceCount = 1,
                basis = "SOURCE_OCCURRENCE_NOT_DECODED",
                defaultApplied = false,
            )
        }
        val value = entry.optJSONArray("value")
        val top = (value?.opt(0) as? Number)?.toLong()
        val left = (value?.opt(1) as? Number)?.toLong()
        val bottom = (value?.opt(2) as? Number)?.toLong()
        val right = (value?.opt(3) as? Number)?.toLong()
        if (
            value?.length() != 4 || top == null || left == null ||
            bottom == null || right == null || top < 0L || left < 0L ||
            bottom <= top || right <= left
        ) {
            return invalidActiveArea(
                "CONTRADICTED_ACTIVE_AREA_RECTANGLE_CONTRACT",
                "EXPECTED_NON_NEGATIVE_TOP_LEFT_AND_STRICTLY_POSITIVE_RECTANGLE",
            )
        }
        if (!imageWidth.resolved || !imageLength.resolved) {
            return ActiveAreaDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_ACTIVE_AREA_IMAGE_DIMENSION_DEPENDENCY_FAIL_CLOSED",
                top = top,
                left = left,
                bottom = bottom,
                right = right,
                occurrenceCount = 1,
                basis = "BOUNDS_CHECK_REQUIRES_RESOLVED_IMAGE_LENGTH_AND_IMAGE_WIDTH",
                defaultApplied = false,
            )
        }
        if (bottom > imageLength.value!! || right > imageWidth.value!!) {
            return invalidActiveArea(
                "CONTRADICTED_ACTIVE_AREA_OUTSIDE_IMAGE_BOUNDS",
                "ACTIVE_AREA_MUST_BE_WITHIN_IMAGE_LENGTH_AND_IMAGE_WIDTH",
            )
        }
        return ActiveAreaDependency(
            state = VALIDATED,
            status = "RESOLVED_EXPLICIT_SOURCE_OCCURRENCE",
            top = top,
            left = left,
            bottom = bottom,
            right = right,
            occurrenceCount = 1,
            basis = "EXACT_IFD_OCCURRENCE",
            defaultApplied = false,
        )
    }

    private fun invalidActiveArea(status: String, basis: String): ActiveAreaDependency =
        ActiveAreaDependency(
            state = CONTRADICTED,
            status = status,
            top = null,
            left = null,
            bottom = null,
            right = null,
            occurrenceCount = 1,
            basis = basis,
            defaultApplied = false,
        )

    private fun resolveDelta(
        entries: JSONArray,
        tag: Int,
        expectedCount: Long?,
        axisName: String,
    ): DeltaDependency {
        val occurrences = entriesByTag(entries, tag)
        if (occurrences.isEmpty()) {
            return DeltaDependency(
                state = if (expectedCount != null) VALIDATED else UNRESOLVED,
                status =
                    if (expectedCount != null) {
                        "RESOLVED_DNG_DEFAULT_ALL_ZERO"
                    } else {
                        "UNRESOLVED_DNG_DEFAULT_AXIS_DIMENSION_FAIL_CLOSED"
                    },
                occurrenceCount = 0,
                expectedCount = expectedCount,
                observedCount = null,
                defaultApplied = expectedCount != null,
                minimum = if (expectedCount != null) 0.0 else null,
                maximum = if (expectedCount != null) 0.0 else null,
            )
        }
        if (occurrences.size != 1) {
            return DeltaDependency(
                state = UNRESOLVED,
                status = "AMBIGUOUS_MULTIPLE_BLACK_LEVEL_DELTA_${axisName}_OCCURRENCES_FAIL_CLOSED",
                occurrenceCount = occurrences.size,
                expectedCount = expectedCount,
                observedCount = null,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }

        val entry = occurrences.single()
        val observedCount = entry.optLong("count", -1L)
        if (entry.optInt("type", -1) != TIFF_TYPE_SRATIONAL) {
            return DeltaDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_BLACK_LEVEL_DELTA_${axisName}_TYPE_CONTRACT",
                occurrenceCount = 1,
                expectedCount = expectedCount,
                observedCount = observedCount,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }
        if (expectedCount == null) {
            return DeltaDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_BLACK_LEVEL_DELTA_${axisName}_AXIS_DIMENSION_FAIL_CLOSED",
                occurrenceCount = 1,
                expectedCount = null,
                observedCount = observedCount,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }
        if (observedCount != expectedCount) {
            return DeltaDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_BLACK_LEVEL_DELTA_${axisName}_COUNT_CONTRACT",
                occurrenceCount = 1,
                expectedCount = expectedCount,
                observedCount = observedCount,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }
        if (entry.optString("decodeStatus", "") != "DECODED") {
            return DeltaDependency(
                state = UNRESOLVED,
                status = "UNRESOLVED_BLACK_LEVEL_DELTA_${axisName}_DECODE_FAIL_CLOSED",
                occurrenceCount = 1,
                expectedCount = expectedCount,
                observedCount = observedCount,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }

        val values = decodedSrationalValues(entry)
        if (values == null || values.size.toLong() != expectedCount) {
            return DeltaDependency(
                state = CONTRADICTED,
                status = "CONTRADICTED_BLACK_LEVEL_DELTA_${axisName}_SRATIONAL_VALUE_CONTRACT",
                occurrenceCount = 1,
                expectedCount = expectedCount,
                observedCount = observedCount,
                defaultApplied = false,
                minimum = null,
                maximum = null,
            )
        }
        return DeltaDependency(
            state = VALIDATED,
            status = "VALIDATED_EXPLICIT_SRATIONAL_SOURCE_OCCURRENCE",
            occurrenceCount = 1,
            expectedCount = expectedCount,
            observedCount = observedCount,
            defaultApplied = false,
            minimum = values.minOrNull(),
            maximum = values.maxOrNull(),
        )
    }

    private fun decodedSrationalValues(entry: JSONObject): List<Double>? {
        val count = entry.optLong("count", -1L)
        if (count <= 0L || count > Int.MAX_VALUE.toLong()) return null
        val raw = entry.opt("value")
        val rationals =
            when {
                count == 1L && raw is JSONObject -> listOf(raw)
                raw is JSONArray && raw.length() == count.toInt() ->
                    (0 until raw.length()).mapNotNull { raw.optJSONObject(it) }
                else -> return null
            }
        if (rationals.size != count.toInt()) return null

        val values = ArrayList<Double>(rationals.size)
        for (rational in rationals) {
            val denominator = (rational.opt("denominator") as? Number)?.toLong() ?: return null
            val value = (rational.opt("value") as? Number)?.toDouble() ?: return null
            if (denominator == 0L || !value.isFinite()) return null
            values += value
        }
        return values
    }

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }
}

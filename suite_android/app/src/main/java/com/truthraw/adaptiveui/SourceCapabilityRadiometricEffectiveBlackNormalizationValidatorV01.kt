package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject
import kotlin.math.max
import kotlin.math.min

/**
 * Shadow-only DNG effective black / normalization-domain validator v0.1.
 *
 * DNG 1.7.1.0 defines per-pixel black as the sum of BlackLevel,
 * BlackLevelDeltaH and BlackLevelDeltaV, and defines the normalization scale
 * factor from WhiteLevel minus the maximum computed black level for each sample
 * plane. This validator proves only that metadata-domain relation.
 *
 * It does not read image sample payload, does not apply LinearizationTable,
 * does not normalize pixels, does not alter Evidence Ledger lifecycle/admission,
 * and does not write to Scientific Master or Appearance.
 */
object SourceCapabilityRadiometricEffectiveBlackNormalizationValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricEffectiveBlackNormalization/0.1"
    const val STATUS = "SHADOW_ONLY_EFFECTIVE_BLACK_NORMALIZATION_DOMAIN"

    const val VALIDATED = "VALIDATED_EFFECTIVE_BLACK_NORMALIZATION_DOMAIN_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_EFFECTIVE_BLACK_NORMALIZATION_DOMAIN_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_EFFECTIVE_BLACK_NORMALIZATION_DOMAIN_FAIL_CLOSED"

    private const val TAG_BLACK_LEVEL_REPEAT_DIM = 50713
    private const val TAG_BLACK_LEVEL = 50714
    private const val TAG_BLACK_LEVEL_DELTA_H = 50715
    private const val TAG_BLACK_LEVEL_DELTA_V = 50716
    private const val TAG_WHITE_LEVEL = 50717

    private data class AxisExtrema(
        val maximumByResidue: DoubleArray,
        val minimumByResidue: DoubleArray,
        val explicit: Boolean,
    )

    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) {
            "Invalid source SHA-256 binding"
        }

        val semantic =
            SourceCapabilityRadiometricSemanticValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            )
        val spatial =
            SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            )
        val semanticByIdentity = reportIndex(semantic.optJSONArray("ifds"))
        val spatialByIdentity = reportIndex(spatial.optJSONArray("ifds"))

        val results = JSONArray()
        var validatedCount = 0
        var contradictedCount = 0
        var unresolvedCount = 0

        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()
        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val rawDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            val rawStatus = rawDomain.getString("domain_status")
            val entries = ifd.optJSONArray("entries") ?: JSONArray()
            val hasFamilyOccurrence =
                entriesByTags(
                    entries,
                    setOf(
                        TAG_BLACK_LEVEL_REPEAT_DIM,
                        TAG_BLACK_LEVEL,
                        TAG_BLACK_LEVEL_DELTA_H,
                        TAG_BLACK_LEVEL_DELTA_V,
                        TAG_WHITE_LEVEL,
                    ),
                ).isNotEmpty()

            if (rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN && !hasFamilyOccurrence) {
                continue
            }

            val identity = ifdIdentity(ifd, i)
            val encoding = SourceCapabilityRadiometricEncodingConsistencyValidatorV01.classifyIfd(ifd)
            val semanticReport = semanticByIdentity[identity]
            val spatialReport = spatialByIdentity[identity]
            val result =
                classifyIfd(
                    ifd = ifd,
                    rawDomain = rawDomain,
                    encoding = encoding,
                    semanticReport = semanticReport,
                    spatialReport = spatialReport,
                    semanticFamilyPresent =
                        entriesByTags(entries, setOf(TAG_BLACK_LEVEL_REPEAT_DIM, TAG_BLACK_LEVEL, TAG_WHITE_LEVEL)).isNotEmpty(),
                )
            results.put(result)
            when (result.getString("normalization_domain_status")) {
                VALIDATED -> validatedCount += 1
                CONTRADICTED -> contradictedCount += 1
                UNRESOLVED -> unresolvedCount += 1
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "DNG_1_7_1_0_CHAPTER_5_MAPPING_RAW_VALUES_TO_LINEAR_REFERENCE_VALUES")
            .put("validated_ifd_count", validatedCount)
            .put("contradicted_ifd_count", contradictedCount)
            .put("unresolved_ifd_count", unresolvedCount)
            .put("ifds", results)
            .put("black_formula", "effective_black=BlackLevel[row_mod_repeat][col_mod_repeat][sample]+BlackLevelDeltaH[col]+BlackLevelDeltaV[row]")
            .put("normalization_denominator_formula", "WhiteLevel[sample]-max_effective_black[sample]")
            .put("linearization_table_processing_validated", false)
            .put("full_raw_to_linear_mapping_validated", false)
            .put("image_sample_payload_read", false)
            .put("record_lifecycle_effect", false)
            .put("scientific_admission_effect", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun classifyIfd(
        ifd: JSONObject,
        rawDomain: JSONObject,
        encoding: JSONObject,
        semanticReport: JSONObject?,
        spatialReport: JSONObject?,
        semanticFamilyPresent: Boolean,
    ): JSONObject {
        val rawStatus = rawDomain.getString("domain_status")
        val encodingStatus = encoding.getString("consistency_status")
        val semanticStatus = semanticReport?.optString("family_status")
        val spatialStatus = spatialReport?.optString("spatial_black_level_status")

        val prerequisiteStatus =
            when {
                rawStatus == SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN -> CONTRADICTED
                encodingStatus == SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED -> CONTRADICTED
                semanticStatus == "CONTRADICTED_FAIL_CLOSED" -> CONTRADICTED
                spatialStatus == SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.CONTRADICTED -> CONTRADICTED
                rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN -> UNRESOLVED
                encodingStatus != SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED -> UNRESOLVED
                semanticFamilyPresent && semanticReport == null -> UNRESOLVED
                semanticStatus == "UNRESOLVED_FAIL_CLOSED" -> UNRESOLVED
                spatialReport == null -> UNRESOLVED
                spatialStatus != SourceCapabilityRadiometricSpatialBlackLevelValidatorV01.VALIDATED -> UNRESOLVED
                else -> VALIDATED
            }

        val base =
            JSONObject()
                .put("ifd_path", ifd.optString("path", "UNKNOWN_IFD"))
                .put("ifd_offset", ifd.optLong("offset", -1L))
                .put("raw_ifd_domain", rawDomain)
                .put("encoding_prerequisite_status", encodingStatus)
                .put("semantic_prerequisite_status", semanticStatus ?: "DEFAULT_ONLY_NO_SEMANTIC_FAMILY_OCCURRENCE")
                .put("spatial_prerequisite_status", spatialStatus ?: "MISSING_SPATIAL_REPORT_FAIL_CLOSED")
                .put("prerequisite_status", prerequisiteStatus)
                .put("image_sample_payload_read", false)
                .put("linearization_table_processing_validated", false)
                .put("normalization_applied_to_pixels", false)
                .put("domain_validation_is_scientific_admission", false)
                .put("source_values_modified", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)

        if (prerequisiteStatus != VALIDATED) {
            return base
                .put("normalization_domain_status", prerequisiteStatus)
                .put("effective_black_values_computed", false)
                .put("normalization_denominator_validated", false)
                .put("sample_planes", JSONArray())
        }

        val entries = ifd.optJSONArray("entries") ?: JSONArray()
        val spp =
            encoding.getJSONObject("samples_per_pixel_dependency")
                .optJSONArray("values")
                ?.optLong(0, -1L)
                ?.takeIf { it > 0L }
                ?: return unresolved(base, "SAMPLES_PER_PIXEL_NOT_RESOLVED")
        if (spp > Int.MAX_VALUE.toLong()) return unresolved(base, "SAMPLES_PER_PIXEL_TOO_LARGE")

        val repeat = resolveRepeat(entries) ?: return unresolved(base, "BLACK_LEVEL_REPEAT_DIM_NOT_RESOLVED")
        val repeatRows = repeat.first
        val repeatCols = repeat.second
        if (repeatRows > Int.MAX_VALUE.toLong() || repeatCols > Int.MAX_VALUE.toLong()) {
            return unresolved(base, "BLACK_LEVEL_REPEAT_DIM_TOO_LARGE")
        }

        val blackValues = resolveBlackValues(encoding, repeatRows, repeatCols, spp)
            ?: return unresolved(base, "BLACK_LEVEL_VALUES_NOT_RESOLVED")
        val whiteValues = resolveWhiteValues(encoding, spp)
            ?: return unresolved(base, "WHITE_LEVEL_VALUES_NOT_RESOLVED")

        val active = spatialReport!!.getJSONObject("active_area_dependency")
        val activeWidth = active.optLong("active_area_width", -1L)
        val activeLength = active.optLong("active_area_length", -1L)
        if (activeWidth <= 0L || activeLength <= 0L) {
            return unresolved(base, "ACTIVE_AREA_DIMENSIONS_NOT_RESOLVED")
        }

        val deltaH = resolveDeltaValues(entries, TAG_BLACK_LEVEL_DELTA_H, activeWidth)
            ?: return unresolved(base, "BLACK_LEVEL_DELTA_H_VALUES_NOT_RESOLVED")
        val deltaV = resolveDeltaValues(entries, TAG_BLACK_LEVEL_DELTA_V, activeLength)
            ?: return unresolved(base, "BLACK_LEVEL_DELTA_V_VALUES_NOT_RESOLVED")

        val horizontal = axisExtrema(deltaH, activeWidth, repeatCols)
            ?: return unresolved(base, "HORIZONTAL_RESIDUE_EXTREMA_NOT_RESOLVED")
        val vertical = axisExtrema(deltaV, activeLength, repeatRows)
            ?: return unresolved(base, "VERTICAL_RESIDUE_EXTREMA_NOT_RESOLVED")

        val planes = JSONArray()
        var relationContradicted = false
        for (sample in 0 until spp.toInt()) {
            var maximum = Double.NEGATIVE_INFINITY
            var minimum = Double.POSITIVE_INFINITY
            for (rowResidue in vertical.maximumByResidue.indices) {
                val maxV = vertical.maximumByResidue[rowResidue]
                val minV = vertical.minimumByResidue[rowResidue]
                if (!maxV.isFinite() || !minV.isFinite()) continue
                for (colResidue in horizontal.maximumByResidue.indices) {
                    val maxH = horizontal.maximumByResidue[colResidue]
                    val minH = horizontal.minimumByResidue[colResidue]
                    if (!maxH.isFinite() || !minH.isFinite()) continue
                    val baseIndex =
                        (((rowResidue.toLong() * repeatCols) + colResidue.toLong()) * spp + sample.toLong())
                    if (baseIndex < 0L || baseIndex >= blackValues.size.toLong()) {
                        return unresolved(base, "BLACK_LEVEL_REPEAT_INDEX_OUT_OF_RANGE")
                    }
                    val baseBlack = blackValues[baseIndex.toInt()]
                    maximum = max(maximum, baseBlack + maxH + maxV)
                    minimum = min(minimum, baseBlack + minH + minV)
                }
            }
            val white = whiteValues[sample]
            val denominator = white - maximum
            val valid = maximum.isFinite() && minimum.isFinite() && white.isFinite() && denominator.isFinite() && denominator > 0.0
            if (!valid) relationContradicted = true
            planes.put(
                JSONObject()
                    .put("sample_plane", sample)
                    .put("minimum_effective_black_level", finiteOrNull(minimum))
                    .put("maximum_effective_black_level", finiteOrNull(maximum))
                    .put("white_level", finiteOrNull(white))
                    .put("normalization_denominator", finiteOrNull(denominator))
                    .put("normalization_scale_factor", if (valid) 1.0 / denominator else JSONObject.NULL)
                    .put("denominator_strictly_positive", valid),
            )
        }

        val finalStatus = if (relationContradicted) CONTRADICTED else VALIDATED
        return base
            .put("normalization_domain_status", finalStatus)
            .put("effective_black_values_computed", true)
            .put("effective_black_computation_scope", "METADATA_LATTICE_EXTREMA_ONLY_NO_IMAGE_SAMPLE_PAYLOAD")
            .put("active_area_width", activeWidth)
            .put("active_area_length", activeLength)
            .put("black_level_repeat_rows", repeatRows)
            .put("black_level_repeat_cols", repeatCols)
            .put("samples_per_pixel", spp)
            .put("black_level_delta_h_default_applied", !horizontal.explicit)
            .put("black_level_delta_v_default_applied", !vertical.explicit)
            .put("sample_planes", planes)
            .put("normalization_denominator_validated", !relationContradicted)
            .put("all_sample_planes_strictly_positive_denominator", !relationContradicted)
            .put("pixel_values_normalized", false)
    }

    private fun unresolved(base: JSONObject, reason: String): JSONObject =
        base
            .put("normalization_domain_status", UNRESOLVED)
            .put("unresolved_reason", reason)
            .put("effective_black_values_computed", false)
            .put("normalization_denominator_validated", false)
            .put("sample_planes", JSONArray())

    private fun resolveRepeat(entries: JSONArray): Pair<Long, Long>? {
        val occurrences = entriesByTag(entries, TAG_BLACK_LEVEL_REPEAT_DIM)
        if (occurrences.isEmpty()) return 1L to 1L
        if (occurrences.size != 1) return null
        val value = occurrences.single().optJSONArray("value") ?: return null
        if (value.length() != 2) return null
        val rows = (value.opt(0) as? Number)?.toLong() ?: return null
        val cols = (value.opt(1) as? Number)?.toLong() ?: return null
        if (rows <= 0L || cols <= 0L) return null
        return rows to cols
    }

    private fun resolveBlackValues(
        encoding: JSONObject,
        repeatRows: Long,
        repeatCols: Long,
        spp: Long,
    ): List<Double>? {
        val black = encoding.getJSONObject("black_level")
        val explicit = black.optJSONArray("decoded_values")
        if (explicit != null) return doubleValues(explicit)
        val defaultValue = (black.opt("effective_default") as? Number)?.toDouble() ?: return null
        val count = safeMul3(repeatRows, repeatCols, spp) ?: return null
        if (count > Int.MAX_VALUE.toLong()) return null
        return List(count.toInt()) { defaultValue }
    }

    private fun resolveWhiteValues(
        encoding: JSONObject,
        spp: Long,
    ): List<Double>? {
        val white = encoding.getJSONObject("white_level")
        val source = white.optJSONArray("decoded_values") ?: white.optJSONArray("effective_defaults") ?: return null
        val values = doubleValues(source) ?: return null
        return values.takeIf { it.size.toLong() == spp }
    }

    private fun resolveDeltaValues(
        entries: JSONArray,
        tag: Int,
        axisLength: Long,
    ): List<Double>? {
        val occurrences = entriesByTag(entries, tag)
        if (occurrences.isEmpty()) return emptyList()
        if (occurrences.size != 1) return null
        val entry = occurrences.single()
        if (entry.optLong("count", -1L) != axisLength || entry.optString("decodeStatus", "") != "DECODED") return null
        val value = entry.optJSONArray("value") ?: return null
        if (value.length().toLong() != axisLength) return null
        val out = ArrayList<Double>(value.length())
        for (i in 0 until value.length()) {
            val item = value.opt(i)
            val number =
                when (item) {
                    is Number -> item.toDouble()
                    is JSONObject -> (item.opt("value") as? Number)?.toDouble()
                    else -> null
                } ?: return null
            if (!number.isFinite()) return null
            out += number
        }
        return out
    }

    private fun axisExtrema(
        explicitValues: List<Double>,
        axisLength: Long,
        repeat: Long,
    ): AxisExtrema? {
        if (axisLength <= 0L || repeat <= 0L || repeat > Int.MAX_VALUE.toLong()) return null
        val residueCount = min(axisLength, repeat).toInt()
        if (residueCount <= 0) return null
        val maxByResidue = DoubleArray(residueCount) { Double.NEGATIVE_INFINITY }
        val minByResidue = DoubleArray(residueCount) { Double.POSITIVE_INFINITY }

        if (explicitValues.isEmpty()) {
            for (i in 0 until residueCount) {
                maxByResidue[i] = 0.0
                minByResidue[i] = 0.0
            }
            return AxisExtrema(maxByResidue, minByResidue, explicit = false)
        }
        if (explicitValues.size.toLong() != axisLength) return null
        for (index in explicitValues.indices) {
            val residue = (index.toLong() % repeat).toInt()
            if (residue >= residueCount) continue
            val value = explicitValues[index]
            maxByResidue[residue] = max(maxByResidue[residue], value)
            minByResidue[residue] = min(minByResidue[residue], value)
        }
        if (maxByResidue.any { !it.isFinite() } || minByResidue.any { !it.isFinite() }) return null
        return AxisExtrema(maxByResidue, minByResidue, explicit = true)
    }

    private fun doubleValues(array: JSONArray): List<Double>? {
        val out = ArrayList<Double>(array.length())
        for (i in 0 until array.length()) {
            val item = array.opt(i)
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

    private fun reportIndex(array: JSONArray?): Map<String, JSONObject> {
        if (array == null) return emptyMap()
        val out = LinkedHashMap<String, JSONObject>()
        for (i in 0 until array.length()) {
            val report = array.optJSONObject(i) ?: continue
            val key = ifdIdentity(report, i)
            if (key !in out) out[key] = report
        }
        return out
    }

    private fun ifdIdentity(ifd: JSONObject, fallbackIndex: Int): String =
        "${ifd.optString("ifd_path", ifd.optString("path", "IFD[$fallbackIndex]"))}|${ifd.optLong("ifd_offset", ifd.optLong("offset", -1L))}"

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }

    private fun entriesByTags(entries: JSONArray, tags: Set<Int>): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) in tags }

    private fun safeMul3(a: Long, b: Long, c: Long): Long? {
        if (a <= 0L || b <= 0L || c <= 0L) return null
        if (a > Long.MAX_VALUE / b) return null
        val ab = a * b
        if (ab > Long.MAX_VALUE / c) return null
        return ab * c
    }

    private fun finiteOrNull(value: Double): Any = if (value.isFinite()) value else JSONObject.NULL
}

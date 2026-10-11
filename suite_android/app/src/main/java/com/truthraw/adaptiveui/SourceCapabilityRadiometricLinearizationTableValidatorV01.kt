package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only DNG Raw-IFD LinearizationTable metadata-contract validator v0.1.
 *
 * This validator consumes only physical TIFF/DNG entry occurrences already
 * exposed by DngContainerMetadataParser. It deliberately keys the explicit
 * source occurrence by tag number 50712 so this step does not rename existing
 * ledger records or change Observation Evidence Ledger identity.
 *
 * It validates metadata dependencies only. It never reads image sample payload,
 * never applies the table to source samples, never changes lifecycle/admission,
 * and never writes to Scientific Master or Appearance.
 */
object SourceCapabilityRadiometricLinearizationTableValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRadiometricLinearizationTable/0.1"
    const val STATUS = "SHADOW_ONLY_LINEARIZATION_TABLE_METADATA_CONTRACT"

    const val VALIDATED = "VALIDATED_LINEARIZATION_TABLE_METADATA_CONTRACT_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_LINEARIZATION_TABLE_METADATA_CONTRACT_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_LINEARIZATION_TABLE_METADATA_CONTRACT_FAIL_CLOSED"

    private const val TAG_LINEARIZATION_TABLE = 50712
    private const val TIFF_TYPE_SHORT = 3
    private const val SAMPLE_FORMAT_UNSIGNED = 1L
    private const val SAMPLE_FORMAT_FLOAT = 3L
    private const val MIN_TABLE_COUNT = 2L
    private const val MAX_TABLE_COUNT = 65536L

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
            val occurrences = entriesByTag(entries, TAG_LINEARIZATION_TABLE)

            if (rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN &&
                occurrences.isEmpty()
            ) {
                continue
            }

            val result = classifyIfd(ifd)
            results.put(result)
            when (result.getString("linearization_table_status")) {
                VALIDATED -> validated += 1
                CONTRADICTED -> contradicted += 1
                UNRESOLVED -> unresolved += 1
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "ADOBE_DNG_1_7_1_0_PLUS_DNG_SDK_VALIDATION")
            .put("validated_ifd_count", validated)
            .put("contradicted_ifd_count", contradicted)
            .put("unresolved_ifd_count", unresolved)
            .put("ifds", results)
            .put("tag_number", TAG_LINEARIZATION_TABLE)
            .put("explicit_tag_type_required", "SHORT")
            .put("explicit_count_min", MIN_TABLE_COUNT)
            .put("explicit_count_max", MAX_TABLE_COUNT)
            .put("table_count_must_equal_two_pow_bits_per_sample", false)
            .put("single_table_applies_to_all_sample_planes", true)
            .put("identity_default_mints_source_occurrence", false)
            .put("parser_tag_rename_performed", false)
            .put("observation_evidence_ledger_identity_changed", false)
            .put("table_application_to_image_samples_validated", false)
            .put("linearization_applied_to_image_samples", false)
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

    internal fun classifyIfd(ifd: JSONObject): JSONObject {
        val entries = ifd.optJSONArray("entries") ?: JSONArray()
        val occurrences = entriesByTag(entries, TAG_LINEARIZATION_TABLE)
        val rawDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
        val rawStatus = rawDomain.getString("domain_status")
        val encoding = SourceCapabilityRadiometricEncodingConsistencyValidatorV01.classifyIfd(ifd)
        val encodingStatus = encoding.getString("consistency_status")

        val selection =
            when {
                occurrences.isEmpty() -> "ABSENT_SPEC_IDENTITY_DEFAULT_NO_SOURCE_OCCURRENCE"
                occurrences.size == 1 -> "UNIQUE_PHYSICAL_OCCURRENCE_BY_TAG_NUMBER_50712"
                else -> "MULTIPLE_PHYSICAL_OCCURRENCES_NO_SELECTION_FAIL_CLOSED"
            }

        val base = JSONObject()
            .put("ifd_path", ifd.optString("path", "UNKNOWN_IFD"))
            .put("ifd_offset", ifd.optLong("offset", -1L))
            .put("raw_ifd_domain", rawDomain)
            .put("radiometric_encoding_consistency", encoding)
            .put("selection_status", selection)
            .put("occurrence_count", occurrences.size)
            .put("identity_default_applied", occurrences.isEmpty())
            .put("identity_default_mints_source_occurrence", false)
            .put("explicit_table_applied_to_pixels", false)
            .put("domain_validation_is_scientific_admission", false)
            .put("record_lifecycle_effect", false)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)

        if (rawStatus == SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "LINEARIZATION_TABLE_PRESENT_OR_EVALUATED_OUTSIDE_VALIDATED_RAW_IFD_DOMAIN")
        }
        if (rawStatus != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN) {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "RAW_IFD_DOMAIN_NOT_VALIDATED")
        }

        if (occurrences.size > 1) {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "DUPLICATE_TAG_50712_NO_SILENT_SELECTION")
        }

        if (encodingStatus == SourceCapabilityRadiometricEncodingConsistencyValidatorV01.CONTRADICTED) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "RADIOMETRIC_ENCODING_CONTRACT_CONTRADICTED")
        }
        if (encodingStatus != SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED) {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "RADIOMETRIC_ENCODING_CONTRACT_NOT_VALIDATED")
        }

        val bits = dependencyLongValues(encoding, "bits_per_sample_dependency")
        val formats = dependencyLongValues(encoding, "sample_format_dependency")
        if (bits == null || formats == null || bits.isEmpty() || bits.size != formats.size) {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "ENCODING_DEPENDENCY_VALUES_UNRESOLVED")
        }

        if (occurrences.isEmpty()) {
            return base
                .put("linearization_table_status", VALIDATED)
                .put("reason", "DNG_SPEC_IDENTITY_TABLE_DEFAULT")
                .put("effective_mapping", "IDENTITY_TABLE_SPEC_DEFAULT")
                .put("table_value_count", JSONObject.NULL)
                .put("table_value_min", JSONObject.NULL)
                .put("table_value_max", JSONObject.NULL)
                .put("table_first_value", JSONObject.NULL)
                .put("table_last_value", JSONObject.NULL)
                .put("out_of_table_range_behavior", "NOT_APPLICABLE_IDENTITY_DEFAULT")
        }

        val entry = occurrences.single()
        if (entry.optInt("type", -1) != TIFF_TYPE_SHORT) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "EXPLICIT_LINEARIZATION_TABLE_TYPE_MUST_BE_SHORT")
                .put("observed_type", entry.optInt("type", -1))
        }

        val count = entry.optLong("count", -1L)
        if (count !in MIN_TABLE_COUNT..MAX_TABLE_COUNT) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "EXPLICIT_LINEARIZATION_TABLE_COUNT_OUTSIDE_2_TO_65536")
                .put("observed_count", count)
        }

        if (formats.any { it == SAMPLE_FORMAT_FLOAT }) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "LINEARIZATION_TABLE_NOT_ALLOWED_FOR_FLOATING_POINT_RAW")
        }
        if (formats.any { it != SAMPLE_FORMAT_UNSIGNED }) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "LINEARIZATION_TABLE_REQUIRES_UNSIGNED_INTEGER_RAW_ENCODING")
        }
        if (bits.any { it > 16L }) {
            return base
                .put("linearization_table_status", CONTRADICTED)
                .put("reason", "LINEARIZATION_TABLE_NOT_ALLOWED_ABOVE_16_BITS_PER_SAMPLE")
        }

        if (entry.optString("decodeStatus", "") != "DECODED") {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "EXPLICIT_LINEARIZATION_TABLE_NOT_DECODED")
                .put("decode_status", entry.optString("decodeStatus", "UNKNOWN"))
        }

        val values = shortValues(entry)
        if (values == null || values.size.toLong() != count) {
            return base
                .put("linearization_table_status", UNRESOLVED)
                .put("reason", "EXPLICIT_LINEARIZATION_TABLE_VALUE_CARDINALITY_OR_VALUE_DOMAIN_UNRESOLVED")
        }

        return base
            .put("linearization_table_status", VALIDATED)
            .put("reason", "EXPLICIT_LINEARIZATION_TABLE_METADATA_CONTRACT_VALIDATED")
            .put("effective_mapping", "TAG_50712_LOOKUP_TABLE_METADATA_ONLY")
            .put("table_value_count", values.size)
            .put("table_value_min", values.minOrNull()!!)
            .put("table_value_max", values.maxOrNull()!!)
            .put("table_first_value", values.first())
            .put("table_last_value", values.last())
            .put("out_of_table_range_behavior", "MAP_TO_LAST_TABLE_ENTRY_PER_DNG_PROCESSING_MODEL")
            .put("table_count_equal_two_pow_bits_per_sample_required", false)
            .put("table_value_monotonicity_enforced", false)
    }

    private fun dependencyLongValues(
        encoding: JSONObject,
        dependencyKey: String,
    ): List<Long>? {
        val dependency = encoding.optJSONObject(dependencyKey) ?: return null
        val values = dependency.optJSONArray("values") ?: return null
        val out = ArrayList<Long>(values.length())
        for (i in 0 until values.length()) {
            val value = values.opt(i) as? Number ?: return null
            out += value.toLong()
        }
        return out
    }

    private fun shortValues(entry: JSONObject): List<Long>? {
        val count = entry.optLong("count", -1L)
        if (count < 0L || count > Int.MAX_VALUE.toLong()) return null
        val raw = entry.opt("value")
        val values = when (raw) {
            is JSONArray -> {
                val out = ArrayList<Any?>(raw.length())
                for (i in 0 until raw.length()) out += raw.opt(i)
                out
            }
            else -> listOf(raw)
        }
        if (values.size.toLong() != count) return null

        val out = ArrayList<Long>(values.size)
        for (item in values) {
            val number = item as? Number ?: return null
            val asDouble = number.toDouble()
            if (!asDouble.isFinite()) return null
            val asLong = number.toLong()
            if (asDouble != asLong.toDouble()) return null
            if (asLong !in 0L..65535L) return null
            out += asLong
        }
        return out
    }

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> {
        val out = mutableListOf<JSONObject>()
        for (i in 0 until entries.length()) {
            val entry = entries.optJSONObject(i) ?: continue
            if (entry.optInt("tag", -1) == tag) out += entry
        }
        return out
    }
}

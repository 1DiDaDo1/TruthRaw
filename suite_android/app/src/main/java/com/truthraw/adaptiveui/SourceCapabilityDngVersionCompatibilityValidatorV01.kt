package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only declared-DNG-version compatibility validator v0.1.
 *
 * Scope is intentionally narrow: DNGVersion / DNGBackwardVersion structure,
 * reader-version ceiling, and the DNG 1.4.0.0 feature floor for floating-point
 * raw image data already validated by the existing encoding-consistency layer.
 * It does not establish whole-file DNG conformance or scientific admission.
 */
object SourceCapabilityDngVersionCompatibilityValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityDngVersionCompatibility/0.1"
    const val STATUS = "SHADOW_ONLY_DECLARED_DNG_VERSION_COMPATIBILITY"

    const val VALIDATED = "VALIDATED_SCOPED_DNG_VERSION_COMPATIBILITY_SHADOW_ONLY"
    const val CONTRADICTED = "CONTRADICTED_SCOPED_DNG_VERSION_COMPATIBILITY_FAIL_CLOSED"
    const val UNRESOLVED = "UNRESOLVED_SCOPED_DNG_VERSION_COMPATIBILITY_FAIL_CLOSED"

    private const val TAG_DNG_VERSION = 50706
    private const val TAG_DNG_BACKWARD_VERSION = 50707
    private const val TIFF_TYPE_BYTE = 1
    private const val SAMPLE_FORMAT_FLOAT = 3L

    private val FIRST_DNG_VERSION = Version(1, 0, 0, 0)
    private val FLOAT_MIN_VERSION = Version(1, 4, 0, 0)
    private val READER_SPEC_VERSION = Version(1, 7, 1, 0)

    private data class Version(val a: Int, val b: Int, val c: Int, val d: Int) : Comparable<Version> {
        override fun compareTo(other: Version): Int {
            val left = intArrayOf(a, b, c, d)
            val right = intArrayOf(other.a, other.b, other.c, other.d)
            for (i in 0..3) {
                if (left[i] != right[i]) return left[i].compareTo(right[i])
            }
            return 0
        }

        fun zeroTail(): Version = Version(a, b, 0, 0)
        fun text(): String = "$a.$b.$c.$d"
        fun json(): JSONArray = JSONArray().put(a).put(b).put(c).put(d)
    }

    private data class VersionDependency(
        val status: String,
        val version: Version?,
        val occurrenceCount: Int,
        val basis: String,
        val outcome: String,
    ) {
        fun toJson(): JSONObject =
            JSONObject()
                .put("status", status)
                .put("version", version?.json() ?: JSONObject.NULL)
                .put("version_text", version?.text() ?: JSONObject.NULL)
                .put("occurrence_count", occurrenceCount)
                .put("basis", basis)
                .put("contract_outcome", outcome)
    }

    fun describe(
        sourceSha256: String,
        parsedContainerMetadata: JSONObject,
    ): JSONObject {
        require(sourceSha256.matches(Regex("[0-9a-fA-F]{64}"))) { "Invalid source SHA-256 binding" }

        val ifds = parsedContainerMetadata.optJSONArray("ifds") ?: JSONArray()
        val ifd0s =
            (0 until ifds.length())
                .mapNotNull { ifds.optJSONObject(it) }
                .filter { it.optString("path", "") == "IFD0" }

        val dngVersion = resolveDngVersion(ifd0s)
        val backwardVersion = resolveBackwardVersion(ifd0s, dngVersion)
        val features = JSONArray()
        var featureContradiction = false
        var featureUnresolved = false

        for (i in 0 until ifds.length()) {
            val ifd = ifds.optJSONObject(i) ?: continue
            val rawDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            if (rawDomain.getString("domain_status") != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN) {
                continue
            }

            val encoding = SourceCapabilityRadiometricEncodingConsistencyValidatorV01.classifyIfd(ifd)
            val encodingStatus = encoding.getString("consistency_status")
            val formats = encoding.getJSONObject("sample_format_dependency").optJSONArray("values")
            val usesFloat = formats != null && (0 until formats.length()).any { formats.optLong(it, -1L) == SAMPLE_FORMAT_FLOAT }
            if (!usesFloat) continue

            val declaredVersionStatus =
                when {
                    dngVersion.version == null -> UNRESOLVED
                    dngVersion.version < FLOAT_MIN_VERSION -> CONTRADICTED
                    else -> VALIDATED
                }

            val mainRelationProven = ifd.optString("path", "") == "IFD0"
            val backwardStatus =
                when {
                    !mainRelationProven -> "UNRESOLVED_MAIN_IMAGE_RELATION_NOT_PROVEN"
                    backwardVersion.version == null -> UNRESOLVED
                    backwardVersion.version < FLOAT_MIN_VERSION -> CONTRADICTED
                    else -> VALIDATED
                }

            if (encodingStatus != SourceCapabilityRadiometricEncodingConsistencyValidatorV01.VALIDATED) {
                featureUnresolved = true
            }
            if (declaredVersionStatus == CONTRADICTED || backwardStatus == CONTRADICTED) featureContradiction = true
            if (declaredVersionStatus == UNRESOLVED || backwardStatus.startsWith("UNRESOLVED")) featureUnresolved = true

            features.put(
                JSONObject()
                    .put("feature", "FLOATING_POINT_RAW_IMAGE_DATA")
                    .put("ifd_path", ifd.optString("path", "UNKNOWN_IFD"))
                    .put("ifd_offset", ifd.optLong("offset", -1L))
                    .put("minimum_dng_version", FLOAT_MIN_VERSION.text())
                    .put("declared_dng_version_floor_status", declaredVersionStatus)
                    .put("main_image_relation_proven", mainRelationProven)
                    .put("main_image_relation_basis", if (mainRelationProven) "VALIDATED_RAW_IFD_IS_IFD0" else "NOT_PROVEN_NO_FIRST_RAW_IFD_HEURISTIC")
                    .put("backward_version_floor_status", backwardStatus)
                    .put("encoding_prerequisite_status", encodingStatus)
                    .put("feature_detection_is_scientific_admission", false),
            )
        }

        val readerCompatibilityStatus =
            when {
                backwardVersion.version == null -> UNRESOLVED
                backwardVersion.version > READER_SPEC_VERSION -> CONTRADICTED
                else -> VALIDATED
            }

        val overall =
            when {
                dngVersion.outcome == CONTRADICTED || backwardVersion.outcome == CONTRADICTED -> CONTRADICTED
                readerCompatibilityStatus == CONTRADICTED || featureContradiction -> CONTRADICTED
                dngVersion.outcome == UNRESOLVED || backwardVersion.outcome == UNRESOLVED -> UNRESOLVED
                readerCompatibilityStatus == UNRESOLVED || featureUnresolved -> UNRESOLVED
                else -> VALIDATED
            }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "DNG_1_7_1_0")
            .put("compatibility_status", overall)
            .put("dng_version", dngVersion.toJson())
            .put("dng_backward_version", backwardVersion.toJson())
            .put("reader_spec_version", READER_SPEC_VERSION.text())
            .put("reader_backward_compatibility_status", readerCompatibilityStatus)
            .put("scoped_features", features)
            .put("floating_point_feature_floor_validated", true)
            .put("compression_feature_floors_validated", false)
            .put("opcode_feature_floors_validated", false)
            .put("whole_file_dng_conformance_validated", false)
            .put("future_unknown_feature_set_exhaustively_validated", false)
            .put("main_image_ifd_inferred_by_first_raw_candidate", false)
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

    private fun resolveDngVersion(ifd0s: List<JSONObject>): VersionDependency {
        if (ifd0s.size != 1) {
            return VersionDependency(
                status = "IFD0_IDENTITY_UNRESOLVED_FAIL_CLOSED",
                version = null,
                occurrenceCount = 0,
                basis = "EXACTLY_ONE_IFD0_REQUIRED",
                outcome = UNRESOLVED,
            )
        }
        val occurrences = entriesByTag(ifd0s.single(), TAG_DNG_VERSION)
        if (occurrences.isEmpty()) {
            return VersionDependency(
                status = "MISSING_REQUIRED_DNG_VERSION_FAIL_CLOSED",
                version = null,
                occurrenceCount = 0,
                basis = "DNG_VERSION_IS_REQUIRED_IN_IFD0",
                outcome = CONTRADICTED,
            )
        }
        if (occurrences.size != 1) {
            return VersionDependency(
                status = "AMBIGUOUS_MULTIPLE_DNG_VERSION_OCCURRENCES_FAIL_CLOSED",
                version = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                outcome = UNRESOLVED,
            )
        }
        val version = decodeVersionOccurrence(occurrences.single())
        if (version == null) {
            return VersionDependency(
                status = "INVALID_DNG_VERSION_TYPE_COUNT_OR_DECODE_FAIL_CLOSED",
                version = null,
                occurrenceCount = 1,
                basis = "BYTE_X4_DECODED_REQUIRED",
                outcome = CONTRADICTED,
            )
        }
        if (version < FIRST_DNG_VERSION) {
            return VersionDependency(
                status = "DECLARED_DNG_VERSION_BEFORE_FIRST_PUBLISHED_VERSION_FAIL_CLOSED",
                version = version,
                occurrenceCount = 1,
                basis = "DNG_FIRST_PUBLISHED_VERSION_1_0_0_0",
                outcome = CONTRADICTED,
            )
        }
        return VersionDependency(
            status = "RESOLVED_EXPLICIT_REQUIRED_SOURCE_OCCURRENCE",
            version = version,
            occurrenceCount = 1,
            basis = "EXACT_IFD0_OCCURRENCE",
            outcome = VALIDATED,
        )
    }

    private fun resolveBackwardVersion(
        ifd0s: List<JSONObject>,
        dngVersion: VersionDependency,
    ): VersionDependency {
        if (ifd0s.size != 1) {
            return VersionDependency("IFD0_IDENTITY_UNRESOLVED_FAIL_CLOSED", null, 0, "EXACTLY_ONE_IFD0_REQUIRED", UNRESOLVED)
        }
        val occurrences = entriesByTag(ifd0s.single(), TAG_DNG_BACKWARD_VERSION)
        if (occurrences.isEmpty()) {
            val declared = dngVersion.version
                ?: return VersionDependency("DNG_VERSION_UNRESOLVED_SO_DEFAULT_UNRESOLVED", null, 0, "DEFAULT_DEPENDS_ON_DNG_VERSION", UNRESOLVED)
            return VersionDependency(
                status = "RESOLVED_SPEC_DEFAULT_FROM_DNG_VERSION_ZERO_TAIL",
                version = declared.zeroTail(),
                occurrenceCount = 0,
                basis = "DNG_SPEC_DEFAULT_NOT_SOURCE_OCCURRENCE",
                outcome = VALIDATED,
            )
        }
        if (occurrences.size != 1) {
            return VersionDependency(
                status = "AMBIGUOUS_MULTIPLE_DNG_BACKWARD_VERSION_OCCURRENCES_FAIL_CLOSED",
                version = null,
                occurrenceCount = occurrences.size,
                basis = "NO_SILENT_DUPLICATE_SELECTION",
                outcome = UNRESOLVED,
            )
        }
        val version = decodeVersionOccurrence(occurrences.single())
        return if (version != null) {
            VersionDependency("RESOLVED_EXPLICIT_SOURCE_OCCURRENCE", version, 1, "EXACT_IFD0_OCCURRENCE", VALIDATED)
        } else {
            VersionDependency("INVALID_DNG_BACKWARD_VERSION_TYPE_COUNT_OR_DECODE_FAIL_CLOSED", null, 1, "BYTE_X4_DECODED_REQUIRED", CONTRADICTED)
        }
    }

    private fun decodeVersionOccurrence(entry: JSONObject): Version? {
        if (entry.optInt("type", -1) != TIFF_TYPE_BYTE || entry.optLong("count", -1L) != 4L) return null
        if (entry.optString("decodeStatus", "") != "DECODED") return null
        val values = entry.optJSONArray("value") ?: return null
        if (values.length() != 4) return null
        val bytes = IntArray(4)
        for (i in 0..3) {
            val v = values.optInt(i, -1)
            if (v !in 0..255) return null
            bytes[i] = v
        }
        return Version(bytes[0], bytes[1], bytes[2], bytes[3])
    }

    private fun entriesByTag(ifd: JSONObject, tag: Int): List<JSONObject> {
        val entries = ifd.optJSONArray("entries") ?: JSONArray()
        return (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }
    }
}

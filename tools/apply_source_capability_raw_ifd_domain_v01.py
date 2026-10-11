#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOMAIN = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRawIfdDomainValidatorV01.kt"
ENVELOPE = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt"
RADIOMETRIC = ROOT / "suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricSemanticValidatorV01.kt"

domain_source = r'''package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Shadow-only DNG Raw-IFD format-domain validator v0.1.
 *
 * This layer classifies TIFF/DNG IFD structure only. It never establishes
 * scientific source admission and never reads or modifies image samples.
 * Specification defaults may resolve format dependencies but never mint a
 * physical source occurrence.
 */
object SourceCapabilityRawIfdDomainValidatorV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRawIfdDomainValidation/0.1"
    const val STATUS = "SHADOW_ONLY_FORMAT_DOMAIN_VALIDATION"

    const val VALIDATED_RAW_DOMAIN = "VALIDATED_RAW_IFD_FORMAT_DOMAIN_SHADOW_ONLY"
    const val CONTRADICTED_NON_RAW_DOMAIN = "CONTRADICTED_NOT_RAW_IFD_FORMAT_DOMAIN"
    const val UNRESOLVED_RAW_DOMAIN = "UNRESOLVED_RAW_IFD_FORMAT_DOMAIN_FAIL_CLOSED"

    private const val TAG_NEW_SUBFILE_TYPE = 254
    private const val TAG_PHOTOMETRIC_INTERPRETATION = 262
    private const val TAG_CFA_REPEAT_PATTERN_DIM = 33421
    private const val TAG_CFA_PATTERN = 33422

    private const val TIFF_TYPE_SHORT = 3
    private const val TIFF_TYPE_LONG = 4

    private const val PHOTOMETRIC_CFA = 32803L
    private const val PHOTOMETRIC_LINEAR_RAW = 34892L

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
            val result = classifyIfd(ifd)
            results.put(result)
            when (result.getString("domain_status")) {
                VALIDATED_RAW_DOMAIN -> validated += 1
                CONTRADICTED_NON_RAW_DOMAIN -> contradicted += 1
                UNRESOLVED_RAW_DOMAIN -> unresolved += 1
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", STATUS)
            .put("source_sha256", sourceSha256.lowercase())
            .put("specification_anchor", "DNG_1_7_1_0_PLUS_TIFF_6_0")
            .put("validated_raw_ifd_count", validated)
            .put("contradicted_non_raw_ifd_count", contradicted)
            .put("unresolved_ifd_count", unresolved)
            .put("ifds", results)
            .put("defaults_mint_source_occurrence", false)
            .put("duplicate_occurrence_selection_allowed", false)
            .put("scientific_admission_effect", false)
            .put("pixel_route_effect", false)
            .put("scientific_master_pixel_effect", false)
            .put("appearance_pixel_effect", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    internal fun classifyIfd(ifd: JSONObject): JSONObject {
        val entries = ifd.optJSONArray("entries") ?: JSONArray()
        val newSubfileType = resolveNewSubfileType(entries)
        val photometric = resolvePhotometricInterpretation(entries)
        val cfaRepeatCount = entriesByTag(entries, TAG_CFA_REPEAT_PATTERN_DIM).size
        val cfaPatternCount = entriesByTag(entries, TAG_CFA_PATTERN).size
        val cfaDependencyStatus =
            when {
                photometric.value != PHOTOMETRIC_CFA -> "NOT_APPLICABLE"
                cfaRepeatCount == 1 && cfaPatternCount == 1 ->
                    "PRESENT_UNIQUE_SAME_IFD_NOT_SEMANTICALLY_VALIDATED"
                cfaRepeatCount > 1 || cfaPatternCount > 1 ->
                    "AMBIGUOUS_DUPLICATE_CFA_DEPENDENCY_FAIL_CLOSED"
                else -> "MISSING_REQUIRED_CFA_DEPENDENCY_FAIL_CLOSED"
            }

        val domainStatus =
            when {
                !newSubfileType.resolved || !photometric.resolved -> UNRESOLVED_RAW_DOMAIN
                newSubfileType.value != 0L -> CONTRADICTED_NON_RAW_DOMAIN
                photometric.value == PHOTOMETRIC_CFA &&
                    cfaDependencyStatus == "PRESENT_UNIQUE_SAME_IFD_NOT_SEMANTICALLY_VALIDATED" ->
                    VALIDATED_RAW_DOMAIN
                photometric.value == PHOTOMETRIC_CFA -> CONTRADICTED_NON_RAW_DOMAIN
                photometric.value == PHOTOMETRIC_LINEAR_RAW -> VALIDATED_RAW_DOMAIN
                else -> CONTRADICTED_NON_RAW_DOMAIN
            }

        val reason =
            when (domainStatus) {
                VALIDATED_RAW_DOMAIN ->
                    if (photometric.value == PHOTOMETRIC_CFA) {
                        "FULL_RESOLUTION_CFA_RAW_FORMAT_DOMAIN_WITH_REQUIRED_SAME_IFD_CFA_TAGS_PRESENT"
                    } else {
                        "FULL_RESOLUTION_LINEAR_RAW_FORMAT_DOMAIN"
                    }
                CONTRADICTED_NON_RAW_DOMAIN ->
                    when {
                        newSubfileType.value != null && newSubfileType.value != 0L ->
                            "NEW_SUBFILE_TYPE_IS_NOT_FULL_RESOLUTION_RAW"
                        photometric.value == PHOTOMETRIC_CFA &&
                            cfaDependencyStatus != "PRESENT_UNIQUE_SAME_IFD_NOT_SEMANTICALLY_VALIDATED" ->
                            "CFA_PHOTOMETRIC_REQUIRES_UNIQUE_SAME_IFD_CFA_DEPENDENCIES"
                        else -> "PHOTOMETRIC_INTERPRETATION_NOT_BOUND_TO_RAW_IFD_FORMAT_DOMAIN"
                    }
                else -> "REQUIRED_FORMAT_DOMAIN_DEPENDENCY_UNRESOLVED"
            }

        return JSONObject()
            .put("ifd_path", ifd.optString("path", "UNKNOWN_IFD"))
            .put("ifd_offset", ifd.optLong("offset", -1L))
            .put("domain_status", domainStatus)
            .put("reason", reason)
            .put("new_subfile_type_dependency", newSubfileType.toJson())
            .put("photometric_interpretation_dependency", photometric.toJson())
            .put("cfa_repeat_pattern_dim_occurrence_count", cfaRepeatCount)
            .put("cfa_pattern_occurrence_count", cfaPatternCount)
            .put("cfa_same_ifd_dependency_status", cfaDependencyStatus)
            .put("domain_validation_is_scientific_admission", false)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun resolveNewSubfileType(entries: JSONArray): ScalarDependency {
        val occurrences = entriesByTag(entries, TAG_NEW_SUBFILE_TYPE)
        if (occurrences.isEmpty()) {
            return ScalarDependency(
                status = "RESOLVED_TIFF_DEFAULT_0",
                value = 0L,
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
            entry.optInt("type", -1) == TIFF_TYPE_LONG &&
                entry.optLong("count", -1L) == 1L &&
                entry.optString("decodeStatus", "") == "DECODED" &&
                value != null && value >= 0L
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

    private fun resolvePhotometricInterpretation(entries: JSONArray): ScalarDependency {
        val occurrences = entriesByTag(entries, TAG_PHOTOMETRIC_INTERPRETATION)
        if (occurrences.isEmpty()) {
            return ScalarDependency(
                status = "MISSING_REQUIRED_DOMAIN_DEPENDENCY_FAIL_CLOSED",
                value = null,
                occurrenceCount = 0,
                basis = "NO_DEFAULT_USED_FOR_RAW_DOMAIN_CLASSIFICATION",
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
                value != null
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

    private fun entriesByTag(entries: JSONArray, tag: Int): List<JSONObject> =
        (0 until entries.length())
            .mapNotNull { entries.optJSONObject(it) }
            .filter { it.optInt("tag", -1) == tag }
}
'''

if DOMAIN.exists():
    existing = DOMAIN.read_text()
    if "SourceCapabilityRawIfdDomainValidatorV01" not in existing:
        raise SystemExit(f"Unexpected existing domain validator file: {DOMAIN}")
else:
    DOMAIN.write_text(domain_source)

text = ENVELOPE.read_text()
marker = 'raw_ifd_domain_validation_v0_1'
if marker not in text:
    old = '''        return envelope.put(
            "radiometric_semantic_validation_v0_1",
            SourceCapabilityRadiometricSemanticValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
'''
    new = '''        envelope.put(
            "raw_ifd_domain_validation_v0_1",
            SourceCapabilityRawIfdDomainValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
        return envelope.put(
            "radiometric_semantic_validation_v0_1",
            SourceCapabilityRadiometricSemanticValidatorV01.describe(
                sourceSha256 = sourceSha256,
                parsedContainerMetadata = parsedContainerMetadata,
            ),
        )
'''
    if old not in text:
        raise SystemExit("SourceCapabilityEnvelopeV02 semantic attachment anchor not found")
    ENVELOPE.write_text(text.replace(old, new, 1))

text = RADIOMETRIC.read_text()
if 'SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)' not in text:
    old = '''            val samplesPerPixel = resolveSamplesPerPixel(entries)
            val repeatDim = resolveBlackLevelRepeatDim(entries)
            val occurrenceReports = JSONArray()
'''
    new = '''            val samplesPerPixel = resolveSamplesPerPixel(entries)
            val repeatDim = resolveBlackLevelRepeatDim(entries)
            val rawIfdDomain = SourceCapabilityRawIfdDomainValidatorV01.classifyIfd(ifd)
            val occurrenceReports = JSONArray()
'''
    if old not in text:
        raise SystemExit("Radiometric dependency anchor not found")
    text = text.replace(old, new, 1)

    old = '''            val familyStatus =
                when {
                    occurrenceReportsHasStatus(occurrenceReports, CONTRADICTED) ->
                        "CONTRADICTED_FAIL_CLOSED"
                    occurrenceReportsHasStatus(occurrenceReports, DEPENDENCY_UNRESOLVED) ||
                        repeatSelection.startsWith("MULTIPLE_") ||
                        blackSelection.startsWith("MULTIPLE_") ||
                        whiteSelection.startsWith("MULTIPLE_") ->
                        "UNRESOLVED_FAIL_CLOSED"
                    else -> "VALIDATED_SHADOW_ONLY"
                }
'''
    new = '''            val rawIfdDomainStatus = rawIfdDomain.getString("domain_status")
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
'''
    if old not in text:
        raise SystemExit("Radiometric family-status anchor not found")
    text = text.replace(old, new, 1)

    old = '''                    .put("family_status", familyStatus)
                    .put("raw_ifd_usage_status", "NOT_INDEPENDENTLY_VALIDATED_V0_1")
                    .put("samples_per_pixel_dependency", samplesPerPixel.toJson())
'''
    new = '''                    .put("family_status", familyStatus)
                    .put("raw_ifd_usage_status", rawIfdUsageStatus(rawIfdDomainStatus))
                    .put("raw_ifd_domain", rawIfdDomain)
                    .put("samples_per_pixel_dependency", samplesPerPixel.toJson())
'''
    if old not in text:
        raise SystemExit("Radiometric raw-IFD usage anchor not found")
    text = text.replace(old, new, 1)

    helper_anchor = '''    private fun occurrenceReportsHasStatus(reports: JSONArray, status: String): Boolean =
'''
    helper = '''    private fun rawIfdUsageStatus(domainStatus: String): String =
        when (domainStatus) {
            SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN ->
                "VALIDATED_RAW_IFD_FORMAT_USAGE_SHADOW_ONLY"
            SourceCapabilityRawIfdDomainValidatorV01.CONTRADICTED_NON_RAW_DOMAIN ->
                "CONTRADICTED_NON_RAW_IFD_USAGE_FAIL_CLOSED"
            else -> "UNRESOLVED_RAW_IFD_USAGE_FAIL_CLOSED"
        }

'''
    if helper_anchor not in text:
        raise SystemExit("Radiometric helper anchor not found")
    text = text.replace(helper_anchor, helper + helper_anchor, 1)
    RADIOMETRIC.write_text(text)

print("SOURCE_CAPABILITY_RAW_IFD_DOMAIN_V01_PATCH_APPLIED")

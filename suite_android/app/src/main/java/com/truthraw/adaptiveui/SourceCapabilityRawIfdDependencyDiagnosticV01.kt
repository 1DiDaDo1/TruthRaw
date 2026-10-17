package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only per-source diagnostic assembled exclusively from the existing
 * Source Capability evidence envelope and its already validated Raw-IFD
 * identity. It never reparses the source, reads image samples, or promotes
 * parser facts into scientific authority.
 */
object SourceCapabilityRawIfdDependencyDiagnosticV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRawIfdDependencyDiagnostic/0.1"
    const val KEY = "source_capability_raw_ifd_dependency_diagnostic_v0_1"
    const val STATUS = "READ_ONLY_SOURCE_BOUND_DIAGNOSTIC"

    private val SHA256 = Regex("[0-9a-f]{64}")

    private data class Dependency(
        val tag: Int,
        val name: String,
    )

    private val dependencies =
        listOf(
            Dependency(50712, "LinearizationTable"),
            Dependency(50715, "BlackLevelDeltaH"),
            Dependency(50716, "BlackLevelDeltaV"),
        )

    fun attach(
        profile: JSONObject,
        authority: SourceCapabilityRawIfdRuntimeAuthorityV01? =
            SourceCapabilityRawIfdRuntimeAuthorityBinderV01.extract(profile),
    ): JSONObject {
        val sourceSha256 =
            profile.optString("source_sha256", "")
                .trim()
                .lowercase()
        val sourceBindingValid = sourceSha256.matches(SHA256)
        val authoritySourceBound =
            authority != null &&
                sourceBindingValid &&
                authority.sourceSha256 == sourceSha256 &&
                authority.ifdPath.isNotBlank() &&
                authority.ifdOffset > 0L

        val envelope =
            profile.optJSONObject("source_capability_envelope_v0_2")
        val envelopeSourceBound =
            envelope != null &&
                sourceBindingValid &&
                envelope.optString("source_sha256", "")
                    .trim()
                    .lowercase() == sourceSha256
        val containerStructureComplete =
            envelopeSourceBound &&
                envelope?.optBoolean("container_structure_complete", false) == true
        val records =
            if (envelopeSourceBound) {
                envelope?.optJSONArray("records") ?: JSONArray()
            } else {
                JSONArray()
            }

        val dependencyJson = JSONArray()
        dependencies.forEach { dependency ->
            var occurrenceCount = 0
            val matchingLifecycle = linkedSetOf<String>()
            val matchingAdmission = linkedSetOf<String>()
            val matchingAuthority = linkedSetOf<String>()

            if (authoritySourceBound) {
                for (index in 0 until records.length()) {
                    val record = records.optJSONObject(index) ?: continue
                    if (record.optInt("tag", -1) != dependency.tag) continue
                    if (record.optLong("ifd_offset", -1L) != authority!!.ifdOffset) continue

                    occurrenceCount += 1
                    record.optString("lifecycle", "")
                        .takeIf { it.isNotBlank() }
                        ?.let { matchingLifecycle += it }
                    record.optString("admission", "")
                        .takeIf { it.isNotBlank() }
                        ?.let { matchingAdmission += it }
                    record.optString("authority", "")
                        .takeIf { it.isNotBlank() }
                        ?.let { matchingAuthority += it }
                }
            }

            val dependencyStatus = when {
                !authoritySourceBound ->
                    "UNKNOWN_NO_UNIQUE_VALIDATED_RAW_IFD"
                occurrenceCount > 0 ->
                    "PRESENT_IN_VALIDATED_RAW_IFD_LEDGER"
                containerStructureComplete ->
                    "ABSENT_FROM_COMPLETE_PARSED_VALIDATED_RAW_IFD_LEDGER"
                else ->
                    "UNKNOWN_CONTAINER_TRAVERSAL_INCOMPLETE"
            }

            dependencyJson.put(
                JSONObject()
                    .put("tag", dependency.tag)
                    .put("name", dependency.name)
                    .put("status", dependencyStatus)
                    .put("ledger_occurrence_count", occurrenceCount)
                    .put("lifecycle_values", JSONArray(matchingLifecycle.toList()))
                    .put("admission_values", JSONArray(matchingAdmission.toList()))
                    .put("authority_values", JSONArray(matchingAuthority.toList()))
                    .put("sample_application_validated", false)
                    .put("scientific_consumer_enabled", false),
            )
        }

        val diagnostic =
            JSONObject()
                .put("schema", SCHEMA)
                .put("status", STATUS)
                .put(
                    "source_sha256",
                    if (sourceBindingValid) sourceSha256 else JSONObject.NULL,
                )
                .put(
                    "raw_ifd_authority_status",
                    if (authoritySourceBound) {
                        "PRESENT_SOURCE_BOUND_VALIDATED_IDENTITY_ONLY"
                    } else {
                        "UNKNOWN_OR_AMBIGUOUS_FAIL_CLOSED"
                    },
                )
                .put(
                    "raw_ifd_path",
                    if (authoritySourceBound) authority!!.ifdPath else JSONObject.NULL,
                )
                .put(
                    "raw_ifd_offset",
                    if (authoritySourceBound) authority!!.ifdOffset else JSONObject.NULL,
                )
                .put("container_structure_complete", containerStructureComplete)
                .put("dependencies", dependencyJson)
                .put("native_decoder_open_observed", false)
                .put(
                    "native_decoder_open_limitation",
                    "This v0.1 diagnostic is a source-bound preflight assembled from the existing evidence ledger; actual decoder SourceAudit.rawIfdOffset remains a separate runtime observation.",
                )
                .put("image_sample_payload_read", false)
                .put("linearization_applied_to_image_samples", false)
                .put("black_subtraction_applied_to_image_samples", false)
                .put("normalization_applied_to_image_samples", false)
                .put("clipping_applied_to_image_samples", false)
                .put("table_application_to_image_samples_validated", false)
                .put("full_raw_to_linear_mapping_validated", false)
                .put("record_lifecycle_effect", false)
                .put("scientific_admission_effect", false)
                .put("downstream_scientific_consumer_enabled", false)
                .put("scientific_master_pixel_effect", false)
                .put("appearance_pixel_effect", false)
                .put("source_values_modified", false)
                .put("creates_new_evidence", false)
                .put("scientific_writeback_allowed", false)

        profile.put(KEY, diagnostic)
        return diagnostic
    }
}

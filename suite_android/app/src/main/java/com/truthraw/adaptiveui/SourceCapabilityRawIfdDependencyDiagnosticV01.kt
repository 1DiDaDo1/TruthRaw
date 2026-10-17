package com.truthraw.adaptiveui

import org.json.JSONArray
import org.json.JSONObject

/**
 * Read-only per-source diagnostic assembled exclusively from the existing
 * Source Capability evidence envelope, its already validated Raw-IFD identity,
 * and (when available) the passive native observation of the exact DNG open.
 * It never reparses the source, reads image samples, or promotes parser/runtime
 * facts into scientific authority.
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
        runtimeObservation: RawIfdRuntimeObservationV01? = null,
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
        val runtimeObservationSourceBound =
            runtimeObservation != null &&
                authoritySourceBound &&
                runtimeObservation.sourceSha256 == sourceSha256 &&
                runtimeObservation.validatedRawIfdOffset == authority!!.ifdOffset

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
        val dependencyStatuses = linkedMapOf<Int, String>()
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
            dependencyStatuses[dependency.tag] = dependencyStatus

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

        fun tagPresence(tag: Int): Any =
            when (dependencyStatuses[tag]) {
                "PRESENT_IN_VALIDATED_RAW_IFD_LEDGER" -> true
                "ABSENT_FROM_COMPLETE_PARSED_VALIDATED_RAW_IFD_LEDGER" -> false
                else -> JSONObject.NULL
            }

        val allDependenciesKnownAbsent =
            dependencies.all {
                dependencyStatuses[it.tag] ==
                    "ABSENT_FROM_COMPLETE_PARSED_VALIDATED_RAW_IFD_LEDGER"
            }
        val anyDependencyKnownPresent =
            dependencies.any {
                dependencyStatuses[it.tag] == "PRESENT_IN_VALIDATED_RAW_IFD_LEDGER"
            }

        val sourceOpenResult =
            if (!runtimeObservationSourceBound) {
                "NOT_OBSERVED"
            } else if (runtimeObservation!!.sourceOpenSucceeded) {
                "SUCCESS"
            } else {
                "FAILED_ADAPTER_STATUS_CODE_${runtimeObservation.sourceOpenResultCode}"
            }

        // This field deliberately reports only the conjunction we can prove.
        // A source-open failure in the presence of an unsupported dependency is
        // not promoted into a claim that this dependency was necessarily the
        // causal rejection unless a later diagnostic can prove that exact cause.
        val dependencyGuardResult =
            when {
                anyDependencyKnownPresent &&
                    runtimeObservationSourceBound &&
                    runtimeObservation!!.sourceOpenSucceeded ->
                    "INCONSISTENT_UNSUPPORTED_DEPENDENCY_PRESENT_BUT_SOURCE_OPEN_SUCCEEDED"
                anyDependencyKnownPresent && runtimeObservationSourceBound ->
                    "UNSUPPORTED_DEPENDENCY_PRESENT_AND_SOURCE_OPEN_FAILED_CODE_${runtimeObservation!!.sourceOpenResultCode}"
                anyDependencyKnownPresent ->
                    "UNSUPPORTED_DEPENDENCY_PRESENT_RUNTIME_NOT_OBSERVED"
                allDependenciesKnownAbsent &&
                    runtimeObservationSourceBound &&
                    runtimeObservation!!.sourceOpenSucceeded ->
                    "PASS_NO_50712_50715_50716_IN_VALIDATED_RAW_IFD_AND_SOURCE_OPEN_SUCCEEDED"
                allDependenciesKnownAbsent && runtimeObservationSourceBound ->
                    "PREFLIGHT_CLEAR_BUT_SOURCE_OPEN_FAILED_CODE_${runtimeObservation!!.sourceOpenResultCode}"
                allDependenciesKnownAbsent ->
                    "PREFLIGHT_CLEAR_RUNTIME_NOT_OBSERVED"
                else ->
                    "UNKNOWN_DEPENDENCY_STATE_FAIL_CLOSED"
            }

        val decoderOpenObserved =
            runtimeObservationSourceBound && runtimeObservation!!.decoderOpenObserved
        val decoderOpenLimitation: Any =
            when {
                decoderOpenObserved -> JSONObject.NULL
                runtimeObservationSourceBound ->
                    "A native source-open result was observed, but no TileNativeDngSource SourceAudit.rawIfdOffset was available for that failed/incomplete open."
                else ->
                    "No source-bound native runtime observation is available yet; run the scientific DNG route and re-export the profile."
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
                .put(
                    "validated_raw_ifd_offset",
                    if (authoritySourceBound) authority!!.ifdOffset else JSONObject.NULL,
                )
                .put(
                    "requested_raw_ifd_offset",
                    if (runtimeObservationSourceBound) {
                        runtimeObservation!!.requestedRawIfdOffset
                    } else {
                        JSONObject.NULL
                    },
                )
                .put(
                    "decoder_source_audit_raw_ifd_offset",
                    if (runtimeObservationSourceBound &&
                        runtimeObservation!!.decoderSourceAuditRawIfdOffset != null
                    ) {
                        runtimeObservation.decoderSourceAuditRawIfdOffset
                    } else {
                        JSONObject.NULL
                    },
                )
                .put(
                    "raw_ifd_authority_match",
                    if (runtimeObservationSourceBound &&
                        runtimeObservation!!.authorityMatch != null
                    ) {
                        runtimeObservation.authorityMatch
                    } else {
                        JSONObject.NULL
                    },
                )
                .put("tag_50712_present", tagPresence(50712))
                .put("tag_50715_present", tagPresence(50715))
                .put("tag_50716_present", tagPresence(50716))
                .put("dependency_guard_result", dependencyGuardResult)
                .put("source_open_result", sourceOpenResult)
                .put(
                    "source_open_result_code",
                    if (runtimeObservationSourceBound) {
                        runtimeObservation!!.sourceOpenResultCode
                    } else {
                        JSONObject.NULL
                    },
                )
                .put("runtime_open_observation_available", runtimeObservationSourceBound)
                .put("container_structure_complete", containerStructureComplete)
                .put("dependencies", dependencyJson)
                .put("native_decoder_open_observed", decoderOpenObserved)
                .put("native_decoder_open_limitation", decoderOpenLimitation)
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

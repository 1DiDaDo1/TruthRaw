package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * A portable, read-only device diagnostic for the validated Raw-IFD authority
 * and the passive native SourceAudit observation. Not source evidence, not a
 * sample-domain or scientific-admission promotion.
 */
object SourceCapabilityRawIfdDeviceEvidenceV01 {
    const val SCHEMA = "D.RAW/SourceCapabilityRawIfdDeviceEvidence/0.1"
    private val SHA256 = Regex("[0-9a-f]{64}")

    fun build(
        profile: JSONObject,
        nativeRegistryProbe: RawIfdNativeRegistryProbeV01? = null,
    ): JSONObject? {
        val sourceSha256 = profile.optString("source_sha256", "").trim().lowercase()
        if (!sourceSha256.matches(SHA256)) return null
        val diagnostic =
            profile.optJSONObject(SourceCapabilityRawIfdDependencyDiagnosticV01.KEY)
                ?: return null
        if (
            diagnostic.optString("schema") !=
                SourceCapabilityRawIfdDependencyDiagnosticV01.SCHEMA ||
            diagnostic.optString("source_sha256", "").trim().lowercase() != sourceSha256
        ) {
            return null
        }
        // Never accept a last-open probe for another requested identity as
        // the basis for this export, even as process-memory diagnostics.
        if (nativeRegistryProbe != null &&
            nativeRegistryProbe.requestedSourceSha256 != sourceSha256
        ) return null
        // Fail closed if an accidental promotion flag reaches the export layer.
        val forbidden = listOf(
            "image_sample_payload_read",
            "linearization_applied_to_image_samples",
            "black_subtraction_applied_to_image_samples",
            "normalization_applied_to_image_samples",
            "clipping_applied_to_image_samples",
            "table_application_to_image_samples_validated",
            "full_raw_to_linear_mapping_validated",
            "record_lifecycle_effect",
            "scientific_admission_effect",
            "downstream_scientific_consumer_enabled",
            "scientific_master_pixel_effect",
            "appearance_pixel_effect",
            "source_values_modified",
            "creates_new_evidence",
            "scientific_writeback_allowed",
        )
        if (forbidden.any { !diagnostic.has(it) || diagnostic.optBoolean(it, true) }) {
            return null
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("status", "SOURCE_BOUND_READ_ONLY_DEVICE_DIAGNOSTIC")
            .put("source_sha256", sourceSha256)
            .put(
                "raw_ifd_dependency_diagnostic_v0_1",
                JSONObject(diagnostic.toString()),
            )
            .put(
                "native_registry_probe_v0_1",
                nativeRegistryProbe?.toJson() ?: JSONObject()
                    .put("schema", "D.RAW/RawIfdNativeRegistryProbe/0.1")
                    .put("status", "UNAVAILABLE_FAIL_CLOSED")
                    .put("requested_source_sha256", sourceSha256)
                    .put("creates_new_evidence", false)
                    .put("scientific_writeback_allowed", false),
            )
            .put("export_is_sealed_source_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_master_pixel_effect", false)
            .put("scientific_writeback_allowed", false)
    }
}

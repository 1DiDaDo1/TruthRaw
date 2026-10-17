package com.truthraw.adaptiveui

import org.json.JSONObject

data class SourceCapabilityRawIfdRuntimeAuthorityV01(
    val sourceSha256: String,
    val ifdPath: String,
    val ifdOffset: Long,
)

internal interface RawIfdRuntimeAuthorityPortV01 {
    fun registerValidatedRawIfdAuthority(
        sourceSha256: String,
        rawIfdOffset: Long,
    ): Boolean

    fun readRawIfdRuntimeObservation(
        sourceSha256: String,
    ): RawIfdRuntimeObservationV01?
}

private object NativeRawIfdRuntimeAuthorityPortV01 : RawIfdRuntimeAuthorityPortV01 {
    override fun registerValidatedRawIfdAuthority(
        sourceSha256: String,
        rawIfdOffset: Long,
    ): Boolean =
        RawIfdAuthorityBridgeV01.registerValidatedRawIfdAuthority(
            sourceSha256 = sourceSha256,
            rawIfdOffset = rawIfdOffset,
        )

    override fun readRawIfdRuntimeObservation(
        sourceSha256: String,
    ): RawIfdRuntimeObservationV01? =
        RawIfdAuthorityBridgeV01.readRawIfdRuntimeObservation(sourceSha256)
}

object SourceCapabilityRawIfdRuntimeAuthorityBinderV01 {
    private val SHA256 = Regex("[0-9a-f]{64}")

    fun extract(profile: JSONObject): SourceCapabilityRawIfdRuntimeAuthorityV01? {
        val source = profile.optString("source_sha256", "").lowercase()
        if (!source.matches(SHA256)) return null

        val envelope = profile.optJSONObject("source_capability_envelope_v0_2") ?: return null
        if (envelope.optString("source_sha256", "").lowercase() != source) return null

        val contract = envelope.optJSONObject("radiometric_raw_to_linear_contract_v0_1") ?: return null
        if (contract.optString("schema") != SourceCapabilityRadiometricRawToLinearContractValidatorV01.SCHEMA ||
            contract.optString("status") != SourceCapabilityRadiometricRawToLinearContractValidatorV01.STATUS ||
            contract.optString("source_sha256", "").lowercase() != source ||
            contract.optString("dependency_ifd_identity_integrity_status") !=
                SourceCapabilityRadiometricRawToLinearContractValidatorV01.IFD_IDENTITY_INTEGRITY_VALIDATED ||
            contract.optString("raw_to_linear_composition_status") !=
                SourceCapabilityRadiometricRawToLinearContractValidatorV01.VALIDATED ||
            !contract.optBoolean("all_existing_dependency_reports_bind_same_source", false) ||
            !contract.optBoolean("metadata_ordering_composition_contract_validated", false)
        ) {
            return null
        }

        // IFD identity may become runtime authority while sample-domain raw-to-linear
        // authority remains deliberately unproven. Any accidental sample claim is
        // therefore a hard blocker for this identity-only handoff.
        val forbiddenTrueFlags =
            listOf(
                "table_application_to_image_samples_validated",
                "full_raw_to_linear_mapping_validated",
                "image_sample_payload_read",
                "linearization_applied_to_image_samples",
                "black_subtraction_applied_to_image_samples",
                "normalization_applied_to_image_samples",
                "clipping_applied_to_image_samples",
                "observed_sample_censoring_validated",
                "physical_sensor_saturation_calibrated",
                "scene_radiance_above_censor_recovered",
                "record_lifecycle_effect",
                "scientific_admission_effect",
                "downstream_scientific_consumer_enabled",
                "pixel_route_effect",
                "scientific_master_pixel_effect",
                "appearance_pixel_effect",
                "source_values_modified",
                "creates_new_evidence",
                "scientific_writeback_allowed",
            )
        if (forbiddenTrueFlags.any { contract.optBoolean(it, true) }) return null

        val rawDomain = envelope.optJSONObject("raw_ifd_domain_validation_v0_1") ?: return null
        if (rawDomain.optString("schema") != SourceCapabilityRawIfdDomainValidatorV01.SCHEMA ||
            rawDomain.optString("status") != SourceCapabilityRawIfdDomainValidatorV01.STATUS ||
            rawDomain.optString("source_sha256", "").lowercase() != source
        ) {
            return null
        }

        val validatedRawDomain = mutableSetOf<Pair<String, Long>>()
        val rawDomainIfds = rawDomain.optJSONArray("ifds") ?: return null
        for (i in 0 until rawDomainIfds.length()) {
            val record = rawDomainIfds.optJSONObject(i) ?: continue
            if (record.optString("domain_status") != SourceCapabilityRawIfdDomainValidatorV01.VALIDATED_RAW_DOMAIN) {
                continue
            }
            val path = record.optString("ifd_path", "")
            val offset = record.optLong("ifd_offset", -1L)
            if (path.isNotBlank() && offset > 0L) {
                validatedRawDomain += path to offset
            }
        }

        val candidates = linkedMapOf<Pair<String, Long>, SourceCapabilityRawIfdRuntimeAuthorityV01>()
        val contractIfds = contract.optJSONArray("ifds") ?: return null
        for (i in 0 until contractIfds.length()) {
            val record = contractIfds.optJSONObject(i) ?: continue
            if (record.optString("raw_to_linear_composition_status") !=
                SourceCapabilityRadiometricRawToLinearContractValidatorV01.VALIDATED ||
                !record.optBoolean("metadata_ordering_composition_contract_validated", false) ||
                record.optBoolean("full_raw_to_linear_mapping_validated", true)
            ) {
                continue
            }
            val path = record.optString("ifd_path", "")
            val offset = record.optLong("ifd_offset", -1L)
            val key = path to offset
            if (path.isNotBlank() && offset > 0L && key in validatedRawDomain) {
                candidates[key] =
                    SourceCapabilityRawIfdRuntimeAuthorityV01(
                        sourceSha256 = source,
                        ifdPath = path,
                        ifdOffset = offset,
                    )
            }
        }

        // v0.1 never guesses between multiple validated raw IFDs.
        return candidates.values.singleOrNull()
    }

    fun bindIfAvailable(profile: JSONObject): Boolean =
        bindIfAvailable(
            profile = profile,
            port = NativeRawIfdRuntimeAuthorityPortV01,
        )

    internal fun bindIfAvailable(
        profile: JSONObject,
        port: RawIfdRuntimeAuthorityPortV01,
    ): Boolean {
        val authority = extract(profile)
        if (authority == null) {
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile = profile,
                authority = null,
                runtimeObservation = null,
            )
            return false
        }

        // Authority must exist in the native registry before any authority-bound
        // DNG open can occur. Reading the passive runtime observation first would
        // permanently snapshot NOT_OBSERVED into a profile that is about to make
        // its first native open.
        val registered =
            runCatching {
                port.registerValidatedRawIfdAuthority(
                    sourceSha256 = authority.sourceSha256,
                    rawIfdOffset = authority.ifdOffset,
                )
            }.getOrDefault(false)
        if (!registered) {
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile = profile,
                authority = authority,
                runtimeObservation = null,
            )
            return false
        }

        val runtimeObservation =
            runCatching {
                port.readRawIfdRuntimeObservation(authority.sourceSha256)
            }.getOrNull()
        SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
            profile = profile,
            authority = authority,
            runtimeObservation = runtimeObservation,
        )
        return true
    }

    /**
     * Re-snapshots the passive native open observation after a synchronous DNG
     * open has returned. This is derived diagnostic refresh only: no source is
     * reparsed, no authority is minted, and no image/sample value is changed.
     */
    fun refreshRuntimeDiagnosticIfAvailable(profile: JSONObject): Boolean =
        refreshRuntimeDiagnosticIfAvailable(
            profile = profile,
            port = NativeRawIfdRuntimeAuthorityPortV01,
        )

    internal fun refreshRuntimeDiagnosticIfAvailable(
        profile: JSONObject,
        port: RawIfdRuntimeAuthorityPortV01,
    ): Boolean {
        val authority = extract(profile)
        if (authority == null) {
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile = profile,
                authority = null,
                runtimeObservation = null,
            )
            return false
        }

        val runtimeObservation =
            runCatching {
                port.readRawIfdRuntimeObservation(authority.sourceSha256)
            }.getOrNull()
        val diagnostic =
            SourceCapabilityRawIfdDependencyDiagnosticV01.attach(
                profile = profile,
                authority = authority,
                runtimeObservation = runtimeObservation,
            )
        return diagnostic.optBoolean("runtime_open_observation_available", false)
    }
}

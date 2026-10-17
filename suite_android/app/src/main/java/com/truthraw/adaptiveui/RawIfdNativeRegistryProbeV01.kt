package com.truthraw.adaptiveui

import org.json.JSONObject

/**
 * Diagnostic process-memory snapshot; it is not part of the sealed evidence,
 * source selection, scientific admission, or image/sample processing.
 * The last native adapter-open may belong to another source and must NEVER
 * be substituted for a missing source-bound runtime observation.
 */
data class RawIfdNativeRegistryProbeV01(
    val requestedSourceSha256: String,
    val authorityRegisteredForRequestedSource: Boolean,
    val registeredRawIfdOffset: Long?,
    val exactSourceObservationPresent: Boolean,
    val lastAdapterOpenRecorded: Boolean,
    val lastAdapterOpenSourceSha256: String?,
    val lastAdapterOpenValidatedOffset: Long?,
    val lastAdapterOpenRequestedOffset: Long?,
    val lastAdapterOpenDecoderOffset: Long?,
    val lastAdapterOpenSucceeded: Boolean?,
    val lastAdapterOpenResultCode: Int?,
    val lastAdapterDecoderOpenObserved: Boolean?,
    val lastAdapterAuthorityMatch: Boolean?,
) {
    fun toJson(): JSONObject =
        JSONObject()
            .put("schema", "D.RAW/RawIfdNativeRegistryProbe/0.1")
            .put("status", "PASSIVE_PROCESS_MEMORY_DIAGNOSTIC_ONLY")
            .put("requested_source_sha256", requestedSourceSha256)
            .put("validated_authority_registered_for_source", authorityRegisteredForRequestedSource)
            .put("registered_raw_ifd_offset", registeredRawIfdOffset ?: JSONObject.NULL)
            .put("exact_source_runtime_observation_present", exactSourceObservationPresent)
            .put("last_adapter_open_recorded", lastAdapterOpenRecorded)
            .put("last_adapter_open_source_sha256", lastAdapterOpenSourceSha256 ?: JSONObject.NULL)
            .put(
                "last_adapter_open_source_matches_requested",
                lastAdapterOpenSourceSha256?.let { it == requestedSourceSha256 } ?: JSONObject.NULL,
            )
            .put("last_adapter_open_validated_offset", lastAdapterOpenValidatedOffset ?: JSONObject.NULL)
            .put("last_adapter_open_requested_offset", lastAdapterOpenRequestedOffset ?: JSONObject.NULL)
            .put("last_adapter_open_decoder_offset", lastAdapterOpenDecoderOffset ?: JSONObject.NULL)
            .put("last_adapter_open_succeeded", lastAdapterOpenSucceeded ?: JSONObject.NULL)
            .put("last_adapter_open_result_code", lastAdapterOpenResultCode ?: JSONObject.NULL)
            .put("last_adapter_decoder_open_observed", lastAdapterDecoderOpenObserved ?: JSONObject.NULL)
            .put("last_adapter_authority_match", lastAdapterAuthorityMatch ?: JSONObject.NULL)
            .put("last_adapter_is_source_evidence", false)
            .put("creates_new_evidence", false)
            .put("scientific_master_pixel_effect", false)
            .put("scientific_writeback_allowed", false)

    companion object {
        private val SHA256 = Regex("[0-9a-f]{64}")

        /** Parser is pure and can be tested on the host JVM without loading JNI. */
        fun parse(
            requestedSourceSha256: String,
            packet: String?,
        ): RawIfdNativeRegistryProbeV01? {
            if (!requestedSourceSha256.matches(SHA256) || packet == null) return null
            val fields = packet.split('|')
            if (fields.size != 13 || fields[0] != "1") return null

            fun binary(index: Int): Boolean? =
                when (fields[index]) {
                    "0" -> false
                    "1" -> true
                    else -> null
                }

            val registered = binary(1) ?: return null
            val rawOffset = fields[2].toLongOrNull() ?: return null
            val exactObservation = binary(3) ?: return null
            val hasLast = binary(4) ?: return null
            val lastSha = fields[5]
            val lastValidated = fields[6].toLongOrNull() ?: return null
            val lastRequested = fields[7].toLongOrNull() ?: return null
            val lastDecoder = fields[8].toLongOrNull() ?: return null
            val lastSucceeded = binary(9) ?: return null
            val lastCode = fields[10].toIntOrNull() ?: return null
            val lastDecoderObserved = binary(11) ?: return null
            val lastMatch = when (fields[12]) {
                "-1" -> null
                "0" -> false
                "1" -> true
                else -> return null
            }

            if ((registered && rawOffset <= 0L) || (!registered && rawOffset != 0L)) return null
            if (rawOffset > 0xffffffffL) return null
            if (hasLast) {
                if (!lastSha.matches(SHA256)) return null
                if (lastValidated !in 0L..0xffffffffL ||
                    lastRequested !in 0L..0xffffffffL ||
                    lastDecoder !in 0L..0xffffffffL
                ) return null
            } else if (lastSha != "-" || lastValidated != 0L ||
                lastRequested != 0L || lastDecoder != 0L ||
                lastSucceeded || lastCode != -1 || lastDecoderObserved || lastMatch != null
            ) {
                return null
            }

            return RawIfdNativeRegistryProbeV01(
                requestedSourceSha256 = requestedSourceSha256,
                authorityRegisteredForRequestedSource = registered,
                registeredRawIfdOffset = rawOffset.takeIf { registered },
                exactSourceObservationPresent = exactObservation,
                lastAdapterOpenRecorded = hasLast,
                lastAdapterOpenSourceSha256 = lastSha.takeIf { hasLast },
                lastAdapterOpenValidatedOffset = lastValidated.takeIf { hasLast && it > 0L },
                lastAdapterOpenRequestedOffset = lastRequested.takeIf { hasLast && it > 0L },
                lastAdapterOpenDecoderOffset = lastDecoder.takeIf { hasLast && it > 0L },
                lastAdapterOpenSucceeded = lastSucceeded.takeIf { hasLast },
                lastAdapterOpenResultCode = lastCode.takeIf { hasLast },
                lastAdapterDecoderOpenObserved = lastDecoderObserved.takeIf { hasLast },
                lastAdapterAuthorityMatch = lastMatch.takeIf { hasLast },
            )
        }
    }
}

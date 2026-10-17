package com.truthraw.adaptiveui

data class RawIfdRuntimeObservationV01(
    val sourceSha256: String,
    val validatedRawIfdOffset: Long,
    val requestedRawIfdOffset: Long,
    val decoderSourceAuditRawIfdOffset: Long?,
    val authorityMatch: Boolean?,
    val decoderOpenObserved: Boolean,
    val sourceOpenSucceeded: Boolean,
    val sourceOpenResultCode: Int,
)

/**
 * Identity-only runtime handoff from the already-validated Source Capability
 * contract into the native DNG open boundary. This does not validate or apply
 * any raw-to-linear sample operation.
 */
object RawIfdAuthorityBridgeV01 {
    private val SHA256 = Regex("[0-9a-f]{64}")

    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun registerValidatedRawIfdAuthority(
        sourceSha256: String,
        rawIfdOffset: Long,
    ): Boolean

    private external fun nativeReadRawIfdRuntimeObservationPacket(
        sourceSha256: String,
    ): LongArray?

    private external fun nativeReadRawIfdRegistryProbePacket(
        sourceSha256: String,
    ): String?

    /**
     * Debugging-only native registry snapshot. The last-open entry, if any,
     * never stands in for this specific source's decoder observation.
     */
    fun readRawIfdNativeRegistryProbe(
        sourceSha256: String,
    ): RawIfdNativeRegistryProbeV01? {
        val normalized = sourceSha256.trim().lowercase()
        if (!normalized.matches(SHA256)) return null
        return RawIfdNativeRegistryProbeV01.parse(
            requestedSourceSha256 = normalized,
            packet = nativeReadRawIfdRegistryProbePacket(normalized),
        )
    }

    fun readRawIfdRuntimeObservation(
        sourceSha256: String,
    ): RawIfdRuntimeObservationV01? {
        val normalized = sourceSha256.trim().lowercase()
        if (!normalized.matches(SHA256)) return null
        val packet = nativeReadRawIfdRuntimeObservationPacket(normalized) ?: return null
        if (packet.size != 8 || packet[0] != 1L) return null

        val authorityMatch =
            when (packet[4]) {
                0L -> false
                1L -> true
                else -> null
            }
        val decoderOpenObserved = packet[5] == 1L
        val decoderOffset =
            packet[3].takeIf { decoderOpenObserved && it > 0L }

        return RawIfdRuntimeObservationV01(
            sourceSha256 = normalized,
            validatedRawIfdOffset = packet[1],
            requestedRawIfdOffset = packet[2],
            decoderSourceAuditRawIfdOffset = decoderOffset,
            authorityMatch = authorityMatch,
            decoderOpenObserved = decoderOpenObserved,
            sourceOpenSucceeded = packet[6] == 1L,
            sourceOpenResultCode = packet[7].toInt(),
        )
    }
}

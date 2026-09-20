package com.truthraw.adaptiveui

import android.media.MediaCodecInfo
import android.media.MediaCodecList
import android.os.Build

data class TruthRawApvCodecDescriptor(
    val name: String,
    val canonicalName: String,
    val encoder: Boolean,
    val hardwareAccelerated: Boolean,
    val vendor: Boolean,
    val softwareOnly: Boolean,
    val colorFormats: List<Int>,
    val profileLevels: List<Pair<Int, Int>>,
    val maxWidth: Int?,
    val maxHeight: Int?,
    val maxBitrate: Int?,
    val performancePoints: List<String>,
)

data class TruthRawApvCapabilities(
    val androidSdk: Int,
    val platformExpected: Boolean,
    val encoders: List<TruthRawApvCodecDescriptor>,
    val decoders: List<TruthRawApvCodecDescriptor>,
) {
    val anyEncoder: Boolean get() = encoders.isNotEmpty()
    val anyDecoder: Boolean get() = decoders.isNotEmpty()
    val hardwareEncoder: Boolean get() = encoders.any { it.hardwareAccelerated }
    val hardwareDecoder: Boolean get() = decoders.any { it.hardwareAccelerated }
    val vendorHardwareEncoder: Boolean
        get() = encoders.any { it.hardwareAccelerated && it.vendor }
    val vendorHardwareDecoder: Boolean
        get() = decoders.any { it.hardwareAccelerated && it.vendor }

    val professionalVideoCandidate: Boolean
        get() = hardwareEncoder && hardwareDecoder

    val summary: String
        get() = buildString {
            append("APV MIME=video/apv · SDK=")
            append(androidSdk)
            append(" · platformExpected=")
            append(platformExpected)
            append("\nencoder=")
            append(anyEncoder)
            append(" · hardware=")
            append(hardwareEncoder)
            append(" · vendorHardware=")
            append(vendorHardwareEncoder)
            append("\ndecoder=")
            append(anyDecoder)
            append(" · hardware=")
            append(hardwareDecoder)
            append(" · vendorHardware=")
            append(vendorHardwareDecoder)
            append("\nrole=")
            append(
                if (professionalVideoCandidate) {
                    "PROFESSIONAL_VIDEO_INTERMEDIATE_CANDIDATE"
                } else {
                    "UNAVAILABLE_OR_SOFTWARE_ONLY"
                },
            )
            append("\nScientific Master replacement=false")
        }
}

object TruthRawApvCapabilitiesProbe {
    private const val APV_MIME = "video/apv"

    fun probe(): Result<TruthRawApvCapabilities> = runCatching {
        val infos = MediaCodecList(MediaCodecList.ALL_CODECS).codecInfos
            .asSequence()
            .filterNot { runCatching { it.isAlias }.getOrDefault(false) }
            .filter { info ->
                info.supportedTypes.any { it.equals(APV_MIME, ignoreCase = true) }
            }
            .mapNotNull(::describe)
            .toList()

        TruthRawApvCapabilities(
            androidSdk = Build.VERSION.SDK_INT,
            // APV is a mandatory Android platform codec beginning with API 36.
            platformExpected = Build.VERSION.SDK_INT >= 36,
            encoders = infos.filter { it.encoder },
            decoders = infos.filterNot { it.encoder },
        )
    }

    private fun describe(info: MediaCodecInfo): TruthRawApvCodecDescriptor? =
        runCatching {
            val caps = info.getCapabilitiesForType(APV_MIME)
            val video = runCatching { caps.videoCapabilities }.getOrNull()

            val performancePoints = runCatching {
                video?.supportedPerformancePoints
                    ?.map { it.toString() }
                    ?: emptyList()
            }.getOrDefault(emptyList())

            TruthRawApvCodecDescriptor(
                name = info.name,
                canonicalName = runCatching { info.canonicalName }.getOrDefault(info.name),
                encoder = info.isEncoder,
                hardwareAccelerated = runCatching {
                    info.isHardwareAccelerated
                }.getOrDefault(false),
                vendor = runCatching { info.isVendor }.getOrDefault(false),
                softwareOnly = runCatching { info.isSoftwareOnly }.getOrDefault(false),
                colorFormats = caps.colorFormats.toList(),
                profileLevels = caps.profileLevels.map { it.profile to it.level },
                maxWidth = runCatching { video?.supportedWidths?.upper }.getOrNull(),
                maxHeight = runCatching { video?.supportedHeights?.upper }.getOrNull(),
                maxBitrate = runCatching { video?.bitrateRange?.upper }.getOrNull(),
                performancePoints = performancePoints,
            )
        }.getOrNull()

    fun compactDetails(capabilities: TruthRawApvCapabilities): String = buildString {
        append(capabilities.summary)
        val all = capabilities.encoders + capabilities.decoders
        if (all.isEmpty()) return@buildString
        append("\n")
        all.forEachIndexed { index, codec ->
            if (index > 0) append("\n")
            append(if (codec.encoder) "ENC " else "DEC ")
            append(codec.name)
            append(" · hw=")
            append(codec.hardwareAccelerated)
            append(" · vendor=")
            append(codec.vendor)
            append(" · software=")
            append(codec.softwareOnly)
            append(" · max=")
            append(codec.maxWidth ?: "?")
            append("×")
            append(codec.maxHeight ?: "?")
            append(" · bitrate≤")
            append(codec.maxBitrate ?: "?")
            append(" · profiles=")
            append(codec.profileLevels.joinToString { (profile, level) -> "$profile/$level" })
            append(" · colorFormats=")
            append(codec.colorFormats.joinToString())
            if (codec.performancePoints.isNotEmpty()) {
                append(" · perf=")
                append(codec.performancePoints.joinToString())
            }
        }
    }
}

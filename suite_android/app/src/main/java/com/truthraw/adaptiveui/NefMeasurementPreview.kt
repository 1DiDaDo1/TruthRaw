package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val NEF_MEASUREMENT_MAGIC = 0x54524e4d
private const val NEF_MEASUREMENT_HEADER_INTS = 26
private const val NEF_MAX_PREVIEW_EDGE = 384
private const val NEF_MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024

object NefMeasurementNativeBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun buildMeasurementCfaPreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
    ): IntArray
}

data class NefMeasurementMetrics(
    val sourceWidth: Int,
    val sourceHeight: Int,
    val cfaCode: Int,
    val sourceResidentUpperBoundBytes: Int,
    val minSampleCodeInPreview: Int,
    val maxSampleCodeInPreview: Int,
    val previewSamplesRead: Int,
    val measurementAdmissionReady: Boolean,
    val scientificAdmissionReady: Boolean,
    val exactCfaSamplesAvailable: Boolean,
    val directSensorAdcClaimAllowed: Boolean,
    val fullRawFrameMaterialized: Boolean,
    val sourceBytes: Long,
    val sourceSha256: String,
)

sealed interface NefMeasurementResult {
    data class Ready(
        val bitmap: Bitmap,
        val metrics: NefMeasurementMetrics,
    ) : NefMeasurementResult

    data class Failed(val reason: String) : NefMeasurementResult
}

object NefMeasurementLoader {
    fun load(resolver: ContentResolver, job: RawJob): NefMeasurementResult {
        if (job.source.format.id != "NIKON_NEF") {
            return NefMeasurementResult.Failed("NEF-meetroute ontving geen Nikon NEF-bron.")
        }

        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return NefMeasurementResult.Failed(
                "Documentprovider gaf geen leesbare descriptor: ${error.message ?: error.javaClass.simpleName}",
            )
        } ?: return NefMeasurementResult.Failed("Documentprovider gaf geen file descriptor.")

        val packet = try {
            descriptor.use { pfd ->
                NefMeasurementNativeBridge.buildMeasurementCfaPreview(
                    pfd.fd,
                    NEF_MAX_PREVIEW_EDGE,
                    NEF_MAX_SOURCE_RESIDENT_BYTES,
                )
            }
        } catch (error: Throwable) {
            return NefMeasurementResult.Failed(
                "Native NEF-meetroute faalde: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        if (packet.size < NEF_MEASUREMENT_HEADER_INTS || packet[0] != NEF_MEASUREMENT_MAGIC) {
            return NefMeasurementResult.Failed("Ongeldig NEF-meetpakket.")
        }
        val status = packet[1]
        if (status != 0) return NefMeasurementResult.Failed(statusDescription(status))

        val width = packet[2]
        val height = packet[3]
        if (width <= 0 || height <= 0 || width > NEF_MAX_PREVIEW_EDGE || height > NEF_MAX_PREVIEW_EDGE) {
            return NefMeasurementResult.Failed("NEF-meetpreview heeft ongeldige afmetingen.")
        }
        val pixelCount = try {
            Math.multiplyExact(width, height)
        } catch (_: ArithmeticException) {
            return NefMeasurementResult.Failed("NEF-meetpreview afmetingen overflowden.")
        }
        if (packet.size != NEF_MEASUREMENT_HEADER_INTS + pixelCount) {
            return NefMeasurementResult.Failed("NEF-meetpreview payloadlengte is ongeldig.")
        }

        val metrics = NefMeasurementMetrics(
            sourceWidth = packet[4],
            sourceHeight = packet[5],
            cfaCode = packet[6],
            sourceResidentUpperBoundBytes = packet[7],
            minSampleCodeInPreview = packet[8],
            maxSampleCodeInPreview = packet[9],
            previewSamplesRead = packet[10],
            measurementAdmissionReady = packet[11] != 0,
            scientificAdmissionReady = packet[12] != 0,
            exactCfaSamplesAvailable = packet[13] != 0,
            directSensorAdcClaimAllowed = packet[14] != 0,
            fullRawFrameMaterialized = packet[15] != 0,
            sourceBytes =
                (packet[16].toLong() and 0xffffffffL) or
                    ((packet[17].toLong() and 0xffffffffL) shl 32),
            sourceSha256 = sha256Hex(packet),
        )

        if (!metrics.measurementAdmissionReady ||
            metrics.scientificAdmissionReady ||
            !metrics.exactCfaSamplesAvailable ||
            metrics.directSensorAdcClaimAllowed ||
            metrics.fullRawFrameMaterialized
        ) {
            return NefMeasurementResult.Failed(
                "Fail-closed: NEF adapter schond measurement/scientific authority-grens.",
            )
        }

        val bitmap = try {
            PortablePreviewEncoder.createSrgbBitmap(
                width,
                height,
                packet,
                NEF_MEASUREMENT_HEADER_INTS,
            )
        } catch (error: Exception) {
            return NefMeasurementResult.Failed(
                "NEF-meetpreview kon niet worden opgebouwd: ${error.message ?: error.javaClass.simpleName}",
            )
        }

        return NefMeasurementResult.Ready(bitmap, metrics)
    }

    private fun sha256Hex(packet: IntArray): String {
        val out = StringBuilder(64)
        for (index in 18 until 26) {
            val word = packet[index]
            for (shift in intArrayOf(0, 8, 16, 24)) {
                val value = (word ushr shift) and 0xff
                out.append(value.toString(16).padStart(2, '0'))
            }
        }
        return out.toString()
    }

    private fun statusDescription(status: Int): String = when (status) {
        -1 -> "NEF-meetroute: ongeldige parameters."
        -2 -> "NEF-meetroute: adapter-authority contract werd geschonden."
        -3 -> "NEF-meetroute: bronafmetingen zijn ongeldig."
        -4 -> "NEF-meetroute: previewbudget werd overschreden."

        2001 -> "Bronsealing: ongeldig argument."
        2002 -> "Bronsealing: bron kon niet volledig worden gelezen."
        2003 -> "Bronsealing: bronseal is ongeldig."
        2004 -> "Bronsealing: bronbytes veranderden tijdens de meetroute."

        4001 -> "NEF tile-read: ongeldig argument."
        4002 -> "NEF tile-read: bronread faalde."

        7001 -> "NEF-adapter: ongeldig argument."
        7002 -> "NEF-adapter: sealed bronlengte kwam niet overeen."
        7003 -> "NEF-adapter: decoder ontbreekt."
        7004 -> "NEF-adapter: dubbele adapterregistratie."
        7005 -> "NEF-adapter: container is ongeldig of niet Nikon."
        7006 -> "NEF-adapter: deze NEF-opslag/compressie/topologie wordt nog niet ondersteund."
        7007 -> "NEF-adapter: decode/read faalde."
        7008 -> "NEF-adapter: memorybudget overschreden."

        else -> "NEF-meetroute faalde met native status $status."
    }
}

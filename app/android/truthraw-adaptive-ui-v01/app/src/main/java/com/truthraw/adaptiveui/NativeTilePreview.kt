package com.truthraw.adaptiveui

import android.content.ContentResolver
import android.graphics.Bitmap

private const val PREVIEW_MAGIC = 0x54525032
private const val HEADER_INTS = 13
private const val MAX_PREVIEW_EDGE = 384
private const val MAX_SOURCE_RESIDENT_BYTES = 8 * 1024 * 1024

object NativeTilePreviewBridge {
    init {
        System.loadLibrary("truthraw_ui_preview_bridge")
    }

    external fun buildCfaPreview(
        fd: Int,
        maxEdge: Int,
        maxSourceResidentBytes: Int,
    ): IntArray
}

data class TilePreviewMetrics(
    val sourceWidth: Int,
    val sourceHeight: Int,
    val sourceResidentUpperBoundBytes: Int,
    val rawPayloadBytesRead: Int,
    val metadataBytesRead: Int,
    val tileReadCalls: Int,
    val fullRawMaterialized: Boolean,
    val hasGainField: Boolean,
    val orientation: Int,
)

sealed interface TilePreviewUiState {
    data object Idle : TilePreviewUiState
    data class Loading(val jobId: String) : TilePreviewUiState
    data class Ready(
        val jobId: String,
        val bitmap: Bitmap,
        val metrics: TilePreviewMetrics,
    ) : TilePreviewUiState
    data class Failed(val jobId: String, val reason: String) : TilePreviewUiState
}

object TilePreviewLoader {
    fun load(resolver: ContentResolver, job: RawJob): TilePreviewUiState {
        val descriptor = try {
            resolver.openFileDescriptor(job.source.uri, "r")
        } catch (error: Exception) {
            return TilePreviewUiState.Failed(job.id, "Documentprovider gaf geen leesbare file descriptor: ${error.message ?: error.javaClass.simpleName}")
        } ?: return TilePreviewUiState.Failed(job.id, "Documentprovider gaf geen file descriptor.")

        val packet = try {
            descriptor.use { pfd ->
                NativeTilePreviewBridge.buildCfaPreview(
                    pfd.fd,
                    MAX_PREVIEW_EDGE,
                    MAX_SOURCE_RESIDENT_BYTES,
                )
            }
        } catch (error: Throwable) {
            return TilePreviewUiState.Failed(job.id, "Native preview bridge faalde: ${error.message ?: error.javaClass.simpleName}")
        }

        if (packet.size < HEADER_INTS || packet[0] != PREVIEW_MAGIC) {
            return TilePreviewUiState.Failed(job.id, "Ongeldig native preview-pakket.")
        }
        val status = packet[1]
        if (status != 0) {
            return TilePreviewUiState.Failed(job.id, nativeStatusDescription(status))
        }

        val width = packet[2]
        val height = packet[3]
        if (width <= 0 || height <= 0 || width > MAX_PREVIEW_EDGE || height > MAX_PREVIEW_EDGE) {
            return TilePreviewUiState.Failed(job.id, "Native preview-afmetingen zijn buiten contract.")
        }
        val pixelCount = width * height
        if (packet.size != HEADER_INTS + pixelCount) {
            return TilePreviewUiState.Failed(job.id, "Native preview-payload heeft een ongeldige lengte.")
        }

        val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
        bitmap.setPixels(packet, HEADER_INTS, width, 0, 0, width, height)
        val metrics = TilePreviewMetrics(
            sourceWidth = packet[4],
            sourceHeight = packet[5],
            sourceResidentUpperBoundBytes = packet[6],
            rawPayloadBytesRead = packet[7],
            metadataBytesRead = packet[8],
            tileReadCalls = packet[9],
            fullRawMaterialized = packet[10] != 0,
            hasGainField = packet[11] != 0,
            orientation = packet[12],
        )
        if (metrics.fullRawMaterialized) {
            bitmap.recycle()
            return TilePreviewUiState.Failed(job.id, "Fail-closed: de source rapporteerde fullRawMaterialized=true.")
        }
        return TilePreviewUiState.Ready(job.id, bitmap, metrics)
    }

    private fun nativeStatusDescription(status: Int): String = when (status) {
        -1 -> "Ongeldige preview-bridge parameters."
        -2 -> "DNG rapporteert ongeldige afmetingen."
        -3 -> "Preview-workspace overschreed de vaste limiet."
        -4 -> "Preview kon niet volledig uit bounded tile-reads worden gevuld."
        1 -> "TileNativeDngSource: I/O-fout of niet-seekbare documentprovider."
        2 -> "TileNativeDngSource: ongeldige TIFF/DNG-container."
        3 -> "TileNativeDngSource: BigTIFF wordt in v0.1 niet ondersteund."
        4 -> "TileNativeDngSource: compressie wordt in v0.1 niet ondersteund."
        5 -> "TileNativeDngSource: sample-opslag wordt in v0.1 niet ondersteund."
        6 -> "TileNativeDngSource: geen ondersteunde CFA-IFD gevonden."
        7 -> "TileNativeDngSource: RAW-topologie wordt niet ondersteund."
        8 -> "TileNativeDngSource: meerdere CFA-IFD's vereisen expliciete binding."
        9 -> "TileNativeDngSource: verplichte DNG-tag ontbreekt."
        10 -> "TileNativeDngSource: ongeldige DNG-tag."
        11 -> "TileNativeDngSource: ongeldige strip/tile-opslag."
        12 -> "TileNativeDngSource: vereiste binding ontbreekt."
        13 -> "TileNativeDngSource: resident-memorybudget overschreden."
        in 1001..1006 -> "TileNativeDngSource: tile-read faalde fail-closed (status ${status - 1000})."
        else -> "Onbekende native preview-status $status."
    }
}

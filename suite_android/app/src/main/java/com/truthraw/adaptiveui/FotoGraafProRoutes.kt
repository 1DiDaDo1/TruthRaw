package com.truthraw.adaptiveui

import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.os.Build
import android.util.Size
import java.util.Locale

/**
 * Runtime-only route discovery for the integrated FotoGraaf Pro camera.
 *
 * Nothing here grants scientific authority. A route is shown only when Camera2
 * advertises the relevant topology and RAW_SENSOR size at runtime. MAX routes
 * additionally require the maximum-resolution stream map. A route becomes
 * capture evidence only after an actual RAW image is timestamp-bound to its
 * TotalCaptureResult (and physical result when requested).
 */
internal data class FotoGraafProRoute(
    val label: String,
    val logicalCameraId: String,
    val physicalCameraId: String?,
    val rawSize: Size,
    val maximumResolution: Boolean,
    val focalLengthMm: Float?,
    val routeClass: String,
) {
    override fun toString(): String = label
}

internal object FotoGraafProRoutes {
    fun scan(manager: CameraManager): List<FotoGraafProRoute> {
        val listed = manager.cameraIdList.toList().sorted()
        val routes = mutableListOf<FotoGraafProRoute>()

        listed.forEach { logicalId ->
            val logical = runCatching { manager.getCameraCharacteristics(logicalId) }.getOrNull()
                ?: return@forEach
            val logicalCaps = logical.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            val logicalRaw = ordinaryRawSizes(logical)
            val logicalMax = maximumRawSizes(logical)

            if (logicalCaps.contains(CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_RAW) && logicalRaw.isNotEmpty()) {
                routes += makeRoute(logicalId, null, logicalRaw.first(), false, logical)
            }
            if (Build.VERSION.SDK_INT >= 31 && logicalMax.isNotEmpty()) {
                routes += makeRoute(logicalId, null, logicalMax.first(), true, logical)
            }

            if (Build.VERSION.SDK_INT >= 28) {
                logical.physicalCameraIds.toList().sorted().forEach { physicalId ->
                    val physical = runCatching { manager.getCameraCharacteristics(physicalId) }.getOrNull()
                        ?: return@forEach
                    ordinaryRawSizes(physical).firstOrNull()?.let { size ->
                        routes += makeRoute(logicalId, physicalId, size, false, physical)
                    }
                    if (Build.VERSION.SDK_INT >= 31) {
                        maximumRawSizes(physical).firstOrNull()?.let { size ->
                            routes += makeRoute(logicalId, physicalId, size, true, physical)
                        }
                    }
                }
            }
        }

        return routes
            .distinctBy {
                listOf(
                    it.logicalCameraId,
                    it.physicalCameraId ?: "-",
                    it.rawSize.width.toString(),
                    it.rawSize.height.toString(),
                    it.maximumResolution.toString(),
                ).joinToString("|")
            }
            .sortedWith(
                compareBy<FotoGraafProRoute> { preferredPhysicalRank(it.physicalCameraId) }
                    .thenBy { if (it.maximumResolution) 1 else 0 }
                    .thenBy { it.logicalCameraId }
                    .thenByDescending { it.rawSize.width.toLong() * it.rawSize.height.toLong() },
            )
    }

    fun effectiveCharacteristics(manager: CameraManager, route: FotoGraafProRoute): CameraCharacteristics =
        manager.getCameraCharacteristics(route.physicalCameraId ?: route.logicalCameraId)

    private fun ordinaryRawSizes(c: CameraCharacteristics): List<Size> =
        c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(ImageFormat.RAW_SENSOR)
            ?.toList()
            .orEmpty()
            .sortedByDescending { it.width.toLong() * it.height.toLong() }

    private fun maximumRawSizes(c: CameraCharacteristics): List<Size> {
        if (Build.VERSION.SDK_INT < 31) return emptyList()
        return c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
            ?.getOutputSizes(ImageFormat.RAW_SENSOR)
            ?.toList()
            .orEmpty()
            .sortedByDescending { it.width.toLong() * it.height.toLong() }
    }

    private fun makeRoute(
        logicalId: String,
        physicalId: String?,
        size: Size,
        maximumResolution: Boolean,
        effective: CameraCharacteristics,
    ): FotoGraafProRoute {
        val focal = effective.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)?.firstOrNull()
        val pixels = size.width.toLong() * size.height.toLong()
        val mp = pixels / 1_000_000.0
        val lens = friendlyLens(physicalId, focal)
        val mode = if (maximumResolution) {
            if (pixels >= 190_000_000L) "MAX · runtime ≈${String.format(Locale.ROOT, "%.1f", mp)} MP"
            else "MAX · runtime ${String.format(Locale.ROOT, "%.1f", mp)} MP"
        } else "RAW"
        val path = if (physicalId != null) "logical $logicalId → physical $physicalId" else "logical $logicalId"
        return FotoGraafProRoute(
            label = "$lens · $mode · $path · ${size.width}×${size.height}",
            logicalCameraId = logicalId,
            physicalCameraId = physicalId,
            rawSize = size,
            maximumResolution = maximumResolution,
            focalLengthMm = focal,
            routeClass = when {
                physicalId != null && maximumResolution -> "RUNTIME_ADVERTISED_PHYSICAL_MAXIMUM_RESOLUTION_RAW_SENSOR"
                physicalId != null -> "RUNTIME_ADVERTISED_PHYSICAL_RAW_SENSOR"
                maximumResolution -> "RUNTIME_ADVERTISED_LOGICAL_MAXIMUM_RESOLUTION_RAW_SENSOR"
                else -> "RUNTIME_ADVERTISED_LOGICAL_RAW_SENSOR"
            },
        )
    }

    private fun friendlyLens(physicalId: String?, focal: Float?): String {
        val byKnownHonorRoute = when (physicalId) {
            "4" -> "0.6× wide"
            "2" -> "1× main"
            "5" -> "3.7× tele"
            else -> null
        }
        val focalText = focal?.let { "${String.format(Locale.ROOT, "%.2f", it)} mm" } ?: "focal ?"
        return if (byKnownHonorRoute != null) "$byKnownHonorRoute · $focalText" else "Camera · $focalText"
    }

    private fun preferredPhysicalRank(id: String?): Int = when (id) {
        "4" -> 0
        "2" -> 1
        "5" -> 2
        null -> 4
        else -> 3
    }
}

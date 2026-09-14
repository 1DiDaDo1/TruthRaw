package com.truthraw.adaptiveui

import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.os.Build
import android.util.Size

internal data class HonorVendorValue(
    val name: String,
    val valueClass: String?,
    val value: String,
)

internal data class HonorCameraInventory(
    val cameraId: String,
    val physicalCameraIds: List<String>,
    val logicalMultiCamera: Boolean,
    val rawSizes: List<Size>,
    val maxResolutionRawSizes: List<Size>,
    val cfa: String,
    val focalLengths: List<Float>,
    val minFocusDistance: Float?,
    val oisModes: List<Int>,
    val honorCharacteristics: List<HonorVendorValue>,
    val honorRequestKeys: List<String>,
    val honorResultKeys: List<String>,
    val honorPhysicalRequestKeys: List<String>,
    val professionalTeleRawLogicalCameraId: Int?,
)

internal data class HonorRawRoute(
    val label: String,
    val logicalCameraId: String,
    val physicalCameraId: String?,
    val rawSize: Size,
    val routeClass: String,
    val stockGuidedCandidate: Boolean,
) {
    override fun toString(): String = label
}

internal data class HonorProbeReport(
    val inventories: List<HonorCameraInventory>,
    val routes: List<HonorRawRoute>,
    val reportText: String,
)

internal object HonorCameraProbe {
    private const val HONOR_PREFIX = "com.hihonor."
    private const val KEY_PRO_TELE_RAW_LOGICAL_ID =
        "com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID"

    @Suppress("UNCHECKED_CAST")
    fun scan(manager: CameraManager): HonorProbeReport {
        val ids = manager.cameraIdList.toList()
        val inventories = ids.mapNotNull { cameraId ->
            runCatching {
                val c = manager.getCameraCharacteristics(cameraId)
                val caps: List<Int> = (c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)
                    ?: intArrayOf()).toList()
                val streamMap = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val rawSizes = streamMap?.getOutputSizes(ImageFormat.RAW_SENSOR)
                    ?.toList().orEmpty().sortedByDescending { it.width.toLong() * it.height.toLong() }
                val maxMap = if (Build.VERSION.SDK_INT >= 31) {
                    c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                } else null
                val maxRawSizes = maxMap?.getOutputSizes(ImageFormat.RAW_SENSOR)
                    ?.toList().orEmpty().sortedByDescending { it.width.toLong() * it.height.toLong() }

                val honorCharacteristics = c.keys.asSequence()
                    .filter { it.name.startsWith(HONOR_PREFIX) }
                    .map { key ->
                        val value = runCatching { c.get(key as CameraCharacteristics.Key<Any>) }.getOrNull()
                        HonorVendorValue(key.name, value?.javaClass?.name, render(value))
                    }
                    .sortedBy { it.name }
                    .toList()

                val requestKeys = c.availableCaptureRequestKeys
                    .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
                val resultKeys = c.availableCaptureResultKeys
                    .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
                val physicalRequestKeys = if (Build.VERSION.SDK_INT >= 28) {
                    c.availablePhysicalCameraRequestKeys
                        .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
                } else emptyList()
                val physicalIds = if (Build.VERSION.SDK_INT >= 28) {
                    c.physicalCameraIds.toList().sorted()
                } else emptyList()

                HonorCameraInventory(
                    cameraId = cameraId,
                    physicalCameraIds = physicalIds,
                    logicalMultiCamera = caps.contains(
                        CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA,
                    ),
                    rawSizes = rawSizes,
                    maxResolutionRawSizes = maxRawSizes,
                    cfa = cfaName(c.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)),
                    focalLengths = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
                        ?.toList().orEmpty(),
                    minFocusDistance = c.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE),
                    oisModes = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION)
                        ?.toList().orEmpty(),
                    honorCharacteristics = honorCharacteristics,
                    honorRequestKeys = requestKeys,
                    honorResultKeys = resultKeys,
                    honorPhysicalRequestKeys = physicalRequestKeys,
                    professionalTeleRawLogicalCameraId = honorCharacteristics
                        .firstOrNull { it.name == KEY_PRO_TELE_RAW_LOGICAL_ID }
                        ?.value?.trim()?.toIntOrNull(),
                )
            }.getOrNull()
        }

        val routes = buildRoutes(manager, ids, inventories)
        return HonorProbeReport(inventories, routes, renderReport(inventories, routes))
    }

    private fun buildRoutes(
        manager: CameraManager,
        cameraIds: List<String>,
        inventories: List<HonorCameraInventory>,
    ): List<HonorRawRoute> {
        val routes = mutableListOf<HonorRawRoute>()
        val proTeleIds = inventories.mapNotNull { it.professionalTeleRawLogicalCameraId }.distinct()

        for (proTeleId in proTeleIds) {
            val id = proTeleId.toString()
            val inventory = inventories.firstOrNull { it.cameraId == id }
            val raw = inventory?.rawSizes?.firstOrNull()
            if (id in cameraIds && raw != null) {
                routes += HonorRawRoute(
                    label = "HONOR ProPhoto RAW tele · logical $id · ${raw.width}×${raw.height}",
                    logicalCameraId = id,
                    physicalCameraId = null,
                    rawSize = raw,
                    routeClass = "HAL_ADVERTISED_PRO_TELE_RAW_LOGICAL",
                    stockGuidedCandidate = true,
                )
            }
        }

        val logical0 = inventories.firstOrNull { it.cameraId == "0" }
        if (logical0?.physicalCameraIds?.contains("5") == true) {
            val physicalRaw = runCatching {
                manager.getCameraCharacteristics("5")
                    .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                    ?.getOutputSizes(ImageFormat.RAW_SENSOR)
                    ?.maxByOrNull { it.width.toLong() * it.height.toLong() }
            }.getOrNull()
            if (physicalRaw != null) {
                routes += HonorRawRoute(
                    label = "Bewezen routekandidaat · logical 0 → physical 5 · ${physicalRaw.width}×${physicalRaw.height}",
                    logicalCameraId = "0",
                    physicalCameraId = "5",
                    rawSize = physicalRaw,
                    routeClass = "LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT",
                    stockGuidedCandidate = false,
                )
            }
        }

        inventories.forEach { inventory ->
            val raw = inventory.rawSizes.firstOrNull() ?: return@forEach
            if (routes.none { it.logicalCameraId == inventory.cameraId && it.physicalCameraId == null }) {
                routes += HonorRawRoute(
                    label = "RAW logical ${inventory.cameraId} · ${raw.width}×${raw.height}",
                    logicalCameraId = inventory.cameraId,
                    physicalCameraId = null,
                    rawSize = raw,
                    routeClass = "ORDINARY_LOGICAL_RAW",
                    stockGuidedCandidate = false,
                )
            }
        }
        return routes
    }

    private fun renderReport(
        inventories: List<HonorCameraInventory>,
        routes: List<HonorRawRoute>,
    ): String = buildString {
        appendLine("TruthRaw FotoGraaf · HONOR runtime inventory v0.2")
        appendLine("Authority: CAPABILITY_OBSERVATION_ONLY")
        appendLine("Build: ${Build.MANUFACTURER} ${Build.MODEL} · ${Build.FINGERPRINT}")
        appendLine()
        appendLine("Route candidates (${routes.size})")
        routes.forEachIndexed { index, route ->
            appendLine("${index + 1}. ${route.label}")
            appendLine("   class=${route.routeClass} · stockGuided=${route.stockGuidedCandidate}")
        }
        appendLine()

        inventories.forEach { c ->
            appendLine("Camera ${c.cameraId}")
            appendLine("  logicalMultiCamera=${c.logicalMultiCamera}")
            appendLine("  physicalIds=${c.physicalCameraIds}")
            appendLine("  RAW=${c.rawSizes.joinToString { "${it.width}×${it.height}" }.ifBlank { "none" }}")
            appendLine("  MAX RAW=${c.maxResolutionRawSizes.joinToString { "${it.width}×${it.height}" }.ifBlank { "none" }}")
            appendLine("  CFA=${c.cfa}")
            appendLine("  focalLengths=${c.focalLengths}")
            appendLine("  minFocusDistance=${c.minFocusDistance}")
            appendLine("  OIS modes=${c.oisModes}")
            appendLine("  professionalTeleRawLogicalCameraID=${c.professionalTeleRawLogicalCameraId}")
            appendLine("  HONOR characteristics=${c.honorCharacteristics.size}")
            c.honorCharacteristics.forEach { v ->
                appendLine("    ${v.name} = ${v.value} [${v.valueClass ?: "null"}]")
            }
            appendLine("  HONOR request keys=${c.honorRequestKeys}")
            appendLine("  HONOR result keys=${c.honorResultKeys}")
            appendLine("  HONOR physical request keys=${c.honorPhysicalRequestKeys}")
            appendLine()
        }
    }

    private fun cfaName(value: Int?): String = when (value) {
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> "RGGB"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> "GRBG"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> "GBRG"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> "BGGR"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB -> "RGB"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_MONO -> "MONO"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_NIR -> "NIR"
        null -> "UNKNOWN"
        else -> "UNKNOWN($value)"
    }

    private fun render(value: Any?): String = when (value) {
        null -> "null"
        is ByteArray -> value.contentToString()
        is ShortArray -> value.contentToString()
        is IntArray -> value.contentToString()
        is LongArray -> value.contentToString()
        is FloatArray -> value.contentToString()
        is DoubleArray -> value.contentToString()
        is BooleanArray -> value.contentToString()
        is Array<*> -> value.contentDeepToString()
        else -> value.toString()
    }
}

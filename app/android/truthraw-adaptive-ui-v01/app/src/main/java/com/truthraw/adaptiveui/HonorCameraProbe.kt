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
    val listedByCameraIdList: Boolean,
    val discoveredAsPhysical: Boolean,
    val readStatus: String,
    val errors: List<String>,
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
    val rawCameraIds: List<String>,
    val discoveredPhysicalIds: List<String>,
    val topLevelDiscoveryError: String?,
    val inventories: List<HonorCameraInventory>,
    val routes: List<HonorRawRoute>,
    val reportText: String,
)

internal object HonorCameraProbe {
    private const val HONOR_PREFIX = "com.hihonor."
    private const val KEY_PRO_TELE_RAW_LOGICAL_ID =
        "com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID"

    fun scan(manager: CameraManager): HonorProbeReport {
        val rawIdsResult = runCatching { manager.cameraIdList.toList().sorted() }
        val rawIds = rawIdsResult.getOrDefault(emptyList())
        val topLevelError = rawIdsResult.exceptionOrNull()?.let(::renderError)

        // Pass 1: inspect every ID returned by CameraManager. Never silently drop an ID.
        val listedInventories = rawIds.map { cameraId ->
            inspectCamera(
                manager = manager,
                cameraId = cameraId,
                listedByCameraIdList = true,
                discoveredAsPhysical = false,
            )
        }

        // Camera2 may hide physical members from cameraIdList. Discover them from logical topology
        // and inspect their characteristics separately. They are NOT promoted to independently
        // openable logical cameras merely because characteristics are readable.
        val physicalIds = listedInventories
            .flatMap { it.physicalCameraIds }
            .distinct()
            .sorted()

        val hiddenPhysicalInventories = physicalIds
            .filterNot { it in rawIds }
            .map { physicalId ->
                inspectCamera(
                    manager = manager,
                    cameraId = physicalId,
                    listedByCameraIdList = false,
                    discoveredAsPhysical = true,
                )
            }

        val inventories = listedInventories + hiddenPhysicalInventories
        val routes = buildRoutes(rawIds, inventories)
        val text = renderReport(rawIds, physicalIds, topLevelError, inventories, routes)
        return HonorProbeReport(rawIds, physicalIds, topLevelError, inventories, routes, text)
    }

    @Suppress("UNCHECKED_CAST")
    private fun inspectCamera(
        manager: CameraManager,
        cameraId: String,
        listedByCameraIdList: Boolean,
        discoveredAsPhysical: Boolean,
    ): HonorCameraInventory {
        val errors = mutableListOf<String>()
        val c = runCatching { manager.getCameraCharacteristics(cameraId) }
            .onFailure { errors += "getCameraCharacteristics: ${renderError(it)}" }
            .getOrNull()

        if (c == null) {
            return HonorCameraInventory(
                cameraId = cameraId,
                listedByCameraIdList = listedByCameraIdList,
                discoveredAsPhysical = discoveredAsPhysical,
                readStatus = "FAILED",
                errors = errors,
                physicalCameraIds = emptyList(),
                logicalMultiCamera = false,
                rawSizes = emptyList(),
                maxResolutionRawSizes = emptyList(),
                cfa = "UNKNOWN",
                focalLengths = emptyList(),
                minFocusDistance = null,
                oisModes = emptyList(),
                honorCharacteristics = emptyList(),
                honorRequestKeys = emptyList(),
                honorResultKeys = emptyList(),
                honorPhysicalRequestKeys = emptyList(),
                professionalTeleRawLogicalCameraId = null,
            )
        }

        fun <T> read(field: String, fallback: T, block: () -> T): T =
            runCatching(block)
                .onFailure { errors += "$field: ${renderError(it)}" }
                .getOrDefault(fallback)

        val caps = read("REQUEST_AVAILABLE_CAPABILITIES", emptyList<Int>()) {
            (c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()).toList()
        }
        val physicalCameraIds = if (Build.VERSION.SDK_INT >= 28) {
            read("physicalCameraIds", emptyList()) { c.physicalCameraIds.toList().sorted() }
        } else emptyList()
        val streamMap = read("SCALER_STREAM_CONFIGURATION_MAP", null) {
            c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        }
        val rawSizes = read("RAW_SENSOR output sizes", emptyList()) {
            streamMap?.getOutputSizes(ImageFormat.RAW_SENSOR)
                ?.toList().orEmpty()
                .sortedByDescending { it.width.toLong() * it.height.toLong() }
        }
        val maxMap = if (Build.VERSION.SDK_INT >= 31) {
            read("SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION", null) {
                c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
            }
        } else null
        val maxRawSizes = read("MAXIMUM_RESOLUTION RAW_SENSOR output sizes", emptyList()) {
            maxMap?.getOutputSizes(ImageFormat.RAW_SENSOR)
                ?.toList().orEmpty()
                .sortedByDescending { it.width.toLong() * it.height.toLong() }
        }

        val honorKeys = read("CameraCharacteristics.keys", emptyList()) {
            c.keys.filter { it.name.startsWith(HONOR_PREFIX) }.sortedBy { it.name }
        }
        var professionalTeleRawLogicalCameraId: Int? = null
        val honorCharacteristics = honorKeys.map { key ->
            val valueResult = runCatching { c.get(key as CameraCharacteristics.Key<Any>) }
            val value = valueResult.getOrNull()
            if (valueResult.isFailure) {
                errors += "vendor characteristic ${key.name}: ${renderError(valueResult.exceptionOrNull()!!)}"
            }
            if (key.name == KEY_PRO_TELE_RAW_LOGICAL_ID) {
                professionalTeleRawLogicalCameraId = (value as? Number)?.toInt()
            }
            HonorVendorValue(
                name = key.name,
                valueClass = value?.javaClass?.name,
                value = if (valueResult.isFailure) "<READ_ERROR>" else render(value),
            )
        }

        val requestKeys = read("availableCaptureRequestKeys", emptyList()) {
            c.availableCaptureRequestKeys
                .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
        }
        val resultKeys = read("availableCaptureResultKeys", emptyList()) {
            c.availableCaptureResultKeys
                .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
        }
        val physicalRequestKeys = if (Build.VERSION.SDK_INT >= 28) {
            read("availablePhysicalCameraRequestKeys", emptyList()) {
                c.availablePhysicalCameraRequestKeys
                    .map { it.name }.filter { it.startsWith(HONOR_PREFIX) }.sorted()
            }
        } else emptyList()

        val cfa = read("SENSOR_INFO_COLOR_FILTER_ARRANGEMENT", "UNKNOWN") {
            cfaName(c.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT))
        }
        val focalLengths = read("LENS_INFO_AVAILABLE_FOCAL_LENGTHS", emptyList()) {
            c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)?.toList().orEmpty()
        }
        val minFocusDistance = read<Float?>("LENS_INFO_MINIMUM_FOCUS_DISTANCE", null) {
            c.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE)
        }
        val oisModes = read("LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION", emptyList()) {
            c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION)?.toList().orEmpty()
        }

        return HonorCameraInventory(
            cameraId = cameraId,
            listedByCameraIdList = listedByCameraIdList,
            discoveredAsPhysical = discoveredAsPhysical,
            readStatus = if (errors.isEmpty()) "PASS" else "PARTIAL",
            errors = errors.toList(),
            physicalCameraIds = physicalCameraIds,
            logicalMultiCamera = caps.contains(
                CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA,
            ),
            rawSizes = rawSizes,
            maxResolutionRawSizes = maxRawSizes,
            cfa = cfa,
            focalLengths = focalLengths,
            minFocusDistance = minFocusDistance,
            oisModes = oisModes,
            honorCharacteristics = honorCharacteristics,
            honorRequestKeys = requestKeys,
            honorResultKeys = resultKeys,
            honorPhysicalRequestKeys = physicalRequestKeys,
            professionalTeleRawLogicalCameraId = professionalTeleRawLogicalCameraId,
        )
    }

    private fun buildRoutes(
        rawCameraIds: List<String>,
        inventories: List<HonorCameraInventory>,
    ): List<HonorRawRoute> {
        val routes = mutableListOf<HonorRawRoute>()
        val listed = inventories.filter { it.listedByCameraIdList }
        val proTeleIds = listed.mapNotNull { it.professionalTeleRawLogicalCameraId }.distinct()

        for (proTeleId in proTeleIds) {
            val id = proTeleId.toString()
            val inventory = listed.firstOrNull { it.cameraId == id }
            val raw = inventory?.rawSizes?.firstOrNull()
            if (id in rawCameraIds && raw != null) {
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

        val logical0 = listed.firstOrNull { it.cameraId == "0" }
        val physical5 = inventories.firstOrNull { it.cameraId == "5" }
        if (logical0?.physicalCameraIds?.contains("5") == true) {
            val physicalRaw = physical5?.rawSizes?.firstOrNull()
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

        listed.forEach { inventory ->
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
        rawCameraIds: List<String>,
        discoveredPhysicalIds: List<String>,
        topLevelDiscoveryError: String?,
        inventories: List<HonorCameraInventory>,
        routes: List<HonorRawRoute>,
    ): String = buildString {
        appendLine("TruthRaw FotoGraaf · Camera2 discovery v0.3")
        appendLine("Authority: CAPABILITY_OBSERVATION_ONLY")
        appendLine("Build: ${Build.MANUFACTURER} ${Build.MODEL} · ${Build.FINGERPRINT}")
        appendLine("raw CameraManager.cameraIdList (${rawCameraIds.size}) = $rawCameraIds")
        appendLine("physical IDs from logical topology (${discoveredPhysicalIds.size}) = $discoveredPhysicalIds")
        appendLine("topLevelDiscoveryError=${topLevelDiscoveryError ?: "none"}")
        appendLine("inventories retained=${inventories.size} (IDs are never dropped because one field failed)")
        appendLine()

        appendLine("Route candidates (${routes.size})")
        routes.forEachIndexed { index, route ->
            appendLine("${index + 1}. ${route.label}")
            appendLine("   class=${route.routeClass} · stockGuided=${route.stockGuidedCandidate}")
        }
        appendLine()

        inventories.forEach { c ->
            appendLine("Camera ${c.cameraId} · status=${c.readStatus}")
            appendLine("  listedByCameraIdList=${c.listedByCameraIdList} · discoveredAsPhysical=${c.discoveredAsPhysical}")
            if (c.errors.isNotEmpty()) {
                appendLine("  readErrors (${c.errors.size}):")
                c.errors.forEach { appendLine("    - $it") }
            }
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

    private fun renderError(error: Throwable): String =
        "${error.javaClass.simpleName}: ${error.message ?: "no message"}"
}

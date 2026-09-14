package com.truthraw.fotograafcapture

import android.app.Activity
import android.content.ContentValues
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.util.Range
import android.util.Size
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.time.Instant
import java.util.Date
import java.util.Locale

/**
 * Read-only Camera2 capability inventory for HONOR BKQ-N49 / Magic8 Pro research.
 *
 * Scientific rule: characteristics are capability observations, not proof that a specific
 * physical lens actually produced a later RAW. Capture-time physical TotalCaptureResult +
 * exact source sealing are still required before C0 authority can advance.
 *
 * The probe intentionally uses reflection for ImageFormat.RAW14 so the current Android-16 /
 * compileSdk-35 branch can remain buildable while still detecting RAW14 on Android 17+ runtimes.
 */
class HonorCapabilityProbeActivity : Activity() {
    private lateinit var cameraManager: CameraManager
    private lateinit var statusView: TextView
    private lateinit var scanButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(CAMERA_SERVICE) as CameraManager
        buildUi()
    }

    private fun buildUi() {
        val pad = (16 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        root.addView(TextView(this).apply {
            text = "TruthRaw FotoGraaf · HONOR physical-camera capability probe v0.1"
            textSize = 20f
        })
        root.addView(TextView(this).apply {
            text = "Inventories logical/physical Camera2 capabilities for standard RAW, maximum-resolution RAW, close-focus/macro controls and runtime RAW14 advertisement. No capture is made and no calibration authority is granted."
            textSize = 14f
            setPadding(0, pad / 2, 0, pad)
        })
        scanButton = Button(this).apply {
            text = "Scan physical camera capabilities"
            setOnClickListener { runProbe() }
        }
        root.addView(scanButton, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        statusView = TextView(this).apply {
            textSize = 12f
            setPadding(0, pad, 0, pad)
            text = "Ready. This is a read-only capability observation."
        }
        root.addView(statusView)
        setContentView(ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        })
    }

    private fun runProbe() {
        scanButton.isEnabled = false
        try {
            val report = buildReport()
            val name = "HONOR_PHYSICAL_CAMERA_CAPABILITY_${SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())}.json"
            saveReport(name, report)
            statusView.text = summarize(report, name)
        } catch (error: Exception) {
            statusView.text = "FAIL CLOSED: ${error.message ?: error.javaClass.simpleName}"
        } finally {
            scanButton.isEnabled = true
        }
    }

    private fun buildReport(): JSONObject {
        val raw14 = raw14FormatOrNull()
        val logicals = JSONArray()
        val physicalRoutes = JSONArray()
        val directCameras = JSONArray()

        for (logicalId in cameraManager.cameraIdList.sorted()) {
            val logicalChars = cameraManager.getCameraCharacteristics(logicalId)
            val logicalCaps = logicalChars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            val logicalMulti = logicalCaps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA)
            val physicalIds = if (logicalMulti) logicalChars.physicalCameraIds.sorted() else emptyList()
            logicals.put(cameraSummary(logicalId, logicalChars, raw14, logicalMulti, physicalIds))

            if (physicalIds.isEmpty()) {
                directCameras.put(cameraSummary(logicalId, logicalChars, raw14, false, emptyList()))
            } else {
                for (physicalId in physicalIds) {
                    val route = JSONObject()
                        .put("logicalCameraId", logicalId)
                        .put("physicalCameraId", physicalId)
                        .put("projectHistoricalRoleHint", historicalRoleHint(physicalId))
                        .put("roleHintAuthority", "HISTORICAL_PROJECT_HINT_NOT_CAPTURE_PROOF")
                        .put("logicalContainer", rawMapsSummary(logicalChars, raw14))
                    try {
                        val physicalChars = cameraManager.getCameraCharacteristics(physicalId)
                        route.put("physicalCharacteristicsReadable", true)
                        route.put("physical", cameraSummary(physicalId, physicalChars, raw14, false, emptyList()))
                        route.put("routeAssessment", assessRoute(logicalChars, physicalChars, raw14))
                    } catch (error: Exception) {
                        route.put("physicalCharacteristicsReadable", false)
                        route.put("physicalCharacteristicsError", error.message ?: error.javaClass.simpleName)
                        route.put("routeAssessment", JSONObject()
                            .put("standardRawStatus", "LOGICAL_OUTPUT_REQUIRES_RUNTIME_SESSION_PROBE")
                            .put("maximumResolutionRawStatus", "UNKNOWN_PHYSICAL_CHARACTERISTICS")
                            .put("raw14Status", if (raw14 == null) "PLATFORM_FIELD_NOT_PRESENT" else "UNKNOWN_PHYSICAL_CHARACTERISTICS")
                            .put("macroStatus", "UNKNOWN_PHYSICAL_CHARACTERISTICS")
                            .put("native200MpStatus", "OPEN_REQUIRES_ADVERTISED_MAX_RAW_AND_CAPTURE_PROOF"))
                    }
                    physicalRoutes.put(route)
                }
            }
        }

        return JSONObject()
            .put("schema", "truthraw.fotograaf-honor-physical-camera-capability-probe.v0.1")
            .put("createdAtUtc", Instant.now().toString())
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("buildFingerprint", Build.FINGERPRINT))
            .put("platform", JSONObject()
                .put("compileStrategy", "RAW14_RUNTIME_REFLECTION")
                .put("raw14ImageFormatFieldPresent", raw14 != null)
                .put("raw14FormatValue", raw14 ?: JSONObject.NULL)
                .put("android17OrLaterRuntime", Build.VERSION.SDK_INT >= 37))
            .put("logicalCameras", logicals)
            .put("physicalRoutes", physicalRoutes)
            .put("directCameras", directCameras)
            .put("authority", JSONObject()
                .put("recordClass", "CAMERA2_CAPABILITY_OBSERVATION_ONLY")
                .put("captureAuthorityGranted", false)
                .put("calibrationAuthorityGranted", false)
                .put("physicalLensIdentityProvenForFutureCapture", false)
                .put("native200MpRawProven", false)
                .put("raw14CaptureProven", false)
                .put("macroCaptureProven", false)
                .put("requiredNextEvidence", JSONArray(listOf(
                    "runtime session support for selected physical output",
                    "physical TotalCaptureResult matches requested physical ID",
                    "RAW/result timestamp identity",
                    "finalized source SHA-256",
                    "C0 capture identity envelope",
                    "sample-domain and gain/readout classification before calibration authority"
                ))))
    }

    private fun cameraSummary(
        cameraId: String,
        chars: CameraCharacteristics,
        raw14: Int?,
        logicalMulti: Boolean,
        physicalIds: List<String>,
    ): JSONObject {
        val caps = chars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
        val focalLengths = chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS) ?: floatArrayOf()
        val minFocus = chars.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE)
        val afModes = chars.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        val oisModes = chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
        val sensitivity = chars.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
        val exposure = chars.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
        val pixel = chars.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
        val active = chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)
        val pixelMax = chars.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)
        val activeMax = chars.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)
        val focusCalibration = chars.get(CameraCharacteristics.LENS_INFO_FOCUS_DISTANCE_CALIBRATION)

        return JSONObject()
            .put("cameraId", cameraId)
            .put("logicalMultiCamera", logicalMulti)
            .put("advertisedPhysicalCameraIds", JSONArray(physicalIds))
            .put("capabilities", JSONArray(caps.toList()))
            .put("rawCapability", caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW))
            .put("manualSensorCapability", caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR))
            .put("ultraHighResolutionSensorCapability", caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_ULTRA_HIGH_RESOLUTION_SENSOR))
            .put("sensor", JSONObject()
                .put("pixelArray", sizeJson(pixel))
                .put("activeArray", rectJson(active))
                .put("maximumResolutionPixelArray", sizeJson(pixelMax))
                .put("maximumResolutionActiveArray", rectJson(activeMax))
                .put("cfaPattern", cfaName(chars.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)))
                .put("sensitivityRange", rangeJson(sensitivity))
                .put("exposureTimeRangeNs", rangeJson(exposure)))
            .put("lens", JSONObject()
                .put("focalLengthsMm", JSONArray(focalLengths.toList()))
                .put("minimumFocusDistanceDiopters", minFocus ?: JSONObject.NULL)
                .put("approximateObjectDistanceCmFromDiopters", minFocus?.takeIf { it > 0f }?.let { 100.0 / it.toDouble() } ?: JSONObject.NULL)
                .put("focusDistanceCalibration", focusCalibrationName(focusCalibration))
                .put("afModes", JSONArray(afModes.toList()))
                .put("afMacroAdvertised", afModes.contains(CameraMetadata.CONTROL_AF_MODE_MACRO))
                .put("oisModes", JSONArray(oisModes.toList())))
            .put("rawMaps", rawMapsSummary(chars, raw14))
    }

    private fun rawMapsSummary(chars: CameraCharacteristics, raw14: Int?): JSONObject {
        val normal = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val max = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        val normalRaw = outputSizes(normal, ImageFormat.RAW_SENSOR)
        val maxRaw = outputSizes(max, ImageFormat.RAW_SENSOR)
        val normalRaw14 = raw14?.let { outputSizes(normal, it) }.orEmpty()
        val maxRaw14 = raw14?.let { outputSizes(max, it) }.orEmpty()
        return JSONObject()
            .put("standardRawSensorSizes", sizesJson(normalRaw))
            .put("maximumResolutionRawSensorSizes", sizesJson(maxRaw))
            .put("standardRaw14Sizes", sizesJson(normalRaw14))
            .put("maximumResolutionRaw14Sizes", sizesJson(maxRaw14))
            .put("largestStandardRawSensor", largestSizeJson(normalRaw))
            .put("largestMaximumResolutionRawSensor", largestSizeJson(maxRaw))
            .put("largestStandardRaw14", largestSizeJson(normalRaw14))
            .put("largestMaximumResolutionRaw14", largestSizeJson(maxRaw14))
    }

    private fun assessRoute(logical: CameraCharacteristics, physical: CameraCharacteristics, raw14: Int?): JSONObject {
        val logicalNormal = outputSizes(logical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP), ImageFormat.RAW_SENSOR)
        val physicalNormal = outputSizes(physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP), ImageFormat.RAW_SENSOR)
        val logicalMax = outputSizes(logical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION), ImageFormat.RAW_SENSOR)
        val physicalMax = outputSizes(physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION), ImageFormat.RAW_SENSOR)
        val physicalRaw14 = raw14?.let { outputSizes(physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP), it) }.orEmpty()
        val physicalRaw14Max = raw14?.let { outputSizes(physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION), it) }.orEmpty()
        val maxLargest = (physicalMax + logicalMax).maxByOrNull { it.width.toLong() * it.height.toLong() }
        val maxPixels = maxLargest?.let { it.width.toLong() * it.height.toLong() } ?: 0L
        val afModes = physical.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        val minFocus = physical.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f

        val standardStatus = when {
            physicalNormal.isNotEmpty() -> "PHYSICAL_RAW_ADVERTISED_REQUIRES_CAPTURE_PROOF"
            logicalNormal.isNotEmpty() -> "LOGICAL_RAW_ONLY_REQUIRES_RUNTIME_PHYSICAL_SESSION_PROBE"
            else -> "NO_RAW_SENSOR_STREAM_ADVERTISED"
        }
        val maxStatus = when {
            physicalMax.isNotEmpty() -> "PHYSICAL_MAX_RAW_ADVERTISED_REQUIRES_CAPTURE_PROOF"
            logicalMax.isNotEmpty() -> "LOGICAL_MAX_RAW_ONLY_REQUIRES_RUNTIME_PHYSICAL_SESSION_PROBE"
            else -> "NO_MAXIMUM_RESOLUTION_RAW_SENSOR_STREAM_ADVERTISED"
        }
        val raw14Status = when {
            raw14 == null -> "PLATFORM_FIELD_NOT_PRESENT"
            physicalRaw14Max.isNotEmpty() -> "PHYSICAL_MAX_RAW14_ADVERTISED_REQUIRES_CAPTURE_PROOF"
            physicalRaw14.isNotEmpty() -> "PHYSICAL_RAW14_ADVERTISED_REQUIRES_CAPTURE_PROOF"
            else -> "RAW14_NOT_ADVERTISED_BY_PHYSICAL_CHARACTERISTICS"
        }
        val macroStatus = when {
            afModes.contains(CameraMetadata.CONTROL_AF_MODE_MACRO) -> "AF_MACRO_ADVERTISED_REQUIRES_CAPTURE_VALIDATION"
            minFocus > 0f -> "CLOSE_FOCUS_CHARACTERISTIC_PRESENT_REQUIRES_CAPTURE_VALIDATION"
            else -> "NO_MACRO_OR_CLOSE_FOCUS_CHARACTERISTIC_ADVERTISED"
        }
        val mp = maxPixels.toDouble() / 1_000_000.0
        val native200 = when {
            maxPixels >= 180_000_000L -> "MAX_RAW_180MP_PLUS_CANDIDATE_REQUIRES_PHYSICAL_CAPTURE_PROOF"
            maxPixels > 0L -> "MAX_RAW_PRESENT_BELOW_180MP_${"%.2f".format(Locale.US, mp)}MP"
            else -> "NO_MAX_RAW_ADVERTISED"
        }

        return JSONObject()
            .put("standardRawStatus", standardStatus)
            .put("maximumResolutionRawStatus", maxStatus)
            .put("raw14Status", raw14Status)
            .put("macroStatus", macroStatus)
            .put("native200MpStatus", native200)
            .put("largestCandidateMaximumRawMegapixels", if (maxPixels > 0L) mp else JSONObject.NULL)
            .put("productionReady", false)
    }

    private fun outputSizes(map: android.hardware.camera2.params.StreamConfigurationMap?, format: Int): List<Size> {
        if (map == null) return emptyList()
        return try {
            map.getOutputSizes(format)?.toList().orEmpty().sortedByDescending { it.width.toLong() * it.height.toLong() }
        } catch (_: Exception) {
            emptyList()
        }
    }

    private fun raw14FormatOrNull(): Int? = try {
        ImageFormat::class.java.getField("RAW14").getInt(null)
    } catch (_: Exception) {
        null
    }

    private fun sizesJson(sizes: List<Size>): JSONArray = JSONArray().also { array ->
        sizes.forEach { size ->
            array.put(JSONObject()
                .put("width", size.width)
                .put("height", size.height)
                .put("pixels", size.width.toLong() * size.height.toLong())
                .put("megapixels", size.width.toDouble() * size.height.toDouble() / 1_000_000.0))
        }
    }

    private fun largestSizeJson(sizes: List<Size>): Any {
        val size = sizes.maxByOrNull { it.width.toLong() * it.height.toLong() } ?: return JSONObject.NULL
        return JSONObject()
            .put("width", size.width)
            .put("height", size.height)
            .put("pixels", size.width.toLong() * size.height.toLong())
            .put("megapixels", size.width.toDouble() * size.height.toDouble() / 1_000_000.0)
    }

    private fun sizeJson(size: Size?): Any = size?.let {
        JSONObject().put("width", it.width).put("height", it.height).put("pixels", it.width.toLong() * it.height.toLong())
    } ?: JSONObject.NULL

    private fun rectJson(rect: android.graphics.Rect?): Any = rect?.let {
        JSONObject().put("left", it.left).put("top", it.top).put("right", it.right).put("bottom", it.bottom)
            .put("width", it.width()).put("height", it.height())
    } ?: JSONObject.NULL

    private fun rangeJson(range: Range<*>?): Any = range?.let {
        JSONObject().put("lower", it.lower.toString()).put("upper", it.upper.toString())
    } ?: JSONObject.NULL

    private fun focusCalibrationName(value: Int?): String = when (value) {
        CameraMetadata.LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED -> "UNCALIBRATED"
        CameraMetadata.LENS_INFO_FOCUS_DISTANCE_CALIBRATION_APPROXIMATE -> "APPROXIMATE"
        CameraMetadata.LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED -> "CALIBRATED"
        null -> "NOT_AVAILABLE"
        else -> "UNRECOGNIZED_$value"
    }

    private fun cfaName(value: Int?): String = when (value) {
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> "RGGB"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> "GRBG"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> "GBRG"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> "BGGR"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB -> "RGB"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_MONO -> "MONO"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_NIR -> "NIR"
        else -> "UNRECOGNIZED_${value ?: -1}"
    }

    private fun historicalRoleHint(physicalId: String): String = when (physicalId) {
        "2" -> "PROJECT_HISTORY_MAIN_CANDIDATE"
        "4" -> "PROJECT_HISTORY_WIDE_CANDIDATE"
        "5" -> "PROJECT_HISTORY_TELE_CANDIDATE"
        else -> "NO_PROJECT_ROLE_HINT"
    }

    private fun saveReport(displayName: String, report: JSONObject) {
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, displayName)
            put(MediaStore.MediaColumns.MIME_TYPE, "application/json")
            put(MediaStore.MediaColumns.RELATIVE_PATH, "${Environment.DIRECTORY_DOWNLOADS}/TruthRawFotoGraaf/capability-probes")
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }
        val uri = contentResolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values)
            ?: throw IllegalStateException("MediaStore insert returned null")
        contentResolver.openOutputStream(uri, "w")!!.bufferedWriter(Charsets.UTF_8).use { writer ->
            writer.write(report.toString(2))
            writer.write("\n")
        }
        val done = ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) }
        if (contentResolver.update(uri, done, null, null) <= 0) {
            throw IllegalStateException("Could not finalize capability report")
        }
    }

    private fun summarize(report: JSONObject, name: String): String {
        val routes = report.getJSONArray("physicalRoutes")
        val lines = mutableListOf<String>()
        lines += "PASS CAPABILITY OBSERVATION: saved $name"
        lines += "Device=${Build.MANUFACTURER} ${Build.MODEL}, SDK=${Build.VERSION.SDK_INT}, RAW14-field=${report.getJSONObject("platform").getBoolean("raw14ImageFormatFieldPresent")}"
        lines += "Physical routes=${routes.length()}"
        for (i in 0 until routes.length()) {
            val route = routes.getJSONObject(i)
            val assessment = route.getJSONObject("routeAssessment")
            lines += "logical ${route.getString("logicalCameraId")} -> physical ${route.getString("physicalCameraId")}: maxRAW=${assessment.getString("maximumResolutionRawStatus")}; 200MP=${assessment.getString("native200MpStatus")}; macro=${assessment.getString("macroStatus")}; RAW14=${assessment.getString("raw14Status")}"
        }
        lines += "No capability line above is capture proof. Physical TotalCaptureResult + sealed RAW remain mandatory."
        return lines.joinToString("\n")
    }
}

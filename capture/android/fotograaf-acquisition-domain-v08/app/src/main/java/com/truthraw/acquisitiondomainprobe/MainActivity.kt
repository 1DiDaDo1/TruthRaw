package com.truthraw.acquisitiondomainprobe

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.graphics.Rect
import android.hardware.camera2.*
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Rational
import android.util.Size
import android.util.SizeF
import android.view.View
import android.widget.TextView
import androidx.core.content.FileProvider
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.concurrent.Executor
import org.json.JSONArray
import org.json.JSONObject

/**
 * TruthRaw FotoGraaf Acquisition Domain Probe v0.8.
 *
 * Evidence-only Android Camera2 probe. This module does not modify TruthRaw's scientific master,
 * does not grant calibration authority, and does not treat APK implementation claims as TruthRaw
 * evidence. Device-runtime outputs are observations that still require independent validation.
 */
class MainActivity : Activity() {
    private lateinit var cm: CameraManager
    private lateinit var logView: TextView
    private val cameraThread = HandlerThread("truthraw-fotograaf-v08-camera").apply { start() }
    private val handler = Handler(cameraThread.looper)
    private val executor = Executor { r -> handler.post(r) }

    private var pendingPermissionAction: (() -> Unit)? = null
    private var standardInventory: JSONObject? = null
    private var honorInventory: JSONObject? = null
    private var latestFiles = mutableListOf<File>()

    private val telePhysicalId = "5"
    private val preferredLogicalId = "0"
    private val teleFocalMm = 22.48
    private val regularRaw = Size(4080, 3072)
    private val maximumRaw = Size(16320, 12288)

    enum class CaptureDomain {
        REGULAR_4080,
        MAXIMUM_200MP
    }

    data class Candidate(
        val physicalId: String,
        val parentLogicalId: String?,
        val openId: String,
        val size: Size,
        val domain: CaptureDomain,
        val physicalCharacteristics: CameraCharacteristics,
        val discoverySource: String
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        cm = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        logView = findViewById(R.id.log)

        findViewById<View>(R.id.standard_inventory).setOnClickListener {
            ensurePermission { handler.post { runStandardInventory() } }
        }
        findViewById<View>(R.id.honor_inventory).setOnClickListener {
            ensurePermission { handler.post { runHonorInventory() } }
        }
        findViewById<View>(R.id.capture_regular).setOnClickListener {
            ensurePermission { handler.post { captureDomain(CaptureDomain.REGULAR_4080) } }
        }
        findViewById<View>(R.id.capture_maximum).setOnClickListener {
            ensurePermission { handler.post { captureDomain(CaptureDomain.MAXIMUM_200MP) } }
        }
        findViewById<View>(R.id.share).setOnClickListener { shareEvidence() }

        log("v0.8 ready. Inventories are discovery-only; captures remain CAMERA2_ACQUISITION_OBSERVATION_ONLY.")
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraThread.quitSafely()
    }

    private fun ensurePermission(action: () -> Unit) {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            action()
        } else {
            pendingPermissionAction = action
            requestPermissions(arrayOf(Manifest.permission.CAMERA), 80)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 80 && grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            pendingPermissionAction?.invoke()
        } else if (requestCode == 80) {
            log("Camera permission denied; no evidence collected.")
        }
        pendingPermissionAction = null
    }

    private fun log(message: String) = runOnUiThread { logView.append(message + "\n") }

    private fun outDir(): File = File(getExternalFilesDir(null), "truthraw_fotograaf_acquisition_domain_v08").apply { mkdirs() }

    private fun runStandardInventory() {
        try {
            val root = buildStandardInventory()
            standardInventory = root
            val file = File(outDir(), "truthraw_standard_camera2_inventory_v08.json")
            file.writeText(root.toString(2))
            remember(file)
            log("Stap 1 klaar: ${file.absolutePath}")
            log(summarizeStandard(root))
        } catch (t: Throwable) {
            log("Stap 1 ERROR: $t")
        }
    }

    private fun runHonorInventory() {
        try {
            val root = buildHonorInventory()
            honorInventory = root
            val file = File(outDir(), "truthraw_honor_vendor_inventory_v08.json")
            file.writeText(root.toString(2))
            remember(file)
            log("Stap 2 klaar: ${file.absolutePath}")
            log("HONOR inventory is vendor observation only; it is not calibration authority.")
        } catch (t: Throwable) {
            log("Stap 2 ERROR: $t")
        }
    }

    private fun ensureInventories() {
        if (standardInventory == null) {
            standardInventory = buildStandardInventory().also {
                val f = File(outDir(), "truthraw_standard_camera2_inventory_v08.json")
                f.writeText(it.toString(2)); remember(f)
            }
        }
        if (honorInventory == null) {
            honorInventory = buildHonorInventory().also {
                val f = File(outDir(), "truthraw_honor_vendor_inventory_v08.json")
                f.writeText(it.toString(2)); remember(f)
            }
        }
    }

    private fun buildStandardInventory(): JSONObject {
        val root = JSONObject()
        root.put("schema", "truthraw.fotograaf-standard-camera2-inventory.v0.8")
        root.put("evidenceLayer", "DISCOVERY_ONLY")
        root.put("authority", "STANDARD_CAMERA2_CAPABILITY_OBSERVATION")
        root.put("calibrationAuthorityGranted", false)
        root.put("createdAtUtc", Instant.now().toString())
        root.put("device", deviceJson())
        root.put("cameraIdList", JSONArray(cm.cameraIdList.toList()))
        val cameras = JSONArray()
        val seen = mutableSetOf<String>()
        for (id in cm.cameraIdList) {
            cameras.put(standardCameraRecord(id, null))
            seen.add(id)
            try {
                val c = cm.getCameraCharacteristics(id)
                for (pid in c.physicalCameraIds) {
                    val key = "$id::$pid"
                    if (seen.add(key)) cameras.put(standardCameraRecord(pid, id))
                }
            } catch (_: Throwable) {}
        }
        root.put("cameras", cameras)
        return root
    }

    private fun buildHonorInventory(): JSONObject {
        val root = JSONObject()
        root.put("schema", "truthraw.fotograaf-honor-vendor-inventory.v0.8")
        root.put("evidenceLayer", "DISCOVERY_ONLY")
        root.put("authority", "HONOR_VENDOR_METADATA_OBSERVATION_ONLY")
        root.put("calibrationAuthorityGranted", false)
        root.put("createdAtUtc", Instant.now().toString())
        root.put("device", deviceJson())
        val cameras = JSONArray()
        val seen = mutableSetOf<String>()
        for (id in cm.cameraIdList) {
            cameras.put(honorCameraRecord(id, null))
            seen.add(id)
            try {
                val c = cm.getCameraCharacteristics(id)
                for (pid in c.physicalCameraIds) {
                    val key = "$id::$pid"
                    if (seen.add(key)) cameras.put(honorCameraRecord(pid, id))
                }
            } catch (_: Throwable) {}
        }
        root.put("cameras", cameras)
        root.put("semantics", "Vendor key names/values are empirical observations. Their meanings are not promoted to TruthRaw calibration facts without independent validation.")
        return root
    }

    private fun standardCameraRecord(id: String, parent: String?): JSONObject {
        val o = JSONObject()
        o.put("cameraId", id)
        o.put("parentLogicalId", parent)
        try {
            val c = cm.getCameraCharacteristics(id)
            val capabilities = c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            o.put("physicalCameraIds", JSONArray(c.physicalCameraIds.toList()))
            o.put("capabilities", JSONArray(capabilities.toList()))
            o.put("lensFacing", c.get(CameraCharacteristics.LENS_FACING))
            o.put("focalLengthsMm", JSONArray((c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS) ?: floatArrayOf()).map { it.toDouble() }))
            o.put("apertures", JSONArray((c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES) ?: floatArrayOf()).map { it.toDouble() }))
            o.put("sensorPhysicalSizeMm", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE)))
            o.put("cfaArrangement", cfaName(c.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)))
            o.put("whiteLevel", c.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL))
            o.put("blackLevelPattern", c.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)?.let { bl ->
                JSONArray(listOf(
                    bl.getOffsetForIndex(0, 0), bl.getOffsetForIndex(1, 0),
                    bl.getOffsetForIndex(0, 1), bl.getOffsetForIndex(1, 1)
                ))
            })
            o.put("pixelArray", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)))
            o.put("activeArray", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
            o.put("preCorrectionActiveArray", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)))
            o.put("pixelArrayMaximumResolution", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
            o.put("activeArrayMaximumResolution", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
            o.put("preCorrectionActiveArrayMaximumResolution", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
            o.put("sensorInfoBinningFactor", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR)))
            o.put("sensorInfoLensShadingApplied", c.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED))
            o.put("defaultRaw", rawFormats(c, false, false))
            o.put("maximumResolutionRaw", rawFormats(c, true, false))
            o.put("maximumResolutionHighResolutionRaw", rawFormats(c, true, true))
            o.put("sensorPixelModeRequestKeyPresent", c.availableCaptureRequestKeys?.any { it.name == "android.sensor.pixelMode" } == true)
            o.put("sensorRawBinningFactorResultKeyPresent", c.availableCaptureResultKeys?.any { it.name == "android.sensor.rawBinningFactorUsed" } == true)
            o.put("physicalOverrideRequestKeyNames", JSONArray(try { c.availablePhysicalCameraRequestKeys?.map { it.name }?.sorted() ?: emptyList<String>() } catch (_: Throwable) { emptyList<String>() }))
        } catch (t: Throwable) {
            o.put("queryError", t.toString())
        }
        return o
    }

    private fun honorCameraRecord(id: String, parent: String?): JSONObject {
        val o = JSONObject()
        o.put("cameraId", id)
        o.put("parentLogicalId", parent)
        try {
            val c = cm.getCameraCharacteristics(id)
            val values = JSONObject()
            for (key in c.keys.sortedBy { it.name }) {
                if (!key.name.startsWith("com.hihonor.")) continue
                values.put(key.name, safeCharacteristicValue(c, key))
            }
            o.put("honorCharacteristics", values)
            o.put("honorRequestKeyNames", JSONArray(c.availableCaptureRequestKeys?.map { it.name }?.filter { it.startsWith("com.hihonor.") }?.sorted() ?: emptyList<String>()))
            o.put("honorResultKeyNames", JSONArray(c.availableCaptureResultKeys?.map { it.name }?.filter { it.startsWith("com.hihonor.") }?.sorted() ?: emptyList<String>()))
        } catch (t: Throwable) {
            o.put("queryError", t.toString())
        }
        return o
    }

    @Suppress("UNCHECKED_CAST")
    private fun safeCharacteristicValue(c: CameraCharacteristics, key: CameraCharacteristics.Key<*>): Any {
        return try {
            jsonValue(c.get(key as CameraCharacteristics.Key<Any>))
        } catch (t: Throwable) {
            JSONObject().put("readError", t.toString())
        }
    }

    private fun rawFormats(c: CameraCharacteristics, maximum: Boolean, highResolutionOnly: Boolean): JSONObject {
        val map = if (maximum) c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                  else c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val out = JSONObject()
        if (map == null) return out
        for ((name, format) in listOf(
            "RAW_SENSOR" to ImageFormat.RAW_SENSOR,
            "RAW10" to ImageFormat.RAW10,
            "RAW12" to ImageFormat.RAW12
        )) {
            try {
                val sizes = if (highResolutionOnly) map.getHighResolutionOutputSizes(format) else map.getOutputSizes(format)
                out.put(name, sizesJson(sizes))
            } catch (_: Throwable) {
                out.put(name, JSONArray())
            }
        }
        return out
    }

    private fun sizesJson(sizes: Array<Size>?): JSONArray = JSONArray(
        (sizes ?: emptyArray()).sortedByDescending { it.width.toLong() * it.height }.map { listOf(it.width, it.height) }
    )

    private fun findCandidate(domain: CaptureDomain): Candidate? {
        val physical = try { cm.getCameraCharacteristics(telePhysicalId) } catch (t: Throwable) {
            log("Physical camera $telePhysicalId characteristics unavailable: $t")
            return null
        }
        val focal = physical.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)?.map { it.toDouble() } ?: emptyList()
        if (focal.none { kotlin.math.abs(it - teleFocalMm) <= 0.05 }) {
            log("Fail closed: camera $telePhysicalId does not report focal length near $teleFocalMm mm: $focal")
            return null
        }

        val parent = findParentLogical(telePhysicalId)
        val openId = when {
            parent != null -> parent
            cm.cameraIdList.contains(telePhysicalId) -> telePhysicalId
            else -> {
                log("Fail closed: camera $telePhysicalId is neither openable nor advertised as a physical child.")
                return null
            }
        }

        val desired = if (domain == CaptureDomain.REGULAR_4080) regularRaw else maximumRaw
        val found = when (domain) {
            CaptureDomain.REGULAR_4080 -> containsSize(
                physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)?.getOutputSizes(ImageFormat.RAW_SENSOR), desired
            )
            CaptureDomain.MAXIMUM_200MP -> containsSize(
                physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR), desired
            )
        }
        if (!found) {
            log("Fail closed: ${domain.name} exact RAW_SENSOR ${desired.width}x${desired.height} was not advertised in its required standard Camera2 list.")
            return null
        }

        return Candidate(
            physicalId = telePhysicalId,
            parentLogicalId = parent,
            openId = openId,
            size = desired,
            domain = domain,
            physicalCharacteristics = physical,
            discoverySource = if (domain == CaptureDomain.REGULAR_4080) "SCALER_STREAM_CONFIGURATION_MAP" else "SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION.getHighResolutionOutputSizes"
        )
    }

    private fun findParentLogical(physicalId: String): String? {
        if (preferredLogicalId in cm.cameraIdList) {
            try {
                if (physicalId in cm.getCameraCharacteristics(preferredLogicalId).physicalCameraIds) return preferredLogicalId
            } catch (_: Throwable) {}
        }
        for (id in cm.cameraIdList) {
            try {
                if (physicalId in cm.getCameraCharacteristics(id).physicalCameraIds) return id
            } catch (_: Throwable) {}
        }
        return null
    }

    private fun containsSize(sizes: Array<Size>?, wanted: Size): Boolean = sizes?.any { it.width == wanted.width && it.height == wanted.height } == true

    private fun captureDomain(domain: CaptureDomain) {
        try {
            ensureInventories()
            val c = findCandidate(domain) ?: return
            log("Stap 3 ${domain.name}: opening ${c.openId}; physical=${c.physicalId}; ${c.size.width}x${c.size.height} RAW_SENSOR")
            @Suppress("MissingPermission")
            cm.openCamera(c.openId, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) = createCaptureSession(device, c)
                override fun onDisconnected(device: CameraDevice) { log("Camera disconnected"); device.close() }
                override fun onError(device: CameraDevice, error: Int) { log("Camera open error=$error"); device.close() }
            }, handler)
        } catch (t: Throwable) {
            log("Capture setup ERROR: $t")
        }
    }

    private fun createCaptureSession(device: CameraDevice, c: Candidate) {
        val reader = ImageReader.newInstance(c.size.width, c.size.height, ImageFormat.RAW_SENSOR, 1)
        var captureResult: TotalCaptureResult? = null
        var image: Image? = null
        var finished = false

        fun cleanup() {
            try { image?.close() } catch (_: Throwable) {}
            try { reader.close() } catch (_: Throwable) {}
            try { device.close() } catch (_: Throwable) {}
        }

        fun finishIfReady() {
            if (finished) return
            val im = image ?: return
            val result = captureResult ?: return
            finished = true
            try { persistCapture(device, c, im, result) }
            catch (t: Throwable) { log("Persist ERROR: $t") }
            finally { cleanup() }
        }

        reader.setOnImageAvailableListener({ r ->
            try {
                image = r.acquireNextImage()
                finishIfReady()
            } catch (t: Throwable) {
                log("Image callback ERROR: $t")
                cleanup()
            }
        }, handler)

        val output = OutputConfiguration(reader.surface)
        if (c.parentLogicalId != null) output.setPhysicalCameraId(c.physicalId)
        if (c.domain == CaptureDomain.MAXIMUM_200MP) {
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        } else {
            try { output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT) } catch (_: Throwable) {}
        }

        val sessionConfig = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            executor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    try {
                        val request = if (c.parentLogicalId != null) {
                            try { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(c.physicalId)) }
                            catch (_: Throwable) { device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE) }
                        } else {
                            device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                        }
                        request.addTarget(reader.surface)
                        val openedChars = cm.getCameraCharacteristics(c.openId)
                        val desiredPixelMode = if (c.domain == CaptureDomain.MAXIMUM_200MP) CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION else CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT
                        var pixelModeGlobalWritten = false
                        var pixelModePhysicalWritten = false
                        if (hasRequestKey(openedChars, "android.sensor.pixelMode")) {
                            try { request.set(CaptureRequest.SENSOR_PIXEL_MODE, desiredPixelMode); pixelModeGlobalWritten = true } catch (_: Throwable) {}
                        }
                        if (c.parentLogicalId != null && physicalOverrideSupported(openedChars, CaptureRequest.SENSOR_PIXEL_MODE)) {
                            try { request.setPhysicalCameraKey(CaptureRequest.SENSOR_PIXEL_MODE, desiredPixelMode, c.physicalId); pixelModePhysicalWritten = true } catch (_: Throwable) {}
                        }
                        request.setTag(JSONObject().put("truthrawDomain", c.domain.name).put("pixelModeGlobalWritten", pixelModeGlobalWritten).put("pixelModePhysicalWritten", pixelModePhysicalWritten).toString())
                        setIfSupported(request, CaptureRequest.CONTROL_ENABLE_ZSL, false, openedChars)
                        applyEvidenceControls(request, openedChars)
                        if (c.parentLogicalId != null) applyPhysicalEvidenceControls(request, openedChars, c.physicalId)
                        session.capture(request.build(), object : CameraCaptureSession.CaptureCallback() {
                            override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                                captureResult = result
                                finishIfReady()
                            }
                            override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: CaptureFailure) {
                                log("Capture failed reason=${failure.reason}")
                                cleanup()
                            }
                        }, handler)
                    } catch (t: Throwable) {
                        log("Capture request ERROR: $t")
                        cleanup()
                    }
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    log("Session configuration failed for ${c.domain.name}; no route proof granted.")
                    cleanup()
                }
            }
        )

        try {
            val supported = try { device.isSessionConfigurationSupported(sessionConfig) } catch (_: Throwable) { null }
            log("SessionConfiguration supported=$supported")
            val supportFile = File(outDir(), "truthraw_${c.domain.name.lowercase()}_session_support_v08.json")
            supportFile.writeText(JSONObject()
                .put("schema", "truthraw.fotograaf-session-support.v0.8")
                .put("authority", "CAMERA2_SESSION_CAPABILITY_OBSERVATION_ONLY")
                .put("openedCameraId", c.openId)
                .put("physicalCameraId", c.physicalId)
                .put("parentLogicalId", c.parentLogicalId)
                .put("size", JSONArray(listOf(c.size.width, c.size.height)))
                .put("domain", c.domain.name)
                .put("isSessionConfigurationSupported", supported)
                .toString(2))
            remember(supportFile)
            if (supported == false) {
                log("Fail closed: Android reports session unsupported.")
                cleanup()
                return
            }
            device.createCaptureSession(sessionConfig)
        } catch (t: Throwable) {
            log("createCaptureSession ERROR: $t")
            cleanup()
        }
    }

    private fun hasRequestKey(c: CameraCharacteristics, name: String): Boolean = c.availableCaptureRequestKeys?.any { it.name == name } == true

    private fun physicalOverrideSupported(c: CameraCharacteristics, key: CaptureRequest.Key<*>): Boolean =
        try { c.availablePhysicalCameraRequestKeys?.any { it.name == key.name } == true } catch (_: Throwable) { false }

    private fun <T> setIfSupported(builder: CaptureRequest.Builder, key: CaptureRequest.Key<T>, value: T, c: CameraCharacteristics) {
        try { if (c.availableCaptureRequestKeys?.contains(key) == true) builder.set(key, value) } catch (_: Throwable) {}
    }

    private fun <T> setPhysicalIfSupported(builder: CaptureRequest.Builder, key: CaptureRequest.Key<T>, value: T, logical: CameraCharacteristics, physicalId: String) {
        try { if (physicalOverrideSupported(logical, key)) builder.setPhysicalCameraKey(key, value, physicalId) } catch (_: Throwable) {}
    }

    private fun modeAvailable(values: IntArray?, wanted: Int): Boolean = values?.contains(wanted) == true

    private fun applyEvidenceControls(builder: CaptureRequest.Builder, c: CameraCharacteristics) {
        if (modeAvailable(c.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES), CaptureRequest.NOISE_REDUCTION_MODE_OFF))
            setIfSupported(builder, CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES), CaptureRequest.HOT_PIXEL_MODE_OFF))
            setIfSupported(builder, CaptureRequest.HOT_PIXEL_MODE, CaptureRequest.HOT_PIXEL_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.SHADING_AVAILABLE_MODES), CaptureRequest.SHADING_MODE_OFF))
            setIfSupported(builder, CaptureRequest.SHADING_MODE, CaptureRequest.SHADING_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES), CaptureRequest.EDGE_MODE_OFF))
            setIfSupported(builder, CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES), CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF))
            setIfSupported(builder, CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE, CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.DISTORTION_CORRECTION_AVAILABLE_MODES), CaptureRequest.DISTORTION_CORRECTION_MODE_OFF))
            setIfSupported(builder, CaptureRequest.DISTORTION_CORRECTION_MODE, CaptureRequest.DISTORTION_CORRECTION_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES), CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF))
            setIfSupported(builder, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION), CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE_OFF))
            setIfSupported(builder, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE_OFF, c)
        if (modeAvailable(c.get(CameraCharacteristics.STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES), CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE_ON))
            setIfSupported(builder, CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE, CaptureRequest.STATISTICS_LENS_SHADING_MAP_MODE_ON, c)
    }

    private fun applyPhysicalEvidenceControls(builder: CaptureRequest.Builder, logical: CameraCharacteristics, physicalId: String) {
        val physical = cm.getCameraCharacteristics(physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES), CaptureRequest.NOISE_REDUCTION_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.NOISE_REDUCTION_MODE, CaptureRequest.NOISE_REDUCTION_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES), CaptureRequest.HOT_PIXEL_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.HOT_PIXEL_MODE, CaptureRequest.HOT_PIXEL_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.SHADING_AVAILABLE_MODES), CaptureRequest.SHADING_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.SHADING_MODE, CaptureRequest.SHADING_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES), CaptureRequest.EDGE_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.EDGE_MODE, CaptureRequest.EDGE_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES), CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE, CaptureRequest.COLOR_CORRECTION_ABERRATION_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.DISTORTION_CORRECTION_AVAILABLE_MODES), CaptureRequest.DISTORTION_CORRECTION_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.DISTORTION_CORRECTION_MODE, CaptureRequest.DISTORTION_CORRECTION_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES), CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE, CaptureRequest.CONTROL_VIDEO_STABILIZATION_MODE_OFF, logical, physicalId)
        if (modeAvailable(physical.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION), CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE_OFF))
            setPhysicalIfSupported(builder, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE_OFF, logical, physicalId)
    }

    private fun persistCapture(device: CameraDevice, c: Candidate, image: Image, result: TotalCaptureResult) {
        val physicalResult = if (c.parentLogicalId != null) result.physicalCameraTotalResults[c.physicalId] else null
        val effectiveResult: CaptureResult = physicalResult ?: result
        val sensorTimestamp = effectiveResult.get(CaptureResult.SENSOR_TIMESTAMP)

        val plane = image.planes[0]
        val pixelStride = plane.pixelStride
        val rowStride = plane.rowStride
        val source = plane.buffer.duplicate().apply { rewind() }
        val accessibleBytes = source.remaining().toLong()
        val expectedContiguousBytes = image.width.toLong() * image.height.toLong() * 2L
        val contiguous = pixelStride == 2 && rowStride == image.width * 2 && accessibleBytes == expectedContiguousBytes

        val captureId = "C2DOMAIN_${System.currentTimeMillis()}_${if (c.domain == CaptureDomain.REGULAR_4080) "REG" else "MAX"}"
        val raw = File(outDir(), "${captureId}_${image.width}x${image.height}.${if (contiguous) "rawsensor" else "rawbuffer"}")
        val digest = MessageDigest.getInstance("SHA-256")
        digest.update(plane.buffer.duplicate().apply { rewind() })
        val rawHash = digest.digest().joinToString("") { "%02x".format(it) }
        FileOutputStream(raw).channel.use { channel ->
            val buf = plane.buffer.duplicate().apply { rewind() }
            while (buf.hasRemaining()) channel.write(buf)
            channel.force(true)
        }
        remember(raw)

        var dng: File? = null
        var dngError: String? = null
        try {
            dng = File(outDir(), "${captureId}_${image.width}x${image.height}.dng")
            FileOutputStream(dng).use { os ->
                DngCreator(c.physicalCharacteristics, effectiveResult).use { creator ->
                    creator.setDescription("TruthRaw FotoGraaf v0.8 app-visible RAW evidence; source raw payload SHA-256=$rawHash")
                    creator.setOrientation(1)
                    creator.writeImage(os, image)
                }
            }
            remember(dng)
        } catch (t: Throwable) {
            dngError = t.toString()
            dng = null
        }

        val observation = JSONObject()
        observation.put("schema", "truthraw.fotograaf-acquisition-domain-observation.v0.8")
        observation.put("captureId", captureId)
        observation.put("createdAtUtc", Instant.now().toString())
        observation.put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
        observation.put("calibrationAuthorityGranted", false)
        observation.put("c0IdentitySealed", false)
        observation.put("scientificMasterModified", false)
        observation.put("physicalFrameCount", 1)
        observation.put("independentEvidenceCount", 1)
        observation.put("device", deviceJson())
        observation.put("inventorySeparation", JSONObject()
            .put("standardCamera2", "truthraw.fotograaf-standard-camera2-inventory.v0.8")
            .put("honorVendor", "truthraw.fotograaf-honor-vendor-inventory.v0.8")
            .put("inventoriesCountAsIndependentCaptureEvidence", false))

        observation.put("route", JSONObject()
            .put("routeClass", if (c.parentLogicalId != null) "LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT" else "DIRECT_PHYSICAL_CAMERA_OPEN")
            .put("openedCameraId", device.id)
            .put("logicalCameraId", c.parentLogicalId)
            .put("requestedPhysicalCameraId", c.physicalId)
            .put("confirmedPhysicalResultCameraId", physicalResult?.cameraId)
            .put("logicalActivePhysicalId", result.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID))
            .put("discoverySource", c.discoverySource)
            .put("stockGuidedCandidate", false))

        observation.put("topology", JSONObject()
            .put("format", "RAW_SENSOR")
            .put("width", image.width)
            .put("height", image.height)
            .put("cfa", cfaName(c.physicalCharacteristics.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)))
            .put("appVisibleDirectCfaSamples", true)
            .put("untouchedPhotodiodeAdcTruthClaimed", false)
            .put("processedRgb", false)
            .put("mergedMultiFrame", false))

        val sampleDomain = buildSampleDomain(c, effectiveResult)
        val captureResultJson = buildCaptureResult(effectiveResult, image.timestamp)
        val timestampPass = sensorTimestamp != null && sensorTimestamp == image.timestamp
        val physicalBindingPass = c.parentLogicalId == null || physicalResult?.cameraId == c.physicalId
        val focalResult = effectiveResult.get(CaptureResult.LENS_FOCAL_LENGTH)?.toDouble()
        val focalPass = focalResult != null && kotlin.math.abs(focalResult - teleFocalMm) <= 0.05
        val exactSizePass = image.width == c.size.width && image.height == c.size.height
        val appliedPixelMode = effectiveResult.get(CaptureResult.SENSOR_PIXEL_MODE)
        val pixelModePass = if (c.domain == CaptureDomain.MAXIMUM_200MP)
            appliedPixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION
        else appliedPixelMode == null || appliedPixelMode == CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT
        val routeProofPass = timestampPass && physicalBindingPass && focalPass && exactSizePass
        val sampleDomainProofPass = routeProofPass && pixelModePass && sampleDomain.optString("classification") != "UNRESOLVED_RAW_BINNING_STATE"

        observation.put("sampleDomain", sampleDomain)
        observation.put("captureResult", captureResultJson)
        observation.put("proofGates", JSONObject()
            .put("exactRawSizePass", exactSizePass)
            .put("timestampIdentityPass", timestampPass)
            .put("physicalBindingPass", physicalBindingPass)
            .put("focalLength22_48mmPass", focalPass)
            .put("requestedPixelModeObservedPass", pixelModePass)
            .put("routeProofPass", routeProofPass)
            .put("sampleDomainProofPass", sampleDomainProofPass)
            .put("calibrationAuthorityGranted", false)
            .put("c0IdentitySealed", false))
        observation.put("honorVendorResults", vendorResultJson(effectiveResult))
        observation.put("rawPayload", JSONObject()
            .put("file", raw.name)
            .put("sha256", rawHash)
            .put("bytes", raw.length())
            .put("rowStride", rowStride)
            .put("pixelStride", pixelStride)
            .put("accessibleBufferBytes", accessibleBytes)
            .put("expectedContiguousBytes", expectedContiguousBytes)
            .put("canonicalContiguousRawSensor", contiguous)
            .put("imageTimestampNs", image.timestamp)
            .put("timestampIdentityPass", sensorTimestamp != null && sensorTimestamp == image.timestamp))
        observation.put("dng", JSONObject()
            .put("attempted", true)
            .put("file", dng?.name)
            .put("sha256", dng?.let { sha256File(it) })
            .put("bytes", dng?.length())
            .put("error", dngError)
            .put("semantics", "DngCreator convenience container. The separately hashed app-visible Image.Plane payload is the primary byte evidence."))
        observation.put("openCalibrationBlockers", JSONArray(listOf(
            "cameraSystemIdMappingIndependentValidation",
            "gainReadoutStateId",
            "focusStateClass",
            "stabilizationStateClass",
            "independentOpticalFovValidation",
            "upstreamSensorHalProcessingBoundary"
        )))

        val obsFile = File(outDir(), "${captureId}_acquisition_domain_observation_v08.json")
        obsFile.writeText(observation.toString(2))
        remember(obsFile)

        log("Capture complete: $captureId")
        log("route=${observation.getJSONObject("route").getString("routeClass")} physicalResult=${physicalResult?.cameraId}")
        log("RAW sha256=$rawHash bytes=${raw.length()} timestampMatch=${sensorTimestamp != null && sensorTimestamp == image.timestamp}")
        log("routeProofPass=${observation.getJSONObject("proofGates").getBoolean("routeProofPass")} sampleDomainProofPass=${observation.getJSONObject("proofGates").getBoolean("sampleDomainProofPass")}")
        log("sampleDomain=${observation.getJSONObject("sampleDomain").optString("classification")}")
        if (dng != null) log("DNG=${dng.name} sha256=${sha256File(dng)}") else log("DNG not written: $dngError")
    }

    private fun buildSampleDomain(c: Candidate, result: CaptureResult): JSONObject {
        val cc = c.physicalCharacteristics
        val capabilities = (cc.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()).toSet()
        val uhr = capabilities.contains(CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_ULTRA_HIGH_RESOLUTION_SENSOR)
        val remosaic = capabilities.contains(CameraMetadata.REQUEST_AVAILABLE_CAPABILITIES_REMOSAIC_REPROCESSING)
        val binningFactor = cc.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR)
        val rawBinningUsed = result.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED)
        val pixelMode = result.get(CaptureResult.SENSOR_PIXEL_MODE)

        val classification = when {
            !uhr -> "REGULAR_BAYER_BY_ANDROID_CAPABILITY_CONTRACT"
            !remosaic -> "REGULAR_BAYER_UHR_NO_REMOSAIC_BY_ANDROID_CAPABILITY_CONTRACT"
            rawBinningUsed == false -> "REGULAR_BAYER_RAW_BINNING_FACTOR_NOT_USED"
            rawBinningUsed == true -> "GROUPED_CFA_RAW_BINNING_FACTOR_USED"
            else -> "UNRESOLVED_RAW_BINNING_STATE"
        }

        val maxArray = cc.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)
        val defaultArray = cc.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)
        val geometry = JSONObject()
        geometry.put("defaultPixelArray", jsonValue(defaultArray))
        geometry.put("maximumResolutionPixelArray", jsonValue(maxArray))
        geometry.put("defaultActiveArray", jsonValue(cc.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
        geometry.put("maximumResolutionActiveArray", jsonValue(cc.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        geometry.put("defaultPreCorrectionActiveArray", jsonValue(cc.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)))
        geometry.put("maximumResolutionPreCorrectionActiveArray", jsonValue(cc.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION)))
        if (maxArray != null && c.size.width > 0 && c.size.height > 0) {
            geometry.put("maxArrayToOutputLinearRatioX", maxArray.width.toDouble() / c.size.width.toDouble())
            geometry.put("maxArrayToOutputLinearRatioY", maxArray.height.toDouble() / c.size.height.toDouble())
            geometry.put("ratioSemantics", "Geometry relation only. It is not by itself proof of hardware binning, remosaic, or one output sample per original photodiode.")
        }

        return JSONObject()
            .put("classificationAuthority", "STANDARD_CAMERA2_METADATA_DERIVATION_ONLY")
            .put("classification", classification)
            .put("captureDomain", c.domain.name)
            .put("requestedPixelMode", if (c.domain == CaptureDomain.MAXIMUM_200MP) "MAXIMUM_RESOLUTION" else "DEFAULT")
            .put("appliedPixelMode", when (pixelMode) {
                CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT -> "DEFAULT"
                CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> "MAXIMUM_RESOLUTION"
                else -> JSONObject.NULL
            })
            .put("sensorInfoBinningFactor", jsonValue(binningFactor))
            .put("sensorRawBinningFactorUsed", rawBinningUsed)
            .put("uhrCapability", uhr)
            .put("remosaicReprocessingCapability", remosaic)
            .put("lensShadingAppliedToRaw", cc.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED))
            .put("geometry", geometry)
            .put("scientificBoundary", "App-visible Camera2 RAW sample domain only. No claim of untouched photodiode/ADC values or absence of upstream sensor/HAL operations.")
    }

    private fun buildCaptureResult(result: CaptureResult, imageTimestamp: Long): JSONObject {
        val sensorTimestamp = result.get(CaptureResult.SENSOR_TIMESTAMP)
        return JSONObject()
            .put("resultCameraId", result.cameraId)
            .put("sensorTimestampNs", sensorTimestamp)
            .put("imageTimestampNs", imageTimestamp)
            .put("timestampIdentityPass", sensorTimestamp != null && sensorTimestamp == imageTimestamp)
            .put("iso", result.get(CaptureResult.SENSOR_SENSITIVITY))
            .put("exposureTimeNs", result.get(CaptureResult.SENSOR_EXPOSURE_TIME))
            .put("frameDurationNs", result.get(CaptureResult.SENSOR_FRAME_DURATION))
            .put("rollingShutterSkewNs", result.get(CaptureResult.SENSOR_ROLLING_SHUTTER_SKEW))
            .put("focalLengthMm", result.get(CaptureResult.LENS_FOCAL_LENGTH))
            .put("focusDistanceDiopters", result.get(CaptureResult.LENS_FOCUS_DISTANCE))
            .put("afMode", result.get(CaptureResult.CONTROL_AF_MODE))
            .put("afState", result.get(CaptureResult.CONTROL_AF_STATE))
            .put("oisMode", result.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE))
            .put("videoStabilizationMode", result.get(CaptureResult.CONTROL_VIDEO_STABILIZATION_MODE))
            .put("noiseReductionMode", result.get(CaptureResult.NOISE_REDUCTION_MODE))
            .put("hotPixelMode", result.get(CaptureResult.HOT_PIXEL_MODE))
            .put("shadingMode", result.get(CaptureResult.SHADING_MODE))
            .put("edgeMode", result.get(CaptureResult.EDGE_MODE))
            .put("aberrationMode", result.get(CaptureResult.COLOR_CORRECTION_ABERRATION_MODE))
            .put("distortionCorrectionMode", result.get(CaptureResult.DISTORTION_CORRECTION_MODE))
            .put("dynamicBlackLevel", jsonValue(result.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL)))
            .put("dynamicWhiteLevel", result.get(CaptureResult.SENSOR_DYNAMIC_WHITE_LEVEL))
            .put("noiseProfile", result.get(CaptureResult.SENSOR_NOISE_PROFILE)?.let { np -> JSONArray(np.flatMap { listOf(it.first, it.second) }) })
    }

    @Suppress("UNCHECKED_CAST")
    private fun vendorResultJson(result: CaptureResult): JSONObject {
        val out = JSONObject()
        for (key in result.keys.sortedBy { it.name }) {
            if (!key.name.startsWith("com.hihonor.")) continue
            try {
                out.put(key.name, jsonValue(result.get(key as CaptureResult.Key<Any>)))
            } catch (t: Throwable) {
                out.put(key.name, JSONObject().put("readError", t.toString()))
            }
        }
        return out
    }

    private fun cfaName(value: Int?): Any = when (value) {
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> "RGGB"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> "GRBG"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> "GBRG"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> "BGGR"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB -> "RGB"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_MONO -> "MONO"
        CameraMetadata.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_NIR -> "NIR"
        null -> JSONObject.NULL
        else -> value
    }

    private fun jsonValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is JSONObject, is JSONArray, is String, is Boolean, is Int, is Long, is Double, is Float -> value
        is Byte -> value.toInt()
        is Short -> value.toInt()
        is Size -> JSONArray(listOf(value.width, value.height))
        is SizeF -> JSONArray(listOf(value.width.toDouble(), value.height.toDouble()))
        is Rect -> JSONArray(listOf(value.left, value.top, value.right, value.bottom))
        is Rational -> if (value.denominator != 0) value.toDouble() else value.toString()
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.map { jsonValue(it) })
        is Collection<*> -> JSONArray(value.map { jsonValue(it) })
        else -> value.toString()
    }

    private fun deviceJson(): JSONObject = JSONObject()
        .put("manufacturer", android.os.Build.MANUFACTURER)
        .put("brand", android.os.Build.BRAND)
        .put("model", android.os.Build.MODEL)
        .put("device", android.os.Build.DEVICE)
        .put("fingerprint", android.os.Build.FINGERPRINT)
        .put("sdkInt", android.os.Build.VERSION.SDK_INT)

    private fun summarizeStandard(root: JSONObject): String {
        val cameras = root.getJSONArray("cameras")
        val lines = mutableListOf<String>()
        for (i in 0 until cameras.length()) {
            val c = cameras.getJSONObject(i)
            if (c.optString("cameraId") !in setOf("0", "2", "4", "5")) continue
            lines.add("id=${c.optString("cameraId")} parent=${c.optString("parentLogicalId")} focal=${c.optJSONArray("focalLengthsMm")} defaultRAW=${c.optJSONObject("defaultRaw")?.optJSONArray("RAW_SENSOR")} maxHighRAW=${c.optJSONObject("maximumResolutionHighResolutionRaw")?.optJSONArray("RAW_SENSOR")}")
        }
        return lines.joinToString("\n")
    }

    private fun remember(file: File) {
        latestFiles.removeAll { it.absolutePath == file.absolutePath }
        latestFiles.add(file)
    }

    private fun sha256File(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input ->
            val buffer = ByteArray(1 shl 20)
            while (true) {
                val n = input.read(buffer)
                if (n < 0) break
                md.update(buffer, 0, n)
            }
        }
        return md.digest().joinToString("") { "%02x".format(it) }
    }

    private fun shareEvidence() {
        val files = latestFiles.filter { it.exists() }
        if (files.isEmpty()) {
            log("No evidence files yet.")
            return
        }
        try {
            val uris = ArrayList<Uri>(files.map { FileProvider.getUriForFile(this, "${packageName}.files", it) })
            val intent = Intent(Intent.ACTION_SEND_MULTIPLE).apply {
                type = "application/octet-stream"
                putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                putExtra(Intent.EXTRA_SUBJECT, "TruthRaw FotoGraaf acquisition-domain v0.8 evidence")
            }
            startActivity(Intent.createChooser(intent, "Share TruthRaw evidence"))
        } catch (t: Throwable) {
            log("Share ERROR: $t")
        }
    }
}

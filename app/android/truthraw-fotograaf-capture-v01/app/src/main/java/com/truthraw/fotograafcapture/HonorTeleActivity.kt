package com.truthraw.fotograafcapture

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.ContentValues
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureFailure
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.DngCreator
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.HandlerThread
import android.provider.MediaStore
import android.text.InputType
import android.util.Size
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.text.SimpleDateFormat
import java.time.Instant
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executor

/**
 * Device-specific research route for HONOR BKQ-N49.
 *
 * It does NOT treat the historic lens mapping as scientific evidence. The route asks the logical
 * multi-camera for physical output ID 5 and accepts a capture only when Camera2 returns a matching
 * physical TotalCaptureResult for ID 5. No sample-domain or gain/readout classification is created.
 */
class HonorTeleActivity : Activity() {
    private data class Route(
        val logicalCameraId: String,
        val physicalCameraId: String,
        val rawSize: Size,
        val rawSizeAuthority: String,
        val cfaPattern: String,
        val physicalRawCapabilityAdvertised: Boolean,
        val logicalManualSensor: Boolean,
        val advertisedPhysicalIds: List<String>,
    )

    private data class HashAndLength(val sha256: String, val byteLength: Long)

    private lateinit var cameraManager: CameraManager
    private lateinit var statusView: TextView
    private lateinit var isoEdit: EditText
    private lateinit var exposureEdit: EditText
    private lateinit var captureButton: Button
    private var route: Route? = null
    private var engine: CaptureEngine? = null
    private val sessionId = "HONOR_TELE_" + SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        buildUi()
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            scanRoute()
        } else {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
        }
    }

    override fun onDestroy() {
        engine?.close()
        engine = null
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA_PERMISSION && grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            scanRoute()
        } else {
            setStatus("CAMERA permission is required.")
        }
    }

    private fun buildUi() {
        val pad = (16 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        root.addView(TextView(this).apply {
            text = "TruthRaw FotoGraaf · HONOR tele forced physical route v0.2"
            textSize = 20f
        })
        root.addView(TextView(this).apply {
            text = "This route is for BKQ-N49 testing. It requests physical camera ID 5 through the logical multi-camera. The capture is rejected unless Camera2 supplies a matching physical result for ID 5. Lens mapping remains a project hypothesis until capture evidence confirms it."
            textSize = 14f
            setPadding(0, pad / 2, 0, pad)
        })
        isoEdit = EditText(this).apply {
            hint = "Requested ISO"
            inputType = InputType.TYPE_CLASS_NUMBER
            setText("100")
        }
        root.addView(isoEdit)
        exposureEdit = EditText(this).apply {
            hint = "Requested exposure ns"
            inputType = InputType.TYPE_CLASS_NUMBER
            setText("16367398")
        }
        root.addView(exposureEdit)
        root.addView(Button(this).apply {
            text = "Rescan HONOR physical tele route"
            setOnClickListener { scanRoute() }
        })
        captureButton = Button(this).apply {
            text = "Force physical ID 5 RAW_SENSOR capture"
            isEnabled = false
            setOnClickListener { capture() }
        }
        root.addView(captureButton, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        statusView = TextView(this).apply {
            textSize = 13f
            setPadding(0, pad, 0, pad)
        }
        root.addView(statusView)
        setContentView(ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        })
    }

    private fun scanRoute() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) return
        try {
            val found = findHonorTeleRoute()
            route = found
            captureButton.isEnabled = found != null && engine == null
            if (found == null) {
                setStatus("BLOCKED: no logical multi-camera advertising physical ID 5 with a usable RAW_SENSOR stream was found.")
            } else {
                setStatus(
                    "READY CANDIDATE: logical=${found.logicalCameraId}, physical=${found.physicalCameraId}, " +
                        "RAW=${found.rawSize.width}×${found.rawSize.height}, CFA=${found.cfaPattern}, " +
                        "sizeAuthority=${found.rawSizeAuthority}, physicalRawAdvertised=${found.physicalRawCapabilityAdvertised}, " +
                        "manual=${found.logicalManualSensor}. Capture acceptance still requires physical result ID 5."
                )
            }
        } catch (error: Exception) {
            route = null
            captureButton.isEnabled = false
            setStatus("Route scan failed: ${error.message ?: error.javaClass.simpleName}")
        }
    }

    private fun findHonorTeleRoute(): Route? {
        for (logicalId in cameraManager.cameraIdList) {
            val logicalChars = cameraManager.getCameraCharacteristics(logicalId)
            val logicalCaps = logicalChars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            if (!logicalCaps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA)) continue
            val physicalIds = logicalChars.physicalCameraIds.sorted()
            if (!physicalIds.contains(PHYSICAL_TELE_ID)) continue

            val physicalChars = cameraManager.getCameraCharacteristics(PHYSICAL_TELE_ID)
            val physicalCaps = physicalChars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            val physicalRaw = physicalCaps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)
            val physicalSizes = physicalChars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                ?.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
            val logicalSizes = logicalChars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                ?.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()

            val chosenPhysical = chooseCalibrationSize(physicalSizes)
            val chosenLogical = chooseCalibrationSize(logicalSizes)
            val chosen = chosenPhysical ?: chosenLogical ?: continue
            val sizeAuthority = if (chosenPhysical != null) "PHYSICAL_RAW_STREAM_MAP" else "LOGICAL_RAW_STREAM_MAP_FALLBACK_FOR_PHYSICAL_OUTPUT"
            val cfa = cfaName(physicalChars.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT))
            val manual = logicalCaps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR)
            if (!manual) continue

            return Route(
                logicalCameraId = logicalId,
                physicalCameraId = PHYSICAL_TELE_ID,
                rawSize = chosen,
                rawSizeAuthority = sizeAuthority,
                cfaPattern = cfa,
                physicalRawCapabilityAdvertised = physicalRaw,
                logicalManualSensor = manual,
                advertisedPhysicalIds = physicalIds,
            )
        }
        return null
    }

    private fun chooseCalibrationSize(sizes: List<Size>): Size? {
        if (sizes.isEmpty()) return null
        return sizes.firstOrNull { it.width == 4096 && it.height == 3072 }
            ?: sizes.firstOrNull { it.width == 4080 && it.height == 3072 }
            ?: sizes.filter { it.width.toLong() * it.height.toLong() <= 20_000_000L }
                .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.minByOrNull { kotlin.math.abs(it.width.toLong() * it.height.toLong() - 12_600_000L) }
    }

    private fun capture() {
        if (engine != null) return
        val selected = route ?: return
        val iso = isoEdit.text.toString().toIntOrNull()
        val exposureNs = exposureEdit.text.toString().toLongOrNull()
        if (iso == null || iso <= 0 || exposureNs == null || exposureNs <= 0L) {
            setStatus("ISO and exposure must be positive integers.")
            return
        }
        captureButton.isEnabled = false
        setStatus("Opening logical ${selected.logicalCameraId} and forcing physical output ${selected.physicalCameraId} …")
        engine = CaptureEngine(selected, iso, exposureNs) { message ->
            runOnUiThread {
                setStatus(message)
                engine?.close()
                engine = null
                captureButton.isEnabled = route != null
            }
        }.also { it.start() }
    }

    private fun setStatus(text: String) {
        statusView.text = text
    }

    private inner class CaptureEngine(
        private val route: Route,
        private val requestedIso: Int,
        private val requestedExposureNs: Long,
        private val done: (String) -> Unit,
    ) {
        private val thread = HandlerThread("truthraw-honor-tele").apply { start() }
        private val handler = Handler(thread.looper)
        private val executor = Executor { handler.post(it) }
        private val lock = Any()
        private var reader: ImageReader? = null
        private var device: CameraDevice? = null
        private var session: CameraCaptureSession? = null
        private var image: Image? = null
        private var result: TotalCaptureResult? = null
        private var closed = false

        @SuppressLint("MissingPermission")
        fun start() {
            try {
                reader = ImageReader.newInstance(route.rawSize.width, route.rawSize.height, ImageFormat.RAW_SENSOR, 2).also { rawReader ->
                    rawReader.setOnImageAvailableListener({ source ->
                        val next = source.acquireNextImage() ?: return@setOnImageAvailableListener
                        synchronized(lock) {
                            if (closed || image != null) next.close() else image = next
                        }
                        maybeFinalize()
                    }, handler)
                }
                cameraManager.openCamera(route.logicalCameraId, executor, object : CameraDevice.StateCallback() {
                    override fun onOpened(camera: CameraDevice) {
                        if (closed) {
                            camera.close()
                            return
                        }
                        device = camera
                        createSession(camera)
                    }
                    override fun onDisconnected(camera: CameraDevice) = fail("Camera disconnected")
                    override fun onError(camera: CameraDevice, error: Int) = fail("Camera error $error")
                })
            } catch (error: Exception) {
                fail("Open failed: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun createSession(camera: CameraDevice) {
            try {
                val rawReader = reader ?: throw IllegalStateException("RAW reader missing")
                val output = OutputConfiguration(rawReader.surface).apply { setPhysicalCameraId(route.physicalCameraId) }
                val config = SessionConfiguration(
                    SessionConfiguration.SESSION_REGULAR,
                    listOf(output),
                    executor,
                    object : CameraCaptureSession.StateCallback() {
                        override fun onConfigured(captureSession: CameraCaptureSession) {
                            session = captureSession
                            issueCapture(camera, captureSession, rawReader)
                        }
                        override fun onConfigureFailed(captureSession: CameraCaptureSession) {
                            fail("PHYSICAL_RAW_SESSION_REJECTED: vendor did not accept RAW_SENSOR output mapped to physical ID ${route.physicalCameraId}")
                        }
                    },
                )
                camera.createCaptureSession(config)
            } catch (error: Exception) {
                fail("Session creation failed: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun issueCapture(camera: CameraDevice, captureSession: CameraCaptureSession, rawReader: ImageReader) {
            try {
                val builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, mutableSetOf(route.physicalCameraId))
                builder.addTarget(rawReader.surface)
                builder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF)
                builder.set(CaptureRequest.SENSOR_SENSITIVITY, requestedIso)
                builder.set(CaptureRequest.SENSOR_EXPOSURE_TIME, requestedExposureNs)
                builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
                builder.set(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f)

                val logicalChars = cameraManager.getCameraCharacteristics(route.logicalCameraId)
                val physicalChars = cameraManager.getCameraCharacteristics(route.physicalCameraId)
                val physicalKeys = logicalChars.availablePhysicalCameraRequestKeys.orEmpty()
                if (physicalKeys.contains(CaptureRequest.SENSOR_SENSITIVITY)) {
                    builder.setPhysicalCameraKey(CaptureRequest.SENSOR_SENSITIVITY, requestedIso, route.physicalCameraId)
                }
                if (physicalKeys.contains(CaptureRequest.SENSOR_EXPOSURE_TIME)) {
                    builder.setPhysicalCameraKey(CaptureRequest.SENSOR_EXPOSURE_TIME, requestedExposureNs, route.physicalCameraId)
                }
                if (physicalKeys.contains(CaptureRequest.LENS_FOCUS_DISTANCE)) {
                    builder.setPhysicalCameraKey(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f, route.physicalCameraId)
                }
                val oisModes = physicalChars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
                if (oisModes.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF)) {
                    builder.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF)
                    if (physicalKeys.contains(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE)) {
                        builder.setPhysicalCameraKey(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF, route.physicalCameraId)
                    }
                }
                val nrModes = physicalChars.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
                if (nrModes.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                    builder.set(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
                }

                captureSession.capture(builder.build(), object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, total: TotalCaptureResult) {
                        synchronized(lock) { result = total }
                        maybeFinalize()
                    }
                    override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: CaptureFailure) {
                        fail("Capture failed: reason=${failure.reason} frame=${failure.frameNumber}")
                    }
                }, handler)
            } catch (error: Exception) {
                fail("Capture request failed: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun maybeFinalize() {
            val pair = synchronized(lock) {
                val localImage = image
                val localResult = result
                if (!closed && localImage != null && localResult != null) {
                    image = null
                    result = null
                    localImage to localResult
                } else null
            }
            pair?.let { finalizeCapture(it.first, it.second) }
        }

        private fun finalizeCapture(rawImage: Image, total: TotalCaptureResult) {
            try {
                val physicalResult = total.physicalCameraTotalResults[route.physicalCameraId]
                    ?: throw IllegalStateException("FAIL CLOSED: no physical TotalCaptureResult for requested ID ${route.physicalCameraId}")
                if (physicalResult.cameraId != route.physicalCameraId) {
                    throw IllegalStateException("FAIL CLOSED: physical result cameraId=${physicalResult.cameraId}, expected=${route.physicalCameraId}")
                }
                val sensorTimestamp = physicalResult.get(CaptureResult.SENSOR_TIMESTAMP)
                    ?: throw IllegalStateException("Physical SENSOR_TIMESTAMP missing")
                if (sensorTimestamp != rawImage.timestamp) {
                    throw IllegalStateException("RAW/result timestamp mismatch: result=$sensorTimestamp image=${rawImage.timestamp}")
                }
                val actualIso = physicalResult.get(CaptureResult.SENSOR_SENSITIVITY)
                    ?: throw IllegalStateException("Physical SENSOR_SENSITIVITY missing")
                val actualExposure = physicalResult.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    ?: throw IllegalStateException("Physical SENSOR_EXPOSURE_TIME missing")
                val focalLength = physicalResult.get(CaptureResult.LENS_FOCAL_LENGTH)
                val activeLogicalPhysical = total.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)

                val captureId = "C2TELE_" + SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(Date())
                val dngName = "${captureId}_${route.rawSize.width}x${route.rawSize.height}.dng"
                val dngUri = createPendingDownload(dngName, "image/x-adobe-dng", "captures")
                try {
                    contentResolver.openOutputStream(dngUri, "w")!!.use { output ->
                        val physicalChars = cameraManager.getCameraCharacteristics(route.physicalCameraId)
                        DngCreator(physicalChars, physicalResult).use { creator ->
                            creator.setDescription("TruthRaw FotoGraaf HONOR forced physical tele candidate v0.2; physical ID 5 verified by Camera2 result; no calibration authority")
                            creator.writeImage(output, rawImage)
                        }
                    }
                } finally {
                    rawImage.close()
                }
                finalizeDownload(dngUri)
                val hash = hashUri(dngUri)
                if (hash.byteLength <= 0L) throw IllegalStateException("Finalized DNG is empty")

                val physicalChars = cameraManager.getCameraCharacteristics(route.physicalCameraId)
                val observation = JSONObject()
                    .put("schema", "truthraw.fotograaf-honor-tele-acquisition-observation.v0.2")
                    .put("captureId", captureId)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("source", JSONObject()
                        .put("displayName", dngName)
                        .put("mediaStoreUri", dngUri.toString())
                        .put("sha256", hash.sha256)
                        .put("byteLength", hash.byteLength)
                        .put("hashTiming", "AFTER_DNG_FINALIZATION"))
                    .put("device", JSONObject()
                        .put("manufacturer", Build.MANUFACTURER)
                        .put("model", Build.MODEL)
                        .put("buildFingerprint", Build.FINGERPRINT))
                    .put("route", JSONObject()
                        .put("routeClass", "LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT")
                        .put("logicalCameraId", route.logicalCameraId)
                        .put("requestedPhysicalCameraId", route.physicalCameraId)
                        .put("confirmedPhysicalResultCameraId", physicalResult.cameraId)
                        .put("activeLogicalPhysicalId", activeLogicalPhysical ?: JSONObject.NULL)
                        .put("advertisedPhysicalCameraIds", JSONArray(route.advertisedPhysicalIds))
                        .put("rawSizeAuthority", route.rawSizeAuthority)
                        .put("physicalRawCapabilityAdvertised", route.physicalRawCapabilityAdvertised))
                    .put("topology", JSONObject()
                        .put("format", "RAW_SENSOR")
                        .put("rawWidth", route.rawSize.width)
                        .put("rawHeight", route.rawSize.height)
                        .put("cfaPattern", route.cfaPattern)
                        .put("directCfaMeasurement", true)
                        .put("processedRgbInput", false)
                        .put("multiFrameEvidenceMerged", false)
                        .put("sensorInfoLensShadingApplied", physicalChars.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED) ?: JSONObject.NULL))
                    .put("request", JSONObject()
                        .put("sensorSensitivityIso", requestedIso)
                        .put("exposureTimeNs", requestedExposureNs)
                        .put("focusDistanceDiopters", 0.0)
                        .put("oisRequestedOff", true))
                    .put("result", JSONObject()
                        .put("sensorSensitivityIso", actualIso)
                        .put("exposureTimeNs", actualExposure)
                        .put("sensorTimestampNs", sensorTimestamp)
                        .put("imageTimestampNs", sensorTimestamp)
                        .put("timestampMatch", true)
                        .put("focalLengthMm", focalLength ?: JSONObject.NULL)
                        .put("focusDistanceDiopters", physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                        .put("oisMode", physicalResult.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE) ?: JSONObject.NULL)
                        .put("noiseReductionMode", physicalResult.get(CaptureResult.NOISE_REDUCTION_MODE) ?: JSONObject.NULL)
                        .put("dynamicBlackLevel", physicalResult.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL)?.let { JSONArray(it.toList()) } ?: JSONObject.NULL)
                        .put("dynamicWhiteLevel", physicalResult.get(CaptureResult.SENSOR_DYNAMIC_WHITE_LEVEL) ?: JSONObject.NULL))
                    .put("authority", JSONObject()
                        .put("recordClass", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
                        .put("physicalRouteConfirmed", true)
                        .put("projectLensRoleCandidate", "TELE_3_7X_PHYSICAL_ID_5")
                        .put("projectLensRolePromotedToCalibrationAuthority", false)
                        .put("calibrationAuthorityGranted", false)
                        .put("c0EnvelopeReady", false)
                        .put("physicalFrameCountForLaterScene", 1)
                        .put("independentEvidenceCountForLaterScene", 1)
                        .put("missingBeforeC0Envelope", JSONArray(listOf(
                            "cameraSystemIdMappingIndependentValidation",
                            "captureSampleDomainId",
                            "gainReadoutStateId",
                            "focusStateClass",
                            "stabilizationState"
                        ))))

                val jsonName = "${captureId}_honor_tele_observation_v0_2.json"
                val jsonUri = createPendingDownload(jsonName, "application/json", "observations")
                contentResolver.openOutputStream(jsonUri, "w")!!.bufferedWriter(Charsets.UTF_8).use {
                    it.write(observation.toString(2))
                    it.write("\n")
                }
                finalizeDownload(jsonUri)

                done(
                    "PASS PHYSICAL ROUTE: requested ID ${route.physicalCameraId}, confirmed physical result=${physicalResult.cameraId}.\n" +
                        "focalLengthMm=${focalLength ?: "n/a"}, RAW=${route.rawSize.width}×${route.rawSize.height} ${route.cfaPattern}.\n" +
                        "DNG sha256=${hash.sha256}\nSaved in Download/TruthRawFotoGraaf/$sessionId/. Calibration authority remains BLOCKED."
                )
            } catch (error: Exception) {
                try { rawImage.close() } catch (_: Exception) {}
                fail("FAIL CLOSED during tele finalization: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun createPendingDownload(displayName: String, mimeType: String, leaf: String): Uri {
            val values = ContentValues().apply {
                put(MediaStore.MediaColumns.DISPLAY_NAME, displayName)
                put(MediaStore.MediaColumns.MIME_TYPE, mimeType)
                put(MediaStore.MediaColumns.RELATIVE_PATH, "${Environment.DIRECTORY_DOWNLOADS}/TruthRawFotoGraaf/$sessionId/$leaf")
                put(MediaStore.MediaColumns.IS_PENDING, 1)
            }
            return contentResolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values)
                ?: throw IllegalStateException("MediaStore insert returned null")
        }

        private fun finalizeDownload(uri: Uri) {
            val values = ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) }
            if (contentResolver.update(uri, values, null, null) <= 0) {
                throw IllegalStateException("MediaStore finalization failed for $uri")
            }
        }

        private fun hashUri(uri: Uri): HashAndLength {
            val digest = MessageDigest.getInstance("SHA-256")
            var length = 0L
            contentResolver.openInputStream(uri)?.use { input ->
                val buffer = ByteArray(1024 * 1024)
                while (true) {
                    val count = input.read(buffer)
                    if (count < 0) break
                    if (count == 0) continue
                    digest.update(buffer, 0, count)
                    length += count
                }
            } ?: throw IllegalStateException("Cannot reopen finalized DNG for hashing")
            return HashAndLength(digest.digest().joinToString("") { "%02x".format(it) }, length)
        }

        private fun fail(message: String) {
            synchronized(lock) {
                if (closed) return
                image?.close()
                image = null
                result = null
            }
            done(message)
        }

        fun close() {
            synchronized(lock) {
                if (closed) return
                closed = true
                image?.close()
                image = null
                result = null
            }
            try { session?.close() } catch (_: Exception) {}
            try { device?.close() } catch (_: Exception) {}
            try { reader?.close() } catch (_: Exception) {}
            thread.quitSafely()
        }
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

    companion object {
        private const val REQUEST_CAMERA_PERMISSION = 1002
        private const val PHYSICAL_TELE_ID = "5"
    }
}

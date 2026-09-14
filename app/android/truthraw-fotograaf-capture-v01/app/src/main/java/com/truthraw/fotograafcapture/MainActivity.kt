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
import android.view.Gravity
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Spinner
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.text.SimpleDateFormat
import java.time.Instant
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executor

class MainActivity : Activity() {
    private data class RawEndpoint(
        val logicalCameraId: String,
        val physicalCameraId: String?,
        val logicalMultiCamera: Boolean,
        val advertisedPhysicalCameraIds: List<String>,
        val rawSize: Size,
        val cfaPattern: String,
        val manualSensor: Boolean,
        val label: String,
    )

    private data class HashAndLength(val sha256: String, val byteLength: Long)

    private lateinit var cameraManager: CameraManager
    private lateinit var endpointSpinner: Spinner
    private lateinit var isoEdit: EditText
    private lateinit var exposureEdit: EditText
    private lateinit var statusView: TextView
    private lateinit var captureButton: Button
    private var endpoints: List<RawEndpoint> = emptyList()
    private var activeEngine: CaptureEngine? = null
    private val sessionId: String = "SESSION_" + SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        buildUi()
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            refreshEndpoints()
        } else {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
        }
    }

    override fun onDestroy() {
        activeEngine?.close()
        activeEngine = null
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA_PERMISSION && grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            refreshEndpoints()
        } else {
            setStatus("CAMERA permission is required for source-side acquisition evidence.")
        }
    }

    private fun buildUi() {
        val pad = (16 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        root.addView(TextView(this).apply {
            text = "TruthRaw FotoGraaf · Camera2 acquisition observation v0.1"
            textSize = 20f
        })
        root.addView(TextView(this).apply {
            text = "This companion records one RAW_SENSOR frame and capture-time Camera2 evidence. It does not classify captureSampleDomainId or gainReadoutStateId and grants no calibration authority."
            textSize = 14f
            setPadding(0, pad / 2, 0, pad)
        })

        endpointSpinner = Spinner(this)
        root.addView(endpointSpinner, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        isoEdit = EditText(this).apply {
            hint = "Requested ISO (e.g. 100)"
            inputType = InputType.TYPE_CLASS_NUMBER
            setText("100")
        }
        root.addView(isoEdit)

        exposureEdit = EditText(this).apply {
            hint = "Requested exposure ns (e.g. 16367398)"
            inputType = InputType.TYPE_CLASS_NUMBER
            setText("16367398")
        }
        root.addView(exposureEdit)

        val rescan = Button(this).apply {
            text = "Rescan Camera2 RAW endpoints"
            setOnClickListener { refreshEndpoints() }
        }
        root.addView(rescan)

        captureButton = Button(this).apply {
            text = "Capture one sealed-source candidate"
            isEnabled = false
            setOnClickListener { captureSelected() }
        }
        root.addView(captureButton)

        statusView = TextView(this).apply {
            textSize = 13f
            setPadding(0, pad, 0, pad)
        }
        root.addView(statusView)

        val scroll = ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        }
        setContentView(scroll)
    }

    private fun refreshEndpoints() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        try {
            endpoints = enumerateRawEndpoints()
            endpointSpinner.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, endpoints.map { it.label })
            captureButton.isEnabled = endpoints.isNotEmpty() && activeEngine == null
            val exact = endpoints.count { it.rawSize.width == TARGET_WIDTH && it.rawSize.height == TARGET_HEIGHT && it.cfaPattern == "BGGR" }
            setStatus(
                "Found ${endpoints.size} RAW endpoint candidate(s); $exact match 4080×3072 BGGR. " +
                    "Session: $sessionId. Camera2 IDs shown here are observed; they are not assumed from historical lens labels."
            )
        } catch (error: Exception) {
            endpoints = emptyList()
            captureButton.isEnabled = false
            setStatus("Endpoint scan failed: ${error.message ?: error.javaClass.simpleName}")
        }
    }

    private fun enumerateRawEndpoints(): List<RawEndpoint> {
        val out = mutableListOf<RawEndpoint>()
        for (cameraId in cameraManager.cameraIdList) {
            val logicalChars = cameraManager.getCameraCharacteristics(cameraId)
            val caps = logicalChars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
            val logicalMulti = caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA)
            val physicalIds = if (logicalMulti) logicalChars.physicalCameraIds.sorted() else emptyList()
            rawEndpointFor(cameraId, null, logicalChars, logicalMulti, physicalIds)?.let(out::add)
            for (physicalId in physicalIds) {
                try {
                    val physicalChars = cameraManager.getCameraCharacteristics(physicalId)
                    rawEndpointFor(cameraId, physicalId, physicalChars, logicalMulti, physicalIds)?.let(out::add)
                } catch (_: Exception) {
                    // A physical ID may be query-limited on vendor stacks; absence is recorded by not offering it.
                }
            }
        }
        return out.distinctBy { Triple(it.logicalCameraId, it.physicalCameraId, "${it.rawSize.width}x${it.rawSize.height}") }
            .sortedWith(compareByDescending<RawEndpoint> { it.rawSize.width == TARGET_WIDTH && it.rawSize.height == TARGET_HEIGHT && it.cfaPattern == "BGGR" }
                .thenBy { it.logicalCameraId }
                .thenBy { it.physicalCameraId ?: "" })
    }

    private fun rawEndpointFor(
        logicalId: String,
        physicalId: String?,
        chars: CameraCharacteristics,
        logicalMulti: Boolean,
        advertisedPhysicalIds: List<String>,
    ): RawEndpoint? {
        val caps = chars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES) ?: intArrayOf()
        if (!caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW)) return null
        val map = chars.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP) ?: return null
        val sizes = map.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
        if (sizes.isEmpty()) return null
        val chosen = sizes.firstOrNull { it.width == TARGET_WIDTH && it.height == TARGET_HEIGHT }
            ?: sizes.maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: return null
        val cfa = cfaName(chars.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT))
        val manual = caps.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR)
        val endpointKind = if (physicalId == null) "direct/logical" else "physical-output"
        val label = "Camera2 $logicalId · $endpointKind${physicalId?.let { " $it" } ?: ""} · ${chosen.width}×${chosen.height} · $cfa · manual=$manual"
        return RawEndpoint(logicalId, physicalId, logicalMulti, advertisedPhysicalIds, chosen, cfa, manual, label)
    }

    private fun captureSelected() {
        if (activeEngine != null) return
        val endpoint = endpoints.getOrNull(endpointSpinner.selectedItemPosition) ?: return
        val iso = isoEdit.text.toString().toIntOrNull()
        val exposureNs = exposureEdit.text.toString().toLongOrNull()
        if (iso == null || iso <= 0 || exposureNs == null || exposureNs <= 0L) {
            setStatus("ISO and exposure time must be positive integers.")
            return
        }
        if (!endpoint.manualSensor) {
            setStatus("FAIL CLOSED: selected endpoint does not advertise MANUAL_SENSOR. A calibration observation must not pretend an exact ISO/exposure request was authoritative.")
            return
        }
        captureButton.isEnabled = false
        setStatus("Opening ${endpoint.label} …")
        val engine = CaptureEngine(endpoint, iso, exposureNs) { message, done ->
            runOnUiThread {
                setStatus(message)
                if (done) {
                    activeEngine?.close()
                    activeEngine = null
                    captureButton.isEnabled = endpoints.isNotEmpty()
                }
            }
        }
        activeEngine = engine
        engine.start()
    }

    private fun setStatus(text: String) {
        statusView.text = text
    }

    private inner class CaptureEngine(
        private val endpoint: RawEndpoint,
        private val requestedIso: Int,
        private val requestedExposureNs: Long,
        private val callback: (String, Boolean) -> Unit,
    ) {
        private val thread = HandlerThread("truthraw-fotograaf-camera2").apply { start() }
        private val handler = Handler(thread.looper)
        private val executor = Executor { command -> handler.post(command) }
        private val lock = Any()
        private var reader: ImageReader? = null
        private var device: CameraDevice? = null
        private var session: CameraCaptureSession? = null
        private var pendingImage: Image? = null
        private var pendingResult: TotalCaptureResult? = null
        private var closed = false

        @SuppressLint("MissingPermission")
        fun start() {
            try {
                reader = ImageReader.newInstance(endpoint.rawSize.width, endpoint.rawSize.height, ImageFormat.RAW_SENSOR, 2).also { rawReader ->
                    rawReader.setOnImageAvailableListener({ source ->
                        val image = source.acquireNextImage() ?: return@setOnImageAvailableListener
                        synchronized(lock) {
                            if (closed || pendingImage != null) image.close() else pendingImage = image
                        }
                        maybeFinalize()
                    }, handler)
                }
                cameraManager.openCamera(endpoint.logicalCameraId, executor, object : CameraDevice.StateCallback() {
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
                val output = OutputConfiguration(rawReader.surface)
                endpoint.physicalCameraId?.let { output.setPhysicalCameraId(it) }
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
                            fail("Capture session configuration failed")
                        }
                    }
                )
                camera.createCaptureSession(config)
            } catch (error: Exception) {
                fail("Session creation failed: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun issueCapture(camera: CameraDevice, captureSession: CameraCaptureSession, rawReader: ImageReader) {
            try {
                val builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                builder.addTarget(rawReader.surface)
                builder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF)
                builder.set(CaptureRequest.SENSOR_SENSITIVITY, requestedIso)
                builder.set(CaptureRequest.SENSOR_EXPOSURE_TIME, requestedExposureNs)
                builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
                builder.set(CaptureRequest.LENS_FOCUS_DISTANCE, 0.0f)
                val sourceChars = cameraManager.getCameraCharacteristics(endpoint.physicalCameraId ?: endpoint.logicalCameraId)
                val oisModes = sourceChars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
                if (oisModes.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF)) {
                    builder.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF)
                }
                val nrModes = sourceChars.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
                if (nrModes.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                    builder.set(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
                }
                endpoint.physicalCameraId?.let { physicalId ->
                    builder.setPhysicalCameraKey(CaptureRequest.SENSOR_SENSITIVITY, requestedIso, physicalId)
                    builder.setPhysicalCameraKey(CaptureRequest.SENSOR_EXPOSURE_TIME, requestedExposureNs, physicalId)
                }
                captureSession.capture(builder.build(), object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                        synchronized(lock) { pendingResult = result }
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
            val pair: Pair<Image, TotalCaptureResult>? = synchronized(lock) {
                val image = pendingImage
                val result = pendingResult
                if (!closed && image != null && result != null) {
                    pendingImage = null
                    pendingResult = null
                    image to result
                } else null
            }
            if (pair != null) finalizeCapture(pair.first, pair.second)
        }

        private fun finalizeCapture(image: Image, total: TotalCaptureResult) {
            try {
                val physicalResult = endpoint.physicalCameraId?.let { total.physicalCameraTotalResults[it] }
                if (endpoint.physicalCameraId != null && physicalResult == null) {
                    throw IllegalStateException("Requested physical RAW output produced no physical TotalCaptureResult")
                }
                val effectiveResult = physicalResult ?: total
                val sensorTimestamp = effectiveResult.get(CaptureResult.SENSOR_TIMESTAMP)
                    ?: total.get(CaptureResult.SENSOR_TIMESTAMP)
                    ?: throw IllegalStateException("CaptureResult SENSOR_TIMESTAMP missing")
                if (sensorTimestamp != image.timestamp) {
                    throw IllegalStateException("RAW/result timestamp mismatch: result=$sensorTimestamp image=${image.timestamp}")
                }
                val actualIso = effectiveResult.get(CaptureResult.SENSOR_SENSITIVITY)
                    ?: throw IllegalStateException("CaptureResult SENSOR_SENSITIVITY missing")
                val actualExposure = effectiveResult.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    ?: throw IllegalStateException("CaptureResult SENSOR_EXPOSURE_TIME missing")

                val captureId = "C2OBS_" + SimpleDateFormat("yyyyMMdd_HHmmss_SSS", Locale.US).format(Date())
                val displayName = "${captureId}_${endpoint.rawSize.width}x${endpoint.rawSize.height}.dng"
                val dngUri = createPendingDownload(displayName, "image/x-adobe-dng", "captures")
                try {
                    contentResolver.openOutputStream(dngUri, "w")!!.use { output ->
                        val dngCameraId = endpoint.physicalCameraId ?: endpoint.logicalCameraId
                        val dngChars = cameraManager.getCameraCharacteristics(dngCameraId)
                        DngCreator(dngChars, effectiveResult).use { creator ->
                            creator.setDescription("TruthRaw FotoGraaf Camera2 acquisition observation v0.1; no calibration authority")
                            creator.writeImage(output, image)
                        }
                    }
                } finally {
                    image.close()
                }
                finalizeDownload(dngUri)
                val hashAndLength = hashUri(dngUri)
                if (hashAndLength.byteLength <= 0L) throw IllegalStateException("Finalized DNG is empty")

                val observation = buildObservation(
                    captureId = captureId,
                    displayName = displayName,
                    sourceUri = dngUri,
                    hashAndLength = hashAndLength,
                    total = total,
                    effectiveResult = effectiveResult,
                    physicalResult = physicalResult,
                    sensorTimestamp = sensorTimestamp,
                    actualIso = actualIso,
                    actualExposure = actualExposure,
                    imageTimestamp = sensorTimestamp,
                )
                val observationName = "${captureId}_camera2_observation_v0_1.json"
                val observationUri = createPendingDownload(observationName, "application/json", "observations")
                contentResolver.openOutputStream(observationUri, "w")!!.bufferedWriter(Charsets.UTF_8).use { writer ->
                    writer.write(observation.toString(2))
                    writer.write("\n")
                }
                finalizeDownload(observationUri)

                val physicalStatus = observation.getJSONObject("camera").getJSONObject("physicalCameraObservation").getString("status")
                callback(
                    "PASS OBSERVATION: $displayName\nsha256=${hashAndLength.sha256}\nbytes=${hashAndLength.byteLength}\n" +
                        "Camera2=${endpoint.logicalCameraId}, physicalStatus=$physicalStatus, result ISO=$actualIso, exposureNs=$actualExposure.\n" +
                        "Saved under Download/TruthRawFotoGraaf/$sessionId/. C0 envelope remains BLOCKED until sample-domain + gain/readout classifiers are validated.",
                    true,
                )
            } catch (error: Exception) {
                try { image.close() } catch (_: Exception) {}
                fail("FAIL CLOSED during finalization: ${error.message ?: error.javaClass.simpleName}")
            }
        }

        private fun buildObservation(
            captureId: String,
            displayName: String,
            sourceUri: Uri,
            hashAndLength: HashAndLength,
            total: TotalCaptureResult,
            effectiveResult: TotalCaptureResult,
            physicalResult: TotalCaptureResult?,
            sensorTimestamp: Long,
            actualIso: Int,
            actualExposure: Long,
            imageTimestamp: Long,
        ): JSONObject {
            val activePhysical = total.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)
            val resultCameraId = effectiveResult.cameraId
            val physicalObservation = when {
                physicalResult != null && endpoint.physicalCameraId != null -> JSONObject()
                    .put("status", "MEASURED_PHYSICAL_OUTPUT_RESULT")
                    .put("value", endpoint.physicalCameraId)
                    .put("method", "CAMERA2_PHYSICAL_OUTPUT_RESULT")
                !endpoint.logicalMultiCamera -> JSONObject()
                    .put("status", "MEASURED_DIRECT_CAMERA_RESULT")
                    .put("value", resultCameraId)
                    .put("method", "CAMERA2_DIRECT_CAMERA_RESULT")
                activePhysical != null -> JSONObject()
                    .put("status", "MEASURED_ACTIVE_PHYSICAL_RESULT")
                    .put("value", activePhysical)
                    .put("method", "CAMERA2_ACTIVE_PHYSICAL_RESULT")
                else -> JSONObject()
                    .put("status", "NOT_AVAILABLE")
                    .put(JSONObject.NULL.toString(), JSONObject.NULL)
            }
            if (physicalObservation.optString("status") == "NOT_AVAILABLE") {
                physicalObservation.remove(JSONObject.NULL.toString())
                physicalObservation.put("value", JSONObject.NULL)
                physicalObservation.put("method", "CAMERA2_RESULT_KEY_NOT_AVAILABLE")
            }

            val sourceChars = cameraManager.getCameraCharacteristics(endpoint.physicalCameraId ?: endpoint.logicalCameraId)
            val dynamicBlack = effectiveResult.get(CaptureResult.SENSOR_DYNAMIC_BLACK_LEVEL)
            val dynamicWhite = effectiveResult.get(CaptureResult.SENSOR_DYNAMIC_WHITE_LEVEL)
            val lensShadingApplied = sourceChars.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED)

            return JSONObject()
                .put("schema", "truthraw.fotograaf-camera2-acquisition-observation.v0.1")
                .put("captureId", captureId)
                .put("createdAtUtc", Instant.now().toString())
                .put("source", JSONObject()
                    .put("displayName", displayName)
                    .put("mediaStoreUri", sourceUri.toString())
                    .put("sha256", hashAndLength.sha256)
                    .put("byteLength", hashAndLength.byteLength)
                    .put("hashTiming", "AFTER_DNG_FINALIZATION"))
                .put("device", JSONObject()
                    .put("manufacturer", Build.MANUFACTURER)
                    .put("model", Build.MODEL)
                    .put("buildFingerprint", Build.FINGERPRINT))
                .put("camera", JSONObject()
                    .put("logicalCameraId", endpoint.logicalCameraId)
                    .put("logicalMultiCamera", endpoint.logicalMultiCamera)
                    .put("requestedPhysicalCameraId", endpoint.physicalCameraId ?: JSONObject.NULL)
                    .put("advertisedPhysicalCameraIds", JSONArray(endpoint.advertisedPhysicalCameraIds))
                    .put("physicalCameraObservation", physicalObservation)
                    .put("resultCameraId", resultCameraId)
                    .put("activePhysicalCameraIdResult", activePhysical ?: JSONObject.NULL))
                .put("topology", JSONObject()
                    .put("format", "RAW_SENSOR")
                    .put("rawWidth", endpoint.rawSize.width)
                    .put("rawHeight", endpoint.rawSize.height)
                    .put("cfaPattern", endpoint.cfaPattern)
                    .put("rawCapability", true)
                    .put("directCfaMeasurement", true)
                    .put("processedRgbInput", false)
                    .put("multiFrameEvidenceMerged", false)
                    .put("sensorInfoLensShadingApplied", lensShadingApplied ?: JSONObject.NULL))
                .put("request", JSONObject()
                    .put("sensorSensitivityIso", requestedIso)
                    .put("exposureTimeNs", requestedExposureNs)
                    .put("focusDistanceDiopters", 0.0)
                    .put("oisRequestedOff", true))
                .put("result", JSONObject()
                    .put("sensorSensitivityIso", actualIso)
                    .put("exposureTimeNs", actualExposure)
                    .put("sensorTimestampNs", sensorTimestamp)
                    .put("imageTimestampNs", imageTimestamp)
                    .put("timestampMatch", true)
                    .put("focusDistanceDiopters", effectiveResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                    .put("oisMode", effectiveResult.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE) ?: JSONObject.NULL)
                    .put("noiseReductionMode", effectiveResult.get(CaptureResult.NOISE_REDUCTION_MODE) ?: JSONObject.NULL)
                    .put("dynamicBlackLevel", dynamicBlack?.let { JSONArray(it.toList()) } ?: JSONObject.NULL)
                    .put("dynamicWhiteLevel", dynamicWhite ?: JSONObject.NULL))
                .put("authority", JSONObject()
                    .put("recordClass", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
                    .put("calibrationAuthorityGranted", false)
                    .put("c0EnvelopeReady", false)
                    .put("physicalFrameCountForLaterScene", 1)
                    .put("independentEvidenceCountForLaterScene", 1)
                    .put("missingBeforeC0Envelope", JSONArray(listOf(
                        "cameraSystemIdMapping",
                        "captureSampleDomainId",
                        "gainReadoutStateId",
                        "focusStateClass",
                        "stabilizationState"
                    ))))
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
                pendingImage?.close()
                pendingImage = null
                pendingResult = null
            }
            callback(message, true)
        }

        fun close() {
            synchronized(lock) {
                if (closed) return
                closed = true
                pendingImage?.close()
                pendingImage = null
                pendingResult = null
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
        private const val REQUEST_CAMERA_PERMISSION = 1001
        private const val TARGET_WIDTH = 4080
        private const val TARGET_HEIGHT = 3072
    }
}

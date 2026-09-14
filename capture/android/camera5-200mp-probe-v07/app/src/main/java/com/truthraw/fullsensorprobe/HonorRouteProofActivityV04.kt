package com.truthraw.fullsensorprobe

import android.Manifest
import android.app.Activity
import android.content.Context
import android.content.Intent
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
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.core.content.FileProvider
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.util.concurrent.Executor
import org.json.JSONArray
import org.json.JSONObject

/**
 * Standalone implementation probe for FotoGraaf HONOR Route Proof v0.4.
 *
 * It deliberately keeps the evidence boundary narrow:
 * - standard Camera2 inventory is discovery only;
 * - the runtime capture opens logical camera 0 and binds RAW output to physical camera 5;
 * - MAXIMUM_RESOLUTION and DEFAULT are separate attempts; there is no silent fallback;
 * - each successful capture remains one physical frame;
 * - no calibration authority or C0 seal is granted here.
 */
class HonorRouteProofActivityV04 : Activity() {

    companion object {
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val MAX_W = 16320
        private const val MAX_H = 12288
        private const val DEFAULT_W = 4080
        private const val DEFAULT_H = 3072
        private const val REQUEST_CAMERA = 44
    }

    private lateinit var cameraManager: CameraManager
    private lateinit var logView: TextView
    private val cameraThread = HandlerThread("truthraw-honor-route-v04").apply { start() }
    private val handler = Handler(cameraThread.looper)
    private val executor = Executor { runnable -> handler.post(runnable) }
    private var pendingPermissionAction: (() -> Unit)? = null
    private var latestStaticObservation: JSONObject? = null
    private val latestEvidenceFiles = mutableListOf<File>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        setContentView(buildUi())
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraThread.quitSafely()
    }

    private fun buildUi(): View {
        val density = resources.displayMetrics.density
        fun dp(v: Int) = (v * density).toInt()

        val column = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20), dp(18), dp(20), dp(32))
        }

        column.addView(TextView(this).apply {
            text = "FotoGraaf · HONOR RAW Sample-Domain Proof v0.4"
            textSize = 25f
        })
        column.addView(TextView(this).apply {
            text = "Standard Camera2 + runtime logical 0 → physical 5. MAXIMUM_RESOLUTION en DEFAULT zijn afzonderlijke evidence attempts. Geen calibration authority."
            textSize = 15f
            setPadding(0, dp(8), 0, dp(16))
        })

        column.addView(button("1 · Scan standard Camera2") {
            ensureCameraPermission { handler.post { scanStandardCamera2() } }
        })
        column.addView(button("2A · Capture MAX RAW 16320×12288") {
            ensureCameraPermission { handler.post { startCaptureAttempt(maximumResolution = true) } }
        })
        column.addView(button("2B · Capture DEFAULT RAW 4080×3072") {
            ensureCameraPermission { handler.post { startCaptureAttempt(maximumResolution = false) } }
        })
        column.addView(button("Deel laatste evidence") { shareEvidence() })

        logView = TextView(this).apply {
            textSize = 13f
            setTextIsSelectable(true)
            setPadding(0, dp(18), 0, 0)
            text = "Nog niet gescand.\n"
        }
        column.addView(logView)

        return ScrollView(this).apply { addView(column) }
    }

    private fun button(label: String, onClick: () -> Unit): Button = Button(this).apply {
        text = label
        setOnClickListener { onClick() }
    }

    private fun ensureCameraPermission(action: () -> Unit) {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            action()
        } else {
            pendingPermissionAction = action
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA && grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            pendingPermissionAction?.invoke()
        }
        pendingPermissionAction = null
    }

    private fun log(message: String) {
        runOnUiThread { logView.append(message + "\n") }
    }

    private fun outDir(): File = File(
        getExternalFilesDir(null),
        "truthraw_honor_route_proof_v04"
    ).apply { mkdirs() }

    private fun scanStandardCamera2() {
        try {
            val ids = cameraManager.cameraIdList.toSet()
            require(ids.contains(LOGICAL_ID)) { "logical camera $LOGICAL_ID is not exposed" }
            require(ids.contains(PHYSICAL_ID)) { "physical camera $PHYSICAL_ID is not directly queryable" }

            val logical = cameraManager.getCameraCharacteristics(LOGICAL_ID)
            val physicalIds = logical.physicalCameraIds
            val routeExposed = physicalIds.contains(PHYSICAL_ID)

            val staticBlock = HonorSampleDomainV04.staticCharacteristics(
                cameraManager,
                LOGICAL_ID,
                PHYSICAL_ID
            )
            staticBlock.put("logicalContainsRequestedPhysicalId", routeExposed)
            staticBlock.put("maximumTargetExposed", supportsRawSize(true, MAX_W, MAX_H))
            staticBlock.put("defaultTargetExposed", supportsRawSize(false, DEFAULT_W, DEFAULT_H))

            val root = JSONObject()
                .put("schema", "truthraw.fotograaf-honor-standard-camera2-inventory.v0.4")
                .put("authority", "CAPABILITY_OBSERVATION_ONLY")
                .put("calibrationAuthorityGranted", false)
                .put("c0IdentitySealed", false)
                .put("scientificMasterModified", false)
                .put("device", deviceBlock())
                .put("standardCamera2", JSONObject().put("staticCharacteristics", staticBlock))
                .put(
                    "evidenceBoundary",
                    "Discovery/capability observation only. This inventory is not an independent captured frame and does not increment capture evidence count."
                )

            val file = File(outDir(), "HONOR_STD_CAMERA2_inventory_v0_4.json")
            file.writeText(root.toString(2))
            rememberFile(file)
            latestStaticObservation = staticBlock

            log("Scan klaar: logical $LOGICAL_ID physicalIds=$physicalIds")
            log("logical $LOGICAL_ID bevat physical $PHYSICAL_ID: $routeExposed")
            log("MAX RAW ${MAX_W}x${MAX_H}: ${staticBlock.optBoolean("maximumTargetExposed")}")
            log("DEFAULT RAW ${DEFAULT_W}x${DEFAULT_H}: ${staticBlock.optBoolean("defaultTargetExposed")}")
            log("Inventory: ${file.name}")
        } catch (t: Throwable) {
            log("SCAN ERROR: $t")
        }
    }

    private fun startCaptureAttempt(maximumResolution: Boolean) {
        val requestedMode = if (maximumResolution) {
            CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION
        } else {
            CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT
        }
        val target = if (maximumResolution) Size(MAX_W, MAX_H) else Size(DEFAULT_W, DEFAULT_H)
        val attemptName = if (maximumResolution) "MAXIMUM_RESOLUTION" else "DEFAULT"

        try {
            val logical = cameraManager.getCameraCharacteristics(LOGICAL_ID)
            if (!logical.physicalCameraIds.contains(PHYSICAL_ID)) {
                saveFailedAttempt(
                    maximumResolution,
                    target,
                    "ROUTE_NOT_EXPOSED",
                    "logical $LOGICAL_ID does not expose physical $PHYSICAL_ID"
                )
                return
            }
            if (!supportsRawSize(maximumResolution, target.width, target.height)) {
                saveFailedAttempt(
                    maximumResolution,
                    target,
                    "RAW_SIZE_NOT_EXPOSED",
                    "$attemptName RAW ${target.width}x${target.height} is not exposed by standard Camera2"
                )
                return
            }

            log("Start $attemptName: open logical $LOGICAL_ID → bind physical $PHYSICAL_ID → RAW ${target.width}x${target.height}")
            openLogicalForCapture(target, requestedMode, maximumResolution)
        } catch (t: Throwable) {
            saveFailedAttempt(maximumResolution, target, "PREPARE_EXCEPTION", t.toString())
        }
    }

    @Suppress("MissingPermission")
    private fun openLogicalForCapture(
        target: Size,
        requestedPixelMode: Int,
        maximumResolution: Boolean
    ) {
        cameraManager.openCamera(LOGICAL_ID, object : CameraDevice.StateCallback() {
            override fun onOpened(device: CameraDevice) {
                configureCapture(device, target, requestedPixelMode, maximumResolution)
            }

            override fun onDisconnected(device: CameraDevice) {
                log("Camera disconnected")
                device.close()
            }

            override fun onError(device: CameraDevice, error: Int) {
                log("Camera open error=$error")
                device.close()
                saveFailedAttempt(
                    maximumResolution,
                    target,
                    "OPEN_CAMERA_ERROR",
                    "CameraDevice error=$error"
                )
            }
        }, handler)
    }

    private fun configureCapture(
        device: CameraDevice,
        target: Size,
        requestedPixelMode: Int,
        maximumResolution: Boolean
    ) {
        val reader = ImageReader.newInstance(target.width, target.height, ImageFormat.RAW_SENSOR, 1)
        var image: Image? = null
        var result: TotalCaptureResult? = null
        var session: CameraCaptureSession? = null
        var finished = false
        var sessionSupport: Boolean? = null

        fun cleanup() {
            try { image?.close() } catch (_: Throwable) {}
            try { session?.close() } catch (_: Throwable) {}
            try { reader.close() } catch (_: Throwable) {}
            try { device.close() } catch (_: Throwable) {}
        }

        fun finishIfReady() {
            if (finished) return
            val im = image ?: return
            val res = result ?: return
            finished = true
            try {
                persistCapture(
                    device,
                    im,
                    res,
                    target,
                    requestedPixelMode,
                    maximumResolution,
                    sessionSupport
                )
            } catch (t: Throwable) {
                log("PERSIST ERROR: $t")
            } finally {
                cleanup()
            }
        }

        reader.setOnImageAvailableListener({ source ->
            try {
                image = source.acquireNextImage()
                finishIfReady()
            } catch (t: Throwable) {
                log("Image acquire error: $t")
                cleanup()
            }
        }, handler)

        try {
            val output = OutputConfiguration(reader.surface)
            output.setPhysicalCameraId(PHYSICAL_ID)
            if (maximumResolution) {
                output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
            }

            val config = SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
                executor,
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(configuredSession: CameraCaptureSession) {
                        session = configuredSession
                        try {
                            val request = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                            request.addTarget(reader.surface)

                            val logicalCharacteristics = cameraManager.getCameraCharacteristics(LOGICAL_ID)
                            if (logicalCharacteristics.availableCaptureRequestKeys?.contains(CaptureRequest.SENSOR_PIXEL_MODE) == true) {
                                request.set(CaptureRequest.SENSOR_PIXEL_MODE, requestedPixelMode)
                            }
                            if (logicalCharacteristics.availableCaptureRequestKeys?.contains(CaptureRequest.CONTROL_ENABLE_ZSL) == true) {
                                request.set(CaptureRequest.CONTROL_ENABLE_ZSL, false)
                            }

                            configuredSession.capture(
                                request.build(),
                                object : CameraCaptureSession.CaptureCallback() {
                                    override fun onCaptureCompleted(
                                        session: CameraCaptureSession,
                                        request: CaptureRequest,
                                        captureResult: TotalCaptureResult
                                    ) {
                                        result = captureResult
                                        finishIfReady()
                                    }

                                    override fun onCaptureFailed(
                                        session: CameraCaptureSession,
                                        request: CaptureRequest,
                                        failure: CaptureFailure
                                    ) {
                                        if (!finished) {
                                            finished = true
                                            saveFailedAttempt(
                                                maximumResolution,
                                                target,
                                                "CAPTURE_FAILED",
                                                "reason=${failure.reason} frame=${failure.frameNumber}",
                                                sessionSupport
                                            )
                                        }
                                        cleanup()
                                    }
                                },
                                handler
                            )
                        } catch (t: Throwable) {
                            if (!finished) {
                                finished = true
                                saveFailedAttempt(
                                    maximumResolution,
                                    target,
                                    "CAPTURE_REQUEST_EXCEPTION",
                                    t.toString(),
                                    sessionSupport
                                )
                            }
                            cleanup()
                        }
                    }

                    override fun onConfigureFailed(failedSession: CameraCaptureSession) {
                        if (!finished) {
                            finished = true
                            saveFailedAttempt(
                                maximumResolution,
                                target,
                                "SESSION_CONFIGURATION_FAILED",
                                "CameraCaptureSession.onConfigureFailed",
                                sessionSupport
                            )
                        }
                        cleanup()
                    }
                }
            )

            sessionSupport = try {
                device.isSessionConfigurationSupported(config)
            } catch (_: Throwable) {
                null
            }

            if (sessionSupport == false) {
                finished = true
                saveFailedAttempt(
                    maximumResolution,
                    target,
                    "SESSION_REPORTED_UNSUPPORTED",
                    "CameraDevice.isSessionConfigurationSupported returned false",
                    sessionSupport
                )
                cleanup()
                return
            }

            device.createCaptureSession(config)
        } catch (t: Throwable) {
            if (!finished) {
                finished = true
                saveFailedAttempt(
                    maximumResolution,
                    target,
                    "SESSION_CREATE_EXCEPTION",
                    t.toString(),
                    sessionSupport
                )
            }
            cleanup()
        }
    }

    private fun persistCapture(
        device: CameraDevice,
        image: Image,
        totalResult: TotalCaptureResult,
        target: Size,
        requestedPixelMode: Int,
        maximumResolution: Boolean,
        sessionSupport: Boolean?
    ) {
        val physicalResult = totalResult.physicalCameraTotalResults[PHYSICAL_ID]
        val effectiveResult: CaptureResult = physicalResult ?: totalResult
        val actualPixelMode = effectiveResult.get(CaptureResult.SENSOR_PIXEL_MODE)
        val rawBinningFactorUsed = effectiveResult.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED)
        val sensorTimestamp = effectiveResult.get(CaptureResult.SENSOR_TIMESTAMP)
        val timestampIdentity = sensorTimestamp != null && sensorTimestamp == image.timestamp

        val plane = image.planes[0]
        val pixelStride = try { plane.pixelStride } catch (_: Throwable) { -1 }
        val rowStride = plane.rowStride
        val accessibleBytes = plane.buffer.duplicate().apply { rewind() }.remaining().toLong()
        val expectedBytes = image.width.toLong() * image.height.toLong() * 2L
        val contiguous = pixelStride == 2 && rowStride == image.width * 2 && accessibleBytes == expectedBytes

        val stamp = System.currentTimeMillis()
        val modeToken = if (maximumResolution) "MAX" else "DEFAULT"
        val base = "C2RAW_${stamp}_${modeToken}_${image.width}x${image.height}"
        val rawFile = File(outDir(), if (contiguous) "$base.rawsensor" else "$base.rawbuffer")

        val rawHash = hashAndWriteBuffer(plane.buffer, rawFile)
        rememberFile(rawFile)

        val physicalCharacteristics = cameraManager.getCameraCharacteristics(PHYSICAL_ID)
        var dngFile: File? = null
        var dngError: String? = null
        try {
            dngFile = File(outDir(), "$base.dng")
            FileOutputStream(dngFile).use { stream ->
                DngCreator(physicalCharacteristics, effectiveResult).use { creator ->
                    creator.setDescription(
                        "TruthRaw FotoGraaf HONOR route proof v0.4; logical $LOGICAL_ID -> physical $PHYSICAL_ID; exact app-visible RAW SHA-256=$rawHash"
                    )
                    creator.writeImage(stream, image)
                }
            }
            rememberFile(dngFile)
        } catch (t: Throwable) {
            dngError = t.toString()
            dngFile = null
        }

        val cfa = cfaName(physicalCharacteristics.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT))
        val maxParentObserved = supportsRawSize(true, MAX_W, MAX_H)
        val sampleDomain = HonorSampleDomainV04.sampleDomain(
            physicalCameraId = PHYSICAL_ID,
            width = image.width,
            height = image.height,
            cfa = cfa,
            actualPixelMode = actualPixelMode,
            rawBinningFactorUsed = rawBinningFactorUsed,
            maximumWidth = if (maxParentObserved) MAX_W else null,
            maximumHeight = if (maxParentObserved) MAX_H else null
        )

        val runtime = HonorSampleDomainV04.runtimeResult(totalResult, PHYSICAL_ID)
        val maxPass = maximumResolution &&
            image.width == MAX_W && image.height == MAX_H &&
            physicalResult != null &&
            actualPixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION &&
            timestampIdentity && rawHash.isNotBlank()
        val defaultPass = !maximumResolution &&
            image.width == DEFAULT_W && image.height == DEFAULT_H &&
            physicalResult != null &&
            timestampIdentity && rawHash.isNotBlank()

        val attempt = JSONObject()
            .put("attemptClass", if (maximumResolution) "MAXIMUM_RESOLUTION_RAW" else "DEFAULT_RAW")
            .put("requestedLogicalCameraId", LOGICAL_ID)
            .put("requestedPhysicalCameraId", PHYSICAL_ID)
            .put("requestedFormat", "RAW_SENSOR")
            .put("requestedWidth", target.width)
            .put("requestedHeight", target.height)
            .put("requestedSensorPixelMode", pixelModeName(requestedPixelMode))
            .put("sessionConfigurationSupported", sessionSupport ?: JSONObject.NULL)
            .put("physicalResultPresent", physicalResult != null)
            .put("actualSensorPixelMode", pixelModeName(actualPixelMode))
            .put("timestampIdentityPass", timestampIdentity)
            .put("pass", if (maximumResolution) maxPass else defaultPass)
            .put(
                "status",
                if (maximumResolution) {
                    if (maxPass) "PASS_MAXIMUM_RESOLUTION_RAW" else "FAIL_MAXIMUM_RESOLUTION_GATE"
                } else {
                    if (defaultPass) "PASS_DEFAULT_RAW_ROUTE" else "FAIL_DEFAULT_RAW_ROUTE_GATE"
                }
            )

        val staticBlock = latestStaticObservation ?: HonorSampleDomainV04.staticCharacteristics(
            cameraManager,
            LOGICAL_ID,
            PHYSICAL_ID
        )

        val observation = JSONObject()
            .put("schema", "truthraw.fotograaf-honor-native-route-observation.v0.4")
            .put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
            .put("calibrationAuthorityGranted", false)
            .put("c0IdentitySealed", false)
            .put("scientificMasterModified", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("device", deviceBlock())
            .put(
                "route",
                JSONObject()
                    .put("routeClass", "LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT")
                    .put("logicalCameraId", LOGICAL_ID)
                    .put("requestedPhysicalCameraId", PHYSICAL_ID)
                    .put("confirmedPhysicalResultCameraIds", JSONArray(totalResult.physicalCameraTotalResults.keys.sorted()))
            )
            .put(
                "topology",
                JSONObject()
                    .put("format", "RAW_SENSOR")
                    .put("width", image.width)
                    .put("height", image.height)
                    .put("cfa", cfa)
                    .put("directCfaInExportedSampleDomain", true)
                    .put("nativePhotodiodeIdentityClaimed", false)
                    .put("processedRgb", false)
                    .put("mergedMultiFrame", false)
            )
            .put("standardCamera2", JSONObject()
                .put("staticCharacteristics", staticBlock)
                .put("runtimeResult", runtime)
            )
            .put("attempts", JSONArray().put(attempt))
            .put("sampleDomain", sampleDomain)
            .put(
                "rawPayload",
                JSONObject()
                    .put("file", rawFile.name)
                    .put("sha256", rawHash)
                    .put("bytes", rawFile.length())
                    .put("rowStride", rowStride)
                    .put("pixelStride", pixelStride)
                    .put("expectedContiguousBytes", expectedBytes)
                    .put("canonicalContiguousRawSensor", contiguous)
                    .put("imageTimestampNs", image.timestamp)
            )
            .put(
                "dngOutput",
                JSONObject()
                    .put("file", dngFile?.name ?: JSONObject.NULL)
                    .put("sha256", dngFile?.let { sha256File(it) } ?: JSONObject.NULL)
                    .put("bytes", dngFile?.length() ?: JSONObject.NULL)
                    .put("error", dngError ?: JSONObject.NULL)
                    .put("semantics", "DNG convenience/evidence container. Exact app-visible RAW payload is the source identity.")
            )
            .put(
                "observedClaims",
                HonorSampleDomainV04.observedClaims(
                    LOGICAL_ID,
                    PHYSICAL_ID,
                    image.width,
                    image.height,
                    physicalResult != null,
                    timestampIdentity
                )
            )
            .put("derivedClaims", derivedClaims(image.width, image.height, maxParentObserved))
            .put("hypotheses", JSONArray())
            .put("claimBoundary", HonorSampleDomainV04.claimBoundary())
            .put(
                "openCalibrationBlockers",
                JSONArray(
                    listOf(
                        "cameraSystemIdMappingIndependentValidation",
                        "captureSampleDomainIdentitySeal",
                        "gainReadoutStateId",
                        "focusStateClass",
                        "stabilizationStateClass"
                    )
                )
            )

        val observationFile = File(outDir(), "${base}_honor_route_observation_v0_4.json")
        observationFile.writeText(observation.toString(2))
        rememberFile(observationFile)

        log("${attempt.optString("status")}")
        log("physicalResult=${physicalResult != null} pixelMode=${pixelModeName(actualPixelMode)} rawBinning=$rawBinningFactorUsed")
        log("RAW SHA256=$rawHash timestampMatch=$timestampIdentity")
        log("Observation: ${observationFile.name}")
        if (dngFile != null) log("DNG: ${dngFile.name}") else log("DNG ERROR: $dngError")
    }

    private fun saveFailedAttempt(
        maximumResolution: Boolean,
        target: Size,
        code: String,
        detail: String,
        sessionSupport: Boolean? = null
    ) {
        try {
            val stamp = System.currentTimeMillis()
            val attempt = JSONObject()
                .put("attemptClass", if (maximumResolution) "MAXIMUM_RESOLUTION_RAW" else "DEFAULT_RAW")
                .put("requestedLogicalCameraId", LOGICAL_ID)
                .put("requestedPhysicalCameraId", PHYSICAL_ID)
                .put("requestedFormat", "RAW_SENSOR")
                .put("requestedWidth", target.width)
                .put("requestedHeight", target.height)
                .put(
                    "requestedSensorPixelMode",
                    if (maximumResolution) "MAXIMUM_RESOLUTION" else "DEFAULT"
                )
                .put("sessionConfigurationSupported", sessionSupport ?: JSONObject.NULL)
                .put("pass", false)
                .put("status", code)
                .put("detail", detail)

            val root = JSONObject()
                .put("schema", "truthraw.fotograaf-honor-native-route-observation.v0.4")
                .put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
                .put("calibrationAuthorityGranted", false)
                .put("c0IdentitySealed", false)
                .put("scientificMasterModified", false)
                .put("physicalFrameCount", 0)
                .put("independentEvidenceCount", 0)
                .put("device", deviceBlock())
                .put("attempts", JSONArray().put(attempt))
                .put("claimBoundary", HonorSampleDomainV04.claimBoundary())

            val mode = if (maximumResolution) "MAX" else "DEFAULT"
            val file = File(outDir(), "C2ATTEMPT_${stamp}_${mode}_FAILED_v0_4.json")
            file.writeText(root.toString(2))
            rememberFile(file)
            log("$code: $detail")
            log("Failed-attempt observation: ${file.name}")
        } catch (t: Throwable) {
            log("Could not save failed attempt: $t")
        }
    }

    private fun supportsRawSize(maximumResolution: Boolean, width: Int, height: Int): Boolean {
        val c = cameraManager.getCameraCharacteristics(PHYSICAL_ID)
        val map = if (maximumResolution) {
            c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
        } else {
            c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        } ?: return false

        val normal = try { map.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList() ?: emptyList() } catch (_: Throwable) { emptyList() }
        val high = if (maximumResolution) {
            try { map.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)?.toList() ?: emptyList() } catch (_: Throwable) { emptyList() }
        } else {
            emptyList()
        }
        return (normal + high).any { it.width == width && it.height == height }
    }

    private fun derivedClaims(width: Int, height: Int, maximumDomainObserved: Boolean): JSONArray {
        val claims = JSONArray()
        if (
            maximumDomainObserved &&
            width > 0 && height > 0 &&
            MAX_W % width == 0 && MAX_H % height == 0
        ) {
            val sx = MAX_W / width
            val sy = MAX_H / height
            if (sx == sy) {
                claims.put(
                    JSONObject()
                        .put("claim", "maximum-to-captured coordinate scale")
                        .put("maximumDomain", JSONArray(listOf(MAX_W, MAX_H)))
                        .put("capturedDomain", JSONArray(listOf(width, height)))
                        .put("linearScale", sx)
                        .put("areaScale", sx.toLong() * sy.toLong())
                        .put("interpretation", "MATHEMATICAL_RELATION_ONLY")
                )
            }
        }
        return claims
    }

    private fun deviceBlock(): JSONObject = JSONObject()
        .put("manufacturer", android.os.Build.MANUFACTURER)
        .put("model", android.os.Build.MODEL)
        .put("device", android.os.Build.DEVICE)
        .put("fingerprint", android.os.Build.FINGERPRINT)
        .put("sdkInt", android.os.Build.VERSION.SDK_INT)

    private fun cfaName(value: Int?): String = when (value) {
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB -> "RGGB"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG -> "GRBG"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG -> "GBRG"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR -> "BGGR"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB -> "RGB"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_MONO -> "MONO"
        CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_NIR -> "NIR"
        else -> "UNKNOWN_${value ?: "NULL"}"
    }

    private fun pixelModeName(value: Int?): String = when (value) {
        CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> "MAXIMUM_RESOLUTION"
        CameraMetadata.SENSOR_PIXEL_MODE_DEFAULT -> "DEFAULT"
        null -> "UNAVAILABLE"
        else -> "UNKNOWN_$value"
    }

    private fun hashAndWriteBuffer(source: java.nio.ByteBuffer, file: File): String {
        val hashBuffer = source.duplicate().apply { rewind() }
        val digest = MessageDigest.getInstance("SHA-256")
        digest.update(hashBuffer)
        val hash = digest.digest().joinToString("") { "%02x".format(it) }

        FileOutputStream(file).channel.use { channel ->
            val writeBuffer = source.duplicate().apply { rewind() }
            while (writeBuffer.hasRemaining()) channel.write(writeBuffer)
            channel.force(true)
        }
        return hash
    }

    private fun sha256File(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input ->
            val buffer = ByteArray(1 shl 20)
            while (true) {
                val count = input.read(buffer)
                if (count < 0) break
                digest.update(buffer, 0, count)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun rememberFile(file: File) {
        synchronized(latestEvidenceFiles) {
            latestEvidenceFiles.removeAll { it.absolutePath == file.absolutePath }
            latestEvidenceFiles.add(file)
        }
    }

    private fun shareEvidence() {
        val files = synchronized(latestEvidenceFiles) {
            latestEvidenceFiles.filter { it.exists() }.takeLast(8)
        }
        if (files.isEmpty()) {
            log("Nog geen evidence om te delen.")
            return
        }
        try {
            val uris = ArrayList<Uri>(files.map {
                FileProvider.getUriForFile(this, "${packageName}.files", it)
            })
            val intent = Intent(Intent.ACTION_SEND_MULTIPLE).apply {
                type = "application/octet-stream"
                putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                putExtra(Intent.EXTRA_SUBJECT, "TruthRaw FotoGraaf HONOR route proof v0.4")
            }
            startActivity(Intent.createChooser(intent, "Deel TruthRaw evidence"))
        } catch (t: Throwable) {
            log("SHARE ERROR: $t")
        }
    }
}

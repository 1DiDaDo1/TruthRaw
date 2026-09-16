package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.DngCreator
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.Locale

/**
 * TruthRaw FotoGraaf staged Camera-5 200MP test v0.7.
 *
 * Crucial runtime design:
 *  0. Activity startup touches no Camera2 object.
 *  1. Capability discovery is explicit and exception-contained.
 *  2. Preview uses logical camera 0 only (no physical output binding), asks for
 *     ~3.7x zoom when supported, and reports ACTIVE_PHYSICAL_ID instead of
 *     assuming physical camera 5.
 *  3. 200MP capture is a separate RAW-only session explicitly bound to physical
 *     camera 5 and requests MAXIMUM_RESOLUTION sensor pixel mode.
 *
 * Preview is framing/3A observation only and never upgrades sensor evidence.
 */
class FotoGraaf200MpStagedActivity : Activity(), TextureView.SurfaceTextureListener {

    private lateinit var preview: TextureView
    private lateinit var status: TextView
    private lateinit var telemetry: TextView
    private lateinit var capabilityButton: Button
    private lateinit var previewButton: Button
    private lateinit var captureButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button

    private var manager: CameraManager? = null
    private var logicalCharacteristics: CameraCharacteristics? = null
    private var physical5Characteristics: CameraCharacteristics? = null
    private var capabilityReady = false

    private val cameraThread = HandlerThread("truthraw-200mp-staged").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var rawReader: ImageReader? = null
    private var previewFrames = 0L
    @Volatile private var lastPreviewResult: TotalCaptureResult? = null
    @Volatile private var lastActivePhysicalId: String? = null

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var finalizing = false

    private var capturedDng: File? = null
    private var capturedJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        status.text = "STAGE 0 PASS · scherm geopend zonder Camera2/HAL-aanroep.\nDruk nu eerst op Stap 1."
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        }
    }

    override fun onPause() {
        closeCameraResources(keepOutputs = true)
        super.onPause()
    }

    override fun onDestroy() {
        closeCameraResources(keepOutputs = true)
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
        }
        cameraThread.quitSafely()
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA) {
            if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
                status.text = "CAMERA permission verleend. Druk op Stap 1; er is nog geen camera geopend."
            } else {
                status.text = "CAMERA permission ontbreekt. De test blijft fail-closed."
            }
        }
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(12), dp(12), dp(16))
            setBackgroundColor(Color.rgb(10, 12, 15))
        }

        root.addView(label("TruthRaw · 200MP Tele Test v0.7", 24f, true))
        root.addView(label(
            "Staged: capability → logical live preview (3.7× request) → aparte physical-5 16320×12288 RAW-only capture.",
            11f,
            false,
            Color.rgb(184, 191, 202),
        ))
        root.addView(space(6))

        preview = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraaf200MpStagedActivity
            setBackgroundColor(Color.BLACK)
        }
        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        root.addView(space(6))

        telemetry = label("Preview nog niet gestart.", 11f, true, Color.rgb(220, 225, 234))
        status = label("Initialiseren…", 11f, false, Color.WHITE)
        root.addView(telemetry)
        root.addView(space(4))
        root.addView(status)
        root.addView(space(6))

        capabilityButton = button("Stap 1 · lees Camera-5 200MP capability") { readCapability() }
        previewButton = button("Stap 2 · start live beeld via logical 0 · 3.7×") { startLogicalPreview() }.apply { isEnabled = false }
        captureButton = button("Stap 3 · CAPTURE physical 5 · 16320×12288") { capture200Mp() }.apply { isEnabled = false }
        saveDngButton = button("200MP DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("200MP evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }

        root.addView(capabilityButton)
        root.addView(previewButton)
        root.addView(captureButton)
        root.addView(saveDngButton)
        root.addView(saveJsonButton)

        root.addView(label(
            "Preview is nooit 200MP-bewijs. Alleen een echte 16320×12288 RAW_SENSOR Image + physical Camera-5 result + exact timestamp + MAX pixel mode kan de capture-gate passeren.",
            9f,
            false,
            Color.rgb(145, 153, 165),
        ))
        return root
    }

    private fun readCapability() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            setStatus("Stap 1 geblokkeerd: CAMERA permission ontbreekt.")
            return
        }
        capabilityButton.isEnabled = false
        previewButton.isEnabled = false
        captureButton.isEnabled = false
        setStatus("Stap 1 bezig · characteristics lezen; er wordt nog géén camera geopend…")

        Thread({
            val result = runCatching {
                val m = getSystemService(CameraManager::class.java)
                val logical = m.getCameraCharacteristics(LOGICAL_ID)
                require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
                    "logical 0 meldt physical 5 niet; physicalIds=${logical.physicalCameraIds}"
                }
                val physical = m.getCameraCharacteristics(PHYSICAL_ID)
                val standardMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val high = standardMap?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
                val maxMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                val maximum = maxMap?.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
                val exactHigh = high.any { it.width == TARGET_W && it.height == TARGET_H }
                val exactMax = maximum.any { it.width == TARGET_W && it.height == TARGET_H }
                require(exactHigh || exactMax) {
                    "16320×12288 RAW_SENSOR ontbreekt; high=$high maximum=$maximum"
                }
                Triple(m, logical, physical) to Pair(high, maximum)
            }

            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { packed ->
                    manager = packed.first.first
                    logicalCharacteristics = packed.first.second
                    physical5Characteristics = packed.first.third
                    capabilityReady = true
                    val high = packed.second.first.joinToString { "${it.width}×${it.height}" }
                    val maximum = packed.second.second.joinToString { "${it.width}×${it.height}" }
                    previewButton.isEnabled = preview.isAvailable
                    setStatus(
                        "STAGE 1 PASS · physical 5 + exact 16320×12288 RAW_SENSOR geadverteerd.\n" +
                            "highResolution=[$high]\nmaximumMap=[$maximum]\n" +
                            "Druk nu Stap 2 voor live beeld."
                    )
                }.onFailure { e ->
                    capabilityReady = false
                    setStatus("STAGE 1 BLOCKED · ${e.javaClass.simpleName}: ${e.message}\nGeen camera geopend.")
                }
            }
        }, "truthraw-200mp-capability-stage").start()
    }

    private fun startLogicalPreview() {
        if (!capabilityReady) {
            setStatus("Voer eerst Stap 1 uit.")
            return
        }
        if (!preview.isAvailable) {
            setStatus("Preview surface is nog niet beschikbaar.")
            return
        }
        val m = manager ?: return
        val logical = logicalCharacteristics ?: return

        closeCameraResources(keepOutputs = true)
        previewFrames = 0L
        lastPreviewResult = null
        lastActivePhysicalId = null
        previewButton.isEnabled = false
        captureButton.isEnabled = false

        val sizes = logical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)?.toList().orEmpty()
        val chosen = sizes.filter { it.width <= 1920 && it.height <= 1440 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.firstOrNull()
            ?: android.util.Size(1280, 720)
        val texture = preview.surfaceTexture ?: return
        texture.setDefaultBufferSize(chosen.width, chosen.height)
        previewSurface = Surface(texture)

        setStatus("Stap 2 · logical camera 0 openen met één preview-surface; physical output wordt NIET geforceerd…")
        runCatching {
            m.openCamera(LOGICAL_ID, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    camera = device
                    createLogicalPreviewSession(device, logical)
                }

                override fun onDisconnected(device: CameraDevice) {
                    device.close()
                    setStatusAny("STAGE 2 DISCONNECTED · logical camera 0")
                    closeCameraResources(keepOutputs = true)
                }

                override fun onError(device: CameraDevice, error: Int) {
                    device.close()
                    setStatusAny("STAGE 2 CAMERA ERROR=$error · logical camera 0")
                    closeCameraResources(keepOutputs = true)
                }
            }, cameraHandler)
        }.onFailure { e ->
            setStatus("STAGE 2 OPEN FAIL · ${e.javaClass.simpleName}: ${e.message}")
            previewButton.isEnabled = true
        }
    }

    private fun createLogicalPreviewSession(device: CameraDevice, logical: CameraCharacteristics) {
        val surface = previewSurface ?: return
        val output = OutputConfiguration(surface) // Deliberately no setPhysicalCameraId().
        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    startLogicalRepeating(device, s, logical)
                }

                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("STAGE 2 SESSION FAIL · logical preview-only combinatie geweigerd.")
                    runOnUiThread { previewButton.isEnabled = true }
                }
            },
        )
        runCatching { device.createCaptureSession(config) }
            .onFailure {
                setStatusAny("STAGE 2 createCaptureSession FAIL · ${it.javaClass.simpleName}: ${it.message}")
                runOnUiThread { previewButton.isEnabled = true }
            }
    }

    private fun startLogicalRepeating(device: CameraDevice, s: CameraCaptureSession, logical: CameraCharacteristics) {
        val surface = previewSurface ?: return
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            b.addTarget(surface)
            b.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            b.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)

            val af = logical.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
            if (af.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
                b.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
            }

            val zoomRange = logical.get(CameraCharacteristics.CONTROL_ZOOM_RATIO_RANGE)
            val requestedZoom = zoomRange?.let { ZOOM_REQUEST.coerceIn(it.lower, it.upper) }
            if (requestedZoom != null) b.set(CaptureRequest.CONTROL_ZOOM_RATIO, requestedZoom)

            s.setRepeatingRequest(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    previewFrames++
                    lastPreviewResult = result
                    lastActivePhysicalId = result.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)
                    val active = lastActivePhysicalId
                    val iso = result.get(CaptureResult.SENSOR_SENSITIVITY)
                    val exp = result.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    val zoom = result.get(CaptureResult.CONTROL_ZOOM_RATIO)
                    val afState = result.get(CaptureResult.CONTROL_AF_STATE)
                    val aeState = result.get(CaptureResult.CONTROL_AE_STATE)
                    if (previewFrames == 1L || previewFrames % 15L == 0L) {
                        runOnUiThread {
                            telemetry.text = buildString {
                                append("LIVE IMAGE · frame=$previewFrames · logical=0 · zoom=${zoom ?: requestedZoom ?: "?"}×")
                                append("\nactivePhysical=${active ?: "not reported"} · ${if (active == PHYSICAL_ID) "TELE 5 CONFIRMED" else "tele 5 nog niet bevestigd"}")
                                append("\nISO=${iso ?: "?"} · t=${exp?.div(1_000_000.0)?.let { String.format(Locale.ROOT, "%.3f ms", it) } ?: "?"} · AF=$afState · AE=$aeState")
                            }
                            captureButton.isEnabled = true
                            previewButton.isEnabled = true
                        }
                    }
                }
            }, cameraHandler)
            setStatusAny(
                "STAGE 2 REQUEST ACTIVE · logical preview-only · zoom request=${requestedZoom ?: "unsupported"}.\n" +
                    "Wacht op LIVE IMAGE; activePhysical bepaalt of Android werkelijk tele 5 kiest."
            )
        } catch (e: Throwable) {
            setStatusAny("STAGE 2 REPEATING FAIL · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { previewButton.isEnabled = true }
        }
    }

    private fun capture200Mp() {
        if (!capabilityReady) {
            setStatus("Stap 1 is niet PASS.")
            return
        }
        if (lastPreviewResult == null) {
            setStatus("Stap 3 geblokkeerd: eerst minimaal één LIVE IMAGE-frame ontvangen.")
            return
        }
        val device = camera ?: run {
            setStatus("Stap 3 geblokkeerd: logical camera 0 is niet open.")
            return
        }
        val physical = physical5Characteristics ?: return

        captureButton.isEnabled = false
        previewButton.isEnabled = false
        clearOutputs()
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            finalizing = false
        }

        runCatching { session?.stopRepeating() }
        runCatching { session?.abortCaptures() }
        runCatching { session?.close() }
        session = null
        runCatching { previewSurface?.release() }
        previewSurface = null

        val reader = runCatching {
            ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW_SENSOR, 1)
        }.getOrElse { e ->
            setStatus("STAGE 3 ImageReader FAIL · ${e.javaClass.simpleName}: ${e.message}")
            previewButton.isEnabled = true
            return
        }
        rawReader = reader
        reader.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
            synchronized(pairLock) {
                pendingImage?.close()
                pendingImage = image
            }
            finalizeIfPaired()
        }, cameraHandler)

        val output = OutputConfiguration(reader.surface)
        val bind = runCatching { output.setPhysicalCameraId(PHYSICAL_ID) }
        if (bind.isFailure) {
            setStatus("STAGE 3 physical-5 output bind FAIL · ${bind.exceptionOrNull()?.message}")
            closeCameraResources(keepOutputs = true)
            previewButton.isEnabled = true
            return
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    submit200MpStill(device, s, physical)
                }

                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("STAGE 3 RAW SESSION BLOCKED · 16320×12288 physical-5 output geweigerd.")
                    closeCameraResources(keepOutputs = true)
                    runOnUiThread { previewButton.isEnabled = true }
                }
            },
        )
        setStatus("STAGE 3 · preview gesloten; RAW-only physical 5 · 16320×12288 session wordt gemaakt…")
        runCatching { device.createCaptureSession(config) }
            .onFailure {
                setStatusAny("STAGE 3 createCaptureSession FAIL · ${it.javaClass.simpleName}: ${it.message}")
                closeCameraResources(keepOutputs = true)
                runOnUiThread { previewButton.isEnabled = true }
            }
    }

    private fun submit200MpStill(device: CameraDevice, s: CameraCaptureSession, physical: CameraCharacteristics) {
        val reader = rawReader ?: return
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            b.addTarget(reader.surface)
            b.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            b.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)
            b.set(CaptureRequest.SENSOR_PIXEL_MODE, CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)

            val nr = physical.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) b.set(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
            val edge = physical.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) b.set(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)

            setStatusAny("STAGE 3 CAPTURE SENT · wachten op Image + physical Camera-5 TotalCaptureResult…")
            s.capture(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }

                override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: android.hardware.camera2.CaptureFailure) {
                    setStatusAny("STAGE 3 CAPTURE FAIL · reason=${failure.reason}")
                    runOnUiThread { previewButton.isEnabled = true }
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatusAny("STAGE 3 request FAIL · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { previewButton.isEnabled = true }
        }
    }

    private fun finalizeIfPaired() {
        val image: Image
        val logicalResult: TotalCaptureResult
        synchronized(pairLock) {
            if (finalizing) return
            image = pendingImage ?: return
            logicalResult = pendingResult ?: return
            finalizing = true
            pendingImage = null
            pendingResult = null
        }
        cameraHandler.post { finalizeCapture(image, logicalResult) }
    }

    private fun finalizeCapture(image: Image, logicalResult: TotalCaptureResult) {
        try {
            require(image.width == TARGET_W && image.height == TARGET_H) {
                "RAW dimensions ${image.width}×${image.height} != ${TARGET_W}×${TARGET_H}"
            }
            val physicalResult = logicalResult.physicalCameraResults[PHYSICAL_ID]
                ?: error("physical Camera-5 TotalCaptureResult ontbreekt; ids=${logicalResult.physicalCameraResults.keys}")
            val sensorTs = physicalResult.get(CaptureResult.SENSOR_TIMESTAMP)
                ?: error("physical Camera-5 SENSOR_TIMESTAMP ontbreekt")
            require(sensorTs == image.timestamp) {
                "Image.timestamp=${image.timestamp} != physical5 SENSOR_TIMESTAMP=$sensorTs"
            }
            val pixelMode = physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE)
            require(pixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION) {
                "physical5 SENSOR_PIXEL_MODE=$pixelMode, MAXIMUM_RESOLUTION vereist"
            }
            val plane = image.planes.singleOrNull() ?: error("RAW_SENSOR planeCount=${image.planes.size}, exact 1 vereist")
            require(plane.pixelStride == 2) { "RAW pixelStride=${plane.pixelStride}, 2 vereist" }
            require(plane.rowStride >= TARGET_W * 2) { "RAW rowStride=${plane.rowStride} te klein" }
            require(TARGET_W.toLong() * TARGET_H.toLong() == TARGET_SAMPLES) { "sample count invariant broken" }

            val rawSha = sha256ValidSampleBytes(image)
            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")
            val stamp = System.currentTimeMillis()
            val dng = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v07.dng")
            FileOutputStream(dng).use { out ->
                DngCreator(physical, physicalResult).use { creator ->
                    creator.setOrientation(1)
                    creator.writeImage(out, image)
                }
            }
            image.close()
            val dngSha = sha256File(dng)
            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_EVIDENCE_v07.json")
            report.writeText(buildEvidence(logicalResult, physicalResult, plane.rowStride, plane.pixelStride, rawSha, dng, dngSha).toString(2))
            capturedDng = dng
            capturedJson = report

            setStatusAny(
                "STAGE 3 CAPTURE PASS · physical 5 · 16320×12288 · 200,540,160 samples · MAX pixel mode · timestamp exact.\n" +
                    "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE · host Step-3B blijft vereist."
            )
            runOnUiThread {
                saveDngButton.isEnabled = true
                saveJsonButton.isEnabled = true
                previewButton.isEnabled = true
            }
        } catch (e: Throwable) {
            runCatching { image.close() }
            setStatusAny("STAGE 3 FAIL CLOSED · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { previewButton.isEnabled = true }
        } finally {
            runCatching { session?.close() }
            session = null
            runCatching { rawReader?.close() }
            rawReader = null
            runCatching { camera?.close() }
            camera = null
        }
    }

    private fun sha256ValidSampleBytes(image: Image): String {
        val plane = image.planes.single()
        val src = plane.buffer.duplicate()
        val base = src.position()
        val limit = src.limit()
        val rowBytes = TARGET_W * 2
        val md = MessageDigest.getInstance("SHA-256")
        for (y in 0 until TARGET_H) {
            val start = base + y * plane.rowStride
            val end = start + rowBytes
            require(start >= base && end <= limit) { "RAW valid row $y buiten buffer: $start..$end limit=$limit" }
            val row = src.duplicate()
            row.position(start)
            row.limit(end)
            md.update(row)
        }
        return md.digest().joinToString("") { "%02x".format(it) }
    }

    private fun buildEvidence(
        logicalResult: TotalCaptureResult,
        physicalResult: CaptureResult,
        rowStride: Int,
        pixelStride: Int,
        rawSha: String,
        dng: File,
        dngSha: String,
    ): JSONObject {
        val activePreview = lastActivePhysicalId
        return JSONObject()
            .put("schema", "truthraw.fotograaf-camera5-200mp-staged-evidence.v0.7")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE")
            .put("calibrationAuthorityGranted", false)
            .put("scientificWriteback", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("preview", JSONObject()
                .put("route", "logical_0_only_no_physical_output_binding")
                .put("requestedZoomRatio", ZOOM_REQUEST)
                .put("lastActivePhysicalId", activePreview ?: JSONObject.NULL)
                .put("previewCreatesEvidence", false))
            .put("captureRoute", JSONObject()
                .put("logicalCameraId", LOGICAL_ID)
                .put("physicalCameraId", PHYSICAL_ID)
                .put("width", TARGET_W)
                .put("height", TARGET_H)
                .put("sampleCount", TARGET_SAMPLES)
                .put("maximumResolutionRequested", true)
                .put("captureResultSensorPixelMode", physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE) ?: JSONObject.NULL)
                .put("reportedPhysicalIds", JSONArray(logicalResult.physicalCameraResults.keys.sorted())))
            .put("rawPlane", JSONObject()
                .put("rowStride", rowStride)
                .put("pixelStride", pixelStride)
                .put("validSampleBytes", TARGET_SAMPLES * 2L)
                .put("validSampleSha256", rawSha))
            .put("captureResult", JSONObject()
                .put("sensorTimestamp", physicalResult.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("iso", physicalResult.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", physicalResult.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("focalLengthMm", physicalResult.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("focusDistanceDiopters", physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                .put("awbState", physicalResult.get(CaptureResult.CONTROL_AWB_STATE) ?: JSONObject.NULL)
                .put("colorCorrectionGains", physicalResult.get(CaptureResult.COLOR_CORRECTION_GAINS)?.toString() ?: JSONObject.NULL)
                .put("colorCorrectionTransform", physicalResult.get(CaptureResult.COLOR_CORRECTION_TRANSFORM)?.toString() ?: JSONObject.NULL))
            .put("dng", JSONObject()
                .put("bytes", dng.length())
                .put("sha256", dngSha)
                .put("orientationRequested", 1))
            .put("boundary", "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_NOT_UNTOUCHED_NATIVE_ADC_PROOF")
            .put("nextGate", "HOST_STEP3B_REPLAY_AND_PROMOTION")
    }

    private fun closeCameraResources(keepOutputs: Boolean) {
        runCatching { session?.stopRepeating() }
        runCatching { session?.close() }
        session = null
        runCatching { camera?.close() }
        camera = null
        runCatching { rawReader?.close() }
        rawReader = null
        runCatching { previewSurface?.release() }
        previewSurface = null
        if (!keepOutputs) clearOutputs()
        if (::captureButton.isInitialized) captureButton.isEnabled = false
        if (::previewButton.isInitialized) previewButton.isEnabled = capabilityReady && preview.isAvailable
    }

    private fun clearOutputs() {
        capturedDng = null
        capturedJson = null
        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false
        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false
    }

    private fun saveFile(file: File?, mime: String, requestCode: Int) {
        if (file == null || !file.exists()) return
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime
            putExtra(Intent.EXTRA_TITLE, file.name)
        }, requestCode)
    }

    @Deprecated("Retained for minSdk31 document export")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        val source = when (requestCode) {
            REQUEST_SAVE_DNG -> capturedDng
            REQUEST_SAVE_JSON -> capturedJson
            else -> null
        } ?: return
        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                FileInputStream(source).use { input -> input.copyTo(out) }
            } ?: error("geen output stream")
        }.onSuccess { setStatus("${source.name} opgeslagen; authority ongewijzigd.") }
            .onFailure { setStatus("Opslaan faalde: ${it.message}") }
    }

    private fun sha256File(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                md.update(buffer, 0, n)
            }
        }
        return md.digest().joinToString("") { "%02x".format(it) }
    }

    private fun setStatus(text: String) { if (::status.isInitialized) status.text = text }
    private fun setStatusAny(text: String) { runOnUiThread { setStatus(text) } }

    private fun label(value: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun button(value: String, action: () -> Unit): Button = Button(this).apply {
        text = value
        isAllCaps = false
        minHeight = dp(48)
        setOnClickListener { action() }
    }

    private fun space(height: Int): View = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        if (::previewButton.isInitialized) previewButton.isEnabled = capabilityReady
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) = Unit
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        closeCameraResources(keepOutputs = true)
        return true
    }
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit

    companion object {
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val TARGET_SAMPLES = 200_540_160L
        private const val ZOOM_REQUEST = 3.7f
        private const val REQUEST_CAMERA = 5700
        private const val REQUEST_SAVE_DNG = 5701
        private const val REQUEST_SAVE_JSON = 5702
    }
}

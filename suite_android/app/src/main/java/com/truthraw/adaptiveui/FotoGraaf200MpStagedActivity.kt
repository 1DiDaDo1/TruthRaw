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
import android.util.Size
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
 * TruthRaw FotoGraaf staged Camera-5 200MP test v0.10.
 *
 * v0.10 restores the exact v0.7 capability route that was lost in v0.9:
 *   SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION
 *     .getHighResolutionOutputSizes(RAW_SENSOR)
 *
 * Runtime route remains the safer v0.8/v0.9 design:
 *   logical camera 0 -> physical OutputConfiguration camera 5.
 *
 * The original app-visible RAW_SENSOR Plane[0] buffer is persisted as primary
 * evidence before the auxiliary DNG is produced. No full-frame Java ByteArray
 * is allocated.
 *
 * Android TextureView does not support background drawables. Never call
 * setBackground/setBackgroundColor on this TextureView; the parent layout owns
 * the black background instead.
 */
class FotoGraaf200MpStagedActivity : Activity(), TextureView.SurfaceTextureListener {

    private data class RawRoutes(
        val standardOutput: List<Size>,
        val standardHigh: List<Size>,
        val maximumOutput: List<Size>,
        val maximumHigh: List<Size>,
        val selectedSource: String,
    )

    private data class RawBufferEvidence(
        val file: File,
        val fileSha256: String,
        val fileBytes: Long,
        val canonicalContiguous: Boolean,
        val validSampleSha256: String,
        val validSampleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
    )

    private lateinit var preview: TextureView
    private lateinit var status: TextView
    private lateinit var telemetry: TextView
    private lateinit var capabilityButton: Button
    private lateinit var previewButton: Button
    private lateinit var captureButton: Button
    private lateinit var saveRawButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button

    private var manager: CameraManager? = null
    private var logicalCharacteristics: CameraCharacteristics? = null
    private var physical5Characteristics: CameraCharacteristics? = null
    private var capabilityRoutes: RawRoutes? = null
    private var capabilityReady = false

    private val cameraThread = HandlerThread("truthraw-200mp-v010").apply { start() }
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

    private var capturedRaw: File? = null
    private var capturedDng: File? = null
    private var capturedJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        status.text = "STAGE 0 PASS · v0.10 UI geopend zonder Camera2/HAL-aanroep.\nDruk nu eerst op Stap 1."
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
            status.text = if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
                "CAMERA permission verleend. Druk op Stap 1; er is nog geen camera geopend."
            } else {
                "CAMERA permission ontbreekt. De test blijft fail-closed."
            }
        }
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(12), dp(12), dp(16))
            setBackgroundColor(Color.rgb(10, 12, 15))
        }

        root.addView(label("TruthRaw · 200MP Tele Test v0.10", 24f, true))
        root.addView(label(
            "MAXIMUM_RESOLUTION high-res discovery → logical 0 live preview → physical 5 RAW-only capture → originele RAW buffer bewaren.",
            11f,
            false,
            Color.rgb(184, 191, 202),
        ))
        root.addView(space(6))

        preview = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraaf200MpStagedActivity
            // IMPORTANT: TextureView rejects background drawables/background colors.
        }
        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        root.addView(space(6))

        telemetry = label("Preview nog niet gestart.", 11f, true, Color.rgb(220, 225, 234))
        status = label("Initialiseren…", 11f, false, Color.WHITE)
        root.addView(telemetry)
        root.addView(space(4))
        root.addView(status)
        root.addView(space(6))

        capabilityButton = button("Stap 1 · lees alle Camera-5 RAW capability-routes") { readCapability() }
        previewButton = button("Stap 2 · start live beeld via logical 0 · 3.7×") { startLogicalPreview() }.apply { isEnabled = false }
        captureButton = button("Stap 3 · CAPTURE physical 5 · 16320×12288 RAW_SENSOR") { capture200Mp() }.apply { isEnabled = false }
        saveRawButton = button("Originele 200MP RAW buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }
        saveDngButton = button("Auxiliary 200MP DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("200MP evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }

        root.addView(capabilityButton)
        root.addView(previewButton)
        root.addView(captureButton)
        root.addView(saveRawButton)
        root.addView(saveDngButton)
        root.addView(saveJsonButton)

        root.addView(label(
            "PASS betekent alleen app-visible 16320×12288 RAW_SENSOR van physical 5. Native/untouched ADC blijft een aparte wetenschappelijke gate.",
            9f,
            false,
            Color.rgb(145, 153, 165),
        ))
        return root
    }

    private fun safeSizes(block: () -> Array<Size>?): List<Size> =
        runCatching { block()?.toList().orEmpty() }.getOrDefault(emptyList())

    private fun containsTarget(xs: List<Size>): Boolean = xs.any { it.width == TARGET_W && it.height == TARGET_H }

    private fun routeText(xs: List<Size>): String = xs.joinToString { "${it.width}×${it.height}" }

    private fun routesJson(routes: RawRoutes): JSONObject = JSONObject()
        .put("standardOutput", sizesJson(routes.standardOutput))
        .put("standardHighResolution", sizesJson(routes.standardHigh))
        .put("maximumOutput", sizesJson(routes.maximumOutput))
        .put("maximumHighResolution", sizesJson(routes.maximumHigh))
        .put("selectedSource", routes.selectedSource)

    private fun sizesJson(xs: List<Size>): JSONArray = JSONArray(xs.map { JSONArray(listOf(it.width, it.height)) })

    private fun readCapability() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            setStatus("Stap 1 geblokkeerd: CAMERA permission ontbreekt.")
            return
        }
        capabilityButton.isEnabled = false
        previewButton.isEnabled = false
        captureButton.isEnabled = false
        setStatus("Stap 1 · standard + maximum-resolution RAW maps uitlezen; er wordt nog géén camera geopend…")

        Thread({
            val result = runCatching {
                val m = getSystemService(CameraManager::class.java)
                val logical = m.getCameraCharacteristics(LOGICAL_ID)
                require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
                    "logical 0 meldt physical 5 niet; physicalIds=${logical.physicalCameraIds}"
                }
                val physical = m.getCameraCharacteristics(PHYSICAL_ID)
                val standardMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val maximumMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)

                val standardOutput = safeSizes { standardMap?.getOutputSizes(ImageFormat.RAW_SENSOR) }
                val standardHigh = safeSizes { standardMap?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumOutput = safeSizes { maximumMap?.getOutputSizes(ImageFormat.RAW_SENSOR) }
                val maximumHigh = safeSizes { maximumMap?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR) }

                val selected = when {
                    containsTarget(maximumHigh) -> "MAXIMUM_MAP_HIGH_RESOLUTION"
                    containsTarget(maximumOutput) -> "MAXIMUM_MAP_OUTPUT"
                    containsTarget(standardHigh) -> "STANDARD_MAP_HIGH_RESOLUTION"
                    containsTarget(standardOutput) -> "STANDARD_MAP_OUTPUT"
                    else -> error(
                        "16320×12288 RAW_SENSOR ontbreekt in alle vier routes; " +
                            "standard.out=[$standardOutput] standard.high=[$standardHigh] " +
                            "maximum.out=[$maximumOutput] maximum.high=[$maximumHigh]"
                    )
                }

                val routes = RawRoutes(standardOutput, standardHigh, maximumOutput, maximumHigh, selected)
                Triple(m, logical, physical) to routes
            }

            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { packed ->
                    manager = packed.first.first
                    logicalCharacteristics = packed.first.second
                    physical5Characteristics = packed.first.third
                    capabilityRoutes = packed.second
                    capabilityReady = true
                    previewButton.isEnabled = preview.isAvailable
                    val r = packed.second
                    setStatus(
                        "STAGE 1 PASS · exact 16320×12288 RAW_SENSOR gevonden via ${r.selectedSource}.\n" +
                            "standard.out=[${routeText(r.standardOutput)}]\n" +
                            "standard.high=[${routeText(r.standardHigh)}]\n" +
                            "maximum.out=[${routeText(r.maximumOutput)}]\n" +
                            "maximum.high=[${routeText(r.maximumHigh)}]\n" +
                            "Druk nu Stap 2 voor live beeld."
                    )
                }.onFailure { e ->
                    capabilityReady = false
                    capabilityRoutes = null
                    setStatus("STAGE 1 BLOCKED · ${e.javaClass.simpleName}: ${e.message}\nGeen camera geopend.")
                }
            }
        }, "truthraw-200mp-v010-capability").start()
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
            ?: Size(1280, 720)
        val texture = preview.surfaceTexture ?: return
        texture.setDefaultBufferSize(chosen.width, chosen.height)
        previewSurface = Surface(texture)

        setStatus("Stap 2 · logical camera 0 openen met preview-only surface…")
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
        val output = OutputConfiguration(surface)
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
                    if (previewFrames == 1L || previewFrames % 15L == 0L) {
                        val active = lastActivePhysicalId
                        val iso = result.get(CaptureResult.SENSOR_SENSITIVITY)
                        val exp = result.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                        val zoom = result.get(CaptureResult.CONTROL_ZOOM_RATIO)
                        val afState = result.get(CaptureResult.CONTROL_AF_STATE)
                        val aeState = result.get(CaptureResult.CONTROL_AE_STATE)
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
                    "Preview is framing/3A; Stap 3 bindt RAW expliciet aan physical 5."
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

        val reader = runCatching { ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW_SENSOR, 1) }
            .getOrElse { e ->
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
        val configured = runCatching {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        }
        if (configured.isFailure) {
            setStatus("STAGE 3 physical/MAX output bind FAIL · ${configured.exceptionOrNull()?.message}")
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
                    setStatusAny("STAGE 3 RAW SESSION BLOCKED · 16320×12288 physical-5/MAX output geweigerd.")
                    closeCameraResources(keepOutputs = true)
                    runOnUiThread { previewButton.isEnabled = true }
                }
            },
        )

        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
        setStatus("STAGE 3 · physical 5 · 16320×12288 · MAX mode; isSessionConfigurationSupported=$support")
        if (support == false) {
            setStatus("STAGE 3 BLOCKED · device meldt de exacte 200MP physical-5/MAX sessie unsupported.")
            closeCameraResources(keepOutputs = true)
            previewButton.isEnabled = true
            return
        }

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

            setStatusAny("STAGE 3 CAPTURE SENT · wachten op RAW Image + physical Camera-5 TotalCaptureResult…")
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

            val stamp = System.currentTimeMillis()
            val rawEvidence = persistOriginalRawBuffer(image, stamp)
            capturedRaw = rawEvidence.file

            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")
            var dng: File? = null
            var dngSha: String? = null
            var dngError: String? = null
            runCatching {
                val candidate = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v010.dng")
                FileOutputStream(candidate).use { out ->
                    DngCreator(physical, physicalResult).use { creator ->
                        creator.setOrientation(1)
                        creator.writeImage(out, image)
                    }
                }
                dng = candidate
                dngSha = sha256File(candidate)
            }.onFailure { e ->
                dngError = "${e.javaClass.simpleName}: ${e.message}"
            }

            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_EVIDENCE_v010.json")
            report.writeText(
                buildEvidence(logicalResult, physicalResult, rawEvidence, dng, dngSha, dngError).toString(2),
            )
            capturedDng = dng
            capturedJson = report
            image.close()

            setStatusAny(
                "STAGE 3 CAPTURE PASS · physical 5 · 16320×12288 · 200,540,160 samples · MAX mode · timestamp exact.\n" +
                    "PRIMARY RAW=${rawEvidence.file.name} · bytes=${rawEvidence.fileBytes} · contiguous=${rawEvidence.canonicalContiguous}.\n" +
                    "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE · untouched/native ADC nog NIET geclaimd."
            )
            runOnUiThread {
                saveRawButton.isEnabled = true
                saveDngButton.isEnabled = dng != null
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

    private fun persistOriginalRawBuffer(image: Image, stamp: Long): RawBufferEvidence {
        val plane = image.planes.single()
        val src = plane.buffer.duplicate()
        val fileBytes = src.remaining().toLong()
        val canonicalContiguous = plane.pixelStride == 2 &&
            plane.rowStride == TARGET_W * 2 &&
            fileBytes == TARGET_RAW_BYTES
        val extension = if (canonicalContiguous) "rawsensor" else "rawbuffer"
        val file = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}_v010.$extension")

        FileOutputStream(file).channel.use { channel ->
            while (src.hasRemaining()) {
                val written = channel.write(src)
                require(written > 0) { "RAW buffer write maakte geen voortgang" }
            }
        }
        require(file.length() == fileBytes) { "RAW buffer file size mismatch ${file.length()} != $fileBytes" }

        return RawBufferEvidence(
            file = file,
            fileSha256 = sha256File(file),
            fileBytes = fileBytes,
            canonicalContiguous = canonicalContiguous,
            validSampleSha256 = sha256ValidSampleBytes(image),
            validSampleBytes = TARGET_RAW_BYTES,
            rowStride = plane.rowStride,
            pixelStride = plane.pixelStride,
        )
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
        return md.digest().toHex()
    }

    private fun buildEvidence(
        logicalResult: TotalCaptureResult,
        physicalResult: CaptureResult,
        raw: RawBufferEvidence,
        dng: File?,
        dngSha: String?,
        dngError: String?,
    ): JSONObject {
        val physical = physical5Characteristics
        val black = physical?.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)
        val blackJson = black?.let {
            JSONArray(listOf(
                it.getOffsetForIndex(0, 0), it.getOffsetForIndex(1, 0),
                it.getOffsetForIndex(0, 1), it.getOffsetForIndex(1, 1),
            ))
        }
        return JSONObject()
            .put("schema", "truthraw.fotograaf-camera5-200mp-staged-evidence.v0.10")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE")
            .put("calibrationAuthorityGranted", false)
            .put("scientificWriteback", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("capability", capabilityRoutes?.let { routesJson(it) } ?: JSONObject.NULL)
            .put("preview", JSONObject()
                .put("route", "logical_0_only_no_physical_output_binding")
                .put("requestedZoomRatio", ZOOM_REQUEST)
                .put("lastActivePhysicalId", lastActivePhysicalId ?: JSONObject.NULL)
                .put("previewCreatesEvidence", false))
            .put("captureRoute", JSONObject()
                .put("openedLogicalCameraId", LOGICAL_ID)
                .put("physicalOutputCameraId", PHYSICAL_ID)
                .put("width", TARGET_W)
                .put("height", TARGET_H)
                .put("sampleCount", TARGET_SAMPLES)
                .put("sensorPixelModeRequested", "MAXIMUM_RESOLUTION")
                .put("outputConfigurationSensorPixelModeUsed", "MAXIMUM_RESOLUTION")
                .put("captureResultSensorPixelMode", physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE) ?: JSONObject.NULL)
                .put("reportedPhysicalIds", JSONArray(logicalResult.physicalCameraResults.keys.sorted())))
            .put("cameraCharacteristics", JSONObject()
                .put("whiteLevel", physical?.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL) ?: JSONObject.NULL)
                .put("blackLevelPattern", blackJson ?: JSONObject.NULL)
                .put("cfaArrangement", physical?.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT) ?: JSONObject.NULL)
                .put("lensShadingAppliedToRaw", physical?.get(CameraCharacteristics.SENSOR_INFO_LENS_SHADING_APPLIED) ?: JSONObject.NULL))
            .put("rawOutput", JSONObject()
                .put("format", "RAW_SENSOR")
                .put("fileName", raw.file.name)
                .put("payloadBytes", raw.fileBytes)
                .put("payloadSha256", raw.fileSha256)
                .put("canonicalContiguousRawsensor", raw.canonicalContiguous)
                .put("rowStride", raw.rowStride)
                .put("pixelStride", raw.pixelStride)
                .put("validSampleBytes", raw.validSampleBytes)
                .put("validSampleSha256", raw.validSampleSha256))
            .put("captureResult", JSONObject()
                .put("sensorTimestamp", physicalResult.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("imageTimestampBoundExact", true)
                .put("iso", physicalResult.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", physicalResult.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("focalLengthMm", physicalResult.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("focusDistanceDiopters", physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                .put("oisMode", physicalResult.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE) ?: JSONObject.NULL)
                .put("awbState", physicalResult.get(CaptureResult.CONTROL_AWB_STATE) ?: JSONObject.NULL)
                .put("colorCorrectionGains", physicalResult.get(CaptureResult.COLOR_CORRECTION_GAINS)?.toString() ?: JSONObject.NULL)
                .put("colorCorrectionTransform", physicalResult.get(CaptureResult.COLOR_CORRECTION_TRANSFORM)?.toString() ?: JSONObject.NULL))
            .put("dng", JSONObject()
                .put("role", "AUXILIARY_DERIVED_CONTAINER")
                .put("created", dng != null)
                .put("fileName", dng?.name ?: JSONObject.NULL)
                .put("bytes", dng?.length() ?: JSONObject.NULL)
                .put("sha256", dngSha ?: JSONObject.NULL)
                .put("error", dngError ?: JSONObject.NULL))
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
        capturedRaw = null
        capturedDng = null
        capturedJson = null
        if (::saveRawButton.isInitialized) saveRawButton.isEnabled = false
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
            REQUEST_SAVE_RAW -> capturedRaw
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
        return md.digest().toHex()
    }

    private fun ByteArray.toHex(): String = joinToString("") { "%02x".format(it) }

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
        private const val TARGET_RAW_BYTES = TARGET_SAMPLES * 2L
        private const val ZOOM_REQUEST = 3.7f
        private const val REQUEST_CAMERA = 5700
        private const val REQUEST_SAVE_RAW = 5701
        private const val REQUEST_SAVE_DNG = 5702
        private const val REQUEST_SAVE_JSON = 5703
    }
}

package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.Rect
import android.graphics.RectF
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
import android.hardware.camera2.params.MeteringRectangle
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.Image
import android.media.ImageReader
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.text.InputType
import android.view.MotionEvent
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Spinner
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.Locale
import kotlin.math.max
import kotlin.math.roundToInt

/**
 * TruthRaw FotoGraaf Pro v0.5.
 *
 * Acquisition/metrology only. Runtime discovery does not prove a sensor mode;
 * an admitted capture requires a real RAW_SENSOR Image, exact SENSOR_TIMESTAMP
 * binding and (when requested) the physical TotalCaptureResult. MAX RAW is
 * admitted only when the capture result itself reports MAXIMUM_RESOLUTION.
 */
class FotoGraafProCameraActivity : Activity(), TextureView.SurfaceTextureListener {
    private lateinit var cameraManager: CameraManager
    private lateinit var previewView: TextureView
    private lateinit var routeSpinner: Spinner
    private lateinit var telemetry: TextView
    private lateinit var status: TextView
    private lateinit var captureButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button
    private lateinit var manualExposureBox: CheckBox
    private lateinit var isoInput: EditText
    private lateinit var exposureUsInput: EditText
    private lateinit var evSeek: SeekBar
    private lateinit var evLabel: TextView
    private lateinit var manualFocusBox: CheckBox
    private lateinit var focusSeek: SeekBar
    private lateinit var focusLabel: TextView
    private lateinit var oisBox: CheckBox

    private val cameraThread = HandlerThread("truthraw-fotograaf-pro-camera").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var routes: List<FotoGraafProRoute> = emptyList()
    private var selectedRoute: FotoGraafProRoute? = null
    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var rawReader: ImageReader? = null
    private var previewSize: android.util.Size? = null
    private var physicalRequestKeys: Set<String> = emptySet()
    private var focusMaxDiopters = 0f
    private var evLower = 0
    private var evUpper = 0
    private var evStep = 0.0
    private var previewFrameCount = 0L
    @Volatile private var lastPreviewResult: TotalCaptureResult? = null
    @Volatile private var lastPreviewWallMs = 0L
    private var lastTelemetryUiMs = 0L

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var pendingRoute: FotoGraafProRoute? = null
    private var finalizing = false

    private var capturedDng: File? = null
    private var capturedJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(CameraManager::class.java)
        setContentView(buildUi())
        scanRoutes()
    }

    override fun onResume() {
        super.onResume()
        if (::previewView.isInitialized && previewView.isAvailable && selectedRoute != null && camera == null) {
            openSelectedRoute()
        }
    }

    override fun onPause() {
        closeCamera()
        super.onPause()
    }

    override fun onDestroy() {
        closeCamera()
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            pendingRoute = null
        }
        cameraThread.quitSafely()
        super.onDestroy()
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(10), dp(12), dp(24))
            setBackgroundColor(Color.rgb(10, 12, 15))
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

        root.addView(text("TruthRaw · FotoGraaf Pro", 23f, true))
        root.addView(text(
            "v0.5 · runtime fysieke lensroute · Pro exposure/focus · één sample-exact gekoppelde RAW",
            12f,
            false,
            Color.rgb(180, 187, 198),
        ))
        root.addView(space(8))

        previewView = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraafProCameraActivity
            setBackgroundColor(Color.BLACK)
            setOnTouchListener { _, event ->
                if (event.action == MotionEvent.ACTION_UP) {
                    tapToFocus(event.x, event.y)
                    true
                } else true
            }
        }
        val previewHeight = (resources.displayMetrics.widthPixels * 4L / 3L).toInt().coerceIn(dp(300), dp(560))
        root.addView(previewView, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, previewHeight))
        root.addView(space(7))

        routeSpinner = Spinner(this)
        root.addView(routeSpinner)
        root.addView(space(5))

        telemetry = text("Wacht op Camera2-route…", 12f, true, Color.rgb(222, 227, 235))
        root.addView(telemetry)
        root.addView(space(8))

        manualExposureBox = CheckBox(this).apply {
            text = "M · handmatige ISO + sluitertijd"
            setTextColor(Color.WHITE)
            setOnCheckedChangeListener { _, enabled ->
                isoInput.isEnabled = enabled
                exposureUsInput.isEnabled = enabled
                evSeek.isEnabled = !enabled && evUpper > evLower
                restartPreview()
            }
        }
        root.addView(manualExposureBox)

        val exposureRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        isoInput = numberInput("ISO", false).apply { isEnabled = false; setText("100") }
        exposureUsInput = numberInput("tijd µs", false).apply { isEnabled = false; setText("10000") }
        exposureRow.addView(isoInput, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        exposureRow.addView(exposureUsInput, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        root.addView(exposureRow)
        root.addView(button("ISO/tijd/EV toepassen") { restartPreview() })
        root.addView(space(5))

        evLabel = text("EV: 0", 11f, false, Color.rgb(180, 188, 199))
        root.addView(evLabel)
        evSeek = SeekBar(this).apply {
            max = 0
            isEnabled = false
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                    updateEvLabel()
                }
                override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                override fun onStopTrackingTouch(seekBar: SeekBar?) { restartPreview() }
            })
        }
        root.addView(evSeek)

        manualFocusBox = CheckBox(this).apply {
            text = "MF · AF uit + focusafstand in dioptrie"
            setTextColor(Color.WHITE)
            setOnCheckedChangeListener { _, enabled ->
                focusSeek.isEnabled = enabled && focusMaxDiopters > 0f
                updateFocusLabel()
                restartPreview()
            }
        }
        root.addView(manualFocusBox)
        focusLabel = text("Focus: runtime route nog niet actief", 11f, false, Color.rgb(180, 188, 199))
        root.addView(focusLabel)
        focusSeek = SeekBar(this).apply {
            max = 1000
            isEnabled = false
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) { updateFocusLabel() }
                override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                override fun onStopTrackingTouch(seekBar: SeekBar?) { restartPreview() }
            })
        }
        root.addView(focusSeek)
        root.addView(text("Tik op de preview voor AF/AE-metering. Tap-focus is operationele metering, geen geometrische metrologie.", 10f, false, Color.rgb(145, 153, 164)))

        oisBox = CheckBox(this).apply {
            text = "OIS aanvragen · requested/actual blijven apart"
            setTextColor(Color.WHITE)
            isChecked = true
            setOnCheckedChangeListener { _, _ -> restartPreview() }
        }
        root.addView(oisBox)
        root.addView(space(7))

        captureButton = button("Maak één sealed RAW") { captureRaw() }.apply { isEnabled = false }
        saveDngButton = button("DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("Evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }
        root.addView(captureButton)
        root.addView(space(4))
        root.addView(saveDngButton)
        root.addView(space(4))
        root.addView(saveJsonButton)
        root.addView(space(7))

        root.addView(button("HONOR Camera2 lab / diagnostics") {
            startActivity(Intent(this, FotoGraafDiagnosticBootstrapActivity::class.java))
        })
        root.addView(space(4))
        root.addView(button("Historische v0.4 live route verifier") {
            startActivity(Intent(this, FotoGraafLiveCameraActivity::class.java))
        })
        root.addView(space(4))
        root.addView(button("Routes opnieuw scannen") { scanRoutes() })
        root.addView(space(8))

        status = text("Initialiseren…", 12f, true)
        root.addView(status)
        root.addView(space(8))
        root.addView(text(
            "Authority: APP_VISIBLE_RAW_SENSOR_CFA_CAPTURE_OBSERVATION. Geen vendor request keys worden geschreven. Preview, discovery, DNG-presentatie en UI wijzigen Scientific Master/Dynamic Authority niet.",
            10f,
            false,
            Color.rgb(145, 153, 164),
        ))
        return scroll
    }

    private fun scanRoutes() {
        setStatus("Camera2-topologie en RAW-routes worden runtime gelezen…")
        captureButton.isEnabled = false
        Thread({
            val result = runCatching { FotoGraafProRoutes.scan(cameraManager) }
            runOnUiThread {
                result.onSuccess { discovered ->
                    routes = discovered
                    routeSpinner.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, routes)
                    routeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                            val route = routes.getOrNull(position) ?: return
                            if (selectedRoute != route) {
                                selectedRoute = route
                                configureControls(route)
                                openSelectedRoute()
                            }
                        }
                        override fun onNothingSelected(parent: AdapterView<*>?) = Unit
                    }
                    if (routes.isEmpty()) {
                        selectedRoute = null
                        setStatus("Geen runtime-advertised RAW_SENSOR route gevonden. Fail-closed: niets wordt verzonnen.")
                    } else {
                        selectedRoute = routes.first()
                        configureControls(routes.first())
                        if (previewView.isAvailable) openSelectedRoute()
                        setStatus("${routes.size} runtime RAW-route(s) gevonden. MAX verschijnt uitsluitend uit de maximum-resolution stream map.")
                    }
                }.onFailure { e -> setStatus("Route discovery faalde: ${e.javaClass.simpleName}: ${e.message}") }
            }
        }, "truthraw-pro-route-scan").start()
    }

    private fun configureControls(route: FotoGraafProRoute) {
        val c = runCatching { FotoGraafProRoutes.effectiveCharacteristics(cameraManager, route) }.getOrNull() ?: return
        focusMaxDiopters = c.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f
        manualFocusBox.isEnabled = focusMaxDiopters > 0f
        if (!manualFocusBox.isEnabled) manualFocusBox.isChecked = false
        focusSeek.isEnabled = manualFocusBox.isChecked && focusMaxDiopters > 0f
        updateFocusLabel()

        val evRange = c.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE)
        val step = c.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP)
        evLower = evRange?.lower ?: 0
        evUpper = evRange?.upper ?: 0
        evStep = step?.toDouble() ?: 0.0
        evSeek.max = max(0, evUpper - evLower)
        evSeek.progress = (0 - evLower).coerceIn(0, evSeek.max)
        evSeek.isEnabled = !manualExposureBox.isChecked && evUpper > evLower
        updateEvLabel()

        val isoRange = c.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
        isoInput.hint = isoRange?.let { "ISO ${it.lower}–${it.upper}" } ?: "ISO"
        val exposureRange = c.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
        exposureUsInput.hint = exposureRange?.let { "µs ${it.lower / 1000}–${it.upper / 1000}" } ?: "tijd µs"

        val ois = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
        oisBox.isEnabled = ois.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)
        if (!oisBox.isEnabled) oisBox.isChecked = false
    }

    private fun openSelectedRoute() {
        val route = selectedRoute ?: return
        if (!previewView.isAvailable) return
        if (checkSelfPermission(android.Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            setStatus("CAMERA-permission ontbreekt. Open via de Suite/permission gate.")
            return
        }
        closeCamera()
        clearOutputs()
        previewFrameCount = 0L
        lastPreviewResult = null
        lastPreviewWallMs = 0L
        captureButton.isEnabled = false

        val effective = runCatching { FotoGraafProRoutes.effectiveCharacteristics(cameraManager, route) }.getOrElse {
            setStatus("Characteristics niet leesbaar: ${it.message}")
            return
        }
        val logical = runCatching { cameraManager.getCameraCharacteristics(route.logicalCameraId) }.getOrNull()
        physicalRequestKeys = logical?.availablePhysicalCameraRequestKeys?.map { it.name }?.toSet().orEmpty()
        val chosenPreview = choosePreviewSize(effective)
        previewSize = chosenPreview
        val texture = previewView.surfaceTexture ?: return
        texture.setDefaultBufferSize(chosenPreview.width, chosenPreview.height)
        configureTransform(previewView.width, previewView.height, chosenPreview)
        previewSurface = Surface(texture)

        rawReader = ImageReader.newInstance(route.rawSize.width, route.rawSize.height, ImageFormat.RAW_SENSOR, 2).also { reader ->
            reader.setOnImageAvailableListener({ r ->
                val image = runCatching { r.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
                synchronized(pairLock) {
                    pendingImage?.close()
                    pendingImage = image
                }
                finalizeIfPaired()
            }, cameraHandler)
        }

        setStatus("Open ${route.label}")
        try {
            cameraManager.openCamera(route.logicalCameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    camera = device
                    createSession(device, route)
                }
                override fun onDisconnected(device: CameraDevice) {
                    setStatusAny("Camera disconnected: ${route.logicalCameraId}")
                    device.close()
                    closeCamera()
                }
                override fun onError(device: CameraDevice, error: Int) {
                    setStatusAny("Camera open error=$error · ${route.logicalCameraId}")
                    device.close()
                    closeCamera()
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatus("Camera open faalde: ${e.javaClass.simpleName}: ${e.message}")
            closeCamera()
        }
    }

    private fun createSession(device: CameraDevice, route: FotoGraafProRoute) {
        val preview = previewSurface ?: return
        val reader = rawReader ?: return
        val previewOutput = OutputConfiguration(preview)
        val rawOutput = OutputConfiguration(reader.surface)
        route.physicalCameraId?.let { id ->
            try {
                previewOutput.setPhysicalCameraId(id)
                rawOutput.setPhysicalCameraId(id)
            } catch (e: Throwable) {
                setStatusAny("FAIL CLOSED: physical outputbinding $id faalde: ${e.message}")
                closeCamera()
                return
            }
        }
        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(previewOutput, rawOutput),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    startRepeating(device, s, route)
                }
                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("Sessionconfiguratie faalde voor ${route.label}. Geen routebewijs.")
                    runOnUiThread { captureButton.isEnabled = false }
                }
            },
        )
        runCatching { device.createCaptureSession(config) }
            .onFailure { setStatusAny("createCaptureSession faalde: ${it.javaClass.simpleName}: ${it.message}") }
    }

    private fun startRepeating(device: CameraDevice, s: CameraCaptureSession, route: FotoGraafProRoute) {
        val preview = previewSurface ?: return
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            b.addTarget(preview)
            applyControls(b, route)
            s.setRepeatingRequest(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    previewFrameCount++
                    lastPreviewResult = result
                    lastPreviewWallMs = System.currentTimeMillis()
                    val now = System.currentTimeMillis()
                    if (now - lastTelemetryUiMs > 180L) {
                        lastTelemetryUiMs = now
                        val line = renderTelemetry(result, route)
                        runOnUiThread {
                            telemetry.text = line
                            captureButton.isEnabled = true
                        }
                    }
                }
            }, cameraHandler)
            setStatusAny("Live preview actief. Route=${route.routeClass}. Tik voor AF of gebruik MF; capture blijft single-frame.")
        } catch (e: Throwable) {
            setStatusAny("Repeating preview faalde: ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { captureButton.isEnabled = false }
        }
    }

    private fun applyControls(b: CaptureRequest.Builder, route: FotoGraafProRoute) {
        setRouteKey(b, route, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
        if (route.maximumResolution) {
            b.set(CaptureRequest.SENSOR_PIXEL_MODE, CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        }

        val c = runCatching { FotoGraafProRoutes.effectiveCharacteristics(cameraManager, route) }.getOrNull()
        if (manualExposureBox.isChecked) {
            setRouteKey(b, route, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_OFF)
            val isoRange = c?.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE)
            val exposureRange = c?.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE)
            val maxFrame = c?.get(CameraCharacteristics.SENSOR_INFO_MAX_FRAME_DURATION)
            val requestedIso = isoInput.text.toString().toIntOrNull() ?: isoRange?.lower ?: 100
            val requestedExposure = (exposureUsInput.text.toString().toLongOrNull() ?: 10_000L) * 1000L
            val iso = isoRange?.let { requestedIso.coerceIn(it.lower, it.upper) } ?: requestedIso
            val exposure = exposureRange?.let { requestedExposure.coerceIn(it.lower, it.upper) } ?: requestedExposure
            setRouteKey(b, route, CaptureRequest.SENSOR_SENSITIVITY, iso)
            setRouteKey(b, route, CaptureRequest.SENSOR_EXPOSURE_TIME, exposure)
            if (maxFrame != null) {
                setRouteKey(b, route, CaptureRequest.SENSOR_FRAME_DURATION, exposure.coerceAtMost(maxFrame))
            }
        } else {
            setRouteKey(b, route, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)
            if (evUpper > evLower) {
                setRouteKey(b, route, CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, selectedEvIndex())
            }
        }

        val afModes = c?.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        if (manualFocusBox.isChecked && focusMaxDiopters > 0f) {
            setRouteKey(b, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
            setRouteKey(b, route, CaptureRequest.LENS_FOCUS_DISTANCE, selectedFocusDiopters())
        } else if (afModes.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
            setRouteKey(b, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
        } else if (afModes.contains(CameraMetadata.CONTROL_AF_MODE_AUTO)) {
            setRouteKey(b, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_AUTO)
        }

        val ois = c?.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
        val requestedOis = if (oisBox.isChecked && ois.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)) {
            CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON
        } else CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF
        if (ois.contains(requestedOis)) setRouteKey(b, route, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, requestedOis)
    }

    private fun <T> setRouteKey(b: CaptureRequest.Builder, route: FotoGraafProRoute, key: CaptureRequest.Key<T>, value: T) {
        val physical = route.physicalCameraId
        if (physical != null && key.name in physicalRequestKeys) {
            runCatching { b.setPhysicalCameraKey(key, value, physical) }.onFailure { b.set(key, value) }
        } else b.set(key, value)
    }

    private fun tapToFocus(x: Float, y: Float) {
        val route = selectedRoute ?: return
        val device = camera ?: return
        val s = session ?: return
        val preview = previewSurface ?: return
        if (manualFocusBox.isChecked) {
            setStatus("Tap-focus genegeerd omdat MF actief is.")
            return
        }
        val c = runCatching { FotoGraafProRoutes.effectiveCharacteristics(cameraManager, route) }.getOrNull() ?: return
        val active = c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE) ?: return
        val maxAf = c.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF) ?: 0
        val maxAe = c.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE) ?: 0
        if (maxAf <= 0 && maxAe <= 0) {
            setStatus("Deze route adverteert geen AF/AE metering regions.")
            return
        }
        val nx = (x / previewView.width.toFloat()).coerceIn(0f, 1f)
        val ny = (y / previewView.height.toFloat()).coerceIn(0f, 1f)
        val sx = (active.left + nx * active.width()).roundToInt()
        val sy = (active.top + ny * active.height()).roundToInt()
        val rw = max(32, active.width() / 12)
        val rh = max(32, active.height() / 12)
        val rect = Rect(
            (sx - rw / 2).coerceIn(active.left, active.right - 1),
            (sy - rh / 2).coerceIn(active.top, active.bottom - 1),
            (sx + rw / 2).coerceIn(active.left + 1, active.right),
            (sy + rh / 2).coerceIn(active.top + 1, active.bottom),
        )
        val region = arrayOf(MeteringRectangle(rect, MeteringRectangle.METERING_WEIGHT_MAX))
        cameraHandler.post {
            try {
                val b = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
                b.addTarget(preview)
                applyControls(b, route)
                setRouteKey(b, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_AUTO)
                if (maxAf > 0) setRouteKey(b, route, CaptureRequest.CONTROL_AF_REGIONS, region)
                if (maxAe > 0) setRouteKey(b, route, CaptureRequest.CONTROL_AE_REGIONS, region)
                setRouteKey(b, route, CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START)
                s.capture(b.build(), object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                        setStatusAny("Tap-focus request voltooid; repeating AF/AE wordt hervat.")
                        startRepeating(device, s, route)
                    }
                }, cameraHandler)
            } catch (e: Throwable) {
                setStatusAny("Tap-focus faalde: ${e.javaClass.simpleName}: ${e.message}")
            }
        }
    }

    private fun captureRaw() {
        val route = selectedRoute ?: return
        val device = camera ?: return
        val s = session ?: return
        val reader = rawReader ?: return
        if (lastPreviewResult == null) {
            setStatus("Wacht op minimaal één live CaptureResult.")
            return
        }
        clearOutputs()
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            pendingRoute = route
            finalizing = false
        }
        captureButton.isEnabled = false
        routeSpinner.isEnabled = false
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            b.addTarget(reader.surface)
            applyControls(b, route)
            val c = FotoGraafProRoutes.effectiveCharacteristics(cameraManager, route)
            val nr = c.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) setRouteKey(b, route, CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
            val edge = c.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) setRouteKey(b, route, CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)

            setStatus("Single RAW verstuurd · exact Image.timestamp ↔ SENSOR_TIMESTAMP + physical/MAX result wordt gecontroleerd…")
            s.capture(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }
                override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: android.hardware.camera2.CaptureFailure) {
                    setStatusAny("RAW capture faalde: reason=${failure.reason}")
                    runOnUiThread { captureButton.isEnabled = true; routeSpinner.isEnabled = true }
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatus("RAW request faalde: ${e.javaClass.simpleName}: ${e.message}")
            captureButton.isEnabled = true
            routeSpinner.isEnabled = true
        }
    }

    private fun finalizeIfPaired() {
        val image: Image
        val result: TotalCaptureResult
        val route: FotoGraafProRoute
        synchronized(pairLock) {
            if (finalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            route = pendingRoute ?: return
            val sensorTs = result.get(CaptureResult.SENSOR_TIMESTAMP)
            if (sensorTs == null || sensorTs != image.timestamp) {
                finalizing = true
                pendingImage = null
                pendingResult = null
                pendingRoute = null
                image.close()
                setStatusAny("FAIL CLOSED: RAW timestamp=${image.timestamp} != SENSOR_TIMESTAMP=$sensorTs")
                runOnUiThread { captureButton.isEnabled = true; routeSpinner.isEnabled = true }
                return
            }
            finalizing = true
            pendingImage = null
            pendingResult = null
            pendingRoute = null
        }
        cameraHandler.post { finalizeCapture(route, image, result) }
    }

    private fun finalizeCapture(route: FotoGraafProRoute, image: Image, result: TotalCaptureResult) {
        try {
            if (image.width != route.rawSize.width || image.height != route.rawSize.height) {
                throw IllegalStateException("RAW dimensions ${image.width}x${image.height} != advertised ${route.rawSize.width}x${route.rawSize.height}")
            }
            val physicalResults = result.physicalCameraResults
            val effective: CaptureResult = route.physicalCameraId?.let { id ->
                physicalResults[id] ?: throw IllegalStateException("requested physical $id missing; reported=${physicalResults.keys}")
            } ?: result
            val pixelMode = effective.get(CaptureResult.SENSOR_PIXEL_MODE)
            if (route.maximumResolution && pixelMode != CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION) {
                throw IllegalStateException("MAX RAW requested but CaptureResult SENSOR_PIXEL_MODE=$pixelMode")
            }
            val effectiveId = route.physicalCameraId ?: route.logicalCameraId
            val characteristics = cameraManager.getCameraCharacteristics(effectiveId)
            val stamp = System.currentTimeMillis()
            val dng = File(cacheDir, "TRUTHRAW_${stamp}_${route.rawSize.width}x${route.rawSize.height}_pro_v05.dng")
            FileOutputStream(dng).use { out ->
                DngCreator(characteristics, effective).use { creator ->
                    creator.setOrientation(1)
                    creator.writeImage(out, image)
                }
            }
            image.close()
            val sha = sha256(dng)
            val report = File(cacheDir, "TRUTHRAW_${stamp}_fotograaf_pro_evidence_v0_5.json")
            report.writeText(buildEvidence(route, result, effective, dng, sha, pixelMode).toString(2))
            capturedDng = dng
            capturedJson = report
            setStatusAny(
                "CAPTURE PASS · ${if (route.maximumResolution) "MAX bevestigd" else "RAW"} · physical=${route.physicalCameraId ?: "logical"} · ${dng.length()} B · SHA=${sha.take(16)}…",
            )
            runOnUiThread {
                captureButton.isEnabled = true
                routeSpinner.isEnabled = true
                saveDngButton.isEnabled = true
                saveJsonButton.isEnabled = true
            }
        } catch (e: Throwable) {
            runCatching { image.close() }
            setStatusAny("Capture finalization FAIL CLOSED: ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { captureButton.isEnabled = true; routeSpinner.isEnabled = true }
        }
    }

    private fun buildEvidence(
        route: FotoGraafProRoute,
        logicalResult: TotalCaptureResult,
        effective: CaptureResult,
        dng: File,
        sha: String,
        pixelMode: Int?,
    ): JSONObject {
        val physicalIds = logicalResult.physicalCameraResults.keys.sorted()
        return JSONObject()
            .put("schema", "truthraw.fotograaf-pro-route-evidence.v0.5")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "APP_VISIBLE_RAW_SENSOR_CFA_CAPTURE_OBSERVATION")
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("scientificWriteback", false)
            .put("vendorRequestsWritten", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("route", JSONObject()
                .put("routeClass", route.routeClass)
                .put("logicalCameraId", route.logicalCameraId)
                .put("requestedPhysicalCameraId", route.physicalCameraId ?: JSONObject.NULL)
                .put("confirmedPhysicalResultCameraIds", JSONArray(physicalIds))
                .put("advertisedRawWidth", route.rawSize.width)
                .put("advertisedRawHeight", route.rawSize.height)
                .put("maximumResolutionRequested", route.maximumResolution)
                .put("captureResultSensorPixelMode", pixelMode ?: JSONObject.NULL)
                .put("maximumResolutionCaptureConfirmed", route.maximumResolution && pixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION))
            .put("captureResult", JSONObject()
                .put("sensorTimestamp", effective.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("iso", effective.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", effective.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("focusDistanceDiopters", effective.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                .put("focalLengthMm", effective.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("oisActual", effective.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE) ?: JSONObject.NULL)
                .put("afState", effective.get(CaptureResult.CONTROL_AF_STATE) ?: JSONObject.NULL)
                .put("aeState", effective.get(CaptureResult.CONTROL_AE_STATE) ?: JSONObject.NULL))
            .put("requestedControls", JSONObject()
                .put("manualExposure", manualExposureBox.isChecked)
                .put("evCompensationIndex", if (!manualExposureBox.isChecked) selectedEvIndex() else JSONObject.NULL)
                .put("manualFocus", manualFocusBox.isChecked)
                .put("focusDistanceDiopters", if (manualFocusBox.isChecked) selectedFocusDiopters() else JSONObject.NULL)
                .put("oisRequested", oisBox.isChecked))
            .put("dng", JSONObject()
                .put("bytes", dng.length())
                .put("sha256", sha)
                .put("orientationRequested", 1))
            .put("boundary", "APP_VISIBLE_RAW_SENSOR_PLANE_NOT_UNTOUCHED_ADC_PROOF")
    }

    private fun renderTelemetry(result: TotalCaptureResult, route: FotoGraafProRoute): String {
        val physical = route.physicalCameraId?.let { result.physicalCameraResults[it] }
        val effective: CaptureResult = physical ?: result
        val activePhysical = result.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID)
        val iso = effective.get(CaptureResult.SENSOR_SENSITIVITY)
        val exp = effective.get(CaptureResult.SENSOR_EXPOSURE_TIME)
        val af = effective.get(CaptureResult.CONTROL_AF_STATE)
        val ae = effective.get(CaptureResult.CONTROL_AE_STATE)
        val focus = effective.get(CaptureResult.LENS_FOCUS_DISTANCE)
        val focal = effective.get(CaptureResult.LENS_FOCAL_LENGTH)
        val ois = effective.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE)
        val pixelMode = effective.get(CaptureResult.SENSOR_PIXEL_MODE)
        val age = (System.currentTimeMillis() - lastPreviewWallMs).coerceAtLeast(0L)
        val expMs = exp?.div(1_000_000.0)
        return buildString {
            append("route=${route.physicalCameraId ?: route.logicalCameraId} · activePhysical=${activePhysical ?: "?"} · frame=$previewFrameCount · age=${age}ms")
            append("\nISO=${iso ?: "?"} · t=${expMs?.let { String.format(Locale.ROOT, "%.3f ms", it) } ?: "?"} · EV=${String.format(Locale.ROOT, "%+.2f", selectedEvIndex() * evStep)}")
            append("\nAF=$af · AE=$ae · focus=${focus ?: "?"}D · focal=${focal ?: "?"}mm · OIS=$ois · pixelMode=${pixelMode ?: "?"}")
            append("\n${if (route.maximumResolution) "MAX candidate — capture-time bevestiging verplicht" else "standard RAW"}")
        }
    }

    private fun restartPreview() {
        val route = selectedRoute ?: return
        val device = camera ?: return
        val s = session ?: return
        cameraHandler.post { startRepeating(device, s, route) }
    }

    private fun selectedEvIndex(): Int = (evLower + evSeek.progress).coerceIn(evLower, evUpper)

    private fun updateEvLabel() {
        val idx = selectedEvIndex()
        evLabel.text = "EV: index=$idx · ${String.format(Locale.ROOT, "%+.2f EV", idx * evStep)}"
    }

    private fun selectedFocusDiopters(): Float = focusMaxDiopters * (focusSeek.progress / 1000f)

    private fun updateFocusLabel() {
        focusLabel.text = if (manualFocusBox.isChecked && focusMaxDiopters > 0f) {
            "MF: ${String.format(Locale.ROOT, "%.3f D", selectedFocusDiopters())} · bereik 0…${String.format(Locale.ROOT, "%.3f", focusMaxDiopters)} D"
        } else "AF: continuous/tap waar ondersteund · MF max=${String.format(Locale.ROOT, "%.3f", focusMaxDiopters)} D"
    }

    private fun choosePreviewSize(c: CameraCharacteristics): android.util.Size {
        val sizes = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)?.toList().orEmpty()
        if (sizes.isEmpty()) return android.util.Size(1280, 720)
        return sizes
            .filter { it.width <= 1920 && it.height <= 1440 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.minBy { kotlin.math.abs(it.width.toLong() * it.height - 1920L * 1080L) }
    }

    private fun configureTransform(viewWidth: Int, viewHeight: Int, size: android.util.Size) {
        if (viewWidth <= 0 || viewHeight <= 0) return
        val matrix = Matrix()
        val viewRect = RectF(0f, 0f, viewWidth.toFloat(), viewHeight.toFloat())
        val bufferRect = RectF(0f, 0f, size.height.toFloat(), size.width.toFloat())
        val cx = viewRect.centerX()
        val cy = viewRect.centerY()
        bufferRect.offset(cx - bufferRect.centerX(), cy - bufferRect.centerY())
        matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL)
        val scale = max(viewHeight.toFloat() / size.height, viewWidth.toFloat() / size.width)
        matrix.postScale(scale, scale, cx, cy)
        val rotation = when (display?.rotation ?: Surface.ROTATION_0) {
            Surface.ROTATION_90 -> 90f
            Surface.ROTATION_180 -> 180f
            Surface.ROTATION_270 -> 270f
            else -> 0f
        }
        matrix.postRotate(rotation, cx, cy)
        previewView.setTransform(matrix)
    }

    private fun closeCamera() {
        runCatching { session?.close() }
        session = null
        runCatching { camera?.close() }
        camera = null
        runCatching { rawReader?.close() }
        rawReader = null
        runCatching { previewSurface?.release() }
        previewSurface = null
    }

    private fun clearOutputs() {
        capturedDng = null
        capturedJson = null
        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false
        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false
    }

    private fun saveFile(file: File?, mime: String, requestCode: Int) {
        if (file == null || !file.exists()) return
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime
            putExtra(Intent.EXTRA_TITLE, file.name)
        }
        startActivityForResult(intent, requestCode)
    }

    @Deprecated("Deprecated in Activity API; retained for minSdk31 document export compatibility")
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
            contentResolver.openOutputStream(uri)?.use { out -> FileInputStream(source).use { it.copyTo(out) } }
                ?: error("geen output stream")
        }.onSuccess { setStatus("${source.name} opgeslagen; scientific authority ongewijzigd.") }
            .onFailure { setStatus("Opslaan faalde: ${it.message}") }
    }

    private fun sha256(file: File): String {
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

    private fun numberInput(label: String, decimal: Boolean): EditText = EditText(this).apply {
        hint = label
        setTextColor(Color.WHITE)
        setHintTextColor(Color.GRAY)
        inputType = if (decimal) InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_DECIMAL else InputType.TYPE_CLASS_NUMBER
        setPadding(dp(8), dp(4), dp(8), dp(4))
    }

    private fun button(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        isAllCaps = false
        setOnClickListener { action() }
    }

    private fun text(value: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(color)
        if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
    }

    private fun space(px: Int): View = View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(px)) }
    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()
    private fun setStatus(value: String) { if (::status.isInitialized) status.text = value }
    private fun setStatusAny(value: String) { runOnUiThread { setStatus(value) } }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) { openSelectedRoute() }
    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        previewSize?.let { configureTransform(width, height, it) }
    }
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean { closeCamera(); return true }
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit

    companion object {
        private const val REQUEST_SAVE_DNG = 5501
        private const val REQUEST_SAVE_JSON = 5502
    }
}

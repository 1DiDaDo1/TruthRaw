package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.RectF
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraAccessException
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
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.CheckBox
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
import kotlin.math.abs

/**
 * FotoGraaf live acquisition camera v0.4.
 *
 * This remains an acquisition/metrology layer. The repeating preview exists to
 * let AF/AE/OIS settle and to make requested-vs-actual camera state visible
 * before a single RAW is admitted. It does not gain scientific authority over
 * the sealed CFA evidence or TruthRaw Scientific Master.
 */
class FotoGraafLiveCameraActivity : Activity(), TextureView.SurfaceTextureListener {

    private lateinit var cameraManager: CameraManager
    private lateinit var previewView: TextureView
    private lateinit var routeSpinner: Spinner
    private lateinit var telemetryView: TextView
    private lateinit var statusView: TextView
    private lateinit var captureButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button
    private lateinit var saveLabButton: Button
    private lateinit var manualFocusBox: CheckBox
    private lateinit var focusSeek: SeekBar
    private lateinit var focusValue: TextView
    private lateinit var oisBox: CheckBox

    private val cameraThread = HandlerThread("truthraw-fotograaf-live-camera").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var routes: List<HonorRawRoute> = emptyList()
    private var selectedRoute: HonorRawRoute? = null
    private var cameraDevice: CameraDevice? = null
    private var cameraSession: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var imageReader: ImageReader? = null
    private var previewRequestBuilder: CaptureRequest.Builder? = null
    private var previewSize: android.util.Size? = null
    private var focusMaxDiopters = 0f
    private var physicalRequestKeyNames: Set<String> = emptySet()

    private var previewFrameCount = 0L
    @Volatile private var lastPreviewResult: TotalCaptureResult? = null
    @Volatile private var lastPreviewResultWallMs = 0L
    private var telemetryLastUiMs = 0L

    private val pairingLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var captureFinalizing = false
    private var preCaptureSnapshot: PreviewSnapshot? = null

    private var capturedDng: File? = null
    private var capturedJson: File? = null
    private var capturedLabJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(CameraManager::class.java)
        setContentView(buildUi())
        scanRuntime()
    }

    override fun onResume() {
        super.onResume()
        if (::previewView.isInitialized && previewView.isAvailable && selectedRoute != null && cameraDevice == null) {
            openPreviewForSelectedRoute()
        }
    }

    override fun onPause() {
        closeCameraResources()
        super.onPause()
    }

    override fun onDestroy() {
        closeCameraResources()
        synchronized(pairingLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
        }
        cameraThread.quitSafely()
        super.onDestroy()
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(12), dp(10), dp(12), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 17))
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

        root.addView(text("FotoGraaf · Live RAW Camera", 22f, true))
        root.addView(text(
            "v0.4 · live physical-camera preview + 3A/OIS readback · single-frame RAW evidence",
            12f,
            false,
            Color.rgb(178, 185, 196),
        ))
        root.addView(space(8))

        previewView = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraafLiveCameraActivity
            setBackgroundColor(Color.BLACK)
        }
        val previewHeight = (resources.displayMetrics.widthPixels * 4L / 3L).toInt().coerceIn(dp(300), dp(560))
        root.addView(previewView, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, previewHeight))
        root.addView(space(8))

        routeSpinner = Spinner(this)
        root.addView(routeSpinner, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        root.addView(space(6))

        telemetryView = text("Preview wacht op route…", 12f, true, Color.rgb(220, 225, 232))
        root.addView(telemetryView)
        root.addView(space(6))

        manualFocusBox = CheckBox(this).apply {
            text = "MF · AF OFF + physical LENS_FOCUS_DISTANCE waar ondersteund"
            setTextColor(Color.WHITE)
            setOnCheckedChangeListener { _, enabled ->
                focusSeek.isEnabled = enabled && focusMaxDiopters > 0f
                updateFocusLabel()
                restartRepeatingRequest()
            }
        }
        root.addView(manualFocusBox)

        focusValue = text("MF: route nog niet actief", 11f, false, Color.rgb(180, 186, 196))
        root.addView(focusValue)
        focusSeek = SeekBar(this).apply {
            max = 1000
            isEnabled = false
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                    updateFocusLabel()
                }
                override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                override fun onStopTrackingTouch(seekBar: SeekBar?) {
                    restartRepeatingRequest()
                }
            })
        }
        root.addView(focusSeek)

        oisBox = CheckBox(this).apply {
            text = "OIS aanvragen · requested en actual blijven gescheiden"
            setTextColor(Color.WHITE)
            isChecked = true
            setOnCheckedChangeListener { _, _ -> restartRepeatingRequest() }
        }
        root.addView(oisBox)
        root.addView(space(6))

        captureButton = button("Maak één sealed RAW") { captureSingleRaw() }.apply { isEnabled = false }
        root.addView(captureButton)
        root.addView(space(6))

        saveDngButton = button("DNG opslaan") { launchSave(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("Compact evidence JSON opslaan") { launchSave(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }
        saveLabButton = button("Volledige HONOR Lab Dump opslaan") { launchSave(capturedLabJson, "application/json", REQUEST_SAVE_LAB) }.apply { isEnabled = false }
        root.addView(saveDngButton)
        root.addView(space(4))
        root.addView(saveJsonButton)
        root.addView(space(4))
        root.addView(saveLabButton)
        root.addView(space(6))

        root.addView(button("Camera-routes opnieuw scannen") { scanRuntime() })
        root.addView(space(4))
        root.addView(button("Open TruthRaw processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space(8))

        statusView = text("Initialiseren…", 12f, true)
        root.addView(statusView)
        root.addView(space(8))
        root.addView(text(
            "Authority: CAMERA2_ACQUISITION_OBSERVATION_ONLY. Live preview, AF/AE/OIS en HONOR metadata veranderen geen evidence count.",
            10f,
            false,
            Color.rgb(145, 152, 162),
        ))
        return scroll
    }

    private fun scanRuntime() {
        status("Camera2/HONOR routes worden gelezen…")
        captureButton.isEnabled = false
        Thread({
            val result = runCatching { HonorCameraProbe.scan(cameraManager) }
            runOnUiThread {
                result.onSuccess { report ->
                    routes = report.routes
                    routeSpinner.adapter = ArrayAdapter(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        routes,
                    )
                    routeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                            val route = routes.getOrNull(position) ?: return
                            if (selectedRoute != route) {
                                selectedRoute = route
                                configureControlsForRoute(route)
                                openPreviewForSelectedRoute()
                            }
                        }
                        override fun onNothingSelected(parent: AdapterView<*>?) = Unit
                    }
                    if (routes.isNotEmpty()) {
                        selectedRoute = routes.first()
                        configureControlsForRoute(routes.first())
                        if (previewView.isAvailable) openPreviewForSelectedRoute()
                    }
                    status(
                        "Discovery: rawIDs=${report.rawCameraIds} · physical=${report.discoveredPhysicalIds} · " +
                            "${routes.size} RAW-route(s). Preview opent de geselecteerde route.",
                    )
                }.onFailure { error ->
                    status("Discovery faalde: ${error.javaClass.simpleName}: ${error.message}")
                }
            }
        }, "truthraw-fotograaf-v04-discovery").start()
    }

    private fun configureControlsForRoute(route: HonorRawRoute) {
        val c = effectiveCharacteristics(route)
        focusMaxDiopters = c?.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f
        manualFocusBox.isEnabled = focusMaxDiopters > 0f
        if (focusMaxDiopters <= 0f) manualFocusBox.isChecked = false
        focusSeek.isEnabled = manualFocusBox.isChecked && focusMaxDiopters > 0f

        val oisModes = c?.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
        oisBox.isEnabled = oisModes.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)
        if (!oisBox.isEnabled) oisBox.isChecked = false
        updateFocusLabel()
    }

    private fun openPreviewForSelectedRoute() {
        val route = selectedRoute ?: return
        if (!previewView.isAvailable) return
        if (checkSelfPermission(android.Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            status("CAMERA permission ontbreekt; ga terug via de FotoGraaf permission gate.")
            return
        }

        closeCameraResources()
        resetCaptureOutputs()
        previewFrameCount = 0
        lastPreviewResult = null
        lastPreviewResultWallMs = 0L
        captureButton.isEnabled = false

        val effective = effectiveCharacteristics(route) ?: run {
            status("Geen characteristics voor ${route.label}")
            return
        }
        val logical = runCatching { cameraManager.getCameraCharacteristics(route.logicalCameraId) }.getOrNull()
        physicalRequestKeyNames = logical?.availablePhysicalCameraRequestKeys?.map { it.name }?.toSet().orEmpty()

        val chosenPreview = choosePreviewSize(effective)
        previewSize = chosenPreview
        val texture = previewView.surfaceTexture ?: return
        texture.setDefaultBufferSize(chosenPreview.width, chosenPreview.height)
        configureTransform(previewView.width, previewView.height, chosenPreview)
        val surface = Surface(texture)
        previewSurface = surface

        val rawReader = ImageReader.newInstance(
            route.rawSize.width,
            route.rawSize.height,
            ImageFormat.RAW_SENSOR,
            2,
        )
        imageReader = rawReader
        rawReader.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
            synchronized(pairingLock) {
                pendingImage?.close()
                pendingImage = image
            }
            tryFinalizeCapturePair()
        }, cameraHandler)

        status("Camera openen: ${route.label} · preview ${chosenPreview.width}×${chosenPreview.height}")
        try {
            cameraManager.openCamera(route.logicalCameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    cameraDevice = camera
                    createLiveSession(camera, route, surface, rawReader)
                }

                override fun onDisconnected(camera: CameraDevice) {
                    statusFromAnyThread("Camera disconnected: ${route.logicalCameraId}")
                    camera.close()
                    closeCameraResources()
                }

                override fun onError(camera: CameraDevice, error: Int) {
                    statusFromAnyThread("Camera open error=$error voor ${route.logicalCameraId}")
                    camera.close()
                    closeCameraResources()
                }
            }, cameraHandler)
        } catch (error: Throwable) {
            status("Open preview faalde: ${error.javaClass.simpleName}: ${error.message}")
            closeCameraResources()
        }
    }

    private fun createLiveSession(
        camera: CameraDevice,
        route: HonorRawRoute,
        preview: Surface,
        rawReader: ImageReader,
    ) {
        val previewOutput = OutputConfiguration(preview)
        val rawOutput = OutputConfiguration(rawReader.surface)
        route.physicalCameraId?.let { physicalId ->
            try {
                previewOutput.setPhysicalCameraId(physicalId)
                rawOutput.setPhysicalCameraId(physicalId)
            } catch (error: Throwable) {
                statusFromAnyThread(
                    "FAIL CLOSED: physical preview/RAW output kon niet aan $physicalId worden gebonden: ${error.message}",
                )
                closeCameraResources()
                return
            }
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(previewOutput, rawOutput),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    cameraSession = session
                    startRepeatingPreview(camera, session, route, preview)
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    status("Live preview session configuratie faalde voor ${route.label}")
                    captureButton.isEnabled = false
                    closeCameraResources()
                }
            },
        )
        try {
            camera.createCaptureSession(config)
        } catch (error: Throwable) {
            statusFromAnyThread("Live session faalde: ${error.javaClass.simpleName}: ${error.message}")
            closeCameraResources()
        }
    }

    private fun startRepeatingPreview(
        camera: CameraDevice,
        session: CameraCaptureSession,
        route: HonorRawRoute,
        preview: Surface,
    ) {
        try {
            val builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            builder.addTarget(preview)
            applyPreviewAndCaptureControls(builder, route)
            previewRequestBuilder = builder
            session.setRepeatingRequest(
                builder.build(),
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        previewFrameCount++
                        lastPreviewResult = result
                        lastPreviewResultWallMs = System.currentTimeMillis()
                        val now = System.currentTimeMillis()
                        if (now - telemetryLastUiMs >= 180L) {
                            telemetryLastUiMs = now
                            val snapshot = buildPreviewSnapshot(result, route)
                            runOnUiThread {
                                telemetryView.text = renderTelemetry(snapshot)
                                captureButton.isEnabled = true
                            }
                        }
                    }
                },
                cameraHandler,
            )
            statusFromAnyThread("Live preview actief · wacht tot AF/AE stabiel zijn voor de beste single-frame RAW.")
        } catch (error: Throwable) {
            statusFromAnyThread("Repeating preview faalde: ${error.javaClass.simpleName}: ${error.message}")
            runOnUiThread { captureButton.isEnabled = false }
        }
    }

    private fun restartRepeatingRequest() {
        val route = selectedRoute ?: return
        val camera = cameraDevice ?: return
        val session = cameraSession ?: return
        val preview = previewSurface ?: return
        cameraHandler.post {
            startRepeatingPreview(camera, session, route, preview)
        }
    }

    private fun applyPreviewAndCaptureControls(
        builder: CaptureRequest.Builder,
        route: HonorRawRoute,
    ) {
        setRouteKey(builder, route, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
        setRouteKey(builder, route, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)

        val c = effectiveCharacteristics(route)
        val afModes = c?.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        if (manualFocusBox.isChecked && focusMaxDiopters > 0f) {
            setRouteKey(builder, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
            setRouteKey(builder, route, CaptureRequest.LENS_FOCUS_DISTANCE, selectedFocusDiopters())
        } else if (afModes.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
            setRouteKey(
                builder,
                route,
                CaptureRequest.CONTROL_AF_MODE,
                CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE,
            )
        } else if (afModes.contains(CameraMetadata.CONTROL_AF_MODE_AUTO)) {
            setRouteKey(builder, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_AUTO)
        } else {
            setRouteKey(builder, route, CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
        }

        val oisModes = c?.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
        val requestedOis = if (
            oisBox.isChecked && oisModes.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)
        ) CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON
        else CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF
        if (oisModes.contains(requestedOis)) {
            setRouteKey(builder, route, CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, requestedOis)
        }
    }

    private fun <T> setRouteKey(
        builder: CaptureRequest.Builder,
        route: HonorRawRoute,
        key: CaptureRequest.Key<T>,
        value: T,
    ) {
        val physicalId = route.physicalCameraId
        if (physicalId != null && key.name in physicalRequestKeyNames) {
            runCatching { builder.setPhysicalCameraKey(key, value, physicalId) }
                .onFailure { builder.set(key, value) }
        } else {
            builder.set(key, value)
        }
    }

    private fun captureSingleRaw() {
        val route = selectedRoute ?: return
        val camera = cameraDevice ?: return
        val session = cameraSession ?: return
        val reader = imageReader ?: return
        val latest = lastPreviewResult ?: run {
            status("Nog geen preview-resultaat; wacht tot live telemetry zichtbaar is.")
            return
        }

        resetCaptureOutputs()
        synchronized(pairingLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            captureFinalizing = false
        }
        preCaptureSnapshot = buildPreviewSnapshot(latest, route)
        captureButton.isEnabled = false

        try {
            val request = camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE).apply {
                addTarget(reader.surface)
                applyPreviewAndCaptureControls(this, route)

                val c = effectiveCharacteristics(route)
                val nrModes = c?.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
                if (nrModes.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                    setRouteKey(this, route, CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
                }
                val edgeModes = c?.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
                if (edgeModes.contains(CameraMetadata.EDGE_MODE_OFF)) {
                    setRouteKey(this, route, CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)
                }
                CameraCalibrationTelemetry.requestLensShadingMap(
                    this,
                    c,
                    route.physicalCameraId,
                )
            }.build()

            val before = preCaptureSnapshot!!
            status(
                "Single RAW verstuurd · 3A-ready=${before.threeAReady} · AF=${before.afState} · AE=${before.aeState} · " +
                    "wachten op exact RAW/result timestamp-paar…",
            )
            session.capture(request, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    synchronized(pairingLock) { pendingResult = result }
                    tryFinalizeCapturePair()
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: android.hardware.camera2.CaptureFailure,
                ) {
                    statusFromAnyThread("RAW capture faalde: reason=${failure.reason}")
                    runOnUiThread { captureButton.isEnabled = true }
                }
            }, cameraHandler)
        } catch (error: Throwable) {
            status("RAW request faalde: ${error.javaClass.simpleName}: ${error.message}")
            captureButton.isEnabled = true
        }
    }

    private fun tryFinalizeCapturePair() {
        val image: Image
        val result: TotalCaptureResult
        val route: HonorRawRoute
        synchronized(pairingLock) {
            if (captureFinalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            route = selectedRoute ?: return
            val sensorTimestamp = result.get(CaptureResult.SENSOR_TIMESTAMP)
            if (sensorTimestamp == null || sensorTimestamp != image.timestamp) {
                captureFinalizing = true
                pendingImage = null
                pendingResult = null
                image.close()
                statusFromAnyThread(
                    "FAIL CLOSED: RAW timestamp=${image.timestamp} != SENSOR_TIMESTAMP=$sensorTimestamp",
                )
                runOnUiThread { captureButton.isEnabled = true }
                return
            }
            captureFinalizing = true
            pendingImage = null
            pendingResult = null
        }
        cameraHandler.post { finalizeCapture(route, image, result) }
    }

    private fun finalizeCapture(route: HonorRawRoute, image: Image, result: TotalCaptureResult) {
        try {
            val physicalResults = result.physicalCameraResults
            if (route.physicalCameraId != null && !physicalResults.containsKey(route.physicalCameraId)) {
                throw IllegalStateException(
                    "requested physical ${route.physicalCameraId}, reported=${physicalResults.keys}",
                )
            }
            val effectiveResult: CaptureResult = route.physicalCameraId?.let { physicalResults[it] } ?: result
            val effectiveId = route.physicalCameraId ?: route.logicalCameraId
            val characteristics = cameraManager.getCameraCharacteristics(effectiveId)

            val stamp = System.currentTimeMillis()
            val dng = File(cacheDir, "C2RAW_${stamp}_${route.rawSize.width}x${route.rawSize.height}_v04.dng")
            FileOutputStream(dng).use { out ->
                DngCreator(characteristics, effectiveResult).use { creator ->
                    // Explicitly request valid TIFF/DNG Orientation=1 rather than inheriting an invalid enum.
                    creator.setOrientation(1)
                    creator.writeImage(out, image)
                }
            }
            image.close()

            val sha = sha256(dng)
            val compact = File(cacheDir, "C2RAW_${stamp}_fotograaf_live_observation_v0_4.json")
            val lab = File(cacheDir, "C2RAW_${stamp}_honor_lab_dump_v0_4.json")
            compact.writeText(buildCompactObservation(route, result, dng, sha).toString(2))
            lab.writeText(buildHonorLabDump(route, result, sha).toString(2))

            capturedDng = dng
            capturedJson = compact
            capturedLabJson = lab
            statusFromAnyThread(
                "ROUTE PROOF PASS · physical=${route.physicalCameraId ?: "logical"} · exact timestamp · " +
                    "DNG=${dng.length()} B · SHA-256=${sha.take(16)}…",
            )
            runOnUiThread {
                captureButton.isEnabled = true
                saveDngButton.isEnabled = true
                saveJsonButton.isEnabled = true
                saveLabButton.isEnabled = true
            }
        } catch (error: Throwable) {
            runCatching { image.close() }
            statusFromAnyThread("Capture finalization fail-closed: ${error.javaClass.simpleName}: ${error.message}")
            runOnUiThread { captureButton.isEnabled = true }
        }
    }

    private fun buildCompactObservation(
        route: HonorRawRoute,
        result: TotalCaptureResult,
        dng: File,
        sha: String,
    ): JSONObject {
        val physicalResults = result.physicalCameraResults
        val effective: CaptureResult = route.physicalCameraId?.let { physicalResults[it] } ?: result
        val before = preCaptureSnapshot
        val after = snapshotFromCaptureResult(effective, result, route)
        val importantHonor = JSONObject()
        listOf(
            "com.hihonor.capture.metadata.previewCameraPhysicalId",
            "com.hihonor.capture.metadata.masterSensorSlotId",
            "com.hihonor.capture.metadata.binningFactor",
            "com.hihonor.capture.metadata.aecLV",
            "com.hihonor.capture.metadata.aecLightLuxValue",
            "com.hihonor.capture.metadata.activeSensors",
        ).forEach { name ->
            importantHonor.put(name.substringAfterLast('.'), readVendorByName(result, name))
        }

        return JSONObject()
            .put("schema", "truthraw.fotograaf-live-route-observation.v0.4")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
            .put("calibrationAuthorityGranted", false)
            .put("c0IdentitySealed", false)
            .put("scientificMasterModified", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("fingerprint", Build.FINGERPRINT)
                .put("sdkInt", Build.VERSION.SDK_INT))
            .put("route", JSONObject()
                .put("routeClass", route.routeClass)
                .put("logicalCameraId", route.logicalCameraId)
                .put("requestedPhysicalCameraId", nullable(route.physicalCameraId))
                .put("confirmedPhysicalResultCameraIds", JSONArray(physicalResults.keys.sorted()))
                .put("rawWidth", route.rawSize.width)
                .put("rawHeight", route.rawSize.height)
                .put("previewSize", previewSize?.let { "${it.width}x${it.height}" } ?: JSONObject.NULL))
            .put("requestedControls", JSONObject()
                .put("manualFocusEnabled", manualFocusBox.isChecked)
                .put("focusDistanceDiopters", if (manualFocusBox.isChecked) selectedFocusDiopters() else JSONObject.NULL)
                .put("oisRequested", oisBox.isChecked)
                .put("vendorRequestsWritten", false)
                .put("standardPhysicalRequestKeysUsedWhenAdvertised", route.physicalCameraId != null))
            .put("previewBeforeCapture", before?.toJson() ?: JSONObject.NULL)
            .put("captureResult", after.toJson())
            .put(
                "cameraCalibrationTelemetry",
                CameraCalibrationTelemetry.toJson(effective, effectiveCharacteristics(route)),
            )
            .put("importantHonorResults", importantHonor)
            .put("sourceDng", JSONObject()
                .put("sha256", sha)
                .put("bytes", dng.length())
                .put("orientationRequested", 1))
            .put("openCalibrationBlockers", JSONArray()
                .put("cameraSystemIdMappingIndependentValidation")
                .put("captureSampleDomainId")
                .put("gainReadoutStateId")
                .put("focusStateClass")
                .put("stabilizationStateClass"))
    }

    @Suppress("UNCHECKED_CAST")
    private fun buildHonorLabDump(
        route: HonorRawRoute,
        result: TotalCaptureResult,
        sourceSha: String,
    ): JSONObject {
        val values = JSONObject()
        result.keys
            .filter { it.name.startsWith("com.hihonor.") }
            .sortedBy { it.name }
            .forEach { key ->
                val value = runCatching { result.get(key as CaptureResult.Key<Any>) }.getOrNull()
                values.put(key.name, renderVendorValue(value))
            }
        return JSONObject()
            .put("schema", "truthraw.honor-lab-dump.v0.4")
            .put("authority", "DIAGNOSTIC_VENDOR_METADATA_ONLY")
            .put("sourceDngSha256", sourceSha)
            .put("routeClass", route.routeClass)
            .put("logicalCameraId", route.logicalCameraId)
            .put("physicalCameraId", nullable(route.physicalCameraId))
            .put("vendorResults", values)
    }

    private data class PreviewSnapshot(
        val frameCount: Long,
        val ageMs: Long,
        val afState: Int?,
        val aeState: Int?,
        val oisMode: Int?,
        val iso: Int?,
        val exposureTimeNs: Long?,
        val focusDistanceDiopters: Float?,
        val focalLengthMm: Float?,
        val activePhysicalId: String?,
        val honorPreviewPhysicalId: Any?,
        val threeAReady: Boolean,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("previewFrameCount", frameCount)
            .put("resultAgeMs", ageMs)
            .put("afState", nullable(afState))
            .put("aeState", nullable(aeState))
            .put("oisMode", nullable(oisMode))
            .put("iso", nullable(iso))
            .put("exposureTimeNs", nullable(exposureTimeNs))
            .put("focusDistanceDiopters", nullable(focusDistanceDiopters))
            .put("focalLengthMm", nullable(focalLengthMm))
            .put("activePhysicalId", nullable(activePhysicalId))
            .put("honorPreviewPhysicalId", honorPreviewPhysicalId ?: JSONObject.NULL)
            .put("threeAReady", threeAReady)
    }

    private fun buildPreviewSnapshot(result: TotalCaptureResult, route: HonorRawRoute): PreviewSnapshot {
        val physical = route.physicalCameraId?.let { result.physicalCameraResults[it] }
        val effective: CaptureResult = physical ?: result
        return snapshotFromCaptureResult(effective, result, route).copy(
            frameCount = previewFrameCount,
            ageMs = (System.currentTimeMillis() - lastPreviewResultWallMs).coerceAtLeast(0L),
        )
    }

    private fun snapshotFromCaptureResult(
        effective: CaptureResult,
        logicalResult: TotalCaptureResult,
        route: HonorRawRoute,
    ): PreviewSnapshot {
        val af = effective.get(CaptureResult.CONTROL_AF_STATE)
        val ae = effective.get(CaptureResult.CONTROL_AE_STATE)
        val afReady = if (manualFocusBox.isChecked) true else
            af == CaptureResult.CONTROL_AF_STATE_PASSIVE_FOCUSED ||
                af == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED
        val aeReady = ae == CaptureResult.CONTROL_AE_STATE_CONVERGED ||
            ae == CaptureResult.CONTROL_AE_STATE_LOCKED
        val active = runCatching { logicalResult.get(CaptureResult.LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID) }.getOrNull()
        return PreviewSnapshot(
            frameCount = previewFrameCount,
            ageMs = 0L,
            afState = af,
            aeState = ae,
            oisMode = effective.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE),
            iso = effective.get(CaptureResult.SENSOR_SENSITIVITY),
            exposureTimeNs = effective.get(CaptureResult.SENSOR_EXPOSURE_TIME),
            focusDistanceDiopters = effective.get(CaptureResult.LENS_FOCUS_DISTANCE),
            focalLengthMm = effective.get(CaptureResult.LENS_FOCAL_LENGTH),
            activePhysicalId = active ?: route.physicalCameraId,
            honorPreviewPhysicalId = readVendorByName(logicalResult, "com.hihonor.capture.metadata.previewCameraPhysicalId"),
            threeAReady = afReady && aeReady,
        )
    }

    private fun renderTelemetry(s: PreviewSnapshot): String {
        val expMs = s.exposureTimeNs?.div(1_000_000.0)
        return buildString {
            append(if (s.threeAReady) "● 3A READY" else "○ 3A settling")
            append(" · frames=${s.frameCount}")
            append("\nphysical=${s.activePhysicalId ?: "?"} / HONOR=${renderCompactVendor(s.honorPreviewPhysicalId)}")
            append(" · AF=${s.afState} · AE=${s.aeState} · OIS=${s.oisMode}")
            append("\nISO=${s.iso ?: "?"} · ")
            append(if (expMs != null) String.format(Locale.US, "%.3f ms", expMs) else "? ms")
            append(" · focus=${s.focusDistanceDiopters ?: "?"} D · focal=${s.focalLengthMm ?: "?"} mm")
        }
    }

    private fun choosePreviewSize(c: CameraCharacteristics): android.util.Size {
        val map = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val sizes = map?.getOutputSizes(SurfaceTexture::class.java)?.toList().orEmpty()
        if (sizes.isEmpty()) return android.util.Size(1280, 960)
        val targetRatio = 4.0 / 3.0
        val bounded = sizes.filter { it.width <= 1920 && it.height <= 1440 }
        val candidates = if (bounded.isNotEmpty()) bounded else sizes
        return candidates.minWithOrNull(
            compareBy<android.util.Size> {
                abs((it.width.toDouble() / it.height.toDouble()) - targetRatio)
            }.thenByDescending { it.width.toLong() * it.height.toLong() },
        ) ?: sizes.first()
    }

    private fun configureTransform(viewWidth: Int, viewHeight: Int, size: android.util.Size) {
        if (viewWidth == 0 || viewHeight == 0) return
        val rotation = display?.rotation ?: Surface.ROTATION_0
        val matrix = Matrix()
        val viewRect = RectF(0f, 0f, viewWidth.toFloat(), viewHeight.toFloat())
        val bufferRect = RectF(0f, 0f, size.height.toFloat(), size.width.toFloat())
        val centerX = viewRect.centerX()
        val centerY = viewRect.centerY()
        if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
            bufferRect.offset(centerX - bufferRect.centerX(), centerY - bufferRect.centerY())
            matrix.setRectToRect(viewRect, bufferRect, Matrix.ScaleToFit.FILL)
            val scale = maxOf(
                viewHeight.toFloat() / size.height.toFloat(),
                viewWidth.toFloat() / size.width.toFloat(),
            )
            matrix.postScale(scale, scale, centerX, centerY)
            matrix.postRotate((90 * (rotation - 2)).toFloat(), centerX, centerY)
        } else if (rotation == Surface.ROTATION_180) {
            matrix.postRotate(180f, centerX, centerY)
        }
        previewView.setTransform(matrix)
    }

    @Suppress("UNCHECKED_CAST")
    private fun readVendorByName(result: CaptureResult, name: String): Any? {
        val key = result.keys.firstOrNull { it.name == name } ?: return null
        val value = runCatching { result.get(key as CaptureResult.Key<Any>) }.getOrNull()
        return when (value) {
            is ByteArray -> JSONArray(value.map { it.toInt() })
            is ShortArray -> JSONArray(value.map { it.toInt() })
            is IntArray -> JSONArray(value.toList())
            is LongArray -> JSONArray(value.toList())
            is FloatArray -> JSONArray(value.toList())
            is DoubleArray -> JSONArray(value.toList())
            is BooleanArray -> JSONArray(value.toList())
            is Array<*> -> JSONArray(value.toList())
            else -> value
        }
    }

    private fun renderCompactVendor(value: Any?): String = when (value) {
        null -> "?"
        is JSONArray -> value.toString()
        else -> value.toString()
    }

    private fun effectiveCharacteristics(route: HonorRawRoute): CameraCharacteristics? =
        runCatching {
            cameraManager.getCameraCharacteristics(route.physicalCameraId ?: route.logicalCameraId)
        }.getOrNull()

    private fun selectedFocusDiopters(): Float =
        if (focusSeek.max <= 0) 0f else focusMaxDiopters * focusSeek.progress.toFloat() / focusSeek.max.toFloat()

    private fun updateFocusLabel() {
        focusValue.text = if (focusMaxDiopters <= 0f) {
            "MF: niet beschikbaar voor deze route"
        } else String.format(
            Locale.US,
            "MF requested %.4f D · bereik 0..%.4f D · actual staat in live telemetry/result",
            selectedFocusDiopters(),
            focusMaxDiopters,
        )
    }

    private fun closeCameraResources() {
        runCatching { cameraSession?.stopRepeating() }
        runCatching { cameraSession?.close() }
        runCatching { cameraDevice?.close() }
        runCatching { imageReader?.close() }
        runCatching { previewSurface?.release() }
        cameraSession = null
        cameraDevice = null
        imageReader = null
        previewSurface = null
        previewRequestBuilder = null
        captureButton.isEnabled = false
    }

    private fun resetCaptureOutputs() {
        capturedDng = null
        capturedJson = null
        capturedLabJson = null
        saveDngButton.isEnabled = false
        saveJsonButton.isEnabled = false
        saveLabButton.isEnabled = false
    }

    private fun launchSave(file: File?, mime: String, requestCode: Int) {
        file ?: return
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime
            putExtra(Intent.EXTRA_TITLE, file.name)
        }, requestCode)
    }

    @Deprecated("Dependency-light research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK || data?.data == null) return
        val source = when (requestCode) {
            REQUEST_SAVE_DNG -> capturedDng
            REQUEST_SAVE_JSON -> capturedJson
            REQUEST_SAVE_LAB -> capturedLabJson
            else -> null
        } ?: return
        runCatching {
            contentResolver.openOutputStream(data.data!!, "w")!!.use { out ->
                FileInputStream(source).use { input -> input.copyTo(out) }
            }
        }.onSuccess { status("Opgeslagen: ${source.name}") }
            .onFailure { status("Opslaan faalde: ${it.message ?: it.javaClass.simpleName}") }
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val count = input.read(buffer)
                if (count <= 0) break
                digest.update(buffer, 0, count)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }

    private fun renderVendorValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is ByteArray -> JSONArray(value.map { it.toInt() })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.toList())
        is DoubleArray -> JSONArray(value.toList())
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.toList())
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun status(message: String) {
        statusView.text = message
    }

    private fun statusFromAnyThread(message: String) = runOnUiThread { status(message) }

    private fun text(value: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun button(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        isAllCaps = false
        setOnClickListener { action() }
    }

    private fun space(heightDp: Int): View = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(heightDp))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        openPreviewForSelectedRoute()
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        previewSize?.let { configureTransform(width, height, it) }
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        closeCameraResources()
        return true
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit

    companion object {
        private const val REQUEST_SAVE_DNG = 501
        private const val REQUEST_SAVE_JSON = 502
        private const val REQUEST_SAVE_LAB = 503

        private fun nullable(value: Any?): Any = value ?: JSONObject.NULL
    }
}

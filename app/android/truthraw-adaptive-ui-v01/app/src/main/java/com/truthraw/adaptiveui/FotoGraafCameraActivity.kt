package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.media.DngCreator
import android.media.Image
import android.media.ImageReader
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
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

/**
 * FotoGraaf acquisition instrument.
 *
 * This activity is intentionally separate from TruthRaw reconstruction. It can
 * discover and prove camera routes, but cannot promote calibration/scientific
 * authority. Vendor APK knowledge only guides which runtime keys we inspect.
 */
class FotoGraafCameraActivity : Activity() {
    private lateinit var cameraManager: CameraManager
    private lateinit var routeSpinner: Spinner
    private lateinit var reportView: TextView
    private lateinit var statusView: TextView
    private lateinit var captureButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button
    private lateinit var manualFocusBox: CheckBox
    private lateinit var focusSeek: SeekBar
    private lateinit var focusValue: TextView
    private lateinit var oisBox: CheckBox

    private var probeReport: HonorProbeReport? = null
    private var routes: List<HonorRawRoute> = emptyList()
    private var focusMaxDiopters = 0f

    private val cameraThread = HandlerThread("truthraw-fotograaf-camera").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var cameraDevice: CameraDevice? = null
    private var cameraSession: CameraCaptureSession? = null
    private var imageReader: ImageReader? = null

    private val pairingLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var activeRoute: HonorRawRoute? = null
    private var captureFinalizing = false

    private var capturedDng: File? = null
    private var capturedJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        cameraManager = getSystemService(CameraManager::class.java)
        setContentView(buildUi())
        scanRuntime()
    }

    override fun onDestroy() {
        closeCaptureResources()
        synchronized(pairingLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
        }
        cameraThread.quitSafely()
        super.onDestroy()
    }

    private fun buildUi(): View {
        val scroll = ScrollView(this)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(24))
            setBackgroundColor(Color.rgb(18, 20, 24))
        }
        scroll.addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        root.addView(text("FotoGraaf · HONOR RAW Route Proof", 23f, true))
        root.addView(text(
            "Acquisitie/metrologie only. Geen vendor-tag, stock APK of captureknop mag TruthRaw-evidence, calibratie, kleur- of noise-authority promoveren.",
            12f,
            false,
            Color.rgb(180, 186, 196),
        ))
        root.addView(space(10))

        root.addView(button("Opnieuw HONOR Camera2 scannen") { scanRuntime() })
        root.addView(space(8))

        routeSpinner = Spinner(this)
        root.addView(routeSpinner, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        root.addView(space(8))

        manualFocusBox = CheckBox(this).apply {
            text = "Manual focus (CONTROL_AF_MODE_OFF + LENS_FOCUS_DISTANCE)"
            setTextColor(Color.WHITE)
            setOnCheckedChangeListener { _, enabled ->
                focusSeek.isEnabled = enabled && focusMaxDiopters > 0f
                updateFocusLabel()
            }
        }
        root.addView(manualFocusBox)

        focusValue = text("MF: niet beschikbaar", 12f, false, Color.rgb(180, 186, 196))
        root.addView(focusValue)
        focusSeek = SeekBar(this).apply {
            max = 1000
            progress = 0
            isEnabled = false
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) = updateFocusLabel()
                override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                override fun onStopTrackingTouch(seekBar: SeekBar?) = Unit
            })
        }
        root.addView(focusSeek)

        oisBox = CheckBox(this).apply {
            text = "OIS aanvragen (actual result wordt apart geregistreerd)"
            setTextColor(Color.WHITE)
            isChecked = false
        }
        root.addView(oisBox)
        root.addView(space(8))

        captureButton = button("Maak één RAW route-proof capture") { captureSelectedRoute() }
        root.addView(captureButton)
        root.addView(space(8))

        saveDngButton = button("DNG opslaan") { launchSaveDng() }.apply { isEnabled = false }
        saveJsonButton = button("Observation JSON opslaan") { launchSaveJson() }.apply { isEnabled = false }
        root.addView(saveDngButton)
        root.addView(space(6))
        root.addView(saveJsonButton)
        root.addView(space(8))
        root.addView(button("Open bestaande TruthRaw processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space(12))

        statusView = text("Nog geen scan.", 13f, true)
        root.addView(statusView)
        root.addView(space(10))
        reportView = text("", 11f, false, Color.rgb(205, 210, 218)).apply {
            setTextIsSelectable(true)
        }
        root.addView(reportView)
        return scroll
    }

    private fun scanRuntime() {
        status("Camera2/HONOR runtime inventory wordt gelezen…")
        captureButton.isEnabled = false
        Thread({
            val result = runCatching { HonorCameraProbe.scan(cameraManager) }
            runOnUiThread {
                result.onSuccess { report ->
                    probeReport = report
                    routes = report.routes
                    reportView.text = report.reportText
                    routeSpinner.adapter = ArrayAdapter(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        routes,
                    )
                    captureButton.isEnabled = routes.isNotEmpty()
                    status(
                        "Scan klaar · ${report.inventories.size} camera-ID's · ${routes.size} RAW-routekandidaten. " +
                            "Authority blijft CAPABILITY_OBSERVATION_ONLY.",
                    )
                    updateControlsForSelectedRoute()
                    routeSpinner.setOnItemSelectedListener(object : android.widget.AdapterView.OnItemSelectedListener {
                        override fun onItemSelected(parent: android.widget.AdapterView<*>?, view: View?, position: Int, id: Long) {
                            updateControlsForSelectedRoute()
                        }
                        override fun onNothingSelected(parent: android.widget.AdapterView<*>?) = Unit
                    })
                }.onFailure { error ->
                    routes = emptyList()
                    reportView.text = error.stackTraceToString()
                    status("Scan faalde: ${error.message ?: error.javaClass.simpleName}")
                }
            }
        }, "truthraw-honor-inventory").start()
    }

    private fun selectedRoute(): HonorRawRoute? = routes.getOrNull(routeSpinner.selectedItemPosition)

    private fun updateControlsForSelectedRoute() {
        val route = selectedRoute() ?: return
        val characteristics = effectiveCharacteristics(route) ?: return
        focusMaxDiopters = characteristics.get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f
        manualFocusBox.isEnabled = focusMaxDiopters > 0f
        if (focusMaxDiopters <= 0f) manualFocusBox.isChecked = false
        focusSeek.isEnabled = manualFocusBox.isChecked && focusMaxDiopters > 0f
        val oisModes = characteristics.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION).orEmpty()
        oisBox.isEnabled = oisModes.isNotEmpty()
        if (!oisBox.isEnabled) oisBox.isChecked = false
        updateFocusLabel()
    }

    private fun updateFocusLabel() {
        if (focusMaxDiopters <= 0f) {
            focusValue.text = "MF: niet beschikbaar voor deze route"
            return
        }
        val value = selectedFocusDiopters()
        focusValue.text = String.format(
            Locale.US,
            "MF: %.4f D · bereik 0..%.4f D · requested ≠ proven actual",
            value,
            focusMaxDiopters,
        )
    }

    private fun selectedFocusDiopters(): Float =
        focusMaxDiopters * (focusSeek.progress.toFloat() / focusSeek.max.toFloat())

    private fun captureSelectedRoute() {
        val route = selectedRoute() ?: return
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            status("Camera-permissie gevraagd. Start de capture na toestemming opnieuw.")
            return
        }

        resetCapturedOutputs()
        closeCaptureResources()
        synchronized(pairingLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            captureFinalizing = false
            activeRoute = route
        }
        captureButton.isEnabled = false
        status("Route openen: ${route.label}")

        val reader = ImageReader.newInstance(
            route.rawSize.width,
            route.rawSize.height,
            ImageFormat.RAW_SENSOR,
            2,
        )
        imageReader = reader
        reader.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
            onRawImage(image)
        }, cameraHandler)

        try {
            cameraManager.openCamera(route.logicalCameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    cameraDevice = camera
                    createSession(camera, route, reader)
                }

                override fun onDisconnected(camera: CameraDevice) {
                    statusFromAnyThread("Camera disconnected: ${route.logicalCameraId}")
                    camera.close()
                    closeCaptureResources()
                }

                override fun onError(camera: CameraDevice, error: Int) {
                    statusFromAnyThread("Camera open error=$error voor ${route.logicalCameraId}")
                    camera.close()
                    closeCaptureResources()
                }
            }, cameraHandler)
        } catch (error: Exception) {
            status("Open camera faalde: ${error.message ?: error.javaClass.simpleName}")
            captureButton.isEnabled = true
            closeCaptureResources()
        }
    }

    private fun createSession(camera: CameraDevice, route: HonorRawRoute, reader: ImageReader) {
        val output = OutputConfiguration(reader.surface)
        route.physicalCameraId?.let { physicalId ->
            runCatching { output.setPhysicalCameraId(physicalId) }
                .onFailure {
                    statusFromAnyThread("FAIL CLOSED: physical output $physicalId kon niet worden gebonden: ${it.message}")
                    closeCaptureResources()
                    return
                }
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    cameraSession = session
                    submitStillCapture(camera, session, route, reader)
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    status("Capture-session configuratie faalde voor ${route.label}")
                    captureButton.isEnabled = true
                    closeCaptureResources()
                }
            },
        )

        try {
            camera.createCaptureSession(config)
        } catch (error: CameraAccessException) {
            statusFromAnyThread("Capture-session faalde: ${error.message}")
            closeCaptureResources()
        }
    }

    private fun submitStillCapture(
        camera: CameraDevice,
        session: CameraCaptureSession,
        route: HonorRawRoute,
        reader: ImageReader,
    ) {
        val characteristics = effectiveCharacteristics(route)
        if (characteristics == null) {
            status("Geen characteristics voor geselecteerde route.")
            captureButton.isEnabled = true
            return
        }

        try {
            val builder = camera.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            builder.addTarget(reader.surface)
            builder.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            builder.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)

            val afModes = characteristics.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES).orEmpty()
            if (manualFocusBox.isChecked && focusMaxDiopters > 0f) {
                builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
                builder.set(CaptureRequest.LENS_FOCUS_DISTANCE, selectedFocusDiopters())
            } else when {
                afModes.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE) ->
                    builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                afModes.contains(CameraMetadata.CONTROL_AF_MODE_AUTO) ->
                    builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_AUTO)
                else -> builder.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_OFF)
            }

            val oisModes = characteristics.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION).orEmpty()
            val requestedOis = if (oisBox.isChecked && oisModes.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)) {
                CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON
            } else {
                CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_OFF
            }
            if (oisModes.contains(requestedOis)) {
                builder.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, requestedOis)
            }

            val nrModes = characteristics.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES).orEmpty()
            if (nrModes.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                builder.set(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
            }
            val edgeModes = characteristics.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES).orEmpty()
            if (edgeModes.contains(CameraMetadata.EDGE_MODE_OFF)) {
                builder.set(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)
            }

            status("Capture verstuurd · ${route.label} · wachten op RAW + TotalCaptureResult timestamp-identiteit…")
            session.capture(builder.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult,
                ) {
                    onCaptureResult(result)
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: android.hardware.camera2.CaptureFailure,
                ) {
                    statusFromAnyThread("Capture faalde: reason=${failure.reason}")
                    runOnUiThread { captureButton.isEnabled = true }
                    closeCaptureResources()
                }
            }, cameraHandler)
        } catch (error: Exception) {
            status("Still capture faalde: ${error.message ?: error.javaClass.simpleName}")
            captureButton.isEnabled = true
            closeCaptureResources()
        }
    }

    private fun onRawImage(image: Image) {
        synchronized(pairingLock) {
            pendingImage?.close()
            pendingImage = image
        }
        tryFinalizePair()
    }

    private fun onCaptureResult(result: TotalCaptureResult) {
        synchronized(pairingLock) { pendingResult = result }
        tryFinalizePair()
    }

    private fun tryFinalizePair() {
        val image: Image
        val result: TotalCaptureResult
        val route: HonorRawRoute
        synchronized(pairingLock) {
            if (captureFinalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            route = activeRoute ?: return
            val sensorTimestamp = result.get(CaptureResult.SENSOR_TIMESTAMP)
            if (sensorTimestamp == null || sensorTimestamp != image.timestamp) {
                captureFinalizing = true
                pendingImage = null
                pendingResult = null
                image.close()
                statusFromAnyThread(
                    "FAIL CLOSED: RAW timestamp=${image.timestamp} != TotalCaptureResult SENSOR_TIMESTAMP=$sensorTimestamp",
                )
                runOnUiThread { captureButton.isEnabled = true }
                closeCaptureResources()
                return
            }
            captureFinalizing = true
            pendingImage = null
            pendingResult = null
        }

        cameraHandler.post {
            finalizeCapture(route, image, result)
        }
    }

    private fun finalizeCapture(route: HonorRawRoute, image: Image, result: TotalCaptureResult) {
        try {
            val physicalResults = if (Build.VERSION.SDK_INT >= 28) result.physicalCameraResults else emptyMap()
            if (route.physicalCameraId != null && !physicalResults.containsKey(route.physicalCameraId)) {
                throw IllegalStateException(
                    "FAIL CLOSED: no physical TotalCaptureResult for requested ID ${route.physicalCameraId}; " +
                        "reported=${physicalResults.keys}",
                )
            }

            val effectiveId = route.physicalCameraId ?: route.logicalCameraId
            val characteristics = cameraManager.getCameraCharacteristics(effectiveId)
            val dngResult: CaptureResult = route.physicalCameraId?.let { physicalResults[it] } ?: result

            val stamp = System.currentTimeMillis()
            val dngFile = File(cacheDir, "C2RAW_${stamp}_${route.rawSize.width}x${route.rawSize.height}.dng")
            FileOutputStream(dngFile).use { out ->
                DngCreator(characteristics, dngResult).use { creator ->
                    creator.writeImage(out, image)
                }
            }
            image.close()

            val sha = sha256(dngFile)
            val jsonFile = File(cacheDir, "C2RAW_${stamp}_honor_route_observation_v0_3.json")
            jsonFile.writeText(buildObservationJson(route, result, image.timestamp, dngFile, sha).toString(2))

            capturedDng = dngFile
            capturedJson = jsonFile
            statusFromAnyThread(
                "CAPTURE PROOF SAVED IN APP CACHE · route=${route.routeClass} · " +
                    "timestamp identity PASS · DNG=${dngFile.length()} bytes · SHA-256=${sha.take(16)}… · " +
                    "authority=CAMERA2_ACQUISITION_OBSERVATION_ONLY",
            )
            runOnUiThread {
                captureButton.isEnabled = true
                saveDngButton.isEnabled = true
                saveJsonButton.isEnabled = true
            }
        } catch (error: Exception) {
            runCatching { image.close() }
            statusFromAnyThread("Capture finalization fail-closed: ${error.message ?: error.javaClass.simpleName}")
            runOnUiThread { captureButton.isEnabled = true }
        } finally {
            closeCaptureResources()
        }
    }

    @Suppress("UNCHECKED_CAST")
    private fun buildObservationJson(
        route: HonorRawRoute,
        result: TotalCaptureResult,
        imageTimestamp: Long,
        dngFile: File,
        dngSha256: String,
    ): JSONObject {
        val physicalResults = if (Build.VERSION.SDK_INT >= 28) result.physicalCameraResults else emptyMap()
        val effectiveResult: CaptureResult = route.physicalCameraId?.let { physicalResults[it] } ?: result
        val c = effectiveCharacteristics(route)
        val vendorResults = JSONObject()
        result.keys.filter { it.name.startsWith("com.hihonor.") }.sortedBy { it.name }.forEach { key ->
            val value = runCatching { result.get(key as CaptureResult.Key<Any>) }.getOrNull()
            vendorResults.put(key.name, renderVendorValue(value))
        }

        val confirmedPhysical = JSONArray()
        physicalResults.keys.sorted().forEach(confirmedPhysical::put)

        val requested = JSONObject()
            .put("manualFocusEnabled", manualFocusBox.isChecked)
            .put("focusDistanceDiopters", if (manualFocusBox.isChecked) selectedFocusDiopters() else JSONObject.NULL)
            .put("oisRequested", oisBox.isChecked)
            .put("vendorRequestsWritten", false)

        val actual = JSONObject()
            .put("sensorTimestampNs", result.get(CaptureResult.SENSOR_TIMESTAMP))
            .put("imageTimestampNs", imageTimestamp)
            .put("timestampIdentityPass", result.get(CaptureResult.SENSOR_TIMESTAMP) == imageTimestamp)
            .put("iso", effectiveResult.get(CaptureResult.SENSOR_SENSITIVITY))
            .put("exposureTimeNs", effectiveResult.get(CaptureResult.SENSOR_EXPOSURE_TIME))
            .put("focalLengthMm", effectiveResult.get(CaptureResult.LENS_FOCAL_LENGTH))
            .put("focusDistanceDiopters", effectiveResult.get(CaptureResult.LENS_FOCUS_DISTANCE))
            .put("afMode", effectiveResult.get(CaptureResult.CONTROL_AF_MODE))
            .put("afState", effectiveResult.get(CaptureResult.CONTROL_AF_STATE))
            .put("oisMode", effectiveResult.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE))
            .put("noiseReductionMode", effectiveResult.get(CaptureResult.NOISE_REDUCTION_MODE))

        return JSONObject()
            .put("schema", "truthraw.fotograaf-honor-native-route-observation.v0.3")
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
                .put("requestedPhysicalCameraId", route.physicalCameraId ?: JSONObject.NULL)
                .put("confirmedPhysicalResultCameraIds", confirmedPhysical)
                .put("stockGuidedCandidate", route.stockGuidedCandidate))
            .put("topology", JSONObject()
                .put("format", "RAW_SENSOR")
                .put("width", route.rawSize.width)
                .put("height", route.rawSize.height)
                .put("cfa", cfaName(c?.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)))
                .put("directCfa", true)
                .put("processedRgb", false)
                .put("mergedMultiFrame", false))
            .put("requestedControls", requested)
            .put("captureResult", actual)
            .put("honorVendorResults", vendorResults)
            .put("sourceDng", JSONObject()
                .put("sha256", dngSha256)
                .put("bytes", dngFile.length()))
            .put("openCalibrationBlockers", JSONArray()
                .put("cameraSystemIdMappingIndependentValidation")
                .put("captureSampleDomainId")
                .put("gainReadoutStateId")
                .put("focusStateClass")
                .put("stabilizationStateClass"))
    }

    private fun effectiveCharacteristics(route: HonorRawRoute): CameraCharacteristics? =
        runCatching { cameraManager.getCameraCharacteristics(route.physicalCameraId ?: route.logicalCameraId) }.getOrNull()

    private fun closeCaptureResources() {
        runCatching { cameraSession?.close() }
        runCatching { cameraDevice?.close() }
        runCatching { imageReader?.close() }
        cameraSession = null
        cameraDevice = null
        imageReader = null
    }

    private fun resetCapturedOutputs() {
        capturedDng = null
        capturedJson = null
        saveDngButton.isEnabled = false
        saveJsonButton.isEnabled = false
    }

    private fun launchSaveDng() {
        val file = capturedDng ?: return
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, file.name)
        }
        startActivityForResult(intent, REQUEST_SAVE_DNG)
    }

    private fun launchSaveJson() {
        val file = capturedJson ?: return
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, file.name)
        }
        startActivityForResult(intent, REQUEST_SAVE_JSON)
    }

    @Deprecated("Dependency-light research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK || data?.data == null) return
        val source = when (requestCode) {
            REQUEST_SAVE_DNG -> capturedDng
            REQUEST_SAVE_JSON -> capturedJson
            else -> null
        } ?: return
        val destination = data.data ?: return
        runCatching {
            contentResolver.openOutputStream(destination, "w")!!.use { out ->
                FileInputStream(source).use { input -> input.copyTo(out) }
            }
        }.onSuccess {
            status("Opgeslagen: ${source.name}")
        }.onFailure {
            status("Opslaan faalde: ${it.message ?: it.javaClass.simpleName}")
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA_PERMISSION) {
            status(if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
                "Camera-permissie toegestaan. Druk opnieuw op capture zodat de routekeuze expliciet blijft."
            } else {
                "Camera-permissie geweigerd; capture blijft geblokkeerd."
            })
        }
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

    companion object {
        private const val REQUEST_CAMERA_PERMISSION = 301
        private const val REQUEST_SAVE_DNG = 302
        private const val REQUEST_SAVE_JSON = 303
    }
}

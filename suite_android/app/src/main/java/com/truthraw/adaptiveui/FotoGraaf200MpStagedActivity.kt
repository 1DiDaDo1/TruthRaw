package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.ContentValues
import android.content.pm.PackageManager
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.RectF
import android.graphics.SurfaceTexture
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.StateListDrawable
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
import android.provider.MediaStore
import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Gravity
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.FrameLayout
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
 * TruthRaw FotoGraaf Camera-5 200MP Android-17 replay of proven v0.14 route.
 *
 * v0.10 proved on-device:
 *  - physical camera 5 advertises RAW_SENSOR 16320x12288 through
 *    SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION.getHighResolutionOutputSizes()
 *  - logical camera 0 preview at 3.7x reports activePhysical=5
 *  - a global-only MAXIMUM_RESOLUTION still request can reach the capture callback but fail there.
 *
 * Historical Android-16 v0.12-v0.14 evidence established an asymmetric HONOR route:
 *  - logical/global SENSOR_PIXEL_MODE was not a valid requirement (globalMAX=false);
 *  - physical Camera-5 scoped builder accepted MAXIMUM_RESOLUTION (physicalMAX=true)
 *    even though the key was not advertised as a physical override;
 *  - the qualifying v0.14 frame returned physical SENSOR_PIXEL_MODE=0;
 *  - therefore primary RAW evidence must be sealed before interpreting that metadata.
 *
 * This Android-17 replay intentionally follows that proven ordering:
 *  - never write logical/global SENSOR_PIXEL_MODE;
 *  - bind the RAW output to physical 5 and declare MAXIMUM_RESOLUTION on the output;
 *  - create a physical-5 scoped still request;
 *  - attempt the exact physical SENSOR_PIXEL_MODE=MAXIMUM_RESOLUTION write regardless of
 *    whether availablePhysicalCameraRequestKeys advertises it;
 *  - seal the 16320x12288 Image.Plane[0] after physical-result/timestamp binding and
 *    before treating the returned SENSOR_PIXEL_MODE as advisory metadata.
 *
 * App-visible RAW is not promoted to untouched ADC truth.
 */
class FotoGraaf200MpStagedActivity : Activity(), TextureView.SurfaceTextureListener {

    private lateinit var preview: AutoFitTextureView
    private lateinit var telemetry: TextView
    private lateinit var status: TextView
    private lateinit var capabilityButton: Button
    private lateinit var previewButton: Button
    private lateinit var captureButton: Button
    private lateinit var saveRawButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button

    private val cameraThread = HandlerThread("truthraw-cam5-8160-raw10-v057").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var manager: CameraManager? = null
    private var logicalCharacteristics: CameraCharacteristics? = null
    private var physical5Characteristics: CameraCharacteristics? = null
    private var capabilityReady = false
    private var capabilitySource: String? = null

    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var previewBufferSize: Size? = null
    private var rawReader: ImageReader? = null

    @Volatile private var lastPreviewResult: TotalCaptureResult? = null
    @Volatile private var lastActivePhysicalId: String? = null
    private var previewFrames = 0L

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var finalizing = false

    private var lastScopedRequestUsed = false
    private var lastScopedRequestError: String? = null
    private var lastGlobalPixelModeWritten = false
    private var lastPhysicalPixelModeAttempted = false
    private var lastPhysicalPixelModeWritten = false
    private var lastPhysicalPixelModeReadback: Int? = null
    private var lastPhysicalPixelModeError: String? = null
    private var lastPhysicalOverrideAdvertised = false

    private var capturedRaw: File? = null
    private var capturedDng: File? = null
    private var capturedJson: File? = null
    private var admittedProcessingDng: File? = null

    private val productionCameraEntry: Boolean
        get() = intent.getBooleanExtra(EXTRA_PRODUCTION_CAMERA_ENTRY, false)
    private var autoStartPreviewWhenReady = false

    data class RawRoutes(
        val standardOutput: List<Size>,
        val standardHigh: List<Size>,
        val maximumOutput: List<Size>,
        val maximumHigh: List<Size>,
        val selectedSource: String,
    )

    data class RawEvidence(
        val file: File,
        val sha256: String,
        val bytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val accessibleBytes: Long,
        val contiguous: Boolean,
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        autoStartPreviewWhenReady = productionCameraEntry
        setStatus(
            if (productionCameraEntry) {
                "CAMERA-INGANG · v0.57 physical-5 8160×6144 RAW10 source-first test wordt voorbereid. " +
                    "Capability-admission en live preview starten automatisch; capture blijft één fysiek frame."
            } else {
                "STAGE 0 · v0.57 RAW10 8160×6144 format-context test.\nDruk eerst op Stap 1."
            },
        )
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        } else if (productionCameraEntry) {
            window.decorView.post { readCapability() }
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (!productionCameraEntry) return

        // Rebuild the presentation hierarchy for the new geometry. The Activity
        // instance and already-admitted Camera-5 capability state survive this
        // handled config change; the TextureView/camera session is reopened through
        // the same route because its surface geometry changed.
        closeCameraResources(keepOutputs = true)
        autoStartPreviewWhenReady = capabilityReady
        setContentView(buildUi())
        setStatus(
            if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
                "LANDSCAPE · Camera-5 route behouden · live preview wordt passend heropend."
            } else {
                "PORTRAIT · Camera-5 route behouden · live preview wordt passend heropend."
            },
        )
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
        if (requestCode != REQUEST_CAMERA) return
        val granted = grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED
        if (!granted) {
            setStatus("CAMERA permission ontbreekt. De route blijft fail-closed.")
            return
        }
        if (productionCameraEntry) {
            autoStartPreviewWhenReady = true
            setStatus("CAMERA permission verleend · capability-admission en live RAW-preview starten.")
            window.decorView.post { readCapability() }
        } else {
            setStatus("CAMERA permission verleend. Druk op Stap 1.")
        }
    }

    private fun buildUi(): View {
        preview = AutoFitTextureView(this).apply {
            surfaceTextureListener = this@FotoGraaf200MpStagedActivity
            // Never set a TextureView background drawable/color: v0.9 fixed that crash.
        }
        telemetry = label("Preview nog niet gestart.", 11f, true, Color.rgb(220, 225, 234)).apply {
            maxLines = if (productionCameraEntry) 3 else Int.MAX_VALUE
        }
        status = label("Initialiseren…", 11f, false, Color.WHITE).apply {
            maxLines = if (productionCameraEntry) 3 else Int.MAX_VALUE
        }

        capabilityButton = button("Stap 1 · lees alle Camera-5 RAW capability-routes") { readCapability() }
        previewButton = button("Stap 2 · start live beeld via logical 0 · 3.7×") { startLogicalPreview() }.apply {
            isEnabled = false
        }
        captureButton = if (productionCameraEntry) {
            shutterButton { capture200Mp() }.apply { isEnabled = false }
        } else {
            button("Stap 3 · PHYSICAL-SCOPED RAW10 TEST · 8160×6144") { capture200Mp() }.apply {
                isEnabled = false
            }
        }
        saveRawButton = button("Originele 8160×6144 RAW10 buffer opslaan") {
            saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW)
        }.apply { isEnabled = false }
        saveDngButton = button(
            if (productionCameraEntry) "Admitted RAW/DNG verwerkingsbron opslaan" else "Auxiliary 200MP DNG opslaan",
        ) {
            saveFile(
                if (productionCameraEntry) admittedProcessingDng ?: capturedDng else capturedDng,
                "image/x-adobe-dng",
                REQUEST_SAVE_DNG,
            )
        }.apply { isEnabled = false }
        saveJsonButton = button("200MP evidence JSON opslaan") {
            saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON)
        }.apply { isEnabled = false }

        return if (productionCameraEntry) {
            buildProductionCameraUi()
        } else {
            buildResearchCameraUi()
        }
    }

    private fun buildProductionCameraUi(): View {
        val landscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.rgb(10, 12, 15))
        }
        applySafeSystemInsets(root)

        val header = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            addView(label("‹", if (landscape) 30f else 34f, false, Color.WHITE).apply {
                gravity = Gravity.CENTER
                contentDescription = "Terug"
                setOnClickListener { finish() }
            }, LinearLayout.LayoutParams(dp(46), dp(46)).apply { marginEnd = dp(6) })
            addView(LinearLayout(this@FotoGraaf200MpStagedActivity).apply {
                orientation = LinearLayout.VERTICAL
                addView(label("TruthRaw Camera", if (landscape) 17f else 20f, true))
                addView(label(
                    "Camera-5 · één fysiek frame · RAW source-first",
                    if (landscape) 8.5f else 9.5f,
                    false,
                    Color.rgb(184, 191, 202),
                ))
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        }

        // Keep the TextureView inside a bounded pane. AutoFitTextureView then receives
        // the current orientation's aspect ratio and cannot grow beyond the viewport.
        val previewPane = FrameLayout(this).apply {
            setBackgroundColor(Color.BLACK)
            clipChildren = true
            clipToPadding = true
            addView(
                preview,
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    Gravity.CENTER,
                ),
            )
        }

        val shutter = shutterPanel(landscape)
        val sourceNote = label(
            "RAW wordt eerst verzegeld; alleen de toegelaten read-only sample-domain gaat verder.",
            if (landscape) 8f else 8.5f,
            false,
            Color.rgb(145, 153, 165),
        )

        if (landscape) {
            // Camera-like landscape composition: live image owns the left side while
            // all controls remain visible in a fixed-width right rail.
            val cameraRow = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                addView(
                    previewPane,
                    LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 1f).apply {
                        marginEnd = dp(10)
                    },
                )
                addView(
                    LinearLayout(this@FotoGraaf200MpStagedActivity).apply {
                        orientation = LinearLayout.VERTICAL
                        gravity = Gravity.CENTER_HORIZONTAL
                        setPadding(dp(8), dp(4), dp(4), dp(4))
                        addView(header)
                        addView(space(6))
                        addView(telemetry)
                        addView(space(3))
                        addView(status)
                        addView(View(this@FotoGraaf200MpStagedActivity), LinearLayout.LayoutParams(1, 0, 1f))
                        addView(shutter)
                        addView(space(5))
                        addView(sourceNote)
                    },
                    LinearLayout.LayoutParams(dp(300), ViewGroup.LayoutParams.MATCH_PARENT),
                )
            }
            root.addView(cameraRow, LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
            ))
        } else {
            // Portrait keeps a 4:3 camera viewport (the physical preview buffer is
            // rotated into portrait by configurePreviewTransform) and reserves a
            // compact control deck underneath. No preview stretching.
            root.addView(header)
            root.addView(space(6))
            root.addView(
                previewPane,
                LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f),
            )
            root.addView(space(6))
            root.addView(telemetry)
            root.addView(space(2))
            root.addView(status)
            root.addView(space(5))
            root.addView(shutter)
            root.addView(space(4))
            root.addView(sourceNote)
        }
        return root
    }

    private fun buildResearchCameraUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.rgb(10, 12, 15))
        }
        applySafeSystemInsets(root)
        root.addView(LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            addView(label("‹", 34f, false, Color.WHITE).apply {
                gravity = Gravity.CENTER
                contentDescription = "Terug"
                setOnClickListener { finish() }
            }, LinearLayout.LayoutParams(dp(46), dp(46)).apply { marginEnd = dp(6) })
            addView(label("TruthRaw · Android 17 · v0.14 route replay v0.53", 22f, true),
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        })
        root.addView(label(
            "Android-16 v0.14 route exact opnieuw: logical 0 → physical 5 → MAX output → physical-only MAX request → RAW eerst verzegelen.",
            11f,
            false,
            Color.rgb(184, 191, 202),
        ))
        root.addView(space(6))
        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        root.addView(space(6))
        root.addView(telemetry)
        root.addView(space(4))
        root.addView(status)
        root.addView(space(6))
        root.addView(capabilityButton)
        root.addView(previewButton)
        root.addView(captureButton)
        root.addView(saveRawButton)
        root.addView(saveDngButton)
        root.addView(saveJsonButton)
        root.addView(label(
            "Donkere preview is geen blokkade. Stage 3 PASS vereist 16320×12288 RAW_SENSOR + physical Camera-5 result + timestampidentiteit. Returned SENSOR_PIXEL_MODE wordt pas ná sealing geïnterpreteerd.",
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
        setStatus("Stap 1 bezig · alle vier RAW capability-routes lezen…")

        Thread({
            val result = runCatching {
                val m = getSystemService(CameraManager::class.java)
                val logical = m.getCameraCharacteristics(LOGICAL_ID)
                require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
                    "logical 0 meldt physical 5 niet; physicalIds=${logical.physicalCameraIds}"
                }
                val physical = m.getCameraCharacteristics(PHYSICAL_ID)
                val standard = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val maximum = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)

                val standardOutput = safeSizes { standard?.getOutputSizes(ImageFormat.RAW10) }
                val standardHigh = safeSizes { standard?.getHighResolutionOutputSizes(ImageFormat.RAW10) }
                val maximumOutput = safeSizes { maximum?.getOutputSizes(ImageFormat.RAW10) }
                val maximumHigh = safeSizes { maximum?.getHighResolutionOutputSizes(ImageFormat.RAW10) }

                val selected = when {
                    containsTarget(maximumHigh) -> "MAXIMUM_MAP_HIGH_RESOLUTION"
                    containsTarget(maximumOutput) -> "MAXIMUM_MAP_OUTPUT"
                    containsTarget(standardHigh) -> "STANDARD_MAP_HIGH_RESOLUTION"
                    containsTarget(standardOutput) -> "STANDARD_MAP_OUTPUT"
                    else -> error(
                        "8160×6144 RAW10 ontbreekt; standard.out=[$standardOutput] standard.high=[$standardHigh] " +
                            "maximum.out=[$maximumOutput] maximum.high=[$maximumHigh]",
                    )
                }
                Triple(m, logical, physical) to RawRoutes(standardOutput, standardHigh, maximumOutput, maximumHigh, selected)
            }

            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { packed ->
                    manager = packed.first.first
                    logicalCharacteristics = packed.first.second
                    physical5Characteristics = packed.first.third
                    capabilitySource = packed.second.selectedSource
                    capabilityReady = true
                    previewButton.isEnabled = preview.isAvailable
                    val r = packed.second
                    setStatus(
                        "STAGE 1 PASS · exact 8160×6144 RAW10 route aangeboden via ${r.selectedSource}.\n" +
                            "standard.out=[${routeText(r.standardOutput)}]\n" +
                            "standard.high=[${routeText(r.standardHigh)}]\n" +
                            "maximum.out=[${routeText(r.maximumOutput)}]\n" +
                            "maximum.high=[${routeText(r.maximumHigh)}]\n" +
                            "Dit is route-capability, niet automatisch 200MP Direct-CFA authority.",
                    )
                    if (autoStartPreviewWhenReady && preview.isAvailable) {
                        autoStartPreviewWhenReady = false
                        startLogicalPreview()
                    }
                }.onFailure { e ->
                    capabilityReady = false
                    capabilitySource = null
                    setStatus("STAGE 1 BLOCKED · ${e.javaClass.simpleName}: ${e.message}\nGeen camera geopend.")
                }
            }
        }, "truthraw-v011-capability").start()
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
        previewFrames = 0
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
        previewBufferSize = chosen
        val landscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        if (landscape) {
            preview.setAspectRatio(chosen.width, chosen.height)
        } else {
            preview.setAspectRatio(chosen.height, chosen.width)
        }
        val texture = preview.surfaceTexture ?: return
        texture.setDefaultBufferSize(chosen.width, chosen.height)
        preview.post { configurePreviewTransform(preview.width, preview.height) }
        previewSurface = Surface(texture)

        setStatus(
            "Preview ${chosen.width}×${chosen.height} · " +
                (if (landscape) "landscape" else "portrait") +
                " · aspectratio behouden; Camera-5 route wordt geopend…",
        )
        try {
            @Suppress("MissingPermission")
            m.openCamera(LOGICAL_ID, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    camera = device
                    createPreviewSession(device, logical)
                }
                override fun onDisconnected(device: CameraDevice) {
                    device.close()
                    setStatusAny("STAGE 2 DISCONNECTED · logical 0")
                    closeCameraResources(keepOutputs = true)
                }
                override fun onError(device: CameraDevice, error: Int) {
                    device.close()
                    setStatusAny("STAGE 2 CAMERA ERROR=$error · logical 0")
                    closeCameraResources(keepOutputs = true)
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatus("STAGE 2 OPEN FAIL · ${e.javaClass.simpleName}: ${e.message}")
            previewButton.isEnabled = true
        }
    }

    private fun createPreviewSession(device: CameraDevice, logical: CameraCharacteristics) {
        val surface = previewSurface ?: return
        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(OutputConfiguration(surface)),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    startPreviewRepeating(device, s, logical)
                }
                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("STAGE 2 SESSION FAIL · logical preview-only sessie geweigerd.")
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

    private fun startPreviewRepeating(device: CameraDevice, s: CameraCaptureSession, logical: CameraCharacteristics) {
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
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
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
            setStatusAny("STAGE 2 REQUEST ACTIVE · logical preview-only · zoom request=${requestedZoom ?: "unsupported"}.\nDonker beeld is toegestaan; activePhysical is de route-observatie.")
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
        val logical = logicalCharacteristics ?: return
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

        val reader = runCatching { ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW10, 1) }
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
        val outputSetup = runCatching {
            output.setPhysicalCameraId(PHYSICAL_ID)
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        }
        if (outputSetup.isFailure) {
            setStatus("STAGE 3 physical/MAX output bind FAIL · ${outputSetup.exceptionOrNull()?.message}")
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
                    submitPhysicalScopedStill(device, s, logical, physical)
                }
                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("STAGE 3 RAW10 SESSION BLOCKED · physical-5/MAX 8160×6144 geweigerd.")
                    closeCameraResources(keepOutputs = true)
                    runOnUiThread { previewButton.isEnabled = true }
                }
            },
        )

        val support = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
        setStatus("STAGE 3 · physical-5/MAX session · isSessionConfigurationSupported=$support")
        if (support == false) {
            setStatus("STAGE 3 BLOCKED · Android meldt de exacte 8160×6144 RAW10 sessie unsupported.")
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

    private fun submitPhysicalScopedStill(
        device: CameraDevice,
        s: CameraCaptureSession,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
    ) {
        val reader = rawReader ?: return
        try {
            var scopedError: String? = null
            val requestBuilder = try {
                lastScopedRequestUsed = true
                device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE, setOf(PHYSICAL_ID))
            } catch (t: Throwable) {
                lastScopedRequestUsed = false
                scopedError = "${t.javaClass.simpleName}: ${t.message}"
                device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            }
            lastScopedRequestError = scopedError
            requestBuilder.addTarget(reader.surface)

            // Android-16 v0.14 authority: DO NOT write logical/global SENSOR_PIXEL_MODE.
            // That earlier global write/gate was the v0.11/v0.12 failure mode.
            lastGlobalPixelModeWritten = false

            // Reproduce the successful physical-only v0.12-v0.14 observation exactly:
            // attempt the physical write even when the logical characteristics do not advertise
            // SENSOR_PIXEL_MODE as an available physical override.
            lastPhysicalOverrideAdvertised = physicalOverrideSupported(logical, CaptureRequest.SENSOR_PIXEL_MODE)
            lastPhysicalPixelModeAttempted = true
            lastPhysicalPixelModeWritten = false
            lastPhysicalPixelModeReadback = null
            lastPhysicalPixelModeError = null
            runCatching {
                requestBuilder.setPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                    PHYSICAL_ID,
                )
                lastPhysicalPixelModeWritten = true
                lastPhysicalPixelModeReadback = requestBuilder.getPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    PHYSICAL_ID,
                )
            }.onFailure { e ->
                lastPhysicalPixelModeError = "${e.javaClass.simpleName}: ${e.message}"
            }

            setIfSupported(requestBuilder, CaptureRequest.CONTROL_ENABLE_ZSL, false, logical)
            setIfSupported(requestBuilder, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO, logical)
            setIfSupported(requestBuilder, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON, logical)

            // Keep v0.11 intentionally minimal: acquisition first. Only controls that the logical
            // request advertises are written globally. Physical RAW processing boundaries are
            // measured in the result/evidence rather than assumed from requested OFF values.
            val nr = logical.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                setIfSupported(requestBuilder, CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF, logical)
            }
            val edge = logical.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) {
                setIfSupported(requestBuilder, CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF, logical)
            }
            CameraCalibrationTelemetry.requestLensShadingMap(
                requestBuilder,
                physical5Characteristics,
                PHYSICAL_ID,
            )

            val request = requestBuilder.build()
            setStatusAny(
                "STAGE 3 CAPTURE SENT · v0.14 replay · scopedRequest=$lastScopedRequestUsed · globalMAX=false · " +
                    "physicalMAXAttempted=$lastPhysicalPixelModeAttempted · physicalMAX=$lastPhysicalPixelModeWritten · " +
                    "physicalReadback=${lastPhysicalPixelModeReadback ?: "null"} · advertised=$lastPhysicalOverrideAdvertised" +
                    (lastPhysicalPixelModeError?.let { "\nphysicalMAX write/readback error=$it" } ?: "") +
                    (lastScopedRequestError?.let { "\nscoped fallback reason=$it" } ?: "") +
                    "\nWachten op RAW Image + physical Camera-5 TotalCaptureResult…",
            )

            s.capture(request, object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }

                override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: CaptureFailure) {
                    setStatusAny(
                        "STAGE 3 CAPTURE FAIL · reason=${failure.reason} · wasImageCaptured=${failure.wasImageCaptured()} · " +
                            "sequenceId=${failure.sequenceId} · frameNumber=${failure.frameNumber}\n" +
                            "scopedRequest=$lastScopedRequestUsed globalMAX=false physicalMAXAttempted=$lastPhysicalPixelModeAttempted physicalMAX=$lastPhysicalPixelModeWritten" +
                            (lastScopedRequestError?.let { "\nscopedFallback=$it" } ?: ""),
                    )
                    runOnUiThread {
                        previewButton.isEnabled = true
                        captureButton.isEnabled = false
                    }
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatusAny("STAGE 3 request FAIL · ${e.javaClass.simpleName}: ${e.message}")
            runOnUiThread { previewButton.isEnabled = true }
        }
    }

    private fun finalizeIfPaired() {
        val image: Image
        val result: TotalCaptureResult
        synchronized(pairLock) {
            if (finalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            finalizing = true
            pendingImage = null
            pendingResult = null
        }
        cameraHandler.post { finalizeCapture(image, result) }
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
            val plane = image.planes.singleOrNull() ?: error("RAW10 planeCount=${image.planes.size}, exact 1 vereist")
            val packedRowBytes = (TARGET_W * 10 + 7) / 8
            require(plane.rowStride >= packedRowBytes) {
                "RAW10 rowStride=${plane.rowStride} te klein voor packedRowBytes=$packedRowBytes"
            }

            // v0.14 source-first rule: seal the primary buffer BEFORE interpreting advisory
            // result metadata such as SENSOR_PIXEL_MODE. The Android-16 qualifying run returned 0.
            val stamp = System.currentTimeMillis()
            val rawEvidence = persistOriginalRawBuffer(image, stamp)
            capturedRaw = rawEvidence.file

            // v0.57: publish sealed packed RAW10 bytes immediately, before any interpretation.
            val sourceExport = runCatching {
                exportResearchFileToDownloadsTruthRaw(rawEvidence.file, "application/octet-stream")
            }.fold(
                onSuccess = { "SOURCE EXPORTED ✓ · $it" },
                onFailure = { "SOURCE EXPORT FAIL · ${it.javaClass.simpleName}: ${it.message}" },
            )
            setStatusAny(sourceExport)

            val returnedPixelMode = physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE)
            val physical = physical5Characteristics ?: error("physical characteristics ontbreken")
            // Packed RAW10 is sealed as byte evidence only. DngCreator/U16 admission is intentionally
            // not attempted in this experiment, so packed bytes can never be misread as RAW_SENSOR U16.
            val dng: File? = null
            val dngSha: String? = null
            val dngError: String? = "NOT_ATTEMPTED_PACKED_RAW10_EXPERIMENT"

            val evidence = buildEvidence(logicalResult, physicalResult, image, rawEvidence, dng, dngSha, dngError)
            evidence.put("v057SourceExport", sourceExport)
            image.close()

            val topologyAdmission = JSONObject()
                .put("status", "RAW10_SEALED_ANALYSIS_ONLY_NO_U16_ADMISSION")
                .put("sourceFormat", "RAW10")
                .put("sourceEnvelopeWidth", TARGET_W)
                .put("sourceEnvelopeHeight", TARGET_H)
                .put("sourceEnvelopeSha256", rawEvidence.sha256)
                .put("scientificMasterCreationAllowed", false)
                .put("truthNegativeAllowed", false)
                .put("reason", "Packed RAW10 must be decoded and audited separately before any geometry or Scientific Master admission.")
            evidence.put("topologyAdmission", topologyAdmission)
            admittedProcessingDng = null

            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_8160_RAW10_EVIDENCE_v057.json")
            report.writeText(evidence.toString(2))
            capturedDng = dng
            capturedJson = report

            val evidenceExport = runCatching {
                exportResearchFileToDownloadsTruthRaw(report, "application/json")
            }.fold(
                onSuccess = { "EVIDENCE EXPORTED ✓ · $it" },
                onFailure = { "EVIDENCE EXPORT FAIL · ${it.javaClass.simpleName}: ${it.message}" },
            )

            val admitted = admittedProcessingDng
            setStatusAny(
                if (admitted != null) {
                    "CAMERA CAPTURE PASS · physical 5 envelope 16320×12288 eerst verzegeld.\n" +
                        "Topology-admission heeft een exacte advertised RAW sample-domain toegelaten; " +
                        "${topologyAdmission.optInt("admittedWidth")}×${topologyAdmission.optInt("admittedHeight")} " +
                        "wordt als derived processing-DNG opnieuw door Main House admitted.\n" +
                        "De envelope zelf wordt NIET als 200MP Scientific Master gepromoveerd."
                } else {
                    "RAW10 CAPTURE SEALED · physical 5 envelope 8160×6144 bewaard.\n" +
                        "Packed RAW10 blijft analysis-only; geen U16/DNG/Main House-admission.\n" +
                        sourceExport + "\n" + evidenceExport
                },
            )
            runOnUiThread {
                saveRawButton.isEnabled = true
                saveDngButton.isEnabled = if (productionCameraEntry) admitted != null else capturedDng != null
                saveJsonButton.isEnabled = true
                previewButton.isEnabled = true

                if (productionCameraEntry && admitted != null) {
                    startActivity(
                        Intent(this, MainActivity::class.java).apply {
                            putExtra(MainActivity.EXTRA_INTERNAL_CAMERA_SOURCE_PATH, admitted.absolutePath)
                            putExtra(MainActivity.EXTRA_INTERNAL_CAMERA_EVIDENCE_PATH, report.absolutePath)
                            putExtra(MainActivity.EXTRA_INTERNAL_CAMERA_UPSTREAM_SHA256, rawEvidence.sha256)
                            putExtra(MainActivity.EXTRA_AUTO_START_TRUTHRAW, true)
                            addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP)
                        },
                    )
                }
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

    private fun persistOriginalRawBuffer(image: Image, stamp: Long): RawEvidence {
        val plane = image.planes.single()
        val source = plane.buffer.duplicate().apply { rewind() }
        val accessible = source.remaining().toLong()
        val expected = TARGET_SAMPLES * 10L / 8L
        val contiguous = plane.rowStride == (TARGET_W * 10 / 8) && accessible == expected
        val file = File(
            cacheDir,
            "TRUTHRAW_${stamp}_CAM5_8160_RAW10_SOURCE_${TARGET_W}x${TARGET_H}_v057.${if (contiguous) "raw10" else "raw10buffer"}",
        )
        val md = MessageDigest.getInstance("SHA-256")
        FileOutputStream(file).channel.use { channel ->
            val buf = plane.buffer.duplicate().apply { rewind() }
            val scratch = ByteArray(1024 * 1024)
            while (buf.hasRemaining()) {
                val n = minOf(buf.remaining(), scratch.size)
                buf.get(scratch, 0, n)
                md.update(scratch, 0, n)
                channel.write(java.nio.ByteBuffer.wrap(scratch, 0, n))
            }
            channel.force(true)
        }
        val hash = md.digest().joinToString("") { "%02x".format(it) }
        return RawEvidence(file, hash, file.length(), plane.rowStride, plane.pixelStride, accessible, contiguous)
    }

    private fun buildEvidence(
        logicalResult: TotalCaptureResult,
        physicalResult: CaptureResult,
        image: Image,
        raw: RawEvidence,
        dng: File?,
        dngSha: String?,
        dngError: String?,
    ): JSONObject {
        return JSONObject()
            .put("schema", "truthraw.fotograaf-camera5-8160x6144-raw10-probe.v0.57")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("capability", JSONObject()
                .put("discoverySource", capabilitySource ?: JSONObject.NULL)
                .put("targetWidth", TARGET_W)
                .put("targetHeight", TARGET_H)\n                .put("imageFormat", "RAW10")\n                .put("bitsPerSamplePacked", 10))
            .put("preview", JSONObject()
                .put("logicalCameraId", LOGICAL_ID)
                .put("requestedZoomRatio", ZOOM_REQUEST)
                .put("lastActivePhysicalId", lastActivePhysicalId ?: JSONObject.NULL)
                .put("previewCreatesEvidence", false))
            .put("requestTopology", JSONObject()
                .put("openedCameraId", LOGICAL_ID)
                .put("requestedPhysicalCameraId", PHYSICAL_ID)
                .put("physicalScopedRequestUsed", lastScopedRequestUsed)
                .put("physicalScopedRequestError", lastScopedRequestError ?: JSONObject.NULL)
                .put("globalSensorPixelModeWritten", false)
                .put("globalSensorPixelModeIntentionallySuppressedByV014Replay", true)
                .put("physicalSensorPixelModeAttempted", lastPhysicalPixelModeAttempted)
                .put("physicalSensorPixelModeWritten", lastPhysicalPixelModeWritten)
                .put("physicalSensorPixelModeReadback", lastPhysicalPixelModeReadback ?: JSONObject.NULL)
                .put("physicalSensorPixelModeWriteError", lastPhysicalPixelModeError ?: JSONObject.NULL)
                .put("physicalOverrideAdvertised", lastPhysicalOverrideAdvertised)
                .put("outputPhysicalBinding", true)
                .put("outputMaximumResolutionModeDeclared", true))
            .put("captureRoute", JSONObject()
                .put("reportedPhysicalIds", JSONArray(logicalResult.physicalCameraResults.keys.sorted()))
                .put("physicalResultCameraId", physicalResult.cameraId)
                .put("width", image.width)
                .put("height", image.height)
                .put("sampleCount", image.width.toLong() * image.height.toLong())
                .put("captureResultSensorPixelMode", physicalResult.get(CaptureResult.SENSOR_PIXEL_MODE) ?: JSONObject.NULL)
                .put("captureResultSensorPixelModeIsAdvisoryAfterSeal", true))
            .put("captureResult", JSONObject()
                .put("sensorTimestampNs", physicalResult.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("imageTimestampNs", image.timestamp)
                .put("timestampIdentityPass", physicalResult.get(CaptureResult.SENSOR_TIMESTAMP) == image.timestamp)
                .put("iso", physicalResult.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", physicalResult.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("frameDurationNs", physicalResult.get(CaptureResult.SENSOR_FRAME_DURATION) ?: JSONObject.NULL)
                .put("focalLengthMm", physicalResult.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("focusDistanceDiopters", physicalResult.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                .put("noiseReductionMode", physicalResult.get(CaptureResult.NOISE_REDUCTION_MODE) ?: JSONObject.NULL)
                .put("edgeMode", physicalResult.get(CaptureResult.EDGE_MODE) ?: JSONObject.NULL)
                .put("rawBinningFactorUsed", physicalResult.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED) ?: JSONObject.NULL))
            .put(
                "cameraCalibrationTelemetry",
                CameraCalibrationTelemetry.toJson(
                    physicalResult,
                    physical5Characteristics,
                ),
            )
            .put("rawPayload", JSONObject()
                .put("file", raw.file.name)
                .put("sha256", raw.sha256)
                .put("bytes", raw.bytes)
                .put("accessibleBufferBytes", raw.accessibleBytes)
                .put("rowStride", raw.rowStride)
                .put("pixelStride", raw.pixelStride)
                .put("imageFormat", "RAW10")
                .put("packing", "ANDROID_IMAGEFORMAT_RAW10_PACKED")
                .put("canonicalContiguousRaw10", raw.contiguous)
                .put("expectedContiguousBytes", TARGET_SAMPLES * 10L / 8L))
            .put("dng", JSONObject()
                .put("attempted", false)
                .put("file", dng?.name ?: JSONObject.NULL)
                .put("bytes", dng?.length() ?: JSONObject.NULL)
                .put("sha256", dngSha ?: JSONObject.NULL)
                .put("error", dngError ?: JSONObject.NULL)
                .put("semantics", "Not attempted for packed RAW10; sealed Image.Plane bytes are the primary evidence."))
            .put("sourceFirstOrdering", "IMAGE_DIMENSIONS -> PHYSICAL5_RESULT -> TIMESTAMP_IDENTITY -> RAW10_PLANE_LAYOUT -> RAW10_SEAL_SHA256 -> ADVISORY_METADATA -> EXPORT")
            .put("android16V014Reference", JSONObject()
                .put("globalSensorPixelModeWritten", false)
                .put("physicalSensorPixelModeWritten", true)
                .put("physicalSensorPixelModeReadback", CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
                .put("physicalOverrideAdvertised", false)
                .put("returnedPhysicalSensorPixelMode", 0)
                .put("qualifyingRawBytes", 401080320L)
                .put("qualifyingRawSha256", "af3ad73e5919b816881a661f00ffd84a7b537f23198a5877c242717c5d7526de"))
            .put("boundary", "APP_VISIBLE_CAMERA2_RAW10_PACKED_BYTES_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF")
    }

    private fun buildTopologyAdmissionAndProcessingDng(
        rawEvidence: RawEvidence,
        physical: CameraCharacteristics,
        physicalResult: CaptureResult,
        stamp: Long,
    ): JSONObject {
        val standardMap = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val standardRawSizes = safeSizes { standardMap?.getOutputSizes(ImageFormat.RAW_SENSOR) }
        require(standardRawSizes.isNotEmpty()) {
            "physical Camera-5 exposes no STANDARD RAW_SENSOR sizes for topology admission"
        }

        val audit = RawSensorRasterAudit.audit(
            file = rawEvidence.file,
            width = TARGET_W,
            height = TARGET_H,
            pixelBytes = 2,
            expectedSealedSha256 = rawEvidence.sha256,
        )
        require(audit.optBoolean("sealedSha256IdentityPass", false)) {
            "sealed RAW_SENSOR SHA-256 changed during read-only topology audit"
        }

        val candidate = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_ADMITTED_candidate.rawpayload")
        val preview = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_ADMITTED_diagnostic.png")
        val decoded = RawPayloadGeometryDecoder.decode(
            source = rawEvidence.file,
            rasterAudit = audit,
            declaredWidth = TARGET_W,
            declaredHeight = TARGET_H,
            advertisedStandardRawSizes = standardRawSizes,
            candidatePayloadFile = candidate,
            diagnosticPreviewFile = preview,
        )
        require(decoded.optString("status") == "UNIQUE_ADVERTISED_STANDARD_RAW_BYTE_MATCH_DECODED") {
            "payload topology is not a unique advertised STANDARD RAW_SENSOR byte match: ${decoded.optString("status")}"
        }

        val selected = decoded.getJSONObject("selectedCandidate")
        val width = selected.getInt("width")
        val height = selected.getInt("height")
        val bytes = selected.getLong("bytesU16")
        require(bytes == candidate.length()) { "candidate payload length mismatch" }
        require(standardRawSizes.any { it.width == width && it.height == height }) {
            "selected sample-domain is not an advertised STANDARD RAW_SENSOR size"
        }

        val processingDng = File(
            cacheDir,
            "TRUTHRAW_${stamp}_CAM5_ADMITTED_${width}x${height}_source.dng",
        )
        FileOutputStream(processingDng, false).use { out ->
            DngCreator(physical, physicalResult).use { creator ->
                creator.setOrientation(1)
                creator.setDescription(
                    "TruthRaw v0.73 camera ingress; exact prefix bytes from sealed app-visible RAW_SENSOR; " +
                        "upstream_sha256=${rawEvidence.sha256}; geometry=${width}x${height}; " +
                        "no claim of native ADC geometry or 200MP optical independence.",
                )
                FileInputStream(candidate).use { input ->
                    creator.writeInputStream(out, Size(width, height), input, 0L)
                }
            }
            out.fd.sync()
        }
        require(processingDng.isFile && processingDng.length() > 0L) {
            "admitted processing DNG was not written"
        }

        val dngSha = sha256File(processingDng)
        val payload = decoded.getJSONObject("candidatePayload")
        return JSONObject()
            .put("status", "ADMITTED_EXACT_STANDARD_RAW_PREFIX_TO_DERIVED_DNG")
            .put("sourceEnvelopeWidth", TARGET_W)
            .put("sourceEnvelopeHeight", TARGET_H)
            .put("sourceEnvelopeSha256", rawEvidence.sha256)
            .put("sourceEnvelopePromotedTo200MpScientificMaster", false)
            .put("admittedWidth", width)
            .put("admittedHeight", height)
            .put("admittedPayloadBytes", bytes)
            .put("admittedPayloadSha256", payload.getString("sha256"))
            .put("copyTransform", payload.optString("copyTransform"))
            .put("geometryAuthority", "EXACT_BYTE_COUNT_MATCH_TO_SINGLE_ADVERTISED_STANDARD_RAW_SENSOR_SIZE")
            .put("nativeAdcGeometryProven", false)
            .put("optical200MpIndependenceProven", false)
            .put("processingDngFile", processingDng.name)
            .put("processingDngBytes", processingDng.length())
            .put("processingDngSha256", dngSha)
            .put("processingDngRole", "DERIVED_CONTAINER_FOR_SAME_MAIN_HOUSE_DNG_ADMISSION")
            .put("mainHouseMustResealDng", true)
            .put("scientificMasterCreationAllowed", true)
            .put("truthNegativeAllowedOnlyAfterMainHouseAdmission", true)
            .put("rasterAudit", audit)
            .put("payloadGeometryDecoder", decoded)
    }

    private fun safeSizes(block: () -> Array<Size>?): List<Size> =
        try { block()?.toList().orEmpty() } catch (_: Throwable) { emptyList() }

    private fun containsTarget(sizes: List<Size>): Boolean = sizes.any { it.width == TARGET_W && it.height == TARGET_H }
    private fun routeText(sizes: List<Size>): String = sizes.joinToString { "${it.width}×${it.height}" }

    private fun physicalOverrideSupported(c: CameraCharacteristics, key: CaptureRequest.Key<*>): Boolean =
        try { c.availablePhysicalCameraRequestKeys?.any { it.name == key.name } == true } catch (_: Throwable) { false }

    private fun <T> setIfSupported(builder: CaptureRequest.Builder, key: CaptureRequest.Key<T>, value: T, c: CameraCharacteristics) {
        try {
            if (c.availableCaptureRequestKeys?.contains(key) == true) builder.set(key, value)
        } catch (_: Throwable) {}
    }

    private fun closeCameraResources(keepOutputs: Boolean) {
        runCatching { session?.stopRepeating() }
        runCatching { session?.close() }
        session = null
        runCatching { rawReader?.close() }
        rawReader = null
        runCatching { camera?.close() }
        camera = null
        runCatching { previewSurface?.release() }
        previewSurface = null
        if (!keepOutputs) clearOutputs()
        if (::captureButton.isInitialized) captureButton.isEnabled = false
        if (::previewButton.isInitialized) previewButton.isEnabled = capabilityReady && ::preview.isInitialized && preview.isAvailable
    }

    private fun clearOutputs() {
        capturedRaw = null
        capturedDng = null
        capturedJson = null
        admittedProcessingDng = null
        if (::saveRawButton.isInitialized) saveRawButton.isEnabled = false
        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false
        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false
    }

    private fun exportResearchFileToDownloadsTruthRaw(file: File, mime: String): String {
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, file.name)
            put(MediaStore.MediaColumns.MIME_TYPE, mime)
            put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS + "/TRUTHRAW")
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }
        val collection = MediaStore.Downloads.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY)
        val uri = contentResolver.insert(collection, values)
            ?: error("Downloads MediaStore insert returned null")
        try {
            contentResolver.openOutputStream(uri, "w")?.use { out ->
                FileInputStream(file).use { input -> input.copyTo(out, 1024 * 1024) }
            } ?: error("Downloads MediaStore output stream unavailable")
            contentResolver.update(
                uri,
                ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) },
                null,
                null,
            )
        } catch (t: Throwable) {
            runCatching { contentResolver.delete(uri, null, null) }
            throw t
        }
        return "Downloads/TRUTHRAW/${file.name}"
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
            REQUEST_SAVE_DNG -> if (productionCameraEntry) admittedProcessingDng ?: capturedDng else capturedDng
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

    private fun applySafeSystemInsets(root: View) {
        val horizontal = dp(12)
        val top = dp(8)
        val bottom = dp(10)
        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(
                horizontal + bars.left,
                top + bars.top,
                horizontal + bars.right,
                bottom + bars.bottom,
            )
            insets
        }
        root.requestApplyInsets()
    }

    private fun shutterButton(action: () -> Unit): Button = Button(this).apply {
        text = "●"
        textSize = 38f
        setTextColor(Color.rgb(18, 22, 27))
        isAllCaps = false
        contentDescription = "Maak RAW-opname"
        minWidth = 0
        minHeight = 0
        minimumWidth = 0
        minimumHeight = 0
        setPadding(0, 0, 0, dp(3))
        background = StateListDrawable().apply {
            addState(
                intArrayOf(android.R.attr.state_enabled),
                GradientDrawable().apply {
                    shape = GradientDrawable.OVAL
                    setColor(Color.WHITE)
                    setStroke(dp(5), Color.rgb(88, 217, 210))
                },
            )
            addState(
                intArrayOf(),
                GradientDrawable().apply {
                    shape = GradientDrawable.OVAL
                    setColor(Color.rgb(92, 96, 102))
                    setStroke(dp(4), Color.rgb(139, 145, 153))
                },
            )
        }
        elevation = dp(5).toFloat()
        setOnClickListener { action() }
    }

    private fun shutterPanel(landscape: Boolean): View = LinearLayout(this).apply {
        orientation = if (landscape) LinearLayout.VERTICAL else LinearLayout.HORIZONTAL
        gravity = Gravity.CENTER
        addView(
            captureButton,
            LinearLayout.LayoutParams(dp(if (landscape) 78 else 86), dp(if (landscape) 78 else 86)),
        )
        if (landscape) {
            addView(space(3))
        } else {
            addView(View(this@FotoGraaf200MpStagedActivity), LinearLayout.LayoutParams(dp(12), 1))
        }
        addView(label("MAAK RAW", if (landscape) 10.5f else 12f, true, Color.rgb(235, 240, 247)).apply {
            gravity = Gravity.CENTER
        })
    }

    private fun configurePreviewTransform(viewWidth: Int, viewHeight: Int) {
        val size = previewBufferSize ?: return
        if (viewWidth <= 0 || viewHeight <= 0) return

        val rotation = preview.display?.rotation ?: Surface.ROTATION_0
        val matrix = Matrix()
        val viewRect = RectF(0f, 0f, viewWidth.toFloat(), viewHeight.toFloat())
        val bufferRect = RectF(0f, 0f, size.height.toFloat(), size.width.toFloat())
        val centerX = viewRect.centerX()
        val centerY = viewRect.centerY()

        if (rotation == Surface.ROTATION_90 || rotation == Surface.ROTATION_270) {
            bufferRect.offset(
                centerX - bufferRect.centerX(),
                centerY - bufferRect.centerY(),
            )
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
        preview.setTransform(matrix)
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
        if (previewBufferSize != null) configurePreviewTransform(width, height)
        if (capabilityReady && autoStartPreviewWhenReady) {
            autoStartPreviewWhenReady = false
            startLogicalPreview()
        }
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) {
        configurePreviewTransform(width, height)
    }
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        closeCameraResources(keepOutputs = true)
        return true
    }
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit

    companion object {
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val TARGET_W = 8160
        private const val TARGET_H = 6144
        private const val TARGET_SAMPLES = 50_135_040L
        private const val ZOOM_REQUEST = 3.7f
        private const val REQUEST_CAMERA = 5800
        private const val REQUEST_SAVE_RAW = 5801
        private const val REQUEST_SAVE_DNG = 5802
        private const val REQUEST_SAVE_JSON = 5803

        const val EXTRA_PRODUCTION_CAMERA_ENTRY =
            "truthraw.extra.PRODUCTION_CAMERA_ENTRY"
    }
}
package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.os.Bundle
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.Spinner
import android.widget.TextView

/**
 * FotoGraaf v0.4.2 staged startup.
 *
 * Phase A performs discovery only and never opens a camera.
 * Phase B opens a preview-only session after an explicit user action.
 * RAW ImageReader/session wiring is intentionally excluded from this bootstrap
 * so a failing preview route can be diagnosed without taking the whole Activity down.
 * This class grants no calibration/scientific authority.
 */
class FotoGraafSafePreviewActivity : Activity(), TextureView.SurfaceTextureListener {

    private lateinit var manager: CameraManager
    private lateinit var preview: TextureView
    private lateinit var routeSpinner: Spinner
    private lateinit var startButton: Button
    private lateinit var stopButton: Button
    private lateinit var status: TextView
    private lateinit var telemetry: TextView

    private var routes: List<HonorRawRoute> = emptyList()
    private var selectedRoute: HonorRawRoute? = null
    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var opening = false
    private var frameCount = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        manager = getSystemService(CameraManager::class.java)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        discoverOnly()
    }

    override fun onPause() {
        closePreview("Preview gesloten omdat FotoGraaf niet voorgrond is.")
        super.onPause()
    }

    override fun onDestroy() {
        closePreview(null)
        super.onDestroy()
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(14), dp(14), dp(14), dp(24))
            setBackgroundColor(Color.rgb(12, 14, 17))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
                view.setPadding(dp(14) + bars.left, dp(14) + bars.top, dp(14) + bars.right, dp(24) + bars.bottom)
                insets
            }
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

        root.addView(label("FotoGraaf · Safe Live Preview", 24f, true))
        root.addView(label("v0.4.2 · fase A discovery → fase B preview-only. Geen RAW-buffer tijdens startup.", 12f, false, Color.LTGRAY))
        root.addView(space(8))

        preview = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraafSafePreviewActivity
            setBackgroundColor(Color.BLACK)
        }
        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(360)))
        root.addView(space(8))

        routeSpinner = Spinner(this)
        root.addView(routeSpinner)

        startButton = button("Start preview van geselecteerde route") { startSelectedPreview() }.apply { isEnabled = false }
        stopButton = button("Stop preview") { closePreview("Preview handmatig gestopt.") }.apply { isEnabled = false }
        root.addView(startButton)
        root.addView(space(4))
        root.addView(stopButton)
        root.addView(space(8))

        telemetry = label("Nog geen preview-resultaat.", 12f, true)
        status = label("Discovery start…", 12f, false, Color.LTGRAY)
        root.addView(telemetry)
        root.addView(space(6))
        root.addView(status)
        root.addView(space(10))

        root.addView(button("Open bewezen RAW route-capture") {
            closePreview(null)
            startActivity(Intent(this, FotoGraafCameraActivity::class.java))
        })
        root.addView(space(4))
        root.addView(button("Open TruthRaw processor") {
            closePreview(null)
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space(8))
        root.addView(label(
            "Authority: CAPABILITY/PREVIEW_OBSERVATION_ONLY. Preview is geen extra evidence; alleen sealed single-frame RAW + exact CaptureResult kan capture-evidence zijn.",
            10f,
            false,
            Color.GRAY,
        ))
        return scroll
    }

    private fun discoverOnly() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            show("CAMERA permission ontbreekt. Ga terug naar de permission gate.")
            return
        }
        startButton.isEnabled = false
        show("Fase A: Camera2/HONOR discovery; camera blijft gesloten…")
        Thread({
            val result = runCatching { HonorCameraProbe.scan(manager) }
            runOnUiThread {
                result.onSuccess { report ->
                    routes = report.routes
                    val preferred = routes.indexOfFirst {
                        it.routeClass == "LOGICAL_MULTI_CAMERA_FORCED_PHYSICAL_OUTPUT" && it.physicalCameraId == "5"
                    }.let { if (it >= 0) it else 0 }
                    routeSpinner.adapter = ArrayAdapter(
                        this,
                        android.R.layout.simple_spinner_dropdown_item,
                        routes,
                    )
                    routeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                        override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                            selectedRoute = routes.getOrNull(position)
                            closePreview(null)
                            startButton.isEnabled = selectedRoute != null && preview.isAvailable
                            selectedRoute?.let { route ->
                                show("Route geselecteerd maar camera nog dicht: ${route.label}\nDruk expliciet op Start preview.")
                            }
                        }
                        override fun onNothingSelected(parent: AdapterView<*>?) = Unit
                    }
                    if (routes.isNotEmpty()) {
                        routeSpinner.setSelection(preferred)
                        selectedRoute = routes[preferred]
                    }
                    startButton.isEnabled = routes.isNotEmpty() && preview.isAvailable
                    show(
                        "Fase A klaar. rawIDs=${report.rawCameraIds}; physical=${report.discoveredPhysicalIds}; routes=${routes.size}.\n" +
                            "Geen camera geopend tijdens discovery."
                    )
                }.onFailure { error ->
                    show("Discovery faalde maar Activity bleef actief: ${error.javaClass.simpleName}: ${error.message}")
                }
            }
        }, "truthraw-safe-discovery").start()
    }

    private fun startSelectedPreview() {
        val route = selectedRoute ?: run {
            show("Geen route geselecteerd.")
            return
        }
        if (!preview.isAvailable) {
            show("Preview surface is nog niet beschikbaar.")
            return
        }
        if (opening || camera != null) {
            show("Preview is al bezig met openen of actief.")
            return
        }
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            show("CAMERA permission ontbreekt.")
            return
        }

        closePreview(null)
        opening = true
        startButton.isEnabled = false
        show("Fase B: alleen preview openen voor ${route.label}…")

        val texture = preview.surfaceTexture ?: run {
            opening = false
            startButton.isEnabled = true
            show("SurfaceTexture ontbreekt; niets geopend.")
            return
        }
        val size = chooseSafePreviewSize(route)
        texture.setDefaultBufferSize(size.first, size.second)
        val surface = Surface(texture)
        previewSurface = surface

        runCatching {
            manager.openCamera(route.logicalCameraId, mainExecutor, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    opening = false
                    camera = device
                    createPreviewSession(device, route, surface)
                }

                override fun onDisconnected(device: CameraDevice) {
                    opening = false
                    device.close()
                    camera = null
                    startButton.isEnabled = true
                    stopButton.isEnabled = false
                    show("Camera disconnected. Activity bleef actief.")
                }

                override fun onError(device: CameraDevice, error: Int) {
                    opening = false
                    device.close()
                    camera = null
                    startButton.isEnabled = true
                    stopButton.isEnabled = false
                    show("Camera open error=$error. Activity bleef actief.")
                }
            })
        }.onFailure { error ->
            opening = false
            runCatching { surface.release() }
            previewSurface = null
            startButton.isEnabled = true
            show("openCamera exception gevangen: ${error.javaClass.simpleName}: ${error.message}")
        }
    }

    private fun createPreviewSession(device: CameraDevice, route: HonorRawRoute, surface: Surface) {
        val output = OutputConfiguration(surface)
        val physical = route.physicalCameraId
        if (physical != null) {
            val bind = runCatching { output.setPhysicalCameraId(physical) }
            if (bind.isFailure) {
                show("Physical preview-binding $physical geweigerd: ${bind.exceptionOrNull()?.javaClass?.simpleName}: ${bind.exceptionOrNull()?.message}")
                closePreview(null)
                return
            }
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(configured: CameraCaptureSession) {
                    session = configured
                    frameCount = 0
                    val request = runCatching {
                        device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW).apply {
                            addTarget(surface)
                            set(CaptureRequest.CONTROL_MODE, CaptureRequest.CONTROL_MODE_AUTO)
                            set(CaptureRequest.CONTROL_AE_MODE, CaptureRequest.CONTROL_AE_MODE_ON)
                            set(CaptureRequest.CONTROL_AF_MODE, CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
                        }.build()
                    }
                    if (request.isFailure) {
                        show("Preview-request kon niet worden gebouwd: ${request.exceptionOrNull()?.javaClass?.simpleName}: ${request.exceptionOrNull()?.message}")
                        closePreview(null)
                        return
                    }
                    runCatching {
                        configured.setRepeatingRequest(request.getOrThrow(), mainExecutor, object : CameraCaptureSession.CaptureCallback() {
                            override fun onCaptureCompleted(
                                session: CameraCaptureSession,
                                request: CaptureRequest,
                                result: TotalCaptureResult,
                            ) {
                                frameCount++
                                val effective: CaptureResult = route.physicalCameraId
                                    ?.let { result.physicalCameraResults[it] }
                                    ?: result
                                val af = effective.get(CaptureResult.CONTROL_AF_STATE)
                                val ae = effective.get(CaptureResult.CONTROL_AE_STATE)
                                val ois = effective.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE)
                                val iso = effective.get(CaptureResult.SENSOR_SENSITIVITY)
                                val shutter = effective.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                                val focal = effective.get(CaptureResult.LENS_FOCAL_LENGTH)
                                telemetry.text = "frames=$frameCount · physical=${route.physicalCameraId ?: "logical"} · AF=$af · AE=$ae · OIS=$ois\nISO=$iso · shutter=${shutter ?: "?"} ns · focal=${focal ?: "?"} mm"
                            }
                        })
                    }.onFailure { error ->
                        show("setRepeatingRequest exception gevangen: ${error.javaClass.simpleName}: ${error.message}")
                        closePreview(null)
                    }
                    stopButton.isEnabled = true
                    show("Preview-only sessie actief. RAW-buffer is bewust nog NIET gekoppeld.")
                }

                override fun onConfigureFailed(failed: CameraCaptureSession) {
                    show("Preview-only session onConfigureFailed. Activity bleef actief; route/combinatie is niet bruikbaar als live preview.")
                    closePreview(null)
                }
            },
        )

        runCatching { device.createCaptureSession(config) }
            .onFailure { error ->
                show("createCaptureSession exception gevangen: ${error.javaClass.simpleName}: ${error.message}")
                closePreview(null)
            }
    }

    private fun chooseSafePreviewSize(route: HonorRawRoute): Pair<Int, Int> {
        val c = runCatching { manager.getCameraCharacteristics(route.physicalCameraId ?: route.logicalCameraId) }.getOrNull()
        val sizes = runCatching {
            c?.get(android.hardware.camera2.CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                ?.getOutputSizes(SurfaceTexture::class.java)
                ?.toList()
                .orEmpty()
        }.getOrDefault(emptyList())
        val candidate = sizes
            .filter { it.width <= 1920 && it.height <= 1440 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.firstOrNull()
        return if (candidate != null) candidate.width to candidate.height else 1280 to 960
    }

    private fun closePreview(message: String?) {
        runCatching { session?.stopRepeating() }
        runCatching { session?.close() }
        runCatching { camera?.close() }
        runCatching { previewSurface?.release() }
        session = null
        camera = null
        previewSurface = null
        opening = false
        frameCount = 0
        if (::startButton.isInitialized) startButton.isEnabled = selectedRoute != null && preview.isAvailable
        if (::stopButton.isInitialized) stopButton.isEnabled = false
        if (message != null && ::status.isInitialized) show(message)
    }

    private fun show(message: String) {
        if (::status.isInitialized) status.text = message
    }

    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun button(text: String, action: () -> Unit): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        setOnClickListener { action() }
    }

    private fun space(dp: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, this@FotoGraafSafePreviewActivity.dp(dp))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        startButton.isEnabled = selectedRoute != null
        show("Preview surface gereed. Camera blijft dicht tot je op Start preview drukt.")
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) = Unit
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        closePreview("Preview surface vernietigd; camera veilig gesloten.")
        return true
    }
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit
}

package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.content.res.Configuration
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
import android.util.Size
import android.view.Gravity
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.view.Surface
import android.view.TextureView
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.security.MessageDigest
import java.time.Instant
import java.util.Locale
import kotlin.math.abs
import kotlin.math.hypot
import kotlin.math.ln
import kotlin.math.sqrt

/**
 * D.RAW Universal Physical Capture Adapter v0.1.
 *
 * Camera2 is transport/plumbing only. Camera IDs, focal lengths and runtime
 * properties are acquisition provenance and UI hints; they never become
 * scientific truth by themselves.
 *
 * Every normal capture first seals the exact app-visible RAW_SENSOR plane as
 * the primary byte-evidence source. A DNG is created only afterwards as a
 * derived compatibility container for the existing MainActivity / Universal
 * Intake route.
 *
 * Normal capture uses only the standard SCALER_STREAM_CONFIGURATION_MAP RAW
 * sizes. Specialized maximum-resolution / 200MP acquisition stays in the
 * separate FotoGraaf200MpStagedActivity route.
 *
 * No AI/ML/learned model is used.
 */
class UniversalPhysicalCaptureActivity : Activity() {

    private enum class LensRole(val title: String) {
        ULTRA_WIDE("Ultra-wide"),
        WIDE_MAIN("Wide / main"),
        TELE("Tele"),
    }

    private data class Candidate(
        val logicalCameraId: String,
        val physicalCameraId: String?,
        val rawSize: Size,
        val focalLengthMm: Float?,
        val effectiveCameraId: String,
        val discovery: String,
    )

    private data class RawSensorSeal(
        val file: File,
        val sha256: String,
        val bytes: Long,
        val imageWidth: Int,
        val imageHeight: Int,
        val imageFormat: Int,
        val planeCount: Int,
        val rowStride: Int,
        val pixelStride: Int,
        val bufferBytes: Long,
        val expectedContiguousBytes: Long,
        val canonicalContiguousRawSensor: Boolean,
    )

    private lateinit var cameraManager: CameraManager
    private lateinit var statusView: TextView
    private lateinit var detailView: TextView
    private lateinit var ultraButton: Button
    private lateinit var wideButton: Button
    private lateinit var teleButton: Button
    private lateinit var previewView: AutoFitTextureView
    private lateinit var previewTelemetry: TextView
    private lateinit var captureButton: Button
    private lateinit var focusLockButton: Button
    private lateinit var loupeButton: Button
    private lateinit var scaleDetector: ScaleGestureDetector

    private val roleCandidates = linkedMapOf<LensRole, Candidate>()

    private val cameraThread =
        HandlerThread("draw-universal-physical-capture").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var cameraDevice: CameraDevice? = null
    private var cameraSession: CameraCaptureSession? = null
    private var imageReader: ImageReader? = null
    private var previewSurface: Surface? = null
    private var previewRequestBuilder: CaptureRequest.Builder? = null
    private var previewBufferSize: Size? = null
    private var lastPreviewResult: TotalCaptureResult? = null
    private var previewFrames: Long = 0
    private var focusLocked = false
    private var currentAfRegion: MeteringRectangle? = null
    private var macroLoupeScale = 1f
    private var pendingPreviewRole: LensRole? = null
    private var touchDownX = 0f
    private var touchDownY = 0f
    private var touchMoved = false
    private var multiTouchGesture = false

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var activeCandidate: Candidate? = null
    private var activeRole: LensRole? = null
    private var finalizing = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        cameraManager = getSystemService(CameraManager::class.java)
        scaleDetector = ScaleGestureDetector(
            this,
            object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
                    multiTouchGesture = true
                    return true
                }

                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    previewView.pivotX = detector.focusX.coerceIn(0f, previewView.width.toFloat())
                    previewView.pivotY = detector.focusY.coerceIn(0f, previewView.height.toFloat())
                    setMacroLoupeScale(
                        (macroLoupeScale * detector.scaleFactor).coerceIn(1f, 8f),
                    )
                    return true
                }
            },
        )
        setContentView(buildUi())
        discoverUniversalRoutes()
    }

    override fun onPause() {
        closeCaptureResources()
        super.onPause()
    }

    override fun onDestroy() {
        closeCaptureResources()
        synchronized(pairLock) {
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
            setPadding(dp(18), dp(18), dp(18), dp(28))
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
        }

        root.addView(text("D.RAW · Universele camera", 26f, true))
        root.addView(space(5))
        root.addView(
            text(
                "Nieuwe fysieke opname → RAW_SENSOR eerst verzegelen → afgeleide DNG → dezelfde Universele Ingang als ieder bestaand RAW-bestand.",
                13f,
                false,
                DrawVisualTheme.MUTED,
            ),
        )
        root.addView(space(5))
        root.addView(
            text(
                "Camera2 is alleen de Android transportlaag. Camera-ID, focal length en lensrol zijn acquisitie/UI-hints en bepalen geen wetenschappelijke waarheid.",
                11f,
                false,
                DrawVisualTheme.MUTED,
            ),
        )
        root.addView(space(12))

        previewView = AutoFitTextureView(this).apply {
            surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                override fun onSurfaceTextureAvailable(
                    surface: SurfaceTexture,
                    width: Int,
                    height: Int,
                ) {
                    configurePreviewTransform(width, height)
                    val role = pendingPreviewRole
                    if (role != null) {
                        pendingPreviewRole = null
                        post { openPreviewRole(role) }
                    }
                }

                override fun onSurfaceTextureSizeChanged(
                    surface: SurfaceTexture,
                    width: Int,
                    height: Int,
                ) {
                    configurePreviewTransform(width, height)
                }

                override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
                    closeCaptureResources()
                    return true
                }

                override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit
            }
            setOnTouchListener { _, event -> handlePreviewTouch(event) }
        }
        val previewPane = FrameLayout(this).apply {
            setBackgroundColor(Color.BLACK)
            clipChildren = true
            clipToPadding = true
            addView(
                previewView,
                FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    Gravity.CENTER,
                ),
            )
        }
        root.addView(
            previewPane,
            LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(360),
            ),
        )
        root.addView(space(6))
        previewTelemetry = text(
            "Live view nog niet gestart · tik een lens om te richten.",
            10.5f,
            false,
            DrawVisualTheme.MUTED,
        )
        root.addView(previewTelemetry)
        root.addView(space(8))

        ultraButton = button("Ultra-wide · zoeken…") {
            openPreviewRole(LensRole.ULTRA_WIDE)
        }.apply { isEnabled = false }
        wideButton = button("Wide / main · zoeken…") {
            openPreviewRole(LensRole.WIDE_MAIN)
        }.apply { isEnabled = false }
        teleButton = button("Tele · zoeken…") {
            openPreviewRole(LensRole.TELE)
        }.apply { isEnabled = false }

        root.addView(ultraButton)
        root.addView(space(7))
        root.addView(wideButton)
        root.addView(space(7))
        root.addView(teleButton)
        root.addView(space(8))

        val assistRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
        }
        focusLockButton = button("AF vergrendelen") { toggleFocusLock() }.apply {
            isEnabled = false
        }
        loupeButton = button("Macro-loep · 1×") { cycleMacroLoupe() }.apply {
            isEnabled = false
        }
        assistRow.addView(
            focusLockButton,
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
        assistRow.addView(
            loupeButton,
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f),
        )
        root.addView(assistRow)
        root.addView(space(7))

        captureButton = button("Maak volledige RAW_SENSOR-opname") {
            captureFromLivePreview()
        }.apply { isEnabled = false }
        root.addView(captureButton)
        root.addView(
            text(
                "Macro-loep/pinch vergroot alleen de live weergave. De RAW_SENSOR-opname blijft op de volledige geselecteerde standaard bronresolutie.",
                10f,
                false,
                DrawVisualTheme.MUTED,
            ),
        )
        root.addView(space(14))

        root.addView(
            button("Speciale 4K → 200MP RAW-route") {
                closeCaptureResources()
                startActivity(
                    Intent(this, FotoGraaf200MpStagedActivity::class.java).apply {
                        putExtra(
                            FotoGraaf200MpStagedActivity.EXTRA_PRODUCTION_CAMERA_ENTRY,
                            true,
                        )
                    },
                )
            },
        )
        root.addView(
            text(
                "Deze Camera-5 route blijft apart: maximum-resolution/200MP-logica wordt nooit op ultra-wide, main of normale tele toegepast.",
                10.5f,
                false,
                DrawVisualTheme.MUTED,
            ),
        )

        root.addView(space(16))
        root.addView(button("Camera's opnieuw ontdekken") { discoverUniversalRoutes() })
        root.addView(space(12))

        statusView = text("Camera-inventaris wordt nog niet gelezen.", 13f, true)
        detailView = text("", 10.5f, false, DrawVisualTheme.MUTED).apply {
            setTextIsSelectable(true)
        }
        root.addView(statusView)
        root.addView(space(7))
        root.addView(detailView)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
            addView(
                root,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
        }
    }

    private fun discoverUniversalRoutes() {
        closeCaptureResources()
        setButtonsEnabled(false)
        captureButton.isEnabled = false
        focusLockButton.isEnabled = false
        loupeButton.isEnabled = false
        focusLocked = false
        currentAfRegion = null
        setMacroLoupeScale(1f)
        previewTelemetry.text =
            "Live view nog niet gestart · tik een lens om te richten."
        status("Universele back-facing RAW-routes worden dynamisch ontdekt…")
        detailView.text = ""

        Thread({
            val result = runCatching { discoverCandidates() }
            runOnUiThread {
                result.onSuccess { candidates ->
                    assignRoles(candidates)
                    renderRoles(candidates)
                }.onFailure { error ->
                    roleCandidates.clear()
                    setButtonsEnabled(false)
                    status(
                        "Camera-ontdekking faalde: " +
                            (error.message ?: error.javaClass.simpleName),
                    )
                    detailView.text = error.stackTraceToString()
                }
            }
        }, "draw-universal-camera-discovery").start()
    }

    private fun discoverCandidates(): List<Candidate> {
        val topLevelIds = cameraManager.cameraIdList.toList()
        val topLevelBack = topLevelIds.filter { id ->
            val c = cameraManager.getCameraCharacteristics(id)
            c.get(CameraCharacteristics.LENS_FACING) ==
                CameraCharacteristics.LENS_FACING_BACK
        }

        val discovered = mutableListOf<Candidate>()

        for (logicalId in topLevelBack) {
            val logical = cameraManager.getCameraCharacteristics(logicalId)
            val logicalRawSizes = rawSizes(logical)
            val physicalCandidates = logical.physicalCameraIds
                .mapNotNull { physicalId ->
                    val physical = runCatching {
                        cameraManager.getCameraCharacteristics(physicalId)
                    }.getOrNull() ?: return@mapNotNull null
                    val physicalRawSizes = rawSizes(physical)
                    if (physicalRawSizes.isEmpty()) return@mapNotNull null
                    val commonRawSizes = physicalRawSizes.filter { physicalSize ->
                        logicalRawSizes.any { logicalSize ->
                            logicalSize.width == physicalSize.width &&
                                logicalSize.height == physicalSize.height
                        }
                    }
                    val size = (commonRawSizes.ifEmpty { physicalRawSizes })
                        .maxByOrNull { it.width.toLong() * it.height.toLong() }
                        ?: return@mapNotNull null
                    Candidate(
                        logicalCameraId = logicalId,
                        physicalCameraId = physicalId,
                        rawSize = size,
                        focalLengthMm = representativeFocalLength(physical),
                        effectiveCameraId = physicalId,
                        discovery = "PHYSICAL_BOUND_TO_LOGICAL",
                    )
                }

            val directSize = largestRawSize(logical)
            if (physicalCandidates.isEmpty()) {
                if (directSize != null) {
                    discovered += Candidate(
                        logicalCameraId = logicalId,
                        physicalCameraId = null,
                        rawSize = directSize,
                        focalLengthMm = representativeFocalLength(logical),
                        effectiveCameraId = logicalId,
                        discovery = "DIRECT_LOGICAL_RAW",
                    )
                }
            } else {
                // When a logical multi-camera exposes individual RAW-capable
                // physical members, classify those members only. The aggregate
                // logical route can advertise multiple focal lengths and must
                // not become a fictitious extra lens in focal-order UI.
                discovered += physicalCandidates
            }
        }

        val directIds = topLevelBack.toSet()
        return discovered
            .groupBy { it.effectiveCameraId }
            .map { (_, sameCamera) ->
                sameCamera.firstOrNull {
                    it.physicalCameraId == null &&
                        it.logicalCameraId in directIds &&
                        largestRawSize(
                            cameraManager.getCameraCharacteristics(
                                it.logicalCameraId,
                            ),
                        ) != null
                } ?: sameCamera.first()
            }
            .sortedWith(
                compareBy<Candidate>(
                    { it.focalLengthMm ?: Float.POSITIVE_INFINITY },
                    { it.effectiveCameraId },
                ),
            )
    }

    private fun rawSizes(c: CameraCharacteristics): List<Size> =
        c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(ImageFormat.RAW_SENSOR)
            ?.toList()
            .orEmpty()

    private fun largestRawSize(c: CameraCharacteristics): Size? =
        rawSizes(c).maxByOrNull { it.width.toLong() * it.height.toLong() }

    private fun representativeFocalLength(c: CameraCharacteristics): Float? =
        c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
            ?.filter { it.isFinite() && it > 0f }
            ?.minOrNull()

    private fun assignRoles(candidates: List<Candidate>) {
        roleCandidates.clear()
        val focal = candidates
            .filter { it.focalLengthMm != null && it.focalLengthMm > 0f }
            .sortedBy { it.focalLengthMm }

        if (focal.isEmpty()) return

        roleCandidates[LensRole.ULTRA_WIDE] = focal.first()

        if (focal.size == 1) {
            roleCandidates[LensRole.WIDE_MAIN] = focal.first()
            return
        }

        roleCandidates[LensRole.TELE] = focal.last()

        if (focal.size >= 3) {
            val low = focal.first().focalLengthMm!!.toDouble()
            val high = focal.last().focalLengthMm!!.toDouble()
            val geometricMid = sqrt(low * high)
            val interior = focal.subList(1, focal.size - 1)
            val main = interior.minByOrNull {
                abs(ln(it.focalLengthMm!!.toDouble() / geometricMid))
            }
            if (main != null) roleCandidates[LensRole.WIDE_MAIN] = main
        }
    }

    private fun renderRoles(all: List<Candidate>) {
        fun label(role: LensRole): String {
            val c = roleCandidates[role]
                ?: return role.title + " · niet als aparte RAW-route gevonden"
            val focal =
                c.focalLengthMm?.let { "%.2fmm".format(it) }
                    ?: "focal UNKNOWN"
            return role.title + " · " + focal + " · " +
                c.rawSize.width + "×" + c.rawSize.height
        }

        ultraButton.text = label(LensRole.ULTRA_WIDE)
        wideButton.text = label(LensRole.WIDE_MAIN)
        teleButton.text = label(LensRole.TELE)

        restoreRoleButtons()

        status(
            "Ontdekking gereed · ${all.size} unieke back-facing RAW-bronnen · " +
                "${roleCandidates.size} universele lensrollen beschikbaar.",
        )

        detailView.text = buildString {
            appendLine(
                "Lensrollen zijn UI_FOCAL_ORDER_HINT_ONLY; normale capture kiest per fysieke camera de hoogste standaard RAW_SENSOR-resolutie.",
            )
            appendLine(
                "Geen toestelmap, geen vaste Camera-ID en geen vendor-key is nodig.",
            )
            appendLine(
                "Resolutiedomein=STANDARD_SCALER_STREAM_CONFIGURATION_MAP; MAXIMUM_RESOLUTION is gereserveerd voor aparte gespecialiseerde routes.",
            )
            appendLine()
            all.forEach { c ->
                append(c.effectiveCameraId)
                append(" · logical=")
                append(c.logicalCameraId)
                append(" · physical=")
                append(c.physicalCameraId ?: "none")
                append(" · focal=")
                append(c.focalLengthMm ?: "UNKNOWN")
                append("mm · RAW=")
                append(c.rawSize.width)
                append("×")
                append(c.rawSize.height)
                append(" · ")
                appendLine(c.discovery)
            }
        }
    }

    private fun openPreviewRole(role: LensRole) {
        val candidate = roleCandidates[role] ?: return

        if (
            checkSelfPermission(Manifest.permission.CAMERA) !=
            PackageManager.PERMISSION_GRANTED
        ) {
            pendingPreviewRole = role
            requestPermissions(
                arrayOf(Manifest.permission.CAMERA),
                REQUEST_CAMERA_PERMISSION,
            )
            status("Camera-permissie gevraagd. De gekozen lens opent daarna als live view.")
            return
        }

        if (!previewView.isAvailable) {
            pendingPreviewRole = role
            status("${role.title}: wachten op de live-view surface…")
            return
        }

        closeCaptureResources()
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            activeCandidate = candidate
            activeRole = role
            finalizing = false
        }

        focusLocked = false
        currentAfRegion = null
        previewFrames = 0
        lastPreviewResult = null
        setMacroLoupeScale(1f)
        setButtonsEnabled(false)
        captureButton.isEnabled = false
        focusLockButton.isEnabled = false
        loupeButton.isEnabled = false
        status(
            "${role.title} wordt geopend voor live richten · capture blijft volledige " +
                "${candidate.rawSize.width}×${candidate.rawSize.height} RAW_SENSOR.",
        )

        val reader = ImageReader.newInstance(
            candidate.rawSize.width,
            candidate.rawSize.height,
            ImageFormat.RAW_SENSOR,
            2,
        )
        imageReader = reader
        reader.setOnImageAvailableListener({ source ->
            val image =
                runCatching { source.acquireNextImage() }.getOrNull()
                    ?: return@setOnImageAvailableListener
            synchronized(pairLock) {
                pendingImage?.close()
                pendingImage = image
            }
            tryFinalizePair()
        }, cameraHandler)

        val effectiveCharacteristics = runCatching {
            cameraManager.getCameraCharacteristics(candidate.effectiveCameraId)
        }.getOrElse { error ->
            status(
                "Preview-capability kon niet worden gelezen: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            restoreRoleButtons()
            closeCaptureResources()
            return
        }
        val chosenPreview = choosePreviewSize(effectiveCharacteristics)
            ?: run {
                status("${role.title}: geen SurfaceTexture-previewformaat gevonden.")
                restoreRoleButtons()
                closeCaptureResources()
                return
            }

        previewBufferSize = chosenPreview
        val landscape =
            resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        previewView.setAspectRatio(
            if (landscape) chosenPreview.width else chosenPreview.height,
            if (landscape) chosenPreview.height else chosenPreview.width,
        )
        val texture = previewView.surfaceTexture
            ?: run {
                pendingPreviewRole = role
                status("${role.title}: preview surface verdween; opnieuw proberen zodra hij terug is.")
                restoreRoleButtons()
                closeCaptureResources()
                return
            }
        texture.setDefaultBufferSize(chosenPreview.width, chosenPreview.height)
        previewView.post {
            configurePreviewTransform(previewView.width, previewView.height)
        }
        previewSurface = Surface(texture)

        try {
            cameraManager.openCamera(
                candidate.logicalCameraId,
                object : CameraDevice.StateCallback() {
                    override fun onOpened(camera: CameraDevice) {
                        cameraDevice = camera
                        createSession(camera, candidate, reader)
                    }

                    override fun onDisconnected(camera: CameraDevice) {
                        camera.close()
                        statusFromAnyThread("Camera werd losgekoppeld.")
                        runOnUiThread {
                            captureButton.isEnabled = false
                            focusLockButton.isEnabled = false
                            loupeButton.isEnabled = false
                            restoreRoleButtons()
                        }
                        closeCaptureResources()
                    }

                    override fun onError(camera: CameraDevice, error: Int) {
                        camera.close()
                        statusFromAnyThread("Camera-open fout=$error.")
                        runOnUiThread {
                            captureButton.isEnabled = false
                            focusLockButton.isEnabled = false
                            loupeButton.isEnabled = false
                            restoreRoleButtons()
                        }
                        closeCaptureResources()
                    }
                },
                cameraHandler,
            )
        } catch (error: Exception) {
            status(
                "Camera kon niet worden geopend: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            restoreRoleButtons()
            closeCaptureResources()
        }
    }

    private fun choosePreviewSize(c: CameraCharacteristics): Size? {
        val sizes = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)
            ?.toList()
            .orEmpty()
        if (sizes.isEmpty()) return null

        return sizes
            .filter { it.width <= 1920 && it.height <= 1440 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.maxByOrNull { it.width.toLong() * it.height.toLong() }
    }

    private fun createSession(
        camera: CameraDevice,
        candidate: Candidate,
        reader: ImageReader,
    ) {
        val preview = previewSurface
            ?: run {
                statusFromAnyThread("Live-view surface ontbreekt.")
                runOnUiThread { restoreRoleButtons() }
                closeCaptureResources()
                return
            }

        val previewOutput = OutputConfiguration(preview)
        val rawOutput = OutputConfiguration(reader.surface)
        candidate.physicalCameraId?.let { physicalId ->
            try {
                previewOutput.setPhysicalCameraId(physicalId)
                rawOutput.setPhysicalCameraId(physicalId)
            } catch (error: Exception) {
                statusFromAnyThread(
                    "Physical preview/RAW-output kon niet worden gebonden: " +
                        (error.message ?: error.javaClass.simpleName),
                )
                runOnUiThread { restoreRoleButtons() }
                closeCaptureResources()
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
                    startPreviewRepeating(camera, session, candidate)
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    statusFromAnyThread(
                        "Preview + RAW capture-session kon niet worden geconfigureerd.",
                    )
                    runOnUiThread {
                        captureButton.isEnabled = false
                        focusLockButton.isEnabled = false
                        loupeButton.isEnabled = false
                        restoreRoleButtons()
                    }
                    closeCaptureResources()
                }
            },
        )

        try {
            camera.createCaptureSession(config)
        } catch (error: Exception) {
            statusFromAnyThread(
                "Preview/capture-session faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            runOnUiThread {
                captureButton.isEnabled = false
                focusLockButton.isEnabled = false
                loupeButton.isEnabled = false
                restoreRoleButtons()
            }
            closeCaptureResources()
        }
    }

    private fun startPreviewRepeating(
        camera: CameraDevice,
        session: CameraCaptureSession,
        candidate: Candidate,
    ) {
        val surface = previewSurface ?: return
        val effectiveCharacteristics = runCatching {
            cameraManager.getCameraCharacteristics(candidate.effectiveCameraId)
        }.getOrElse { error ->
            statusFromAnyThread(
                "Preview characteristics ontbreken: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            closeCaptureResources()
            return
        }

        try {
            val builder =
                camera.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW).apply {
                    addTarget(surface)
                    set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
                    set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)
                    setPreviewAfMode(this, effectiveCharacteristics, continuous = true)
                }
            previewRequestBuilder = builder

            session.setRepeatingRequest(
                builder.build(),
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        previewFrames++
                        lastPreviewResult = result
                        if (previewFrames == 1L || previewFrames % 12L == 0L) {
                            val effective =
                                effectiveCaptureResult(candidate, result) ?: result
                            val afState =
                                effective.get(CaptureResult.CONTROL_AF_STATE)
                            val focusDistance =
                                effective.get(CaptureResult.LENS_FOCUS_DISTANCE)
                            val iso =
                                effective.get(CaptureResult.SENSOR_SENSITIVITY)
                            val exp =
                                effective.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                            runOnUiThread {
                                previewTelemetry.text = buildString {
                                    append("LIVE · ")
                                    append(activeRole?.title ?: candidate.effectiveCameraId)
                                    append(" · loep=")
                                    append(String.format(Locale.ROOT, "%.1f×", macroLoupeScale))
                                    append(" · AF=")
                                    append(afState ?: "UNKNOWN")
                                    append(" · focus=")
                                    append(
                                        focusDistance?.let {
                                            String.format(Locale.ROOT, "%.3f D", it)
                                        } ?: "UNKNOWN",
                                    )
                                    append("\nISO=")
                                    append(iso ?: "UNKNOWN")
                                    append(" · t=")
                                    append(
                                        exp?.let {
                                            String.format(Locale.ROOT, "%.3f ms", it / 1_000_000.0)
                                        } ?: "UNKNOWN",
                                    )
                                    append(" · display zoom verandert RAW niet")
                                }
                                captureButton.isEnabled = true
                                focusLockButton.isEnabled =
                                    supportsAutoFocus(effectiveCharacteristics)
                                loupeButton.isEnabled = true
                                restoreRoleButtons()
                            }
                        }
                    }
                },
                cameraHandler,
            )

            statusFromAnyThread(
                "${activeRole?.title ?: "Camera"} live · tik voor AF · pinch of Macro-loep voor alleen schermvergroting.",
            )
        } catch (error: Exception) {
            statusFromAnyThread(
                "Live preview faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            runOnUiThread {
                captureButton.isEnabled = false
                focusLockButton.isEnabled = false
                loupeButton.isEnabled = false
                restoreRoleButtons()
            }
            closeCaptureResources()
        }
    }

    private fun supportsAutoFocus(c: CameraCharacteristics): Boolean =
        (c.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf())
            .any { it != CameraMetadata.CONTROL_AF_MODE_OFF }

    private fun setPreviewAfMode(
        builder: CaptureRequest.Builder,
        c: CameraCharacteristics,
        continuous: Boolean,
    ) {
        val modes =
            c.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
        when {
            continuous &&
                modes.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE) ->
                builder.set(
                    CaptureRequest.CONTROL_AF_MODE,
                    CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE,
                )
            modes.contains(CameraMetadata.CONTROL_AF_MODE_AUTO) ->
                builder.set(
                    CaptureRequest.CONTROL_AF_MODE,
                    CameraMetadata.CONTROL_AF_MODE_AUTO,
                )
            else ->
                builder.set(
                    CaptureRequest.CONTROL_AF_MODE,
                    CameraMetadata.CONTROL_AF_MODE_OFF,
                )
        }
    }

    private fun configurePreviewTransform(
        viewWidth: Int,
        viewHeight: Int,
    ) {
        val size = previewBufferSize ?: return
        if (viewWidth <= 0 || viewHeight <= 0) return

        val rotation = previewView.display?.rotation ?: Surface.ROTATION_0
        val matrix = Matrix()
        val viewRect =
            RectF(0f, 0f, viewWidth.toFloat(), viewHeight.toFloat())
        val bufferRect =
            RectF(0f, 0f, size.height.toFloat(), size.width.toFloat())
        val centerX = viewRect.centerX()
        val centerY = viewRect.centerY()

        if (
            rotation == Surface.ROTATION_90 ||
            rotation == Surface.ROTATION_270
        ) {
            bufferRect.offset(
                centerX - bufferRect.centerX(),
                centerY - bufferRect.centerY(),
            )
            matrix.setRectToRect(
                viewRect,
                bufferRect,
                Matrix.ScaleToFit.FILL,
            )
            val scale =
                maxOf(
                    viewHeight.toFloat() / size.height.toFloat(),
                    viewWidth.toFloat() / size.width.toFloat(),
                )
            matrix.postScale(scale, scale, centerX, centerY)
            matrix.postRotate(
                (90 * (rotation - 2)).toFloat(),
                centerX,
                centerY,
            )
        } else if (rotation == Surface.ROTATION_180) {
            matrix.postRotate(180f, centerX, centerY)
        }
        previewView.setTransform(matrix)
    }

    private fun handlePreviewTouch(event: MotionEvent): Boolean {
        scaleDetector.onTouchEvent(event)

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                touchDownX = event.x
                touchDownY = event.y
                touchMoved = false
                multiTouchGesture = false
            }
            MotionEvent.ACTION_POINTER_DOWN -> {
                multiTouchGesture = true
            }
            MotionEvent.ACTION_MOVE -> {
                if (
                    hypot(
                        (event.x - touchDownX).toDouble(),
                        (event.y - touchDownY).toDouble(),
                    ) > dp(12).toDouble()
                ) {
                    touchMoved = true
                }
            }
            MotionEvent.ACTION_UP -> {
                if (!multiTouchGesture && !touchMoved) {
                    previewView.pivotX = event.x
                    previewView.pivotY = event.y
                    tapToFocus(event.x, event.y)
                }
                multiTouchGesture = false
            }
            MotionEvent.ACTION_CANCEL -> {
                multiTouchGesture = false
            }
        }
        return true
    }

    private fun cycleMacroLoupe() {
        val next =
            when {
                macroLoupeScale < 1.5f -> 2f
                macroLoupeScale < 3f -> 4f
                macroLoupeScale < 6f -> 8f
                else -> 1f
            }
        if (next == 1f) {
            previewView.pivotX = previewView.width / 2f
            previewView.pivotY = previewView.height / 2f
        }
        setMacroLoupeScale(next)
    }

    private fun setMacroLoupeScale(value: Float) {
        macroLoupeScale = value.coerceIn(1f, 8f)
        if (::previewView.isInitialized) {
            previewView.scaleX = macroLoupeScale
            previewView.scaleY = macroLoupeScale
        }
        if (::loupeButton.isInitialized) {
            loupeButton.text =
                "Macro-loep · " +
                    String.format(Locale.ROOT, "%.1f×", macroLoupeScale)
        }
    }

    private fun toggleFocusLock() {
        if (focusLocked) {
            unlockFocus()
        } else {
            tapToFocus(
                previewView.width / 2f,
                previewView.height / 2f,
            )
        }
    }

    private fun tapToFocus(x: Float, y: Float) {
        val candidate = activeCandidate ?: return
        val session = cameraSession ?: return
        val builder = previewRequestBuilder ?: return
        val characteristics = runCatching {
            cameraManager.getCameraCharacteristics(candidate.effectiveCameraId)
        }.getOrNull() ?: return

        if (!supportsAutoFocus(characteristics)) {
            status("Deze RAW-camera rapporteert geen bruikbare autofocusmodus.")
            return
        }

        val maxRegions =
            characteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF) ?: 0
        val region =
            if (maxRegions > 0) {
                focusRegionForTouch(x, y, characteristics)
            } else {
                null
            }
        currentAfRegion = region

        try {
            builder.set(
                CaptureRequest.CONTROL_AF_MODE,
                CameraMetadata.CONTROL_AF_MODE_AUTO,
            )
            if (region != null) {
                builder.set(
                    CaptureRequest.CONTROL_AF_REGIONS,
                    arrayOf(region),
                )
            }

            builder.set(
                CaptureRequest.CONTROL_AF_TRIGGER,
                CameraMetadata.CONTROL_AF_TRIGGER_CANCEL,
            )
            session.capture(builder.build(), null, cameraHandler)

            builder.set(
                CaptureRequest.CONTROL_AF_TRIGGER,
                CameraMetadata.CONTROL_AF_TRIGGER_START,
            )
            session.capture(
                builder.build(),
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        val effective =
                            effectiveCaptureResult(candidate, result) ?: result
                        val state =
                            effective.get(CaptureResult.CONTROL_AF_STATE)
                        val distance =
                            effective.get(CaptureResult.LENS_FOCUS_DISTANCE)
                        focusLocked =
                            state == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED ||
                                state == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED
                        runOnUiThread {
                            focusLockButton.text =
                                if (focusLocked) {
                                    "AF ontgrendelen"
                                } else {
                                    "AF vergrendelen"
                                }
                            status(
                                "Tap-focus · AF=${state ?: "UNKNOWN"} · focus=" +
                                    (distance?.let {
                                        String.format(Locale.ROOT, "%.3f D", it)
                                    } ?: "UNKNOWN"),
                            )
                        }
                    }
                },
                cameraHandler,
            )

            builder.set(
                CaptureRequest.CONTROL_AF_TRIGGER,
                CameraMetadata.CONTROL_AF_TRIGGER_IDLE,
            )
            session.setRepeatingRequest(
                builder.build(),
                null,
                cameraHandler,
            )
            focusLocked = true
            focusLockButton.text = "AF ontgrendelen"
        } catch (error: Exception) {
            status(
                "Tap-focus faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
        }
    }

    private fun unlockFocus() {
        val candidate = activeCandidate ?: return
        val session = cameraSession ?: return
        val camera = cameraDevice ?: return
        val builder = previewRequestBuilder ?: return

        runCatching {
            builder.set(
                CaptureRequest.CONTROL_AF_TRIGGER,
                CameraMetadata.CONTROL_AF_TRIGGER_CANCEL,
            )
            session.capture(builder.build(), null, cameraHandler)
        }
        focusLocked = false
        currentAfRegion = null
        focusLockButton.text = "AF vergrendelen"
        startPreviewRepeating(camera, session, candidate)
        status("AF ontgrendeld · continuous autofocus hervat.")
    }

    private fun focusRegionForTouch(
        x: Float,
        y: Float,
        c: CameraCharacteristics,
    ): MeteringRectangle? {
        val active =
            c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)
                ?: return null
        if (previewView.width <= 0 || previewView.height <= 0) return null

        val nx =
            (x / previewView.width.toFloat()).coerceIn(0f, 1f)
        val ny =
            (y / previewView.height.toFloat()).coerceIn(0f, 1f)

        val sensorOrientation =
            c.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 0
        val displayDegrees =
            when (previewView.display?.rotation ?: Surface.ROTATION_0) {
                Surface.ROTATION_90 -> 90
                Surface.ROTATION_180 -> 180
                Surface.ROTATION_270 -> 270
                else -> 0
            }
        val relative =
            (sensorOrientation - displayDegrees + 360) % 360

        val sensorNorm =
            when (relative) {
                90 -> Pair(ny, 1f - nx)
                180 -> Pair(1f - nx, 1f - ny)
                270 -> Pair(1f - ny, nx)
                else -> Pair(nx, ny)
            }

        val cx =
            active.left +
                (sensorNorm.first * active.width()).toInt()
        val cy =
            active.top +
                (sensorNorm.second * active.height()).toInt()
        val side = maxOf(48, minOf(active.width(), active.height()) / 10)
        val half = side / 2

        val left =
            (cx - half).coerceIn(active.left, active.right - side)
        val top =
            (cy - half).coerceIn(active.top, active.bottom - side)
        val rect = Rect(left, top, left + side, top + side)
        return MeteringRectangle(
            rect,
            MeteringRectangle.METERING_WEIGHT_MAX,
        )
    }

    private fun captureFromLivePreview() {
        val camera = cameraDevice
            ?: run {
                status("Geen live camera geopend.")
                return
            }
        val session = cameraSession
            ?: run {
                status("Geen actieve preview/capture-session.")
                return
            }
        val candidate = activeCandidate
            ?: run {
                status("Geen actieve RAW-camera geselecteerd.")
                return
            }
        val reader = imageReader
            ?: run {
                status("RAW ImageReader ontbreekt.")
                return
            }

        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            finalizing = false
        }
        captureButton.isEnabled = false
        focusLockButton.isEnabled = false
        loupeButton.isEnabled = false
        setButtonsEnabled(false)
        submitCapture(camera, session, candidate, reader)
    }

    private fun submitCapture(
        camera: CameraDevice,
        session: CameraCaptureSession,
        candidate: Candidate,
        reader: ImageReader,
    ) {
        val effectiveCharacteristics = runCatching {
            cameraManager.getCameraCharacteristics(candidate.effectiveCameraId)
        }.getOrElse {
            status(
                "Camera characteristics alleen voor transport konden niet worden gelezen.",
            )
            restoreRoleButtons()
            return
        }

        try {
            val request =
                camera.createCaptureRequest(
                    CameraDevice.TEMPLATE_STILL_CAPTURE,
                ).apply {
                    addTarget(reader.surface)
                    set(
                        CaptureRequest.CONTROL_MODE,
                        CameraMetadata.CONTROL_MODE_AUTO,
                    )
                    set(
                        CaptureRequest.CONTROL_AE_MODE,
                        CameraMetadata.CONTROL_AE_MODE_ON,
                    )

                    if (focusLocked) {
                        set(
                            CaptureRequest.CONTROL_AF_MODE,
                            CameraMetadata.CONTROL_AF_MODE_AUTO,
                        )
                        currentAfRegion?.let { region ->
                            set(
                                CaptureRequest.CONTROL_AF_REGIONS,
                                arrayOf(region),
                            )
                        }
                        set(
                            CaptureRequest.CONTROL_AF_TRIGGER,
                            CameraMetadata.CONTROL_AF_TRIGGER_IDLE,
                        )
                    } else {
                        setPreviewAfMode(
                            this,
                            effectiveCharacteristics,
                            continuous = true,
                        )
                    }

                    val nrModes =
                        effectiveCharacteristics.get(
                            CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
                        ) ?: intArrayOf()
                    if (
                        nrModes.contains(
                            CameraMetadata.NOISE_REDUCTION_MODE_OFF,
                        )
                    ) {
                        set(
                            CaptureRequest.NOISE_REDUCTION_MODE,
                            CameraMetadata.NOISE_REDUCTION_MODE_OFF,
                        )
                    }

                    val edgeModes =
                        effectiveCharacteristics.get(
                            CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES,
                        ) ?: intArrayOf()
                    if (edgeModes.contains(CameraMetadata.EDGE_MODE_OFF)) {
                        set(
                            CaptureRequest.EDGE_MODE,
                            CameraMetadata.EDGE_MODE_OFF,
                        )
                    }
                }.build()

            session.capture(
                request,
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        synchronized(pairLock) { pendingResult = result }
                        tryFinalizePair()
                    }

                    override fun onCaptureFailed(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        failure: android.hardware.camera2.CaptureFailure,
                    ) {
                        statusFromAnyThread(
                            "RAW capture faalde: reason=${failure.reason}",
                        )
                        runOnUiThread { restoreRoleButtons() }
                        closeCaptureResources()
                    }
                },
                cameraHandler,
            )
            statusFromAnyThread(
                "RAW capture verstuurd · volledige bronresolutie · schermloep=" +
                    String.format(Locale.ROOT, "%.1f×", macroLoupeScale) +
                    " blijft presentatie-only.",
            )
        } catch (error: Exception) {
            statusFromAnyThread(
                "Still capture faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            runOnUiThread { restoreRoleButtons() }
            closeCaptureResources()
        }
    }

    private fun tryFinalizePair() {
        val image: Image
        val result: TotalCaptureResult
        val candidate: Candidate
        val role: LensRole

        synchronized(pairLock) {
            if (finalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            candidate = activeCandidate ?: return
            role = activeRole ?: return

            val effectiveResult =
                effectiveCaptureResult(candidate, result)
                    ?: run {
                        finalizing = true
                        pendingImage = null
                        pendingResult = null
                        image.close()
                        statusFromAnyThread(
                            "Capture faalde gesloten: physical result ontbreekt voor de gekozen route.",
                        )
                        runOnUiThread { restoreRoleButtons() }
                        closeCaptureResources()
                        return
                    }

            val timestamp =
                effectiveResult.get(CaptureResult.SENSOR_TIMESTAMP)
            if (timestamp == null || timestamp != image.timestamp) {
                finalizing = true
                pendingImage = null
                pendingResult = null
                image.close()
                statusFromAnyThread(
                    "Capture faalde gesloten: RAW timestamp=${image.timestamp} != SENSOR_TIMESTAMP=$timestamp.",
                )
                runOnUiThread { restoreRoleButtons() }
                closeCaptureResources()
                return
            }

            finalizing = true
            pendingImage = null
            pendingResult = null
        }

        cameraHandler.post {
            finalizeCapture(role, candidate, image, result)
        }
    }

    private fun effectiveCaptureResult(
        candidate: Candidate,
        result: TotalCaptureResult,
    ): CaptureResult? {
        val physicalId = candidate.physicalCameraId ?: return result
        return result.physicalCameraResults[physicalId]
    }

    private fun finalizeCapture(
        role: LensRole,
        candidate: Candidate,
        image: Image,
        result: TotalCaptureResult,
    ) {
        try {
            val effectiveResult =
                effectiveCaptureResult(candidate, result)
                    ?: error("Physical capture result ontbreekt.")
            val characteristics =
                cameraManager.getCameraCharacteristics(
                    candidate.effectiveCameraId,
                )

            require(image.format == ImageFormat.RAW_SENSOR) {
                "Verwacht RAW_SENSOR maar kreeg format=${image.format}."
            }
            require(image.planes.size == 1) {
                "RAW_SENSOR moet exact één app-zichtbaar plane hebben; observed=${image.planes.size}."
            }

            val captureDir =
                File(filesDir, "draw-captures").apply { mkdirs() }
            val stamp = System.currentTimeMillis()
            val roleStem = role.name.lowercase()

            val rawSensorFile =
                File(
                    captureDir,
                    "DRAW_CAPTURE_${stamp}_${roleStem}_" +
                        "${image.width}x${image.height}.rawsensor",
                )
            val rawSeal = sealRawSensorPlane(image, rawSensorFile)

            val dngFile =
                File(
                    captureDir,
                    "DRAW_CAPTURE_${stamp}_${roleStem}_" +
                        "${image.width}x${image.height}.dng",
                )

            var dngSha: String? = null
            var dngError: String? = null
            runCatching {
                FileOutputStream(dngFile).use { out ->
                    DngCreator(characteristics, effectiveResult).use { creator ->
                        creator.writeImage(out, image)
                    }
                }
                dngSha = sha256(dngFile)
            }.onFailure { error ->
                dngError = error.message ?: error.javaClass.simpleName
                runCatching { dngFile.delete() }
            }
            image.close()

            val evidenceFile =
                File(
                    captureDir,
                    "DRAW_CAPTURE_${stamp}_${roleStem}_acquisition_v0_2.json",
                )
            evidenceFile.writeText(
                buildAcquisitionEvidence(
                    role = role,
                    candidate = candidate,
                    result = result,
                    effectiveResult = effectiveResult,
                    rawSeal = rawSeal,
                    dngFile = dngFile.takeIf { dngSha != null },
                    dngSha = dngSha,
                    dngError = dngError,
                ).toString(2),
            )

            if (dngSha == null) {
                statusFromAnyThread(
                    "${role.title} RAW_SENSOR is wél verzegeld · SHA-256=${rawSeal.sha256.take(16)}… · " +
                        "DNG-afleiding faalde: ${dngError ?: "UNKNOWN"}.",
                )
                runOnUiThread { restoreRoleButtons() }
                return
            }

            statusFromAnyThread(
                "${role.title} RAW_SENSOR verzegeld · SHA-256=${rawSeal.sha256.take(16)}… · " +
                    "DNG afgeleid · overdracht naar D.RAW Universele Ingang.",
            )

            runOnUiThread {
                startActivity(
                    Intent(this, MainActivity::class.java).apply {
                        flags =
                            Intent.FLAG_ACTIVITY_REORDER_TO_FRONT or
                                Intent.FLAG_ACTIVITY_SINGLE_TOP
                        putExtra(
                            MainActivity.EXTRA_INTERNAL_CAMERA_SOURCE_PATH,
                            dngFile.absolutePath,
                        )
                        putExtra(
                            MainActivity.EXTRA_INTERNAL_CAMERA_EVIDENCE_PATH,
                            evidenceFile.absolutePath,
                        )
                        putExtra(
                            MainActivity.EXTRA_INTERNAL_CAMERA_UPSTREAM_SHA256,
                            rawSeal.sha256,
                        )
                        putExtra(
                            MainActivity.EXTRA_AUTO_START_TRUTHRAW,
                            true,
                        )
                    },
                )
                restoreRoleButtons()
            }
        } catch (error: Exception) {
            runCatching { image.close() }
            statusFromAnyThread(
                "RAW_SENSOR/DNG-finalisatie faalde gesloten: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            runOnUiThread { restoreRoleButtons() }
        } finally {
            closeCaptureResources()
        }
    }

    private fun sealRawSensorPlane(
        image: Image,
        output: File,
    ): RawSensorSeal {
        val plane = image.planes.single()
        val source = plane.buffer.duplicate().apply { position(0) }
        val bufferBytes = source.remaining().toLong()
        val expectedContiguousBytes =
            image.width.toLong() * image.height.toLong() * 2L

        FileOutputStream(output).use { out ->
            val chunk = ByteArray(1024 * 1024)
            while (source.hasRemaining()) {
                val n = minOf(source.remaining(), chunk.size)
                source.get(chunk, 0, n)
                out.write(chunk, 0, n)
            }
            out.fd.sync()
        }

        val writtenBytes = output.length()
        require(writtenBytes == bufferBytes) {
            "RAW_SENSOR seal write mismatch: buffer=$bufferBytes file=$writtenBytes."
        }

        return RawSensorSeal(
            file = output,
            sha256 = sha256(output),
            bytes = writtenBytes,
            imageWidth = image.width,
            imageHeight = image.height,
            imageFormat = image.format,
            planeCount = image.planes.size,
            rowStride = plane.rowStride,
            pixelStride = plane.pixelStride,
            bufferBytes = bufferBytes,
            expectedContiguousBytes = expectedContiguousBytes,
            canonicalContiguousRawSensor =
                plane.pixelStride == 2 &&
                    plane.rowStride == image.width * 2 &&
                    writtenBytes == expectedContiguousBytes,
        )
    }

    private fun buildAcquisitionEvidence(
        role: LensRole,
        candidate: Candidate,
        result: TotalCaptureResult,
        effectiveResult: CaptureResult,
        rawSeal: RawSensorSeal,
        dngFile: File?,
        dngSha: String?,
        dngError: String?,
    ): JSONObject {
        val physicalResults =
            JSONArray().also { out ->
                result.physicalCameraResults.keys.sorted().forEach(out::put)
            }

        return JSONObject()
            .put("schema", "D.RAW/UniversalPhysicalCapture/0.3")
            .put("created_at_utc", Instant.now().toString())
            .put("authority", "ACQUISITION_PROVENANCE_ONLY")
            .put("transport_backend", "ANDROID_CAMERA2")
            .put("transport_backend_is_scientific_truth", false)
            .put("device_map_required", false)
            .put("vendor_requests_written", false)
            .put("uses_ai_or_learned_model", false)
            .put("physical_frame_count", 1)
            .put("independent_evidence_count", 1)
            .put(
                "normal_resolution_policy",
                JSONObject()
                    .put(
                        "source",
                        "SCALER_STREAM_CONFIGURATION_MAP_RAW_SENSOR",
                    )
                    .put("highest_standard_raw_resolution_selected", true)
                    .put("maximum_resolution_map_used", false)
                    .put("special_200mp_route_used", false)
                    .put(
                        "open_world_output_resolution_may_differ",
                        true,
                    )
                    .put(
                        "open_world_output_resolution_upgrades_source_authority",
                        false,
                    ),
            )
            .put(
                "lens_role_ui_hint",
                JSONObject()
                    .put("role", role.name)
                    .put("authority", "UI_FOCAL_ORDER_HINT_ONLY")
                    .put("may_upgrade_scientific_authority", false),
            )
            .put(
                "capture_transport",
                JSONObject()
                    .put("logical_camera_id", candidate.logicalCameraId)
                    .put(
                        "requested_physical_camera_id",
                        candidate.physicalCameraId ?: JSONObject.NULL,
                    )
                    .put(
                        "effective_camera_id",
                        candidate.effectiveCameraId,
                    )
                    .put("discovery", candidate.discovery)
                    .put(
                        "reported_physical_result_ids",
                        physicalResults,
                    )
                    .put(
                        "characteristics_focal_length_mm_hint",
                        candidate.focalLengthMm ?: JSONObject.NULL,
                    )
                    .put("requested_format", "RAW_SENSOR")
                    .put("requested_width", candidate.rawSize.width)
                    .put("requested_height", candidate.rawSize.height),
            )
            .put(
                "preview_focus_assist",
                JSONObject()
                    .put(
                        "authority",
                        "PRESENTATION_AND_ACQUISITION_ASSIST_ONLY",
                    )
                    .put("preview_creates_evidence", false)
                    .put("display_loupe_scale", macroLoupeScale.toDouble())
                    .put("display_loupe_modifies_raw_capture", false)
                    .put("capture_zoom_ratio_written", false)
                    .put("capture_crop_region_written", false)
                    .put("tap_to_focus_available", true)
                    .put("focus_lock_requested", focusLocked)
                    .put(
                        "af_region",
                        currentAfRegion?.rect?.let { rect ->
                            JSONObject()
                                .put("left", rect.left)
                                .put("top", rect.top)
                                .put("right", rect.right)
                                .put("bottom", rect.bottom)
                        } ?: JSONObject.NULL,
                    )
                    .put("uses_ai_or_learned_model", false),
            )
            .put(
                "capture_result_provenance",
                JSONObject()
                    .put(
                        "sensor_timestamp_ns",
                        effectiveResult.get(
                            CaptureResult.SENSOR_TIMESTAMP,
                        ),
                    )
                    .put(
                        "iso",
                        effectiveResult.get(
                            CaptureResult.SENSOR_SENSITIVITY,
                        ),
                    )
                    .put(
                        "exposure_time_ns",
                        effectiveResult.get(
                            CaptureResult.SENSOR_EXPOSURE_TIME,
                        ),
                    )
                    .put(
                        "focal_length_mm",
                        effectiveResult.get(
                            CaptureResult.LENS_FOCAL_LENGTH,
                        ),
                    )
                    .put(
                        "focus_distance_diopters",
                        effectiveResult.get(
                            CaptureResult.LENS_FOCUS_DISTANCE,
                        ),
                    ),
            )
            .put(
                "sealed_primary_rawsensor",
                JSONObject()
                    .put("file", rawSeal.file.name)
                    .put("path_role", "PRIMARY_APP_VISIBLE_RAW_SENSOR_BYTE_EVIDENCE")
                    .put("sha256", rawSeal.sha256)
                    .put("bytes", rawSeal.bytes)
                    .put("image_width", rawSeal.imageWidth)
                    .put("image_height", rawSeal.imageHeight)
                    .put("image_format", rawSeal.imageFormat)
                    .put("plane_count", rawSeal.planeCount)
                    .put("row_stride", rawSeal.rowStride)
                    .put("pixel_stride", rawSeal.pixelStride)
                    .put("accessible_buffer_bytes", rawSeal.bufferBytes)
                    .put(
                        "expected_contiguous_raw16_bytes",
                        rawSeal.expectedContiguousBytes,
                    )
                    .put(
                        "canonical_contiguous_raw_sensor",
                        rawSeal.canonicalContiguousRawSensor,
                    )
                    .put("source_modified", false)
                    .put(
                        "authority_boundary",
                        "APP_VISIBLE_CAMERA_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF",
                    ),
            )
            .put(
                "derived_dng",
                JSONObject()
                    .put("attempted", true)
                    .put("file", dngFile?.name ?: JSONObject.NULL)
                    .put("sha256", dngSha ?: JSONObject.NULL)
                    .put("bytes", dngFile?.length() ?: JSONObject.NULL)
                    .put("error", dngError ?: JSONObject.NULL)
                    .put(
                        "role",
                        "DERIVED_COMPATIBILITY_CONTAINER_FOR_CURRENT_MAIN_HOUSE_INGRESS",
                    )
                    .put("replaces_primary_rawsensor", false),
            )
            .put(
                "source_first_ordering",
                "RAW_SENSOR_IMAGE -> EXACT_PLANE_SEAL_SHA256 -> ACQUISITION_METADATA -> DERIVED_DNG",
            )
            .put(
                "current_main_house_handoff",
                "DERIVED_DNG_TO_SAME_UNIVERSAL_SOURCE_INTAKE_AS_IMPORTED_RAW",
            )
            .put(
                "future_native_rawsensor_ingress_allowed",
                true,
            )
            .put(
                "scientific_master_created_by_capture_adapter",
                false,
            )
            .put("drawnegative_created_by_capture_adapter", false)
            .put(
                "frontside_or_backside_inference_created_by_capture_adapter",
                false,
            )
            .put("special_4k_to_200mp_route", false)
    }

    private fun restoreRoleButtons() {
        ultraButton.isEnabled =
            roleCandidates.containsKey(LensRole.ULTRA_WIDE)
        wideButton.isEnabled =
            roleCandidates.containsKey(LensRole.WIDE_MAIN)
        teleButton.isEnabled =
            roleCandidates.containsKey(LensRole.TELE)
    }

    private fun setButtonsEnabled(enabled: Boolean) {
        ultraButton.isEnabled =
            enabled && roleCandidates.containsKey(LensRole.ULTRA_WIDE)
        wideButton.isEnabled =
            enabled && roleCandidates.containsKey(LensRole.WIDE_MAIN)
        teleButton.isEnabled =
            enabled && roleCandidates.containsKey(LensRole.TELE)
    }

    private fun closeCaptureResources() {
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
        previewBufferSize = null
        lastPreviewResult = null
        previewFrames = 0
        focusLocked = false
        currentAfRegion = null
        activeCandidate = null
        activeRole = null
    }

    private fun sha256(file: File): String {
        val digest = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                digest.update(buffer, 0, n)
            }
        }
        return digest.digest().joinToString("") {
            "%02x".format(it)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(
            requestCode,
            permissions,
            grantResults,
        )
        if (requestCode == REQUEST_CAMERA_PERMISSION) {
            val granted =
                grantResults.firstOrNull() ==
                    PackageManager.PERMISSION_GRANTED
            if (!granted) {
                pendingPreviewRole = null
                status("Camera-permissie geweigerd; fysieke capture blijft geblokkeerd.")
                return
            }

            val role = pendingPreviewRole
            pendingPreviewRole = null
            if (role != null) {
                status("Camera-permissie toegestaan · ${role.title} live view wordt geopend.")
                previewView.post { openPreviewRole(role) }
            } else {
                status("Camera-permissie toegestaan. Kies een lensrol voor live view.")
            }
        }
    }

    private fun status(value: String) {
        statusView.text = value
    }

    private fun statusFromAnyThread(value: String) =
        runOnUiThread { status(value) }

    private fun text(
        value: String,
        size: Float,
        bold: Boolean,
        color: Int = DrawVisualTheme.INK,
    ): TextView =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(color)
            if (bold) {
                setTypeface(
                    typeface,
                    android.graphics.Typeface.BOLD,
                )
            }
            setLineSpacing(0f, 1.12f)
        }

    private fun button(
        label: String,
        action: () -> Unit,
    ): Button =
        Button(this).apply {
            text = label
            isAllCaps = false
            gravity = Gravity.CENTER
            setTextColor(Color.BLACK)
            setOnClickListener { action() }
        }

    private fun space(heightDp: Int): View =
        View(this).apply {
            layoutParams =
                LinearLayout.LayoutParams(1, dp(heightDp))
        }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_CAMERA_PERMISSION = 9201
    }
}

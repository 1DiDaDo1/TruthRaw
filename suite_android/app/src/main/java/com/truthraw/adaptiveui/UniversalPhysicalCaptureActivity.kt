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

        ultraButton = button("Ultra-wide · zoeken…") {
            captureRole(LensRole.ULTRA_WIDE)
        }.apply { isEnabled = false }
        wideButton = button("Wide / main · zoeken…") {
            captureRole(LensRole.WIDE_MAIN)
        }.apply { isEnabled = false }
        teleButton = button("Tele · zoeken…") {
            captureRole(LensRole.TELE)
        }.apply { isEnabled = false }

        root.addView(ultraButton)
        root.addView(space(7))
        root.addView(wideButton)
        root.addView(space(7))
        root.addView(teleButton)
        root.addView(space(14))

        root.addView(
            button("Speciale 4K → 200MP RAW-route") {
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
        setButtonsEnabled(false)
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

    private fun captureRole(role: LensRole) {
        val candidate = roleCandidates[role] ?: return

        if (
            checkSelfPermission(Manifest.permission.CAMERA) !=
            PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(
                arrayOf(Manifest.permission.CAMERA),
                REQUEST_CAMERA_PERMISSION,
            )
            status("Camera-permissie gevraagd. Kies daarna opnieuw ${role.title}.")
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
        setButtonsEnabled(false)
        status(
            "${role.title} wordt technisch geopend; RAW_SENSOR wordt eerst byte-exact verzegeld, DNG volgt alleen als afgeleide container.",
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
                        runOnUiThread { restoreRoleButtons() }
                        closeCaptureResources()
                    }

                    override fun onError(camera: CameraDevice, error: Int) {
                        camera.close()
                        statusFromAnyThread("Camera-open fout=$error.")
                        runOnUiThread { restoreRoleButtons() }
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

    private fun createSession(
        camera: CameraDevice,
        candidate: Candidate,
        reader: ImageReader,
    ) {
        val output = OutputConfiguration(reader.surface)
        candidate.physicalCameraId?.let { physicalId ->
            try {
                output.setPhysicalCameraId(physicalId)
            } catch (error: Exception) {
                statusFromAnyThread(
                    "Physical RAW-output kon niet worden gebonden: " +
                        (error.message ?: error.javaClass.simpleName),
                )
                runOnUiThread { restoreRoleButtons() }
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
                    submitCapture(camera, session, candidate, reader)
                }

                override fun onConfigureFailed(session: CameraCaptureSession) {
                    status("RAW capture-session kon niet worden geconfigureerd.")
                    restoreRoleButtons()
                    closeCaptureResources()
                }
            },
        )

        try {
            camera.createCaptureSession(config)
        } catch (error: Exception) {
            statusFromAnyThread(
                "Capture-session faalde: " +
                    (error.message ?: error.javaClass.simpleName),
            )
            runOnUiThread { restoreRoleButtons() }
            closeCaptureResources()
        }
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

                    val afModes =
                        effectiveCharacteristics.get(
                            CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES,
                        ) ?: intArrayOf()
                    when {
                        afModes.contains(
                            CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE,
                        ) ->
                            set(
                                CaptureRequest.CONTROL_AF_MODE,
                                CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE,
                            )
                        afModes.contains(CameraMetadata.CONTROL_AF_MODE_AUTO) ->
                            set(
                                CaptureRequest.CONTROL_AF_MODE,
                                CameraMetadata.CONTROL_AF_MODE_AUTO,
                            )
                        else ->
                            set(
                                CaptureRequest.CONTROL_AF_MODE,
                                CameraMetadata.CONTROL_AF_MODE_OFF,
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
                "RAW capture verstuurd · wachten op één Image + exact capture-resultaat…",
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
            .put("schema", "D.RAW/UniversalPhysicalCapture/0.2")
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
        runCatching { cameraSession?.close() }
        runCatching { cameraDevice?.close() }
        runCatching { imageReader?.close() }
        cameraSession = null
        cameraDevice = null
        imageReader = null
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
            status(
                if (
                    grantResults.firstOrNull() ==
                    PackageManager.PERMISSION_GRANTED
                ) {
                    "Camera-permissie toegestaan. Kies nu opnieuw een lensrol."
                } else {
                    "Camera-permissie geweigerd; fysieke capture blijft geblokkeerd."
                },
            )
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

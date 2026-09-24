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
import android.widget.ScrollView
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
 * Dedicated Camera-5 maximum-resolution acquisition test.
 *
 * Preview and RAW capture deliberately use separate Camera2 sessions. Preview
 * stays in ordinary sensor pixel mode for framing and 3A. Only the RAW-only
 * still-capture request asks for SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION. This
 * avoids requiring HONOR to support a preview + 200MP RAW stream combination.
 */
class FotoGraaf200MpTestActivity : Activity(), TextureView.SurfaceTextureListener {
    private lateinit var manager: CameraManager
    private lateinit var preview: TextureView
    private lateinit var status: TextView
    private lateinit var telemetry: TextView
    private lateinit var captureButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button
    private lateinit var restartPreviewButton: Button

    private val cameraThread = HandlerThread("truthraw-200mp-test").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var route: FotoGraafProRoute? = null
    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var previewSurface: Surface? = null
    private var rawReader: ImageReader? = null
    private var previewFrameCount = 0L
    @Volatile private var lastPreviewResult: TotalCaptureResult? = null

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var finalizing = false

    private var capturedDng: File? = null
    private var capturedJson: File? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        manager = getSystemService(CameraManager::class.java)
        setContentView(buildUi())
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
        } else discover200MpRoute()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA && grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            discover200MpRoute()
        } else setStatus("CAMERA permission ontbreekt; 200MP-test kan niet starten.")
    }

    override fun onPause() {
        closeAll()
        super.onPause()
    }

    override fun onDestroy() {
        closeAll()
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
            setPadding(dp(12), dp(12), dp(12), dp(24))
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        }

        root.addView(label("D.RAW · 200MP Tele Test", 25f, true))
        root.addView(label(
            "Één vaste route: logical 0 → physical 5 → MAX RAW 16320×12288. De live preview is alleen framing in normale previewmode; 200MP wordt pas bij de RAW-capture aangevraagd.",
            12f,
            false,
            Color.rgb(184, 191, 202),
        ))
        root.addView(space(8))

        preview = TextureView(this).apply {
            surfaceTextureListener = this@FotoGraaf200MpTestActivity
            setBackgroundColor(Color.BLACK)
        }
        val previewHeight = (resources.displayMetrics.widthPixels * 4L / 3L).toInt().coerceIn(dp(300), dp(560))
        root.addView(preview, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, previewHeight))
        root.addView(space(8))

        telemetry = label("Nog geen Camera-5 preview-resultaat.", 12f, true)
        status = label("Zoeken naar exact geadverteerde 200MP-route…", 12f, false, Color.WHITE)
        root.addView(telemetry)
        root.addView(space(6))
        root.addView(status)
        root.addView(space(8))

        captureButton = button("CAPTURE 200MP TEST · 16320×12288") { capture200Mp() }.apply { isEnabled = false }
        restartPreviewButton = button("Herstart tele-preview") { openPreview() }.apply { isEnabled = false }
        saveDngButton = button("200MP DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("200MP evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }
        root.addView(captureButton)
        root.addView(space(4))
        root.addView(restartPreviewButton)
        root.addView(space(4))
        root.addView(saveDngButton)
        root.addView(space(4))
        root.addView(saveJsonButton)
        root.addView(space(10))

        root.addView(label(
            "PASS vereist tegelijk: physical result camera 5, Image 16320×12288, exact timestamp-paar, RAW plane pixelStride=2, én CaptureResult SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION. Anders FAIL CLOSED.",
            10f,
            false,
            Color.rgb(150, 158, 170),
        ))
        return scroll
    }

    private fun discover200MpRoute() {
        setStatus("Camera2 maximum-resolution routes worden gelezen…")
        Thread({
            val found = runCatching {
                FotoGraafProRoutes.scan(manager).firstOrNull {
                    it.logicalCameraId == "0" &&
                        it.physicalCameraId == "5" &&
                        it.maximumResolution &&
                        it.rawSize.width == TARGET_W &&
                        it.rawSize.height == TARGET_H
                }
            }
            runOnUiThread {
                found.onSuccess { r ->
                    route = r
                    if (r == null) {
                        setStatus("BLOCKED: Camera2 adverteert nu geen physical-5 MAX RAW 16320×12288. Niets wordt geforceerd of verzonnen.")
                        captureButton.isEnabled = false
                    } else {
                        setStatus("ROUTE READY: ${r.label}\nPreview = ordinary tele framing. Capture = MAX-resolution RAW-only session.")
                        restartPreviewButton.isEnabled = preview.isAvailable
                        if (preview.isAvailable) openPreview()
                    }
                }.onFailure { e ->
                    setStatus("Route discovery faalde: ${e.javaClass.simpleName}: ${e.message}")
                }
            }
        }, "truthraw-200mp-discovery").start()
    }

    private fun openPreview() {
        val r = route ?: return
        if (!preview.isAvailable) return
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) return
        closeAll(keepOutputs = true)
        previewFrameCount = 0
        lastPreviewResult = null
        captureButton.isEnabled = false
        restartPreviewButton.isEnabled = false

        val characteristics = runCatching { FotoGraafProRoutes.effectiveCharacteristics(manager, r) }.getOrElse {
            setStatus("Camera-5 characteristics niet leesbaar: ${it.message}")
            return
        }
        val size = choosePreviewSize(characteristics)
        val texture = preview.surfaceTexture ?: return
        texture.setDefaultBufferSize(size.width, size.height)
        previewSurface = Surface(texture)
        setStatus("Tele-preview openen op physical 5 · ${size.width}×${size.height} · normale previewmode…")

        try {
            manager.openCamera(r.logicalCameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    camera = device
                    createPreviewSession(device, r)
                }
                override fun onDisconnected(device: CameraDevice) {
                    device.close()
                    setStatusAny("Camera disconnected.")
                    closeAll(keepOutputs = true)
                }
                override fun onError(device: CameraDevice, error: Int) {
                    device.close()
                    setStatusAny("Camera open error=$error")
                    closeAll(keepOutputs = true)
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatus("openCamera faalde: ${e.javaClass.simpleName}: ${e.message}")
            closeAll(keepOutputs = true)
        }
    }

    private fun createPreviewSession(device: CameraDevice, r: FotoGraafProRoute) {
        val surface = previewSurface ?: return
        val output = OutputConfiguration(surface)
        try {
            output.setPhysicalCameraId("5")
        } catch (e: Throwable) {
            setStatusAny("Physical-5 preview-binding faalde: ${e.javaClass.simpleName}: ${e.message}")
            closeAll(keepOutputs = true)
            return
        }
        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    startPreviewRepeating(device, s, r)
                }
                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("Preview-only session onConfigureFailed. Dit is een Camera2/HONOR routeprobleem; 200MP-capture wordt niet gestart.")
                    closeAll(keepOutputs = true)
                }
            },
        )
        runCatching { device.createCaptureSession(config) }
            .onFailure { setStatusAny("Preview createCaptureSession faalde: ${it.javaClass.simpleName}: ${it.message}") }
    }

    private fun startPreviewRepeating(device: CameraDevice, s: CameraCaptureSession, r: FotoGraafProRoute) {
        val surface = previewSurface ?: return
        val c = FotoGraafProRoutes.effectiveCharacteristics(manager, r)
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            b.addTarget(surface)
            b.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            b.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)
            val af = c.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
            if (af.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
                b.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
            }
            val ois = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
            if (ois.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)) {
                b.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)
            }
            // Deliberately DO NOT set SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION for preview.
            s.setRepeatingRequest(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    previewFrameCount++
                    lastPreviewResult = result
                    val effective: CaptureResult = result.physicalCameraResults["5"] ?: result
                    val iso = effective.get(CaptureResult.SENSOR_SENSITIVITY)
                    val exp = effective.get(CaptureResult.SENSOR_EXPOSURE_TIME)
                    val focal = effective.get(CaptureResult.LENS_FOCAL_LENGTH)
                    val afState = effective.get(CaptureResult.CONTROL_AF_STATE)
                    val aeState = effective.get(CaptureResult.CONTROL_AE_STATE)
                    val awbState = effective.get(CaptureResult.CONTROL_AWB_STATE)
                    runOnUiThread {
                        telemetry.text = "TELE PREVIEW OK · frames=$previewFrameCount · physical=5\nISO=${iso ?: "?"} · t=${exp?.div(1_000_000.0)?.let { String.format(Locale.ROOT, "%.3f ms", it) } ?: "?"} · focal=${focal ?: "?"}mm · AF=$afState · AE=$aeState · AWB=$awbState\n200MP nog NIET actief; alleen bij capture."
                        captureButton.isEnabled = true
                        restartPreviewButton.isEnabled = true
                    }
                }
            }, cameraHandler)
            setStatusAny("LIVE PREVIEW ACTIVE · physical 5. Je kunt nu bewust één 200MP RAW-test starten.")
        } catch (e: Throwable) {
            setStatusAny("Preview repeating faalde: ${e.javaClass.simpleName}: ${e.message}")
        }
    }

    private fun capture200Mp() {
        val r = route ?: return
        val device = camera ?: run {
            setStatus("Geen tele-preview/camera actief. Herstart eerst de preview.")
            return
        }
        if (lastPreviewResult == null) {
            setStatus("Wacht tot minimaal één physical-5 preview-resultaat zichtbaar is.")
            return
        }
        captureButton.isEnabled = false
        restartPreviewButton.isEnabled = false
        clearCaptureOutputs()
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

        rawReader = ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW_SENSOR, 1).also { reader ->
            reader.setOnImageAvailableListener({ source ->
                val image = runCatching { source.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
                synchronized(pairLock) {
                    pendingImage?.close()
                    pendingImage = image
                }
                finalizeIfPaired()
            }, cameraHandler)
        }

        val output = OutputConfiguration(rawReader!!.surface)
        try {
            output.setPhysicalCameraId("5")
        } catch (e: Throwable) {
            setStatus("FAIL CLOSED: physical-5 RAW outputbinding faalde: ${e.message}")
            restorePreviewAfterFailure()
            return
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    submit200MpStill(device, s, r)
                }
                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("FAIL CLOSED: RAW-only 16320×12288 session onConfigureFailed.")
                    restorePreviewAfterFailure()
                }
            },
        )
        setStatus("200MP RAW-only session wordt opgebouwd · 16320×12288 · maxImages=1…")
        runCatching { device.createCaptureSession(config) }
            .onFailure {
                setStatusAny("FAIL CLOSED: 200MP createCaptureSession: ${it.javaClass.simpleName}: ${it.message}")
                restorePreviewAfterFailure()
            }
    }

    private fun submit200MpStill(device: CameraDevice, s: CameraCaptureSession, r: FotoGraafProRoute) {
        val reader = rawReader ?: return
        val c = FotoGraafProRoutes.effectiveCharacteristics(manager, r)
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            b.addTarget(reader.surface)
            b.set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            b.set(CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON)
            b.set(CaptureRequest.SENSOR_PIXEL_MODE, CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
            val af = c.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES) ?: intArrayOf()
            if (af.contains(CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)) {
                b.set(CaptureRequest.CONTROL_AF_MODE, CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE)
            }
            val ois = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) ?: intArrayOf()
            if (ois.contains(CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)) {
                b.set(CaptureRequest.LENS_OPTICAL_STABILIZATION_MODE, CameraMetadata.LENS_OPTICAL_STABILIZATION_MODE_ON)
            }
            val nr = c.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) b.set(CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF)
            val edge = c.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) b.set(CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF)

            setStatusAny("200MP request verstuurd. Wachten op één RAW Image + exact TotalCaptureResult…")
            s.capture(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }
                override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: android.hardware.camera2.CaptureFailure) {
                    setStatusAny("FAIL CLOSED: 200MP CaptureFailure reason=${failure.reason}")
                    restorePreviewAfterFailure()
                }
            }, cameraHandler)
        } catch (e: Throwable) {
            setStatusAny("FAIL CLOSED: 200MP request bouwen/versturen: ${e.javaClass.simpleName}: ${e.message}")
            restorePreviewAfterFailure()
        }
    }

    private fun finalizeIfPaired() {
        val image: Image
        val result: TotalCaptureResult
        synchronized(pairLock) {
            if (finalizing) return
            image = pendingImage ?: return
            result = pendingResult ?: return
            val logicalTs = result.get(CaptureResult.SENSOR_TIMESTAMP)
            if (logicalTs == null || logicalTs != image.timestamp) {
                finalizing = true
                pendingImage = null
                pendingResult = null
                image.close()
                setStatusAny("FAIL CLOSED: Image.timestamp=${image.timestamp} != logical SENSOR_TIMESTAMP=$logicalTs")
                restorePreviewAfterFailure()
                return
            }
            finalizing = true
            pendingImage = null
            pendingResult = null
        }
        cameraHandler.post { finalize200Mp(image, result) }
    }

    private fun finalize200Mp(image: Image, result: TotalCaptureResult) {
        try {
            if (image.width != TARGET_W || image.height != TARGET_H) {
                throw IllegalStateException("RAW dimensions ${image.width}×${image.height}, verwacht ${TARGET_W}×${TARGET_H}")
            }
            val physical = result.physicalCameraResults["5"]
                ?: throw IllegalStateException("physical Camera-5 TotalCaptureResult ontbreekt; reported=${result.physicalCameraResults.keys}")
            val physicalTs = physical.get(CaptureResult.SENSOR_TIMESTAMP)
            if (physicalTs != null && physicalTs != image.timestamp) {
                throw IllegalStateException("physical SENSOR_TIMESTAMP=$physicalTs != Image.timestamp=${image.timestamp}")
            }
            val pixelMode = physical.get(CaptureResult.SENSOR_PIXEL_MODE)
            if (pixelMode != CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION) {
                throw IllegalStateException("physical Camera-5 SENSOR_PIXEL_MODE=$pixelMode, MAX vereist")
            }
            val planeSeal = sealRawPlane(image)
            val c = manager.getCameraCharacteristics("5")
            val stamp = System.currentTimeMillis()
            val dng = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_${TARGET_W}x${TARGET_H}.dng")
            FileOutputStream(dng).use { out ->
                DngCreator(c, physical).use { creator ->
                    creator.setOrientation(1)
                    creator.writeImage(out, image)
                }
            }
            image.close()
            val dngSha = sha256(dng)
            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_200MP_STEP3B_evidence_v0_6.json")
            report.writeText(buildEvidence(result, physical, planeSeal, dng, dngSha, pixelMode).toString(2))
            capturedDng = dng
            capturedJson = report
            runOnUiThread {
                saveDngButton.isEnabled = true
                saveJsonButton.isEnabled = true
                restartPreviewButton.isEnabled = true
                telemetry.text = "200MP CAPTURE PASS CANDIDATE\nphysical=5 · ${TARGET_W}×${TARGET_H} · samples=$TARGET_SAMPLES\nrawPayloadSHA=${planeSeal.sha256.take(20)}… · DNG SHA=${dngSha.take(20)}…"
                setStatus("PASS APP-VISIBLE MAX RAW CANDIDATE · MAX pixel mode bevestigd. Dit bewijst nog niet untouched native ADC en wordt nog door Step-3B host/replay gevalideerd.")
            }
            closeCaptureSessionOnly()
        } catch (e: Throwable) {
            runCatching { image.close() }
            setStatusAny("200MP FINALIZE FAIL CLOSED: ${e.javaClass.simpleName}: ${e.message}")
            restorePreviewAfterFailure()
        }
    }

    private data class RawPlaneSeal(
        val sha256: String,
        val rowStride: Int,
        val pixelStride: Int,
        val validPayloadBytes: Long,
        val bufferRemainingBytes: Int,
    )

    private fun sealRawPlane(image: Image): RawPlaneSeal {
        val plane = image.planes.singleOrNull() ?: throw IllegalStateException("RAW_SENSOR moet exact één plane hebben")
        val pixelStride = plane.pixelStride
        val rowStride = plane.rowStride
        if (pixelStride != 2) throw IllegalStateException("RAW pixelStride=$pixelStride; exact 16-bit sample container verwacht")
        val rowBytes = image.width * pixelStride
        if (rowStride < rowBytes) throw IllegalStateException("RAW rowStride=$rowStride < rowBytes=$rowBytes")
        val src = plane.buffer.duplicate()
        val base = src.position()
        val needed = base.toLong() + (image.height - 1L) * rowStride.toLong() + rowBytes.toLong()
        if (needed > src.limit().toLong()) throw IllegalStateException("RAW plane buffer te klein voor stride geometry")
        val md = MessageDigest.getInstance("SHA-256")
        val scratch = ByteArray(1024 * 1024)
        for (y in 0 until image.height) {
            src.position(base + y * rowStride)
            var left = rowBytes
            while (left > 0) {
                val n = minOf(left, scratch.size)
                src.get(scratch, 0, n)
                md.update(scratch, 0, n)
                left -= n
            }
        }
        return RawPlaneSeal(
            sha256 = md.digest().joinToString("") { "%02x".format(it) },
            rowStride = rowStride,
            pixelStride = pixelStride,
            validPayloadBytes = image.width.toLong() * image.height.toLong() * pixelStride.toLong(),
            bufferRemainingBytes = plane.buffer.remaining(),
        )
    }

    private fun buildEvidence(
        logical: TotalCaptureResult,
        physical: CaptureResult,
        plane: RawPlaneSeal,
        dng: File,
        dngSha: String,
        pixelMode: Int?,
    ): JSONObject {
        val gains = physical.get(CaptureResult.COLOR_CORRECTION_GAINS)
        val transform = physical.get(CaptureResult.COLOR_CORRECTION_TRANSFORM)
        return JSONObject()
            .put("schema", "truthraw.camera5-200mp-step3b-device-evidence.v0.6")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE")
            .put("calibrationAuthorityGranted", false)
            .put("scientificWriteback", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("route", JSONObject()
                .put("logicalCameraId", "0")
                .put("physicalCameraId", "5")
                .put("width", TARGET_W)
                .put("height", TARGET_H)
                .put("sampleCount", TARGET_SAMPLES)
                .put("sensorPixelMode", pixelMode ?: JSONObject.NULL)
                .put("maximumResolutionConfirmed", pixelMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
                .put("reportedPhysicalCameraIds", JSONArray(logical.physicalCameraResults.keys.sorted())))
            .put("timestampBinding", JSONObject()
                .put("imageTimestampNs", physical.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("logicalSensorTimestampNs", logical.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL))
            .put("rawPlane", JSONObject()
                .put("sha256ValidSampleBytes", plane.sha256)
                .put("rowStride", plane.rowStride)
                .put("pixelStride", plane.pixelStride)
                .put("validPayloadBytes", plane.validPayloadBytes)
                .put("bufferRemainingBytes", plane.bufferRemainingBytes))
            .put("captureResult", JSONObject()
                .put("iso", physical.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", physical.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("frameDurationNs", physical.get(CaptureResult.SENSOR_FRAME_DURATION) ?: JSONObject.NULL)
                .put("focusDistanceDiopters", physical.get(CaptureResult.LENS_FOCUS_DISTANCE) ?: JSONObject.NULL)
                .put("focalLengthMm", physical.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("oisMode", physical.get(CaptureResult.LENS_OPTICAL_STABILIZATION_MODE) ?: JSONObject.NULL)
                .put("afState", physical.get(CaptureResult.CONTROL_AF_STATE) ?: JSONObject.NULL)
                .put("aeState", physical.get(CaptureResult.CONTROL_AE_STATE) ?: JSONObject.NULL)
                .put("awbState", physical.get(CaptureResult.CONTROL_AWB_STATE) ?: JSONObject.NULL)
                .put("colorCorrectionGains", gains?.toString() ?: JSONObject.NULL)
                .put("colorCorrectionTransform", transform?.toString() ?: JSONObject.NULL))
            .put("dng", JSONObject()
                .put("bytes", dng.length())
                .put("sha256", dngSha)
                .put("orientationRequested", 1))
            .put("boundary", "APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_NOT_UNTOUCHED_NATIVE_ADC_PROOF")
    }

    private fun restorePreviewAfterFailure() {
        runOnUiThread { restartPreviewButton.isEnabled = true }
        closeCaptureSessionOnly()
    }

    private fun closeCaptureSessionOnly() {
        runCatching { session?.close() }
        session = null
        runCatching { rawReader?.close() }
        rawReader = null
    }

    private fun closeAll(keepOutputs: Boolean = false) {
        runCatching { session?.close() }
        session = null
        runCatching { rawReader?.close() }
        rawReader = null
        runCatching { camera?.close() }
        camera = null
        runCatching { previewSurface?.release() }
        previewSurface = null
        if (!keepOutputs) clearCaptureOutputs()
    }

    private fun clearCaptureOutputs() {
        capturedDng = null
        capturedJson = null
        if (::saveDngButton.isInitialized) saveDngButton.isEnabled = false
        if (::saveJsonButton.isInitialized) saveJsonButton.isEnabled = false
    }

    private fun choosePreviewSize(c: CameraCharacteristics): android.util.Size {
        val sizes = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)?.toList().orEmpty()
        return sizes.filter { it.width <= 1920 && it.height <= 1440 }
            .maxByOrNull { it.width.toLong() * it.height.toLong() }
            ?: sizes.firstOrNull()
            ?: android.util.Size(1280, 960)
    }

    private fun saveFile(file: File?, mime: String, requestCode: Int) {
        if (file == null || !file.exists()) return
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime
            putExtra(Intent.EXTRA_TITLE, file.name)
        }, requestCode)
    }

    @Deprecated("Activity result API retained for minSdk31 simple document export")
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
        }.onSuccess { setStatus("${source.name} opgeslagen.") }
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

    private fun label(value: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(color)
        if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
    }

    private fun button(value: String, action: () -> Unit): Button = Button(this).apply {
        text = value
        isAllCaps = false
        minHeight = dp(52)
        setOnClickListener { action() }
    }

    private fun space(height: Int): View = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun setStatus(value: String) { if (::status.isInitialized) status.text = value }
    private fun setStatusAny(value: String) { runOnUiThread { setStatus(value) } }
    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        restartPreviewButton.isEnabled = route != null
        if (route != null) openPreview()
    }
    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture, width: Int, height: Int) = Unit
    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean { closeAll(keepOutputs = true); return true }
    override fun onSurfaceTextureUpdated(surface: SurfaceTexture) = Unit

    companion object {
        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val TARGET_SAMPLES = 200_540_160L
        private const val REQUEST_CAMERA = 6200
        private const val REQUEST_SAVE_DNG = 6201
        private const val REQUEST_SAVE_JSON = 6202
    }
}

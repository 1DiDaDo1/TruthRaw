package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.ImageFormat
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
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
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
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest
import java.time.Instant

/**
 * TruthRaw FotoGraaf direct Camera-5 full-raster experiment v0.15.
 *
 * Why this exists:
 * - v0.14 proved that logical 0 -> physical 5 can deliver an app-visible Image labelled
 *   RAW_SENSOR 16320x12288 with exact physical-5 timestamp binding.
 * - the returned physical SENSOR_PIXEL_MODE was DEFAULT(0), not MAXIMUM_RESOLUTION.
 * - host inspection of the auxiliary v0.14 DNG found only the first 768 full-width rows
 *   non-zero. 768*16320*2 == 4080*3072*2 == 25,067,520 bytes: exactly one sixteenth
 *   of the declared 401,080,320-byte 200MP raster and exactly one standard 4080x3072
 *   16-bit RAW payload.
 *
 * v0.15 therefore tests the next falsifiable route instead of promoting v0.14:
 * - open public Camera ID 5 directly, if Android exposes it in cameraIdList;
 * - configure a 16320x12288 RAW_SENSOR output from the maximum-resolution high-res map;
 * - mark the OutputConfiguration for SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION;
 * - write SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION as the direct camera's normal/global key;
 * - seal the original Image.Plane[0] buffer before any DNG conversion;
 * - hash sixteen equal 25,067,520-byte raster bands independently;
 * - refuse to call the raster complete when any whole band is exactly all zero;
 * - only create an auxiliary DNG from DngCreator.writeByteBuffer when the source buffer
 *   itself has complete non-zero band coverage.
 *
 * This is still CAMERA2_ACQUISITION_OBSERVATION_ONLY. Even a full-raster PASS does not
 * establish untouched photodiode/ADC truth, independent ADC conversion per photosite,
 * optical resolution, electron calibration, or absence of on-sensor/HAL processing.
 */
class FotoGraaf200MpFullRasterV015Activity : Activity() {

    private lateinit var status: TextView
    private lateinit var capabilityButton: Button
    private lateinit var captureButton: Button
    private lateinit var saveRawButton: Button
    private lateinit var saveDngButton: Button
    private lateinit var saveJsonButton: Button

    private val cameraThread = HandlerThread("truthraw-cam5-full-raster-v015").apply { start() }
    private val cameraHandler = Handler(cameraThread.looper)

    private var manager: CameraManager? = null
    private var camera5Characteristics: CameraCharacteristics? = null
    private var camera5Listed = false
    private var maxHighTargetAdvertised = false
    private var sensorPixelModeRequestAdvertised = false
    private var capabilityReady = false

    private var camera: CameraDevice? = null
    private var session: CameraCaptureSession? = null
    private var reader: ImageReader? = null

    private val pairLock = Any()
    private var pendingImage: Image? = null
    private var pendingResult: TotalCaptureResult? = null
    private var finalizing = false

    private var requestPixelModeWriteSucceeded = false
    private var requestPixelModeReadback: Int? = null
    private var sessionSupport: Boolean? = null

    private var capturedRaw: File? = null
    private var capturedDng: File? = null
    private var capturedJson: File? = null

    data class RawSeal(
        val file: File,
        val sha256: String,
        val bytes: Long,
        val accessibleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val contiguous: Boolean,
        val bandHashes: List<String>,
        val allZeroBandIndices: List<Int>,
        val fullRasterBandCoveragePass: Boolean,
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(buildUi())
        setStatus(
            "STAGE 0 PASS · v0.15 opent zonder camera.\n" +
                "Doel: direct Camera 5 + echte MAX request + 16-band rastercontrole.\nDruk Stap 1.",
        )
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA)
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
        }
        cameraThread.quitSafely()
        super.onDestroy()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA) {
            setStatus(
                if (grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED)
                    "CAMERA permission verleend. Druk Stap 1."
                else
                    "CAMERA permission ontbreekt; test blijft fail-closed.",
            )
        }
    }

    private fun buildUi(): View {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(14), dp(14), dp(14), dp(20))
            setBackgroundColor(Color.rgb(9, 11, 14))
        }
        root.addView(text("TruthRaw · Tele 200MP Full Raster v0.15", 23f, true, Color.WHITE))
        root.addView(text(
            "Nieuwe route: Camera ID 5 rechtstreeks openen. Geen preview nodig voor deze payloadtest. De RAW-buffer wordt vóór DNG verzegeld en in 16 gelijke banden gehasht.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        root.addView(space(10))

        status = text("Initialiseren…", 12f, false, Color.WHITE)
        root.addView(status, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))

        capabilityButton = button("Stap 1 · controleer directe Camera-5 MAX route") { readCapability() }
        captureButton = button("Stap 2 · DIRECT Camera 5 · capture 16320×12288") { captureDirect() }.apply { isEnabled = false }
        saveRawButton = button("Originele RAW-buffer opslaan") { saveFile(capturedRaw, "application/octet-stream", REQUEST_SAVE_RAW) }.apply { isEnabled = false }
        saveDngButton = button("Gevalideerde auxiliary DNG opslaan") { saveFile(capturedDng, "image/x-adobe-dng", REQUEST_SAVE_DNG) }.apply { isEnabled = false }
        saveJsonButton = button("v0.15 evidence JSON opslaan") { saveFile(capturedJson, "application/json", REQUEST_SAVE_JSON) }.apply { isEnabled = false }

        root.addView(capabilityButton)
        root.addView(captureButton)
        root.addView(saveRawButton)
        root.addView(saveDngButton)
        root.addView(saveJsonButton)
        root.addView(text(
            "PASS betekent app-visible volledige rasterinhoud voor deze Camera2-route. Het betekent niet untouched/native photodiode-ADC truth.",
            10f, false, Color.rgb(145, 153, 165),
        ))
        return root
    }

    private fun readCapability() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            setStatus("Stap 1 geblokkeerd: CAMERA permission ontbreekt.")
            return
        }
        capabilityButton.isEnabled = false
        captureButton.isEnabled = false
        setStatus("Stap 1 bezig · cameraIdList + Camera-5 maximum-resolution map lezen…")

        Thread({
            val result = runCatching {
                val m = getSystemService(CameraManager::class.java)
                val ids = m.cameraIdList.toList()
                val listed = ids.contains(CAMERA_ID)
                val c5 = m.getCameraCharacteristics(CAMERA_ID)
                val maxMap = c5.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                val maxHigh = runCatching {
                    maxMap?.getHighResolutionOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
                }.getOrDefault(emptyList())
                val target = maxHigh.any { it.width == TARGET_W && it.height == TARGET_H }
                val keyAdvertised = c5.availableCaptureRequestKeys?.contains(CaptureRequest.SENSOR_PIXEL_MODE) == true
                val logicalPhysical = runCatching {
                    m.getCameraCharacteristics(LOGICAL_ID).physicalCameraIds.sorted()
                }.getOrDefault(emptyList())
                val maxPixelArray = c5.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION)
                Capability(ids, listed, maxHigh, target, keyAdvertised, logicalPhysical, maxPixelArray)
            }
            runOnUiThread {
                capabilityButton.isEnabled = true
                result.onSuccess { cap ->
                    manager = getSystemService(CameraManager::class.java)
                    camera5Characteristics = manager!!.getCameraCharacteristics(CAMERA_ID)
                    camera5Listed = cap.camera5Listed
                    maxHighTargetAdvertised = cap.targetAdvertised
                    sensorPixelModeRequestAdvertised = cap.pixelModeKeyAdvertised
                    capabilityReady = cap.camera5Listed && cap.targetAdvertised
                    captureButton.isEnabled = capabilityReady
                    setStatus(buildString {
                        append("STAGE 1 ${if (capabilityReady) "PASS" else "BLOCKED"}\n")
                        append("cameraIdList=${cap.cameraIds}\n")
                        append("Camera 5 direct listed=${cap.camera5Listed}\n")
                        append("logical0 physicalIds=${cap.logicalPhysicalIds}\n")
                        append("maximum.high RAW=[${cap.maxHigh.joinToString { "${it.width}×${it.height}" }}]\n")
                        append("target 16320×12288 advertised=${cap.targetAdvertised}\n")
                        append("Camera5 global SENSOR_PIXEL_MODE request-key advertised=${cap.pixelModeKeyAdvertised}\n")
                        append("maximum pixel array=${cap.maximumPixelArray ?: "not reported"}\n")
                        if (capabilityReady) append("Druk Stap 2. Camera 5 wordt nu rechtstreeks geopend.")
                        else if (!cap.camera5Listed) append("Direct-open route is niet publiek beschikbaar; niets wordt geforceerd.")
                    })
                }.onFailure { e ->
                    capabilityReady = false
                    setStatus("STAGE 1 FAIL · ${e.javaClass.simpleName}: ${e.message}")
                }
            }
        }, "truthraw-v015-capability").start()
    }

    private fun captureDirect() {
        if (!capabilityReady || !camera5Listed || !maxHighTargetAdvertised) {
            setStatus("Stap 2 geblokkeerd: directe Camera-5 16320×12288 capability is niet PASS.")
            return
        }
        clearOutputs()
        closeCamera()
        synchronized(pairLock) {
            pendingImage?.close()
            pendingImage = null
            pendingResult = null
            finalizing = false
        }
        captureButton.isEnabled = false
        capabilityButton.isEnabled = false
        setStatus("Stap 2 · Camera ID 5 DIRECT openen…")

        val m = manager ?: getSystemService(CameraManager::class.java)
        try {
            @Suppress("MissingPermission")
            m.openCamera(CAMERA_ID, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    camera = device
                    createRawSession(device)
                }

                override fun onDisconnected(device: CameraDevice) {
                    device.close()
                    setStatusAny("DIRECT Camera 5 disconnected.")
                    finishAttemptUi()
                }

                override fun onError(device: CameraDevice, error: Int) {
                    device.close()
                    setStatusAny("DIRECT Camera 5 OPEN ERROR=$error")
                    finishAttemptUi()
                }
            }, cameraHandler)
        } catch (t: Throwable) {
            setStatus("DIRECT Camera 5 open FAIL · ${t.javaClass.simpleName}: ${t.message}")
            finishAttemptUi()
        }
    }

    private fun createRawSession(device: CameraDevice) {
        val r = try {
            ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW_SENSOR, 1)
        } catch (t: Throwable) {
            setStatusAny("ImageReader 16320×12288 FAIL · ${t.javaClass.simpleName}: ${t.message}")
            finishAttemptUi()
            return
        }
        reader = r
        r.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull() ?: return@setOnImageAvailableListener
            synchronized(pairLock) {
                pendingImage?.close()
                pendingImage = image
            }
            finalizeIfPaired()
        }, cameraHandler)

        val output = OutputConfiguration(r.surface)
        val outputMode = runCatching {
            output.addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
        }
        if (outputMode.isFailure) {
            setStatusAny("DIRECT output MAX bind FAIL · ${outputMode.exceptionOrNull()?.javaClass?.simpleName}: ${outputMode.exceptionOrNull()?.message}")
            finishAttemptUi()
            return
        }

        val config = SessionConfiguration(
            SessionConfiguration.SESSION_REGULAR,
            listOf(output),
            mainExecutor,
            object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(s: CameraCaptureSession) {
                    session = s
                    submitDirectMaxRequest(device, s)
                }

                override fun onConfigureFailed(s: CameraCaptureSession) {
                    setStatusAny("DIRECT Camera-5 MAX RAW session configure FAIL")
                    finishAttemptUi()
                }
            },
        )

        sessionSupport = runCatching { device.isSessionConfigurationSupported(config) }.getOrNull()
        setStatusAny("DIRECT Camera 5 geopend · MAX RAW session support=$sessionSupport")
        if (sessionSupport == false) {
            setStatusAny("DIRECT route BLOCKED · Android meldt exacte 200MP MAX sessie unsupported.")
            finishAttemptUi()
            return
        }
        runCatching { device.createCaptureSession(config) }
            .onFailure { e ->
                setStatusAny("DIRECT createCaptureSession FAIL · ${e.javaClass.simpleName}: ${e.message}")
                finishAttemptUi()
            }
    }

    private fun submitDirectMaxRequest(device: CameraDevice, s: CameraCaptureSession) {
        val r = reader ?: return
        val chars = camera5Characteristics ?: return
        try {
            val b = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            b.addTarget(r.surface)

            requestPixelModeWriteSucceeded = runCatching {
                b.set(CaptureRequest.SENSOR_PIXEL_MODE, CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
            }.isSuccess
            requestPixelModeReadback = runCatching { b.get(CaptureRequest.SENSOR_PIXEL_MODE) }.getOrNull()
            if (!requestPixelModeWriteSucceeded || requestPixelModeReadback != CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION) {
                setStatusAny(
                    "DIRECT request BLOCKED vóór submit · SENSOR_PIXEL_MODE MAX kon niet exact worden gezet.\n" +
                        "advertised=$sensorPixelModeRequestAdvertised write=$requestPixelModeWriteSucceeded readback=$requestPixelModeReadback",
                )
                finishAttemptUi()
                return
            }

            setIfSupported(b, CaptureRequest.CONTROL_ENABLE_ZSL, false, chars)
            setIfSupported(b, CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO, chars)
            setIfSupported(b, CaptureRequest.CONTROL_AE_MODE, CameraMetadata.CONTROL_AE_MODE_ON, chars)
            val nr = chars.get(CameraCharacteristics.NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) ?: intArrayOf()
            if (nr.contains(CameraMetadata.NOISE_REDUCTION_MODE_OFF)) {
                setIfSupported(b, CaptureRequest.NOISE_REDUCTION_MODE, CameraMetadata.NOISE_REDUCTION_MODE_OFF, chars)
            }
            val edge = chars.get(CameraCharacteristics.EDGE_AVAILABLE_EDGE_MODES) ?: intArrayOf()
            if (edge.contains(CameraMetadata.EDGE_MODE_OFF)) {
                setIfSupported(b, CaptureRequest.EDGE_MODE, CameraMetadata.EDGE_MODE_OFF, chars)
            }

            setStatusAny(
                "DIRECT CAPTURE SENT · camera=5 · outputMAX=true · globalMAX=$requestPixelModeReadback\n" +
                    "Wachten op RAW Image + Camera-5 TotalCaptureResult…",
            )
            s.capture(b.build(), object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(session: CameraCaptureSession, request: CaptureRequest, result: TotalCaptureResult) {
                    synchronized(pairLock) { pendingResult = result }
                    finalizeIfPaired()
                }

                override fun onCaptureFailed(session: CameraCaptureSession, request: CaptureRequest, failure: CaptureFailure) {
                    setStatusAny(
                        "DIRECT CAPTURE FAIL · reason=${failure.reason} · wasImageCaptured=${failure.wasImageCaptured()} · " +
                            "sequenceId=${failure.sequenceId} · frameNumber=${failure.frameNumber}",
                    )
                    finishAttemptUi()
                }
            }, cameraHandler)
        } catch (t: Throwable) {
            setStatusAny("DIRECT request FAIL · ${t.javaClass.simpleName}: ${t.message}")
            finishAttemptUi()
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

    private fun finalizeCapture(image: Image, result: TotalCaptureResult) {
        var seal: RawSeal? = null
        var dng: File? = null
        var dngSha: String? = null
        var dngError: String? = null
        val stamp = System.currentTimeMillis()
        try {
            require(image.width == TARGET_W && image.height == TARGET_H) {
                "Image dimensions=${image.width}×${image.height}; exact ${TARGET_W}×${TARGET_H} required"
            }
            val sensorTs = result.get(CaptureResult.SENSOR_TIMESTAMP)
                ?: error("DIRECT Camera-5 SENSOR_TIMESTAMP missing")
            require(sensorTs == image.timestamp) {
                "Image.timestamp=${image.timestamp} != Camera5 SENSOR_TIMESTAMP=$sensorTs"
            }
            val plane = image.planes.singleOrNull()
                ?: error("RAW_SENSOR planeCount=${image.planes.size}; exact 1 required")
            require(plane.pixelStride == 2) { "pixelStride=${plane.pixelStride}; expected 2" }
            require(plane.rowStride >= TARGET_W * 2) { "rowStride=${plane.rowStride}; too small" }

            // The source buffer is sealed before interpreting result pixel mode or making a DNG.
            seal = persistAndBandHash(image, stamp)
            capturedRaw = seal.file

            val returnedPixelMode = result.get(CaptureResult.SENSOR_PIXEL_MODE)
            val fullRaster = seal.fullRasterBandCoveragePass

            if (fullRaster && seal.contiguous) {
                runCatching {
                    val chars = camera5Characteristics ?: error("Camera-5 characteristics unavailable")
                    val outFile = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_DIRECT_200MP_${TARGET_W}x${TARGET_H}_v015.dng")
                    val src = image.planes[0].buffer.duplicate().apply {
                        rewind()
                        order(ByteOrder.nativeOrder())
                    }
                    require(src.remaining().toLong() == EXPECTED_BYTES) {
                        "DNG source bytes=${src.remaining()} != $EXPECTED_BYTES"
                    }
                    FileOutputStream(outFile).use { out ->
                        DngCreator(chars, result).use { creator ->
                            creator.setOrientation(1)
                            creator.writeByteBuffer(out, Size(TARGET_W, TARGET_H), src, 0L)
                        }
                    }
                    dng = outFile
                    dngSha = sha256File(outFile)
                }.onFailure { e ->
                    dngError = "${e.javaClass.simpleName}: ${e.message}"
                }
            } else {
                dngError = "DNG intentionally skipped: source RAW failed full-raster band-coverage gate"
            }

            val report = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_DIRECT_200MP_EVIDENCE_v015.json")
            report.writeText(buildEvidence(image, result, seal, dng, dngSha, dngError).toString(2))
            capturedDng = dng
            capturedJson = report

            val modeText = returnedPixelMode?.toString() ?: "not reported"
            val bandText = if (seal.allZeroBandIndices.isEmpty()) "none" else seal.allZeroBandIndices.joinToString()
            setStatusAny(
                if (fullRaster) {
                    "STAGE 2 FULL-RASTER PASS · DIRECT Camera 5 · 16320×12288.\n" +
                        "16/16 bands contain data; returned SENSOR_PIXEL_MODE=$modeText.\n" +
                        "RAW sealed first; DNG writer=DngCreator.writeByteBuffer."
                } else {
                    "STAGE 2 CONTENT BLOCKED · DIRECT Camera 5 delivered the declared 16320×12288 buffer, " +
                        "but whole all-zero raster bands remain: [$bandText].\n" +
                        "RAW + JSON are preserved; DNG is not promoted. returned SENSOR_PIXEL_MODE=$modeText"
                },
            )
            runOnUiThread {
                saveRawButton.isEnabled = true
                saveJsonButton.isEnabled = true
                saveDngButton.isEnabled = capturedDng != null
            }
        } catch (t: Throwable) {
            if (seal != null) capturedRaw = seal.file
            setStatusAny("DIRECT FINALIZE FAIL CLOSED · ${t.javaClass.simpleName}: ${t.message}")
            runOnUiThread {
                saveRawButton.isEnabled = capturedRaw != null
                saveJsonButton.isEnabled = capturedJson != null
            }
        } finally {
            runCatching { image.close() }
            closeCamera()
            runOnUiThread {
                capabilityButton.isEnabled = true
                captureButton.isEnabled = capabilityReady
            }
        }
    }

    private fun persistAndBandHash(image: Image, stamp: Long): RawSeal {
        val plane = image.planes.single()
        val src = plane.buffer.duplicate().apply { rewind() }
        val accessible = src.remaining().toLong()
        val contiguous = plane.pixelStride == 2 && plane.rowStride == TARGET_W * 2 && accessible == EXPECTED_BYTES
        val file = File(
            cacheDir,
            "TRUTHRAW_${stamp}_CAM5_DIRECT_200MP_${TARGET_W}x${TARGET_H}_v015.${if (contiguous) "rawsensor" else "rawbuffer"}",
        )
        val whole = MessageDigest.getInstance("SHA-256")
        val bandDigests = List(BAND_COUNT) { MessageDigest.getInstance("SHA-256") }
        val scratch = ByteArray(1024 * 1024)
        var rasterOffset = 0L

        FileOutputStream(file).channel.use { channel ->
            while (src.hasRemaining()) {
                val n = minOf(src.remaining(), scratch.size)
                src.get(scratch, 0, n)
                whole.update(scratch, 0, n)

                var p = 0
                while (p < n && rasterOffset < EXPECTED_BYTES) {
                    val band = (rasterOffset / BAND_BYTES).toInt().coerceIn(0, BAND_COUNT - 1)
                    val withinBand = rasterOffset % BAND_BYTES
                    val take = minOf(
                        n - p,
                        (BAND_BYTES - withinBand).toInt(),
                        (EXPECTED_BYTES - rasterOffset).toInt(),
                    )
                    bandDigests[band].update(scratch, p, take)
                    p += take
                    rasterOffset += take.toLong()
                }

                val writeBuf = ByteBuffer.wrap(scratch, 0, n)
                while (writeBuf.hasRemaining()) channel.write(writeBuf)
            }
            channel.force(true)
        }

        val wholeHash = whole.digest().hex()
        val bandHashes = bandDigests.map { it.digest().hex() }
        val allZero = bandHashes.mapIndexedNotNull { index, hash ->
            index.takeIf { hash == ZERO_BAND_SHA256 }
        }
        val coverage = contiguous && bandHashes.size == BAND_COUNT && allZero.isEmpty()
        return RawSeal(
            file = file,
            sha256 = wholeHash,
            bytes = file.length(),
            accessibleBytes = accessible,
            rowStride = plane.rowStride,
            pixelStride = plane.pixelStride,
            contiguous = contiguous,
            bandHashes = bandHashes,
            allZeroBandIndices = allZero,
            fullRasterBandCoveragePass = coverage,
        )
    }

    private fun buildEvidence(
        image: Image,
        result: TotalCaptureResult,
        raw: RawSeal,
        dng: File?,
        dngSha: String?,
        dngError: String?,
    ): JSONObject {
        val returnedMode = result.get(CaptureResult.SENSOR_PIXEL_MODE)
        return JSONObject()
            .put("schema", "truthraw.fotograaf-camera5-direct-full-raster-evidence.v0.15")
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_ACQUISITION_OBSERVATION_ONLY")
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("route", JSONObject()
                .put("openedCameraId", CAMERA_ID)
                .put("directCamera5Listed", camera5Listed)
                .put("targetFromMaximumResolutionHighResRawList", maxHighTargetAdvertised)
                .put("outputMaximumResolutionModeDeclared", true)
                .put("sensorPixelModeRequestKeyAdvertised", sensorPixelModeRequestAdvertised)
                .put("requestSensorPixelModeWriteSucceeded", requestPixelModeWriteSucceeded)
                .put("requestSensorPixelModeReadback", requestPixelModeReadback ?: JSONObject.NULL)
                .put("sessionConfigurationSupported", sessionSupport ?: JSONObject.NULL))
            .put("capture", JSONObject()
                .put("resultCameraId", result.cameraId)
                .put("width", image.width)
                .put("height", image.height)
                .put("declaredSampleCount", image.width.toLong() * image.height.toLong())
                .put("sensorTimestampNs", result.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("imageTimestampNs", image.timestamp)
                .put("timestampIdentityPass", result.get(CaptureResult.SENSOR_TIMESTAMP) == image.timestamp)
                .put("returnedSensorPixelMode", returnedMode ?: JSONObject.NULL)
                .put("returnedSensorPixelModeIsMaximumResolution", returnedMode == CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
                .put("rawBinningFactorUsed", result.get(CaptureResult.SENSOR_RAW_BINNING_FACTOR_USED) ?: JSONObject.NULL)
                .put("iso", result.get(CaptureResult.SENSOR_SENSITIVITY) ?: JSONObject.NULL)
                .put("exposureTimeNs", result.get(CaptureResult.SENSOR_EXPOSURE_TIME) ?: JSONObject.NULL)
                .put("focalLengthMm", result.get(CaptureResult.LENS_FOCAL_LENGTH) ?: JSONObject.NULL)
                .put("noiseReductionMode", result.get(CaptureResult.NOISE_REDUCTION_MODE) ?: JSONObject.NULL)
                .put("edgeMode", result.get(CaptureResult.EDGE_MODE) ?: JSONObject.NULL))
            .put("rawPayload", JSONObject()
                .put("file", raw.file.name)
                .put("sha256", raw.sha256)
                .put("bytes", raw.bytes)
                .put("accessibleBufferBytes", raw.accessibleBytes)
                .put("expectedBytes", EXPECTED_BYTES)
                .put("rowStride", raw.rowStride)
                .put("pixelStride", raw.pixelStride)
                .put("canonicalContiguousRawSensor", raw.contiguous)
                .put("bandCount", BAND_COUNT)
                .put("bandBytes", BAND_BYTES)
                .put("bandGeometryInterpretation", "Each band is 768 rows at declared 16320 width; its byte count also equals 4080x3072x2 exactly.")
                .put("allZeroBandReferenceSha256", ZERO_BAND_SHA256)
                .put("bandSha256", JSONArray(raw.bandHashes))
                .put("allZeroBandIndices", JSONArray(raw.allZeroBandIndices))
                .put("fullRasterBandCoveragePass", raw.fullRasterBandCoveragePass))
            .put("dng", JSONObject()
                .put("attempted", raw.fullRasterBandCoveragePass && raw.contiguous)
                .put("writer", "DngCreator.writeByteBuffer")
                .put("file", dng?.name ?: JSONObject.NULL)
                .put("bytes", dng?.length() ?: JSONObject.NULL)
                .put("sha256", dngSha ?: JSONObject.NULL)
                .put("error", dngError ?: JSONObject.NULL)
                .put("semantics", "AUXILIARY_DERIVED_CONTAINER_FROM_SEALED_APP_VISIBLE_RAW_BUFFER"))
            .put("classification",
                if (raw.fullRasterBandCoveragePass)
                    "DIRECT_CAMERA5_200MP_DECLARED_RASTER_WITH_16_OF_16_NONZERO_BAND_COVERAGE"
                else
                    "DIRECT_CAMERA5_DECLARED_200MP_SURFACE_BUT_FULL_RASTER_CONTENT_NOT_PROVEN")
            .put("boundary", "APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF")
            .put("untouchedPhotodiodeAdcRawProven", false)
    }

    private fun setIfSupported(
        builder: CaptureRequest.Builder,
        key: CaptureRequest.Key<Int>,
        value: Int,
        chars: CameraCharacteristics,
    ) {
        if (chars.availableCaptureRequestKeys?.contains(key) == true) runCatching { builder.set(key, value) }
    }

    private fun setIfSupported(
        builder: CaptureRequest.Builder,
        key: CaptureRequest.Key<Boolean>,
        value: Boolean,
        chars: CameraCharacteristics,
    ) {
        if (chars.availableCaptureRequestKeys?.contains(key) == true) runCatching { builder.set(key, value) }
    }

    private fun finishAttemptUi() {
        closeCamera()
        runOnUiThread {
            capabilityButton.isEnabled = true
            captureButton.isEnabled = capabilityReady
        }
    }

    private fun closeCamera() {
        runCatching { session?.close() }
        session = null
        runCatching { reader?.close() }
        reader = null
        runCatching { camera?.close() }
        camera = null
    }

    private fun clearOutputs() {
        capturedRaw = null
        capturedDng = null
        capturedJson = null
        saveRawButton.isEnabled = false
        saveDngButton.isEnabled = false
        saveJsonButton.isEnabled = false
    }

    private fun saveFile(file: File?, mime: String, requestCode: Int) {
        if (file == null || !file.isFile) {
            setStatus("Bestand ontbreekt; niets opgeslagen.")
            return
        }
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = mime
            putExtra(Intent.EXTRA_TITLE, file.name)
        }
        pendingSaveFile = file
        startActivityForResult(intent, requestCode)
    }

    @Deprecated("Legacy document picker callback is used deliberately for this isolated evidence app.")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK) return
        val src = pendingSaveFile ?: return
        val uri = data?.data ?: return
        runCatching {
            contentResolver.openOutputStream(uri, "w")!!.use { out ->
                FileInputStream(src).use { input -> input.copyTo(out, 1024 * 1024) }
            }
        }.onSuccess {
            setStatus("Opgeslagen: ${src.name}\nbytes=${src.length()} · sha256=${sha256File(src)}")
        }.onFailure { e ->
            setStatus("Opslaan FAIL · ${e.javaClass.simpleName}: ${e.message}")
        }
        pendingSaveFile = null
    }

    private fun sha256File(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buf = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buf)
                if (n <= 0) break
                md.update(buf, 0, n)
            }
        }
        return md.digest().hex()
    }

    private fun ByteArray.hex(): String = joinToString("") { "%02x".format(it) }

    private fun setStatus(text: String) {
        status.text = text
    }

    private fun setStatusAny(text: String) {
        runOnUiThread { status.text = text }
    }

    private fun button(label: String, action: () -> Unit) = Button(this).apply {
        text = label
        isAllCaps = false
        setOnClickListener { action() }
    }

    private fun text(value: String, size: Float, bold: Boolean, color: Int) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(color)
        if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        setPadding(0, dp(4), 0, dp(4))
    }

    private fun space(px: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(px))
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    data class Capability(
        val cameraIds: List<String>,
        val camera5Listed: Boolean,
        val maxHigh: List<Size>,
        val targetAdvertised: Boolean,
        val pixelModeKeyAdvertised: Boolean,
        val logicalPhysicalIds: List<String>,
        val maximumPixelArray: Size?,
    )

    companion object {
        private const val REQUEST_CAMERA = 5015
        private const val REQUEST_SAVE_RAW = 5115
        private const val REQUEST_SAVE_DNG = 5215
        private const val REQUEST_SAVE_JSON = 5315

        private const val CAMERA_ID = "5"
        private const val LOGICAL_ID = "0"
        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val TARGET_SAMPLES = 200_540_160L
        private const val EXPECTED_BYTES = TARGET_SAMPLES * 2L

        private const val BAND_COUNT = 16
        private const val BAND_BYTES = EXPECTED_BYTES / BAND_COUNT
        private const val ZERO_BAND_SHA256 = "c2c6b5c221fefd4bad354ce0941fe48cf5e1e1734b2c3c16020c566569850b96"

        @Volatile private var pendingSaveFile: File? = null
    }
}

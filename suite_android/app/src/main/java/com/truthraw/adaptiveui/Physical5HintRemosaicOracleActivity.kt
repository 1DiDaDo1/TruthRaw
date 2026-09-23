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
import android.hardware.camera2.CaptureFailure
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
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
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.security.MessageDigest
import java.time.Instant
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

class Physical5HintRemosaicOracleActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        refreshStatus()
    }

    override fun onResume() {
        super.onResume()
        if (::status.isInitialized && ::saveButton.isInitialized) refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }
        body.addView(label("TruthRaw v0.75 · Visible OEM State Oracle", 21f, true))
        body.addView(label(
            "OEM-route: cameraSceneMode=53 → trace zichtbare HONOR/QTI runtime-state → lees hintUserValue indien zichtbaar → één physical-5 RAW10/MAX frame.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run scene53 → hint → remosaic → RAW") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        status = label("Nog geen v0.73 report.", 10f, false)
        body.addView(status)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(12, 14, 18))
            addView(body, ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
        }
    }

    private fun runOracle() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V075_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("scientificMasterModified", false)
                    .put("calibrationAuthorityGranted", false)
            }
            writeCheckpoint(report)
            runOnUiThread { refreshStatus() }
        }.start()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CAMERA_PERMISSION &&
            grantResults.isNotEmpty() &&
            grantResults[0] == PackageManager.PERMISSION_GRANTED
        ) runOracle()
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)
        val physicalChildPresent = logical.physicalCameraIds.contains(PHYSICAL_ID)

        val logicalReq = logical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val physicalReq = physical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val logicalSession = logical.availableSessionKeys.orEmpty().map { it.name }.toSet()
        val logicalResult = logical.availableCaptureResultKeys.orEmpty().associateBy { it.name }
        val physicalResult = physical.availableCaptureResultKeys.orEmpty().associateBy { it.name }

        @Suppress("UNCHECKED_CAST")
        val sceneLogical = logicalReq[SCENE_KEY] as? CaptureRequest.Key<Any>
        @Suppress("UNCHECKED_CAST")
        val scenePhysical = physicalReq[SCENE_KEY] as? CaptureRequest.Key<Any>
        @Suppress("UNCHECKED_CAST")
        val remosaicLogical = logicalReq[REMOSAIC_KEY] as? CaptureRequest.Key<Any>
        @Suppress("UNCHECKED_CAST")
        val remosaicPhysical = physicalReq[REMOSAIC_KEY] as? CaptureRequest.Key<Any>
        @Suppress("UNCHECKED_CAST")
        val hintLogical = logicalResult[HINT_KEY] as? CaptureResult.Key<Any>
        @Suppress("UNCHECKED_CAST")
        val hintPhysical = physicalResult[HINT_KEY] as? CaptureResult.Key<Any>

        val sceneKeyPresent = sceneLogical != null || scenePhysical != null
        val advertisedHintPresent = hintLogical != null || hintPhysical != null

        val report = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("stockStaticBasis", JSONObject()
                .put("cameraSceneMode", 53)
                .put("cameraSceneModeMeaning", "HONOR .452 UltraHighPixel/200M")
                .put("hintUserValueMeaning", "HAL/result-driven SMART_SCENE_MODE source")
                .put("rawMfHintValues", JSONArray(RAW_MF_HINTS.toList()))
                .put("qcomRemosaicEnableJavaType", "java.lang.Integer")
                .put("stockRemosaicRule", "hintUserValue == 5 -> 1; otherwise -> 0")
                .put("analysisBasisHonorCameraApkBytes", 84167938)
                .put("analysisBasisHonorCameraApkSha256", "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"))
            .put("runtimeSurfaces", JSONObject()
                .put("physical5ListedByLogical0", physicalChildPresent)
                .put("sceneLogicalRequestPresent", sceneLogical != null)
                .put("scenePhysicalRequestPresent", scenePhysical != null)
                .put("sceneLogicalSessionPresent", SCENE_KEY in logicalSession)
                .put("remosaicLogicalRequestPresent", remosaicLogical != null)
                .put("remosaicPhysicalRequestPresent", remosaicPhysical != null)
                .put("hintLogicalCharacteristicsAdvertised", hintLogical != null)
                .put("hintPhysicalCharacteristicsAdvertised", hintPhysical != null)
                .put("hintAdvertisedAnywhere", advertisedHintPresent))
            .put("diagnosticPolicy", JSONObject()
                .put("fatalOnHintNotAdvertised", false)
                .put("runtimeResultKeyDiscoveryEnabled", true)
                .put("constructedHintKeyFallbackEnabled", true))
            .put("stage", "INITIALIZED")
            .put("previewPrimingFrameCount", 0)
            .put("rawPhysicalFrameCount", 0)
            .put("physicalFrameCount", 1)
            .put("independentEvidenceCount", 1)
            .put("multiFrameFusionUsed", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("boundary", BOUNDARY)
        writeCheckpoint(report)

        if (!physicalChildPresent) {
            return report
                .put("stage", "COMPLETE_WITH_ROUTE_REJECTION")
                .put("classification", "V075_PHYSICAL5_NOT_LISTED_BY_LOGICAL0")
        }
        if (!sceneKeyPresent) {
            return report
                .put("stage", "COMPLETE_WITH_ROUTE_REJECTION")
                .put("classification", "V075_SCENE53_REQUEST_KEY_NOT_AVAILABLE")
        }

        val opened = openLogicalCamera(cm)
        val device = opened.device ?: return report
            .put("stage", "CAMERA_OPEN_FAILED")
            .put("classification", "V075_CAMERA_OPEN_FAILED")
            .put("cameraOpenError", opened.error ?: JSONObject.NULL)

        try {
            report.put("stage", "BEFORE_SCENE53_PRIME")
            writeCheckpoint(report)

            val prime = runPrime(
                device,
                opened.executor!!,
                physical,
                sceneLogical,
                scenePhysical,
                SCENE_KEY in logicalSession,
                hintLogical,
                hintPhysical,
            )
            report.put("scene53Prime", prime.toJson())
                .put("previewPrimingFrameCount", if (prime.frameCompleted) 1 else 0)
                .put("stage", "SCENE53_PRIME_RETURNED")
            writeCheckpoint(report)

            if (!prime.frameCompleted) {
                return report
                    .put("classification", "V075_SCENE53_PRIME_FAILED")
                    .put("stage", "COMPLETE_WITH_PRIME_FAILURE")
            }

            val chosenHint = prime.physicalHint ?: prime.logicalHint
            val derivedRemosaic = if (chosenHint == 5) 1 else 0
            val rawMfPrime = chosenHint != null && RAW_MF_HINTS.contains(chosenHint)

            report.put("derivedState", JSONObject()
                .put("chosenHintSource",
                    if (prime.physicalHint != null) "PHYSICAL5"
                    else if (prime.logicalHint != null) "LOGICAL0"
                    else "NONE")
                .put("chosenHintUserValue", chosenHint ?: JSONObject.NULL)
                .put("rawMfHintReachedDuringPrime", rawMfPrime)
                .put("derivedQcomRemosaicEnable", derivedRemosaic)
                .put("derivationRule", "hintUserValue == 5 -> 1; otherwise -> 0")
                .put("derivationBasedOnObservedHint", chosenHint != null))
                .put("stage", "DERIVED_REMOSAIC_STATE")
            writeCheckpoint(report)

            val raw = runRaw(
                device,
                opened.executor,
                physical,
                sceneLogical,
                scenePhysical,
                SCENE_KEY in logicalSession,
                remosaicLogical,
                remosaicPhysical,
                derivedRemosaic,
                hintLogical,
                hintPhysical,
            )

            val rawHint = raw.physicalHint ?: raw.logicalHint
            val rawMfRaw = rawHint != null && RAW_MF_HINTS.contains(rawHint)

            report.put("rawCapture", raw.toJson())
                .put("rawPhysicalFrameCount", if (raw.frameCaptured) 1 else 0)
                .put("stage", "RAW_RETURNED")
                .put("rawMfHintReachedDuringRaw", rawMfRaw)
                .put("returnedRawHintUserValue", rawHint ?: JSONObject.NULL)

            return report
                .put("stage", "COMPLETE")
                .put("classification",
                    when {
                        rawMfPrime -> "V075_RAW_MF_HINT_REACHED_DURING_SCENE53_PRIME"
                        rawMfRaw -> "V075_RAW_MF_HINT_REACHED_DURING_RAW_CAPTURE"
                        raw.frameCaptured && chosenHint == null && rawHint == null ->
                            "V075_VISIBLE_OEM_STATE_TRACE_COMPLETED_HINT_NOT_OBSERVED"
                        raw.frameCaptured ->
                            "V075_VISIBLE_OEM_STATE_TRACE_COMPLETED_NON_RAW_MF_HINT"
                        else -> "V075_RAW_CAPTURE_FAILED_AFTER_PRIME"
                    })
        } finally {
            device.close()
            opened.closedLatch?.await(2, TimeUnit.SECONDS)
            opened.executor?.shutdown()
            opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
            opened.executor?.shutdownNow()
        }
    }

    private data class OpenResult(
        val device: CameraDevice?,
        val executor: ExecutorService?,
        val closedLatch: CountDownLatch?,
        val error: String?,
    )

    private fun openLogicalCamera(cm: CameraManager): OpenResult {
        val opened = CountDownLatch(1)
        val closed = CountDownLatch(1)
        val executor = Executors.newSingleThreadExecutor()
        var device: CameraDevice? = null
        var error: String? = null

        try {
            cm.openCamera(LOGICAL_ID, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    device = camera
                    opened.countDown()
                }
                override fun onDisconnected(camera: CameraDevice) {
                    error = "CAMERA_DISCONNECTED"
                    camera.close()
                    opened.countDown()
                }
                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_" + code
                    camera.close()
                    opened.countDown()
                }
                override fun onClosed(camera: CameraDevice) {
                    closed.countDown()
                }
            })
            if (!opened.await(6, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        }

        if (device == null) {
            executor.shutdownNow()
            return OpenResult(null, null, null, error)
        }
        return OpenResult(device, executor, closed, error)
    }

    private data class PrimeOutcome(
        val frameCompleted: Boolean,
        val sessionConfigured: Boolean,
        val sceneSessionAttached: Boolean,
        val sceneLogicalWritten: Boolean,
        val scenePhysicalWritten: Boolean,
        val logicalHintRaw: Any?,
        val physicalHintRaw: Any?,
        val logicalHint: Int?,
        val physicalHint: Int?,
        val logicalHintKeySource: String,
        val physicalHintKeySource: String,
        val logicalHintReadError: String?,
        val physicalHintReadError: String?,
        val logicalRuntimeResultKeyNames: List<String>,
        val physicalRuntimeResultKeyNames: List<String>,
        val errorClass: String?,
        val errorMessage: String?,
        val logicalVisibleOemState: JSONObject? = null,
        val physicalVisibleOemState: JSONObject? = null,
    ) {
        fun toJson() = JSONObject()
            .put("frameCompleted", frameCompleted)
            .put("sessionConfigured", sessionConfigured)
            .put("sceneSessionAttached", sceneSessionAttached)
            .put("sceneLogicalWritten", sceneLogicalWritten)
            .put("scenePhysicalWritten", scenePhysicalWritten)
            .put("logicalHintUserValueRaw", jsonValue(logicalHintRaw))
            .put("physicalHintUserValueRaw", jsonValue(physicalHintRaw))
            .put("logicalHintUserValue", logicalHint ?: JSONObject.NULL)
            .put("physicalHintUserValue", physicalHint ?: JSONObject.NULL)
            .put("logicalHintKeySource", logicalHintKeySource)
            .put("physicalHintKeySource", physicalHintKeySource)
            .put("logicalHintReadError", logicalHintReadError ?: JSONObject.NULL)
            .put("physicalHintReadError", physicalHintReadError ?: JSONObject.NULL)
            .put("logicalRuntimeResultKeyNames", JSONArray(logicalRuntimeResultKeyNames))
            .put("physicalRuntimeResultKeyNames", JSONArray(physicalRuntimeResultKeyNames))
            .put("logicalVisibleOemState", logicalVisibleOemState ?: JSONObject.NULL)
            .put("physicalVisibleOemState", physicalVisibleOemState ?: JSONObject.NULL)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
    }

    private fun runPrime(
        device: CameraDevice,
        executor: ExecutorService,
        physical: CameraCharacteristics,
        sceneLogical: CaptureRequest.Key<Any>?,
        scenePhysical: CaptureRequest.Key<Any>?,
        sceneLogicalSessionPresent: Boolean,
        hintLogical: CaptureResult.Key<Any>?,
        hintPhysical: CaptureResult.Key<Any>?,
    ): PrimeOutcome {
        val sizes = physical.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)?.toList().orEmpty()
        val size = choosePreviewSize(sizes)
        val texture = SurfaceTexture(false)
        texture.setDefaultBufferSize(size.width, size.height)
        val surface = Surface(texture)

        val sessionLatch = CountDownLatch(1)
        val closedLatch = CountDownLatch(1)
        var session: CameraCaptureSession? = null
        var configured = false

        try {
            val output = OutputConfiguration(surface).apply { setPhysicalCameraId(PHYSICAL_ID) }
            val config = SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
                executor,
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(s: CameraCaptureSession) {
                        configured = true
                        session = s
                        sessionLatch.countDown()
                    }
                    override fun onConfigureFailed(s: CameraCaptureSession) {
                        session = s
                        sessionLatch.countDown()
                    }
                    override fun onClosed(s: CameraCaptureSession) {
                        closedLatch.countDown()
                    }
                }
            )

            var sessionAttached = false
            if (sceneLogicalSessionPresent && sceneLogical != null) {
                val sb = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
                sb.set(sceneLogical, intArrayOf(SCENE_ULTRA_HIGH_PIXEL))
                config.setSessionParameters(sb.build())
                sessionAttached = true
            }

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !configured || session == null) {
                runCatching { session?.close() }
                closedLatch.await(2, TimeUnit.SECONDS)
                return PrimeOutcome(
                    false, false, sessionAttached, false, false,
                    null, null, null, null,
                    "UNAVAILABLE", "UNAVAILABLE", null, null,
                    emptyList(), emptyList(),
                    "PRIME_SESSION_FAILED", null
                )
            }

            val builder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            builder.addTarget(surface)
            var logicalWritten = false
            var physicalWritten = false

            if (sceneLogical != null) {
                builder.set(sceneLogical, intArrayOf(SCENE_ULTRA_HIGH_PIXEL))
                logicalWritten = true
            } else if (scenePhysical != null) {
                builder.setPhysicalCameraKey(
                    scenePhysical,
                    intArrayOf(SCENE_ULTRA_HIGH_PIXEL),
                    PHYSICAL_ID
                )
                physicalWritten = true
            }

            val resultLatch = CountDownLatch(1)
            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            session!!.captureSingleRequest(
                builder.build(),
                executor,
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        s: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        resultRef.set(result)
                        resultLatch.countDown()
                    }
                    override fun onCaptureFailed(
                        s: CameraCaptureSession,
                        request: CaptureRequest,
                        failure: CaptureFailure,
                    ) {
                        resultLatch.countDown()
                    }
                }
            )

            if (!resultLatch.await(8, TimeUnit.SECONDS) || resultRef.get() == null) {
                session?.close()
                closedLatch.await(2, TimeUnit.SECONDS)
                return PrimeOutcome(
                    false, true, sessionAttached, logicalWritten, physicalWritten,
                    null, null, null, null,
                    "UNAVAILABLE", "UNAVAILABLE", null, null,
                    emptyList(), emptyList(),
                    "PRIME_CAPTURE_FAILED", null
                )
            }

            val result = resultRef.get()!!
            val physicalResult = result.physicalCameraResults[PHYSICAL_ID]
            val logicalHintRead = readHintWithRuntimeDiscovery(result, hintLogical)
            val physicalHintRead = readHintWithRuntimeDiscovery(physicalResult, hintPhysical)
            val logicalVisibleState = snapshotVisibleOemState(result)
            val physicalVisibleState = snapshotVisibleOemState(physicalResult)

            session?.close()
            closedLatch.await(2, TimeUnit.SECONDS)

            return PrimeOutcome(
                true, true, sessionAttached, logicalWritten, physicalWritten,
                logicalHintRead.raw, physicalHintRead.raw,
                logicalHintRead.value, physicalHintRead.value,
                logicalHintRead.keySource, physicalHintRead.keySource,
                logicalHintRead.error, physicalHintRead.error,
                logicalHintRead.runtimeKeyNames, physicalHintRead.runtimeKeyNames,
                null, null,
                logicalVisibleOemState = logicalVisibleState,
                physicalVisibleOemState = physicalVisibleState,
            )
        } catch (e: Throwable) {
            runCatching { session?.close() }
            closedLatch.await(2, TimeUnit.SECONDS)
            return PrimeOutcome(
                false, configured, false, false, false,
                null, null, null, null,
                "UNAVAILABLE", "UNAVAILABLE", null, null,
                emptyList(), emptyList(),
                e.javaClass.name, e.message
            )
        } finally {
            surface.release()
            texture.release()
        }
    }

    private data class RawOutcome(
        val frameCaptured: Boolean,
        val sessionConfigured: Boolean,
        val sceneSessionAttached: Boolean,
        val sceneLogicalWritten: Boolean,
        val scenePhysicalWritten: Boolean,
        val remosaicLogicalWritten: Boolean,
        val remosaicPhysicalWritten: Boolean,
        val remosaicReadback: Any?,
        val physicalPixelModeReadback: Any?,
        val logicalHintRaw: Any?,
        val physicalHintRaw: Any?,
        val logicalHint: Int?,
        val physicalHint: Int?,
        val logicalHintKeySource: String?,
        val physicalHintKeySource: String?,
        val logicalHintReadError: String?,
        val physicalHintReadError: String?,
        val logicalRuntimeResultKeyNames: List<String>?,
        val physicalRuntimeResultKeyNames: List<String>?,
        val accessibleBytes: Long?,
        val rowStride: Int?,
        val pixelStride: Int?,
        val firstNonZeroByte: Long?,
        val lastNonZeroByte: Long?,
        val tailNonZeroByteCount: Long?,
        val effectiveRowsWithData: Int?,
        val effectivePadNonZeroByteCount: Long?,
        val fullPlaneSha256: String?,
        val knownV059TopologyMatch: Boolean?,
        val errorClass: String?,
        val errorMessage: String?,
        val logicalVisibleOemState: JSONObject? = null,
        val physicalVisibleOemState: JSONObject? = null,
    ) {
        fun toJson() = JSONObject()
            .put("frameCaptured", frameCaptured)
            .put("sessionConfigured", sessionConfigured)
            .put("sceneSessionAttached", sceneSessionAttached)
            .put("sceneLogicalWritten", sceneLogicalWritten)
            .put("scenePhysicalWritten", scenePhysicalWritten)
            .put("remosaicLogicalWritten", remosaicLogicalWritten)
            .put("remosaicPhysicalWritten", remosaicPhysicalWritten)
            .put("remosaicReadback", jsonValue(remosaicReadback))
            .put("physicalSensorPixelModeReadback", jsonValue(physicalPixelModeReadback))
            .put("logicalHintUserValueRaw", jsonValue(logicalHintRaw))
            .put("physicalHintUserValueRaw", jsonValue(physicalHintRaw))
            .put("logicalHintUserValue", logicalHint ?: JSONObject.NULL)
            .put("physicalHintUserValue", physicalHint ?: JSONObject.NULL)
            .put("logicalHintKeySource", logicalHintKeySource ?: JSONObject.NULL)
            .put("physicalHintKeySource", physicalHintKeySource ?: JSONObject.NULL)
            .put("logicalHintReadError", logicalHintReadError ?: JSONObject.NULL)
            .put("physicalHintReadError", physicalHintReadError ?: JSONObject.NULL)
            .put("logicalRuntimeResultKeyNames",
                logicalRuntimeResultKeyNames?.let { JSONArray(it) } ?: JSONObject.NULL)
            .put("physicalRuntimeResultKeyNames",
                physicalRuntimeResultKeyNames?.let { JSONArray(it) } ?: JSONObject.NULL)
            .put("accessibleBytes", accessibleBytes ?: JSONObject.NULL)
            .put("rowStride", rowStride ?: JSONObject.NULL)
            .put("pixelStride", pixelStride ?: JSONObject.NULL)
            .put("firstNonZeroByte", firstNonZeroByte ?: JSONObject.NULL)
            .put("lastNonZeroByte", lastNonZeroByte ?: JSONObject.NULL)
            .put("tailNonZeroByteCount", tailNonZeroByteCount ?: JSONObject.NULL)
            .put("effectiveRowsWithData", effectiveRowsWithData ?: JSONObject.NULL)
            .put("effectivePadNonZeroByteCount", effectivePadNonZeroByteCount ?: JSONObject.NULL)
            .put("fullPlaneSha256", fullPlaneSha256 ?: JSONObject.NULL)
            .put("knownV059TopologyMatch", knownV059TopologyMatch ?: JSONObject.NULL)
            .put("logicalVisibleOemState", logicalVisibleOemState ?: JSONObject.NULL)
            .put("physicalVisibleOemState", physicalVisibleOemState ?: JSONObject.NULL)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
    }

    private fun runRaw(
        device: CameraDevice,
        executor: ExecutorService,
        physical: CameraCharacteristics,
        sceneLogical: CaptureRequest.Key<Any>?,
        scenePhysical: CaptureRequest.Key<Any>?,
        sceneLogicalSessionPresent: Boolean,
        remosaicLogical: CaptureRequest.Key<Any>?,
        remosaicPhysical: CaptureRequest.Key<Any>?,
        derivedRemosaic: Int,
        hintLogical: CaptureResult.Key<Any>?,
        hintPhysical: CaptureResult.Key<Any>?,
    ): RawOutcome {
        val reader = ImageReader.newInstance(TARGET_W, TARGET_H, ImageFormat.RAW10, 1)
        val imageThread = HandlerThread("truthraw-v075-raw").apply { start() }
        val imageHandler = Handler(imageThread.looper)
        val imageRef = AtomicReference<Image?>(null)
        val imageLatch = CountDownLatch(1)
        reader.setOnImageAvailableListener({ source ->
            val image = runCatching { source.acquireNextImage() }.getOrNull()
            if (image != null && imageRef.compareAndSet(null, image)) imageLatch.countDown()
            else image?.close()
        }, imageHandler)

        val sessionLatch = CountDownLatch(1)
        val closedLatch = CountDownLatch(1)
        var session: CameraCaptureSession? = null
        var configured = false

        try {
            val output = OutputConfiguration(reader.surface).apply {
                setPhysicalCameraId(PHYSICAL_ID)
                addSensorPixelModeUsed(CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
            }
            val config = SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
                executor,
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(s: CameraCaptureSession) {
                        configured = true
                        session = s
                        sessionLatch.countDown()
                    }
                    override fun onConfigureFailed(s: CameraCaptureSession) {
                        session = s
                        sessionLatch.countDown()
                    }
                    override fun onClosed(s: CameraCaptureSession) {
                        closedLatch.countDown()
                    }
                }
            )

            var sceneSessionAttached = false
            if (sceneLogicalSessionPresent && sceneLogical != null) {
                val sb = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                sb.set(sceneLogical, intArrayOf(SCENE_ULTRA_HIGH_PIXEL))
                config.setSessionParameters(sb.build())
                sceneSessionAttached = true
            }

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !configured || session == null) {
                return RawOutcome(
                    frameCaptured = false,
                    sessionConfigured = false,
                    sceneSessionAttached = sceneSessionAttached,
                    sceneLogicalWritten = false,
                    scenePhysicalWritten = false,
                    remosaicLogicalWritten = false,
                    remosaicPhysicalWritten = false,
                    remosaicReadback = null,
                    physicalPixelModeReadback = null,
                    logicalHintRaw = null,
                    physicalHintRaw = null,
                    logicalHint = null,
                    physicalHint = null,
                    logicalHintKeySource = null,
                    physicalHintKeySource = null,
                    logicalHintReadError = null,
                    physicalHintReadError = null,
                    logicalRuntimeResultKeyNames = null,
                    physicalRuntimeResultKeyNames = null,
                    accessibleBytes = null,
                    rowStride = null,
                    pixelStride = null,
                    firstNonZeroByte = null,
                    lastNonZeroByte = null,
                    tailNonZeroByteCount = null,
                    effectiveRowsWithData = null,
                    effectivePadNonZeroByteCount = null,
                    fullPlaneSha256 = null,
                    knownV059TopologyMatch = null,
                    errorClass = "RAW_SESSION_FAILED",
                    errorMessage = null,
                )
            }

            val builder = device.createCaptureRequest(
                CameraDevice.TEMPLATE_STILL_CAPTURE,
                setOf(PHYSICAL_ID)
            )
            builder.addTarget(reader.surface)

            var sceneLogicalWritten = false
            var scenePhysicalWritten = false
            if (sceneLogical != null) {
                builder.set(sceneLogical, intArrayOf(SCENE_ULTRA_HIGH_PIXEL))
                sceneLogicalWritten = true
            } else if (scenePhysical != null) {
                builder.setPhysicalCameraKey(
                    scenePhysical,
                    intArrayOf(SCENE_ULTRA_HIGH_PIXEL),
                    PHYSICAL_ID
                )
                scenePhysicalWritten = true
            }

            var remosaicLogicalWritten = false
            var remosaicPhysicalWritten = false
            var remosaicReadback: Any? = null
            if (remosaicLogical != null) {
                builder.set(remosaicLogical, derivedRemosaic)
                remosaicLogicalWritten = true
                remosaicReadback = builder.get(remosaicLogical)
            } else if (remosaicPhysical != null) {
                builder.setPhysicalCameraKey(remosaicPhysical, derivedRemosaic, PHYSICAL_ID)
                remosaicPhysicalWritten = true
                remosaicReadback = builder.getPhysicalCameraKey(remosaicPhysical, PHYSICAL_ID)
            }

            builder.setPhysicalCameraKey(
                CaptureRequest.SENSOR_PIXEL_MODE,
                CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                PHYSICAL_ID
            )
            val pixelReadback = builder.getPhysicalCameraKey(
                CaptureRequest.SENSOR_PIXEL_MODE,
                PHYSICAL_ID
            )

            val resultLatch = CountDownLatch(1)
            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            session!!.captureSingleRequest(
                builder.build(),
                executor,
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        s: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        resultRef.set(result)
                        resultLatch.countDown()
                    }
                    override fun onCaptureFailed(
                        s: CameraCaptureSession,
                        request: CaptureRequest,
                        failure: CaptureFailure,
                    ) {
                        resultLatch.countDown()
                    }
                }
            )

            val resultReady = resultLatch.await(10, TimeUnit.SECONDS)
            val imageReady = imageLatch.await(10, TimeUnit.SECONDS)
            val result = resultRef.get()
            val image = imageRef.get()

            if (!resultReady || !imageReady || result == null || image == null) {
                return RawOutcome(
                    frameCaptured = false,
                    sessionConfigured = true,
                    sceneSessionAttached = sceneSessionAttached,
                    sceneLogicalWritten = sceneLogicalWritten,
                    scenePhysicalWritten = scenePhysicalWritten,
                    remosaicLogicalWritten = remosaicLogicalWritten,
                    remosaicPhysicalWritten = remosaicPhysicalWritten,
                    remosaicReadback = remosaicReadback,
                    physicalPixelModeReadback = pixelReadback,
                    logicalHintRaw = null,
                    physicalHintRaw = null,
                    logicalHint = null,
                    physicalHint = null,
                    logicalHintKeySource = null,
                    physicalHintKeySource = null,
                    logicalHintReadError = null,
                    physicalHintReadError = null,
                    logicalRuntimeResultKeyNames = null,
                    physicalRuntimeResultKeyNames = null,
                    accessibleBytes = null,
                    rowStride = null,
                    pixelStride = null,
                    firstNonZeroByte = null,
                    lastNonZeroByte = null,
                    tailNonZeroByteCount = null,
                    effectiveRowsWithData = null,
                    effectivePadNonZeroByteCount = null,
                    fullPlaneSha256 = null,
                    knownV059TopologyMatch = null,
                    errorClass = "RAW_CAPTURE_PAIRING_FAILED",
                    errorMessage = "resultReady=" + resultReady + " imageReady=" + imageReady,
                )
            }

            try {
                val physicalResult = result.physicalCameraResults[PHYSICAL_ID]
                val logicalHintRead = readHintWithRuntimeDiscovery(result, hintLogical)
                val physicalHintRead = readHintWithRuntimeDiscovery(physicalResult, hintPhysical)
                val logicalVisibleState = snapshotVisibleOemState(result)
                val physicalVisibleState = snapshotVisibleOemState(physicalResult)

                val topo = analyzeRaw(image)

                return RawOutcome(
                    frameCaptured = true,
                    sessionConfigured = true,
                    sceneSessionAttached = sceneSessionAttached,
                    sceneLogicalWritten = sceneLogicalWritten,
                    scenePhysicalWritten = scenePhysicalWritten,
                    remosaicLogicalWritten = remosaicLogicalWritten,
                    remosaicPhysicalWritten = remosaicPhysicalWritten,
                    remosaicReadback = remosaicReadback,
                    physicalPixelModeReadback = pixelReadback,
                    logicalHintRaw = logicalHintRead.raw,
                    physicalHintRaw = physicalHintRead.raw,
                    logicalHint = logicalHintRead.value,
                    physicalHint = physicalHintRead.value,
                    logicalHintKeySource = logicalHintRead.keySource,
                    physicalHintKeySource = physicalHintRead.keySource,
                    logicalHintReadError = logicalHintRead.error,
                    physicalHintReadError = physicalHintRead.error,
                    logicalRuntimeResultKeyNames = logicalHintRead.runtimeKeyNames,
                    physicalRuntimeResultKeyNames = physicalHintRead.runtimeKeyNames,
                    accessibleBytes = topo.accessibleBytes,
                    rowStride = topo.rowStride,
                    pixelStride = topo.pixelStride,
                    firstNonZeroByte = topo.firstNonZero,
                    lastNonZeroByte = topo.lastNonZero,
                    tailNonZeroByteCount = topo.tailNonZero,
                    effectiveRowsWithData = topo.rowsWithData,
                    effectivePadNonZeroByteCount = topo.padNonZero,
                    fullPlaneSha256 = topo.fullSha,
                    knownV059TopologyMatch = topo.knownMatch,
                    errorClass = null,
                    errorMessage = null,
                    logicalVisibleOemState = logicalVisibleState,
                    physicalVisibleOemState = physicalVisibleState,
                )
            } finally {
                image.close()
                session?.close()
                closedLatch.await(2, TimeUnit.SECONDS)
            }
        } catch (e: Throwable) {
            return RawOutcome(
                frameCaptured = false,
                sessionConfigured = configured,
                sceneSessionAttached = false,
                sceneLogicalWritten = false,
                scenePhysicalWritten = false,
                remosaicLogicalWritten = false,
                remosaicPhysicalWritten = false,
                remosaicReadback = null,
                physicalPixelModeReadback = null,
                logicalHintRaw = null,
                physicalHintRaw = null,
                logicalHint = null,
                physicalHint = null,
                logicalHintKeySource = null,
                physicalHintKeySource = null,
                logicalHintReadError = null,
                physicalHintReadError = null,
                logicalRuntimeResultKeyNames = null,
                physicalRuntimeResultKeyNames = null,
                accessibleBytes = null,
                rowStride = null,
                pixelStride = null,
                firstNonZeroByte = null,
                lastNonZeroByte = null,
                tailNonZeroByteCount = null,
                effectiveRowsWithData = null,
                effectivePadNonZeroByteCount = null,
                fullPlaneSha256 = null,
                knownV059TopologyMatch = null,
                errorClass = e.javaClass.name,
                errorMessage = e.message,
            )
        } finally {
            reader.close()
            imageThread.quitSafely()
            runCatching { session?.close() }
            closedLatch.await(2, TimeUnit.SECONDS)
        }
    }

    private data class HintRead(
        val raw: Any?,
        val value: Int?,
        val keySource: String,
        val error: String?,
        val runtimeKeyNames: List<String>,
    )

    @Suppress("UNCHECKED_CAST")
    private fun readHintWithRuntimeDiscovery(
        result: CaptureResult?,
        advertisedKey: CaptureResult.Key<Any>?,
    ): HintRead {
        if (result == null) {
            return HintRead(null, null, "NO_RESULT", "NO_RESULT", emptyList())
        }

        val runtimeKeys = runCatching { result.keys.toList() }.getOrElse { emptyList() }
        val runtimeNames = runtimeKeys.map { it.name }.sorted()
        val runtimeExact = runtimeKeys.firstOrNull { it.name == HINT_KEY } as? CaptureResult.Key<Any>

        val candidates = ArrayList<Pair<String, CaptureResult.Key<Any>>>()
        if (runtimeExact != null) candidates.add("RUNTIME_RESULT_KEY" to runtimeExact)
        if (advertisedKey != null && advertisedKey !== runtimeExact) {
            candidates.add("CHARACTERISTICS_ADVERTISED_KEY" to advertisedKey)
        }
        val constructed = runCatching {
            CaptureResult.Key(HINT_KEY, Int::class.javaObjectType) as CaptureResult.Key<Any>
        }.getOrNull()
        if (constructed != null) candidates.add("CONSTRUCTED_VENDOR_KEY" to constructed)

        if (candidates.isEmpty()) {
            return HintRead(null, null, "NO_KEY_CANDIDATE", "NO_KEY_CANDIDATE", runtimeNames)
        }

        val errors = ArrayList<String>()
        for ((source, key) in candidates) {
            try {
                val raw = result.get(key)
                if (raw != null) return HintRead(raw, intValue(raw), source, null, runtimeNames)
                errors.add(source + ":NULL")
            } catch (e: Throwable) {
                errors.add(source + ":" + e.javaClass.name + ":" + (e.message ?: ""))
            }
        }
        return HintRead(null, null, candidates.first().first, errors.joinToString(" | "), runtimeNames)
    }

    @Suppress("UNCHECKED_CAST")
    private fun snapshotVisibleOemState(result: CaptureResult?): JSONObject {
        val out = JSONObject()
        if (result == null) {
            out.put("_resultPresent", false)
            return out
        }
        out.put("_resultPresent", true)
        val byName = runCatching {
            result.keys.associateBy { it.name }
        }.getOrElse { emptyMap() }

        for (name in VISIBLE_OEM_STATE_KEYS) {
            val entry = JSONObject()
            val key = byName[name] as? CaptureResult.Key<Any>
            entry.put("presentInRuntimeResultKeys", key != null)
            if (key == null) {
                entry.put("value", JSONObject.NULL)
                entry.put("javaClass", JSONObject.NULL)
                entry.put("readError", JSONObject.NULL)
            } else {
                try {
                    val raw = result.get(key)
                    entry.put("value", jsonValue(raw))
                    entry.put("javaClass", raw?.javaClass?.name ?: JSONObject.NULL)
                    entry.put("readError", JSONObject.NULL)
                } catch (e: Throwable) {
                    entry.put("value", JSONObject.NULL)
                    entry.put("javaClass", JSONObject.NULL)
                    entry.put("readError", e.javaClass.name + ": " + (e.message ?: ""))
                }
            }
            out.put(name, entry)
        }
        return out
    }

    private data class Topology(
        val accessibleBytes: Long,
        val rowStride: Int,
        val pixelStride: Int,
        val firstNonZero: Long?,
        val lastNonZero: Long?,
        val tailNonZero: Long,
        val rowsWithData: Int?,
        val padNonZero: Long?,
        val fullSha: String,
        val knownMatch: Boolean,
    )

    private fun analyzeRaw(image: Image): Topology {
        val plane = image.planes.single()
        val buf = plane.buffer.duplicate().apply { rewind() }
        val accessible = buf.remaining().toLong()
        val md = MessageDigest.getInstance("SHA-256")
        val scratch = ByteArray(1024 * 1024)
        var absolute = 0L
        var first: Long? = null
        var last: Long? = null
        var tail = 0L

        while (buf.hasRemaining()) {
            val n = minOf(buf.remaining(), scratch.size)
            buf.get(scratch, 0, n)
            md.update(scratch, 0, n)
            for (i in 0 until n) {
                if (scratch[i].toInt() != 0) {
                    val pos = absolute + i
                    if (first == null) first = pos
                    last = pos
                    if (pos >= EFFECTIVE_PREFIX_BYTES) tail++
                }
            }
            absolute += n
        }

        var rowsWithData: Int? = null
        var padNonZero: Long? = null
        if (accessible >= EFFECTIVE_PREFIX_BYTES) {
            val row = ByteArray(EFFECTIVE_ROW_BYTES)
            val b = plane.buffer.duplicate().apply { rewind() }
            var rows = 0
            var pads = 0L
            for (y in 0 until EFFECTIVE_ROWS) {
                b.position(y * EFFECTIVE_ROW_BYTES)
                b.get(row)
                if ((0 until EFFECTIVE_PACKED_BYTES_PER_ROW).any { row[it].toInt() != 0 }) rows++
                for (i in EFFECTIVE_PACKED_BYTES_PER_ROW until EFFECTIVE_ROW_BYTES) {
                    if (row[i].toInt() != 0) pads++
                }
            }
            rowsWithData = rows
            padNonZero = pads
        }

        val knownMatch =
            accessible == EXPECTED_DECLARED_RAW10_BYTES &&
                plane.rowStride == DECLARED_RAW10_ROW_STRIDE &&
                first == 0L &&
                last != null &&
                last < EFFECTIVE_PREFIX_BYTES &&
                tail == 0L &&
                rowsWithData == EFFECTIVE_ROWS &&
                padNonZero == 0L

        return Topology(
            accessible,
            plane.rowStride,
            plane.pixelStride,
            first,
            last,
            tail,
            rowsWithData,
            padNonZero,
            hex(md.digest()),
            knownMatch,
        )
    }

    private fun choosePreviewSize(sizes: List<Size>): Size {
        for (p in listOf(Size(1920,1080), Size(1280,720), Size(640,480))) {
            sizes.firstOrNull { it.width == p.width && it.height == p.height }?.let { return it }
        }
        return sizes.minByOrNull { it.width.toLong() * it.height.toLong() }
            ?: error("no preview sizes")
    }

    private fun writeCheckpoint(report: JSONObject) {
        val target = reportFile()
        val tmp = File(filesDir, REPORT_FILENAME + ".tmp")
        tmp.writeText(report.toString(2))
        if (target.exists() && !target.delete()) {
            target.writeText(report.toString(2))
            tmp.delete()
            return
        }
        if (!tmp.renameTo(target)) {
            target.writeText(report.toString(2))
            tmp.delete()
        }
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.75 report."
            return
        }
        val r = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (r == null) {
            status.text = "v0.75 report kon niet worden gelezen."
            return
        }
        val d = r.optJSONObject("derivedState")
        status.text = buildString {
            append("stage=").append(r.optString("stage", "?")).append('\n')
            append("prime hint=").append(d?.opt("chosenHintUserValue") ?: "?").append('\n')
            append("derived remosaic=").append(d?.optInt("derivedQcomRemosaicEnable", -1) ?: -1).append('\n')
            append("classification=").append(r.optString("classification", "IN_PROGRESS"))
        }
    }

    @Suppress("DEPRECATION")
    private fun saveReport() {
        val source = reportFile()
        if (!source.exists()) return
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, source.name)
        }, REQUEST_SAVE_JSON)
    }

    @Deprecated("Document export bridge")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_SAVE_JSON || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                reportFile().inputStream().use { it.copyTo(out) }
            } ?: error("no output stream")
        }.onSuccess {
            status.text = "v0.75 JSON opgeslagen."
        }.onFailure {
            status.text = "Opslaan faalde: " + it.javaClass.simpleName + ": " + it.message
        }
    }

    private fun reportFile() = File(filesDir, REPORT_FILENAME)

    private fun button(text: String, action: () -> Unit) = Button(this).apply {
        this.text = text
        isAllCaps = false
        minHeight = dp(52)
        setOnClickListener { action() }
    }

    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE) =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun space(height: Int) =
        View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(height)) }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val SCHEMA = "truthraw.physical5-visible-oem-state-oracle.v0.75"
        private const val AUTHORITY = "CAMERA2_OEM_SCENE53_VISIBLE_STATE_SINGLE_FRAME_ORACLE"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_VISIBLE_OEM_STATE_ORACLE_v075.json"
        private const val REQUEST_CAMERA_PERMISSION = 67362
        private const val REQUEST_SAVE_JSON = 67363
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val SCENE_KEY = "com.hihonor.capture.metadata.cameraSceneMode"
        private const val HINT_KEY = "com.hihonor.capture.metadata.hintUserValue"
        private const val REMOSAIC_KEY = "com.hihonor.capture.metadata.qcomRemosaicEnable"
        private const val SCENE_ULTRA_HIGH_PIXEL = 53
        private val RAW_MF_HINTS = setOf(23,24,32,33)
        private val VISIBLE_OEM_STATE_KEYS = listOf(
            "android.logicalMultiCamera.activePhysicalId",
            "android.sensor.pixelMode",
            "android.sensor.rawBinningFactorUsed",
            "com.hihonor.capture.metadata.cameraSceneMode",
            "com.hihonor.capture.metadata.aiCaptureHint",
            "com.hihonor.capture.metadata.smartSuggestHint",
            "com.hihonor.capture.metadata.FrameType",
            "com.hihonor.capture.metadata.masterSensorSlotId",
            "com.hihonor.capture.metadata.previewCameraPhysicalId",
            "com.hihonor.capture.metadata.previewPhysicalCam",
            "com.hihonor.capture.metadata.activeSensors",
            "com.hihonor.capture.metadata.binningFactor",
            "com.hihonor.capture.metadata.EnvBrightnessForA200Decision",
            "com.hihonor.capture.metadata.hwFirstValidFrame",
            "com.qti.chi.metadataOwnerInfo.MetadataOwner",
            "com.qti.chi.multicamerainfo.ActiveCameraInfo",
            "com.qti.chi.multicamerainfo.MasterCamera",
            "com.qti.chi.multicamerainfo.MultiCameraIds",
            "org.quic.camera2.tuning.feature.Feature1Mode",
            "org.quic.camera2.tuning.feature.Feature2Mode",
        )

        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val DECLARED_RAW10_ROW_STRIDE = 20400
        private const val EXPECTED_DECLARED_RAW10_BYTES = 250675200L
        private const val EFFECTIVE_ROWS = 3072
        private const val EFFECTIVE_ROW_BYTES = 5120
        private const val EFFECTIVE_PACKED_BYTES_PER_ROW = 5100
        private const val EFFECTIVE_PREFIX_BYTES = 15728640L

        private const val BOUNDARY =
            "THIS_ORACLE_TRACES_APP_VISIBLE_HONOR_QTI_RUNTIME_STATE_AROUND_SCENE53_AND_PHYSICAL5_RAW10_MAX; IT_DOES_NOT_EQUATE_VISIBLE_STATE_WITH_STOCK_SERVICEHOST_INTERNALS_OR_PROVE_NATIVE_SENSOR_GEOMETRY_DIRECT_CFA_200MP_ADC_BIT_DEPTH_OR_CALIBRATION_TRUTH"

        private fun intValue(v: Any?): Int? = when (v) {
            is Int -> v
            is Number -> v.toInt()
            is IntArray -> v.firstOrNull()
            is ByteArray -> v.firstOrNull()?.toInt()
            else -> null
        }

        private fun jsonValue(v: Any?): Any = when (v) {
            null -> JSONObject.NULL
            is IntArray -> JSONArray(v.toList())
            is ByteArray -> JSONArray(v.map { it.toInt() and 0xff })
            is LongArray -> JSONArray(v.toList())
            is FloatArray -> JSONArray(v.map { it.toDouble() })
            is DoubleArray -> JSONArray(v.toList())
            is Number, is Boolean, is String -> v
            else -> v.toString()
        }

        private fun hex(bytes: ByteArray) =
            bytes.joinToString("") { b -> "%02x".format(b) }
    }
}

package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.os.Bundle
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
import java.time.Instant
import java.util.concurrent.CountDownLatch
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

/**
 * v0.73 OEM-orchestration hint oracle.
 *
 * Static HONOR .452 route reconstructed before this runtime probe:
 * - UltraHighPixelMode writes cameraSceneMode=53 to capture + preview flows.
 * - HAL returns com.hihonor.capture.metadata.hintUserValue.
 * - UltraHighPixelMode mirrors that result into internal SMART_SCENE_MODE.
 * - UltraHighPixelModeProcessor maps 23/24/32/33 to pipeline4rawmfultrahighpixelcap.json.
 * - For UltraHighPixel remosaic support, hint != 5 drives qcomRemosaicEnable=1 before capture.
 *
 * This probe deliberately stops before RAW capture. It asks one narrow question:
 * can a direct Camera2 physical-5 preview observation reproduce the OEM hint states
 * when applying the OEM-authored mode request(s)?
 *
 * Each candidate:
 * - fresh logical camera 0 open;
 * - one physical-camera-5 SurfaceTexture output;
 * - one captureSingleRequest only;
 * - no repeating request;
 * - no ImageReader;
 * - no RAW evidence;
 * - no Scientific Master mutation.
 */
class Physical5OemHintOrchestrationActivity : Activity() {
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
        body.addView(label("TruthRaw v0.73 · OEM hint orchestration", 21f, true))
        body.addView(label(
            "Test de gereconstrueerde HONOR high-pixel request-keten vóór RAW: " +
                "cameraSceneMode → hintUserValue → remosaic state. Iedere kandidaat krijgt exact één preview-observatieframe.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run OEM hint oracle") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Geen RAW capture. Deze test bepaalt alleen of direct Camera2 dezelfde HAL hint-staten zichtbaar maakt als de stock orchestration.",
            11f, false, Color.rgb(155, 165, 180)
        ))
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
            status.text = "v0.73 vraagt CAMERA-toestemming."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeOracle()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return
        if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            executeOracle()
        } else {
            status.text = "CAMERA-toestemming niet verleend; niets getest."
        }
    }

    private fun executeOracle() {
        saveButton.isEnabled = false
        status.text = "v0.73 initialiseert…"
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V073_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("primaryRawFrameCount", 0)
                    .put("independentRawEvidenceCount", 0)
                    .put("scientificMasterModified", false)
                    .put("calibrationAuthorityGranted", false)
            }
            writeCheckpoint(report)
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private enum class ValueEncoding(val label: String) {
        SCALAR_INT("java.lang.Integer"),
        INT_ARRAY("int[]"),
        BYTE_ARRAY("byte[]"),
    }

    private data class ValueProbe(
        val name: String,
        val scalarIntAccepted: Boolean,
        val intArrayAccepted: Boolean,
        val byteArrayAccepted: Boolean,
        val chosen: ValueEncoding?,
        val scalarError: String?,
        val intArrayError: String?,
        val byteArrayError: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("name", name)
            .put("scalarIntAccepted", scalarIntAccepted)
            .put("intArrayAccepted", intArrayAccepted)
            .put("byteArrayAccepted", byteArrayAccepted)
            .put("chosenEncoding", chosen?.label ?: JSONObject.NULL)
            .put("scalarError", scalarError ?: JSONObject.NULL)
            .put("intArrayError", intArrayError ?: JSONObject.NULL)
            .put("byteArrayError", byteArrayError ?: JSONObject.NULL)
    }

    private data class CandidateSpec(
        val shortName: String,
        val sceneValue: Int?,
        val remosaicMode: RemosaicMode,
        val semanticBasis: String,
    )

    private enum class RemosaicMode {
        NONE,
        FORCE_ONE,
        STOCK_DERIVED_FROM_SCENE_HINT,
    }

    private data class OpenResult(
        val device: CameraDevice?,
        val executor: ExecutorService?,
        val closedLatch: CountDownLatch?,
        val error: String?,
    )

    private fun openLogicalCamera(cm: CameraManager): OpenResult {
        val openedLatch = CountDownLatch(1)
        val closedLatch = CountDownLatch(1)
        val executor = Executors.newSingleThreadExecutor()
        var device: CameraDevice? = null
        var error: String? = null

        try {
            cm.openCamera(LOGICAL_ID, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    device = camera
                    openedLatch.countDown()
                }

                override fun onDisconnected(camera: CameraDevice) {
                    error = "CAMERA_DISCONNECTED"
                    camera.close()
                    openedLatch.countDown()
                }

                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_$code"
                    camera.close()
                    openedLatch.countDown()
                }

                override fun onClosed(camera: CameraDevice) {
                    closedLatch.countDown()
                }
            })
            if (!openedLatch.await(6, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        }

        if (device == null) {
            executor.shutdownNow()
            return OpenResult(null, null, null, error)
        }
        return OpenResult(device, executor, closedLatch, error)
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)

        require(logical.physicalCameraIds.contains(PHYSICAL_ID)) {
            "physical camera 5 is not disclosed by logical camera 0"
        }

        val logicalRequestByName = logical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val physicalRequestByName = physical.availableCaptureRequestKeys.orEmpty().associateBy { it.name }
        val logicalResultByName = logical.availableCaptureResultKeys.orEmpty().associateBy { it.name }
        val physicalResultByName = physical.availableCaptureResultKeys.orEmpty().associateBy { it.name }

        val sceneLogicalKey = logicalRequestByName[CAMERA_SCENE_KEY]
        val remosaicLogicalKey = logicalRequestByName[QCOM_REMOSAIC_KEY]

        val typeProbe = probeLocalTypes(cm, sceneLogicalKey, remosaicLogicalKey)
        val sceneProbe = typeProbe.first
        val remosaicProbe = typeProbe.second

        val previewSizes = physical
            .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)
            ?.toList()
            .orEmpty()
        require(previewSizes.isNotEmpty()) { "physical camera 5 has no SurfaceTexture output sizes" }
        val previewSize = choosePreviewSize(previewSizes)

        val results = JSONArray()
        val report = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("physicalId5ListedByLogicalCharacteristics", true)
            .put("staticRouteBasis", JSONObject()
                .put("ultraHighPixelCameraSceneMode", 53)
                .put("ultraResolutionCameraSceneMode", 110)
                .put("proPhotoRawCameraSceneModeReference", 66)
                .put("hintUserValueKey", HINT_USER_VALUE_KEY)
                .put("rawMfUltraHighPixelHintValues", JSONArray(listOf(23, 24, 32, 33)))
                .put("stockRemosaicRule", "hintUserValue present and != 5 -> qcomRemosaicEnable=1; null or 5 -> 0"))
            .put("keySurface", JSONObject()
                .put("cameraSceneModeLogicalRequestPresent", sceneLogicalKey != null)
                .put("cameraSceneModePhysicalRequestPresent", physicalRequestByName.containsKey(CAMERA_SCENE_KEY))
                .put("qcomRemosaicEnableLogicalRequestPresent", remosaicLogicalKey != null)
                .put("qcomRemosaicEnablePhysicalRequestPresent", physicalRequestByName.containsKey(QCOM_REMOSAIC_KEY))
                .put("hintUserValueLogicalResultPresent", logicalResultByName.containsKey(HINT_USER_VALUE_KEY))
                .put("hintUserValuePhysicalResultPresent", physicalResultByName.containsKey(HINT_USER_VALUE_KEY)))
            .put("localMarshaling", JSONObject()
                .put("cameraSceneMode", sceneProbe.toJson())
                .put("qcomRemosaicEnable", remosaicProbe.toJson()))
            .put("previewSurface", JSONObject()
                .put("class", "android.graphics.SurfaceTexture")
                .put("width", previewSize.width)
                .put("height", previewSize.height)
                .put("physicalCameraBinding", PHYSICAL_ID))
            .put("candidateCount", CANDIDATES.size)
            .put("results", results)
            .put("stage", "INITIALIZED")
            .put("previewObservationFrameCount", 0)
            .put("primaryRawFrameCount", 0)
            .put("independentRawEvidenceCount", 0)
            .put("repeatingRequestUsed", false)
            .put("imageReaderCreated", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("boundary", BOUNDARY)

        writeCheckpoint(report)

        var observed = 0
        var rawMfHints = 0
        var failed = 0

        for ((index, spec) in CANDIDATES.withIndex()) {
            runOnUiThread {
                status.text = "v0.73 " + (index + 1) + "/" + CANDIDATES.size + " · " + spec.shortName
            }

            val item = JSONObject()
                .put("index", index)
                .put("shortName", spec.shortName)
                .put("sceneValue", spec.sceneValue ?: JSONObject.NULL)
                .put("remosaicMode", spec.remosaicMode.name)
                .put("semanticBasis", spec.semanticBasis)
                .put("pairing", "ONE_FRESH_PREVIEW_OBSERVATION_FRAME")
            results.put(item)

            report
                .put("candidateIndexInFlight", index)
                .put("candidateNameInFlight", spec.shortName)
                .put("stage", "BEFORE_CANDIDATE")
            writeCheckpoint(report)

            val observation = runCandidate(
                cm = cm,
                logical = logical,
                physical = physical,
                previewSize = previewSize,
                sceneLogicalKey = sceneLogicalKey,
                sceneEncoding = sceneProbe.chosen,
                remosaicLogicalKey = remosaicLogicalKey,
                remosaicEncoding = remosaicProbe.chosen,
                logicalHintKey = logicalResultByName[HINT_USER_VALUE_KEY],
                physicalHintKey = physicalResultByName[HINT_USER_VALUE_KEY],
                spec = spec,
            )

            item.put("observation", observation)
            if (observation.optBoolean("frameObserved", false)) {
                observed++
                val chosenHint = observation.optInt("preferredHintUserValue", Int.MIN_VALUE)
                if (chosenHint in RAW_MF_HINTS) rawMfHints++
                item.put(
                    "classification",
                    if (chosenHint in RAW_MF_HINTS)
                        "RAW_MF_ULTRAHIGHPIXEL_HINT_OBSERVED"
                    else
                        "PREVIEW_HINT_OBSERVED__NOT_RAW_MF_ULTRAHIGHPIXEL"
                )
            } else {
                failed++
                item.put("classification", "PREVIEW_OBSERVATION_FAILED")
            }

            report
                .put("previewObservationFrameCount", observed)
                .put("rawMfUltraHighPixelHintObservationCount", rawMfHints)
                .put("failedCandidateCount", failed)
                .put("stage", "CANDIDATE_RECORDED")
            writeCheckpoint(report)
        }

        return report
            .put("candidateIndexInFlight", JSONObject.NULL)
            .put("candidateNameInFlight", JSONObject.NULL)
            .put("stage", "ORACLE_COMPLETE")
            .put("previewObservationFrameCount", observed)
            .put("rawMfUltraHighPixelHintObservationCount", rawMfHints)
            .put("failedCandidateCount", failed)
            .put("classification",
                if (rawMfHints > 0)
                    "DIRECT_CAMERA2_OEM_MODE_REQUEST_CAN_SURFACE_RAW_MF_ULTRAHIGHPIXEL_HINT"
                else
                    "DIRECT_CAMERA2_OEM_MODE_REQUEST_DID_NOT_SURFACE_RAW_MF_ULTRAHIGHPIXEL_HINT_IN_THIS_ORACLE")
    }

    private fun probeLocalTypes(
        cm: CameraManager,
        sceneKeyAny: CaptureRequest.Key<*>?,
        remosaicKeyAny: CaptureRequest.Key<*>?,
    ): Pair<ValueProbe, ValueProbe> {
        val opened = openLogicalCamera(cm)
        val device = opened.device ?: return Pair(
            ValueProbe(CAMERA_SCENE_KEY, false, false, false, null, "camera open failed", "camera open failed", "camera open failed"),
            ValueProbe(QCOM_REMOSAIC_KEY, false, false, false, null, "camera open failed", "camera open failed", "camera open failed"),
        )

        return try {
            Pair(
                probeOneKey(device, CAMERA_SCENE_KEY, sceneKeyAny),
                probeOneKey(device, QCOM_REMOSAIC_KEY, remosaicKeyAny),
            )
        } finally {
            device.close()
            opened.closedLatch?.await(2, TimeUnit.SECONDS)
            opened.executor?.shutdown()
            opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
            opened.executor?.shutdownNow()
        }
    }

    private fun probeOneKey(
        device: CameraDevice,
        name: String,
        keyAny: CaptureRequest.Key<*>?,
    ): ValueProbe {
        if (keyAny == null) {
            return ValueProbe(name, false, false, false, null, "key not advertised", "key not advertised", "key not advertised")
        }

        @Suppress("UNCHECKED_CAST")
        val key = keyAny as CaptureRequest.Key<Any>

        fun tryValue(value: Any): Pair<Boolean, String?> {
            return try {
                val builder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW, setOf(PHYSICAL_ID))
                builder.set(key, value)
                val readback = builder.get(key)
                val ok = valuesEquivalent(value, readback)
                Pair(ok, if (ok) null else "readback mismatch: " + jsonValue(readback))
            } catch (e: Throwable) {
                Pair(false, e.javaClass.name + ": " + e.message)
            }
        }

        val scalar = tryValue(1)
        val ints = tryValue(intArrayOf(1))
        val bytes = tryValue(byteArrayOf(1))

        val accepted = listOf(
            ValueEncoding.SCALAR_INT to scalar.first,
            ValueEncoding.INT_ARRAY to ints.first,
            ValueEncoding.BYTE_ARRAY to bytes.first,
        ).filter { it.second }.map { it.first }

        val chosen = if (accepted.size == 1) accepted.single() else null

        return ValueProbe(
            name = name,
            scalarIntAccepted = scalar.first,
            intArrayAccepted = ints.first,
            byteArrayAccepted = bytes.first,
            chosen = chosen,
            scalarError = scalar.second,
            intArrayError = ints.second,
            byteArrayError = bytes.second,
        )
    }

    private fun runCandidate(
        cm: CameraManager,
        logical: CameraCharacteristics,
        physical: CameraCharacteristics,
        previewSize: Size,
        sceneLogicalKey: CaptureRequest.Key<*>?,
        sceneEncoding: ValueEncoding?,
        remosaicLogicalKey: CaptureRequest.Key<*>?,
        remosaicEncoding: ValueEncoding?,
        logicalHintKey: CaptureResult.Key<*>?,
        physicalHintKey: CaptureResult.Key<*>?,
        spec: CandidateSpec,
    ): JSONObject {
        val opened = openLogicalCamera(cm)
        val device = opened.device ?: return JSONObject()
            .put("frameObserved", false)
            .put("errorClass", "CAMERA_OPEN_FAILED")
            .put("errorMessage", opened.error ?: JSONObject.NULL)

        val texture = SurfaceTexture(false)
        texture.setDefaultBufferSize(previewSize.width, previewSize.height)
        val surface = Surface(texture)
        val sessionLatch = CountDownLatch(1)
        val sessionClosedLatch = CountDownLatch(1)
        val sessionRef = AtomicReference<CameraCaptureSession?>(null)
        var configured = false
        var configureError: String? = null

        try {
            val output = OutputConfiguration(surface).apply {
                setPhysicalCameraId(PHYSICAL_ID)
            }
            val config = SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
                opened.executor!!,
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(session: CameraCaptureSession) {
                        configured = true
                        sessionRef.set(session)
                        sessionLatch.countDown()
                    }

                    override fun onConfigureFailed(session: CameraCaptureSession) {
                        configureError = "onConfigureFailed"
                        sessionRef.set(session)
                        sessionLatch.countDown()
                    }

                    override fun onClosed(session: CameraCaptureSession) {
                        sessionClosedLatch.countDown()
                    }
                }
            )

            device.createCaptureSession(config)
            if (!sessionLatch.await(8, TimeUnit.SECONDS) || !configured) {
                return JSONObject()
                    .put("frameObserved", false)
                    .put("sessionConfigured", false)
                    .put("errorClass", "SESSION_CONFIGURATION_FAILED_OR_TIMED_OUT")
                    .put("errorMessage", configureError ?: JSONObject.NULL)
            }

            val session = sessionRef.get() ?: error("configured session missing")
            val builder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW, setOf(PHYSICAL_ID))
            builder.addTarget(surface)

            var sceneWrite = JSONObject().put("requested", false)
            if (spec.sceneValue != null) {
                sceneWrite = applyEncodedGlobal(
                    builder = builder,
                    keyAny = sceneLogicalKey,
                    encoding = sceneEncoding,
                    intValue = spec.sceneValue,
                    keyName = CAMERA_SCENE_KEY,
                )
            }

            var remosaicWrite = JSONObject().put("requested", false)
            if (spec.remosaicMode == RemosaicMode.FORCE_ONE) {
                remosaicWrite = applyEncodedGlobal(
                    builder = builder,
                    keyAny = remosaicLogicalKey,
                    encoding = remosaicEncoding,
                    intValue = 1,
                    keyName = QCOM_REMOSAIC_KEY,
                )
            }

            val resultLatch = CountDownLatch(1)
            val resultRef = AtomicReference<TotalCaptureResult?>(null)
            var captureError: String? = null

            session.captureSingleRequest(
                builder.build(),
                opened.executor,
                object : CameraCaptureSession.CaptureCallback() {
                    override fun onCaptureCompleted(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        result: TotalCaptureResult,
                    ) {
                        resultRef.set(result)
                        resultLatch.countDown()
                    }

                    override fun onCaptureFailed(
                        session: CameraCaptureSession,
                        request: CaptureRequest,
                        failure: android.hardware.camera2.CaptureFailure,
                    ) {
                        captureError =
                            "reason=" + failure.reason +
                                " wasImageCaptured=" + failure.wasImageCaptured() +
                                " frameNumber=" + failure.frameNumber
                        resultLatch.countDown()
                    }
                }
            )

            val ready = resultLatch.await(8, TimeUnit.SECONDS)
            val result = resultRef.get()
            if (!ready || result == null) {
                return JSONObject()
                    .put("frameObserved", false)
                    .put("sessionConfigured", true)
                    .put("sceneWrite", sceneWrite)
                    .put("remosaicWrite", remosaicWrite)
                    .put("errorClass", "CAPTURE_RESULT_FAILED_OR_TIMED_OUT")
                    .put("errorMessage", captureError ?: JSONObject.NULL)
            }

            val physicalResult = result.physicalCameraResults[PHYSICAL_ID]
            val logicalHint = readResultValue(result, logicalHintKey)
            val physicalHint = readResultValue(physicalResult, physicalHintKey)
            val logicalHintInt = extractFirstInt(logicalHint)
            val physicalHintInt = extractFirstInt(physicalHint)
            val preferredHint = physicalHintInt ?: logicalHintInt

            if (spec.remosaicMode == RemosaicMode.STOCK_DERIVED_FROM_SCENE_HINT) {
                // This candidate intentionally records the stock-derived value only.
                // It does not retroactively mutate the already-submitted single request.
                val derived = if (preferredHint != null && preferredHint != 5) 1 else 0
                remosaicWrite = JSONObject()
                    .put("requested", false)
                    .put("derivedAfterObservation", derived)
                    .put("derivation", "hintUserValue present and != 5 -> 1; null or 5 -> 0")
            }

            return JSONObject()
                .put("frameObserved", true)
                .put("sessionConfigured", true)
                .put("sceneWrite", sceneWrite)
                .put("remosaicWrite", remosaicWrite)
                .put("logicalHintUserValueRaw", jsonValue(logicalHint))
                .put("physical5HintUserValueRaw", jsonValue(physicalHint))
                .put("logicalHintUserValueInt", logicalHintInt ?: JSONObject.NULL)
                .put("physical5HintUserValueInt", physicalHintInt ?: JSONObject.NULL)
                .put("preferredHintUserValue", preferredHint ?: JSONObject.NULL)
                .put("rawMfUltraHighPixelHintObserved", preferredHint in RAW_MF_HINTS)
                .put("physicalResultPresent", physicalResult != null)
                .put("physicalResultCameraId", physicalResult?.cameraId ?: JSONObject.NULL)
                .put("logicalSensorTimestampNs", result.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("physical5SensorTimestampNs", physicalResult?.get(CaptureResult.SENSOR_TIMESTAMP) ?: JSONObject.NULL)
                .put("errorClass", JSONObject.NULL)
                .put("errorMessage", JSONObject.NULL)
        } catch (e: Throwable) {
            return JSONObject()
                .put("frameObserved", false)
                .put("sessionConfigured", configured)
                .put("errorClass", e.javaClass.name)
                .put("errorMessage", e.message ?: JSONObject.NULL)
        } finally {
            runCatching { sessionRef.get()?.close() }
            sessionClosedLatch.await(2, TimeUnit.SECONDS)
            runCatching { surface.release() }
            runCatching { texture.release() }
            device.close()
            opened.closedLatch?.await(2, TimeUnit.SECONDS)
            opened.executor?.shutdown()
            opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
            opened.executor?.shutdownNow()
        }
    }

    private fun applyEncodedGlobal(
        builder: CaptureRequest.Builder,
        keyAny: CaptureRequest.Key<*>?,
        encoding: ValueEncoding?,
        intValue: Int,
        keyName: String,
    ): JSONObject {
        if (keyAny == null) {
            return JSONObject()
                .put("requested", true)
                .put("written", false)
                .put("reason", "key not advertised on logical request surface")
        }
        if (encoding == null) {
            return JSONObject()
                .put("requested", true)
                .put("written", false)
                .put("reason", "local marshaling representation unresolved")
        }

        @Suppress("UNCHECKED_CAST")
        val key = keyAny as CaptureRequest.Key<Any>
        val value: Any = when (encoding) {
            ValueEncoding.SCALAR_INT -> intValue
            ValueEncoding.INT_ARRAY -> intArrayOf(intValue)
            ValueEncoding.BYTE_ARRAY -> byteArrayOf(intValue.toByte())
        }

        return try {
            builder.set(key, value)
            val readback = builder.get(key)
            JSONObject()
                .put("requested", true)
                .put("keyName", keyName)
                .put("encoding", encoding.label)
                .put("value", jsonValue(value))
                .put("written", true)
                .put("readback", jsonValue(readback))
                .put("readbackEquivalent", valuesEquivalent(value, readback))
        } catch (e: Throwable) {
            JSONObject()
                .put("requested", true)
                .put("keyName", keyName)
                .put("encoding", encoding.label)
                .put("value", jsonValue(value))
                .put("written", false)
                .put("errorClass", e.javaClass.name)
                .put("errorMessage", e.message ?: JSONObject.NULL)
        }
    }

    private fun readResultValue(
        result: CaptureResult?,
        keyAny: CaptureResult.Key<*>?,
    ): Any? {
        if (result == null || keyAny == null) return null
        return runCatching {
            @Suppress("UNCHECKED_CAST")
            val key = keyAny as CaptureResult.Key<Any>
            result.get(key)
        }.getOrNull()
    }

    private fun extractFirstInt(value: Any?): Int? = when (value) {
        null -> null
        is Number -> value.toInt()
        is IntArray -> value.firstOrNull()
        is ByteArray -> value.firstOrNull()?.toInt()?.and(0xff)
        is LongArray -> value.firstOrNull()?.toInt()
        else -> null
    }

    private fun valuesEquivalent(a: Any?, b: Any?): Boolean = when {
        a is IntArray && b is IntArray -> a.contentEquals(b)
        a is ByteArray && b is ByteArray -> a.contentEquals(b)
        a is LongArray && b is LongArray -> a.contentEquals(b)
        else -> a == b
    }

    private fun choosePreviewSize(sizes: List<Size>): Size {
        for (p in listOf(Size(1920, 1080), Size(1280, 720), Size(640, 480))) {
            sizes.firstOrNull { it.width == p.width && it.height == p.height }?.let { return it }
        }
        return sizes.minByOrNull { it.width.toLong() * it.height.toLong() }!!
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
            status.text = "Nog geen v0.73 report."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.73 report bestaat maar kon niet worden gelezen."
            return
        }
        status.text = buildString {
            append("stage=").append(report.optString("stage", "?")).append('\n')
            append("preview observations=").append(report.optInt("previewObservationFrameCount", 0)).append('\n')
            append("raw-MF hints=").append(report.optInt("rawMfUltraHighPixelHintObservationCount", 0)).append('\n')
            append("failed=").append(report.optInt("failedCandidateCount", 0)).append('\n')
            append(report.optString("classification", "IN_PROGRESS"))
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
        val source = reportFile()
        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                source.inputStream().use { input -> input.copyTo(out) }
            } ?: error("Geen output stream")
        }.onSuccess {
            status.text = "v0.73 JSON opgeslagen."
        }.onFailure {
            status.text = "Opslaan faalde: " + it.javaClass.simpleName + ": " + it.message
        }
    }

    private fun reportFile(): File = File(filesDir, REPORT_FILENAME)

    private fun button(text: String, action: () -> Unit): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        minHeight = dp(52)
        setOnClickListener { action() }
    }

    private fun label(text: String, size: Float, bold: Boolean, color: Int = Color.WHITE): TextView =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun space(height: Int): View =
        View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(height)) }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val SCHEMA = "truthraw.physical5-oem-hint-orchestration.v0.73"
        private const val AUTHORITY = "CAMERA2_SINGLE_PREVIEW_RESULT_OEM_ORCHESTRATION_OBSERVATION"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_OEM_HINT_ORCHESTRATION_v073.json"
        private const val REQUEST_CAMERA_PERMISSION = 67362
        private const val REQUEST_SAVE_JSON = 67363
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private const val CAMERA_SCENE_KEY = "com.hihonor.capture.metadata.cameraSceneMode"
        private const val QCOM_REMOSAIC_KEY = "com.hihonor.capture.metadata.qcomRemosaicEnable"
        private const val HINT_USER_VALUE_KEY = "com.hihonor.capture.metadata.hintUserValue"

        private val RAW_MF_HINTS = setOf(23, 24, 32, 33)

        private val CANDIDATES = listOf(
            CandidateSpec(
                "control",
                null,
                RemosaicMode.NONE,
                "No OEM high-pixel vendor request."
            ),
            CandidateSpec(
                "scene53_ultrahighpixel",
                53,
                RemosaicMode.STOCK_DERIVED_FROM_SCENE_HINT,
                "HONOR .452 UltraHighPixelMode cameraSceneMode=53; observe HAL hint before applying derived remosaic state."
            ),
            CandidateSpec(
                "qcomRemosaicEnable_1_only",
                null,
                RemosaicMode.FORCE_ONE,
                "HONOR .452 Qualcomm pre-capture remosaic enabled state, isolated from scene request."
            ),
            CandidateSpec(
                "scene53_plus_qcomRemosaicEnable_1",
                53,
                RemosaicMode.FORCE_ONE,
                "Direct approximation of OEM UltraHighPixel scene plus enabled Qualcomm remosaic request."
            ),
            CandidateSpec(
                "scene110_ultraresolution_reference",
                110,
                RemosaicMode.NONE,
                "HONOR .452 UltraResolution/50M cameraSceneMode reference."
            ),
            CandidateSpec(
                "scene66_proraw_reference",
                66,
                RemosaicMode.NONE,
                "HONOR .452 Pro Photo RAW scene reference."
            ),
        )

        private const val BOUNDARY =
            "THIS_ORACLE_OBSERVES_ONE_PREVIEW_RESULT_PER_ISOLATED_DIRECT_CAMERA2_REQUEST; HINT_USER_VALUE_IS_RUNTIME_EVIDENCE, BUT ABSENCE_OF_23_24_32_33_DOES_NOT_DISPROVE_THE_STOCK_SERVICEHOST_ROUTE; NO_RAW_SOURCE_OR_CALIBRATION_AUTHORITY_IS_CREATED"

        private fun jsonValue(value: Any?): Any = when (value) {
            null -> JSONObject.NULL
            is IntArray -> JSONArray(value.toList())
            is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
            is LongArray -> JSONArray(value.toList())
            is FloatArray -> JSONArray(value.map { it.toDouble() })
            is DoubleArray -> JSONArray(value.toList())
            is Number, is Boolean, is String -> value
            else -> value.toString()
        }
    }
}

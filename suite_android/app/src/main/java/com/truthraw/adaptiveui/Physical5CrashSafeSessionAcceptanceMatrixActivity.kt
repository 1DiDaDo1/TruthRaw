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
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit

class Physical5CrashSafeSessionAcceptanceMatrixActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }
        body.addView(label("TruthRaw v0.69 · Physical-5 session matrix", 21f, true))
        body.addView(label(
            "Alle resterende v0.65-types in één APK. Intern blijft iedere key geïsoleerd: fresh camera open → control-session → exact één candidate-session → camera dicht. Geen capture, repeating request of pixels.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Run complete acceptance matrix") { runMatrix() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Session-configuratie = alleen acceptatie in deze context. Geen semantiek-, 200MP-, RAW- of sensorbewijs.",
            11f, false, Color.rgb(155, 165, 180)
        ))
        body.addView(space(10))
        status = label("Nog geen v0.69 report.", 10f, false)
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

    private fun runMatrix() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            status.text = "v0.69 vraagt CAMERA-toestemming voor session-acceptance tests."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeMatrix()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return
        if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            executeMatrix()
        } else {
            status.text = "CAMERA-toestemming niet verleend; niets getest."
        }
    }

    private fun executeMatrix() {
        saveButton.isEnabled = false
        status.text = "v0.69 start complete matrix…"
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V069_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("repeatingRequestSubmittedByTruthRaw", false)
                    .put("imageBufferAccessedByTruthRaw", false)
            }
            reportFile().writeText(report.toString(2))
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val requestByName = physicalRequest.associateBy { it.name }
        val sessionNames = physicalSession.map { it.name }.toSet()

        val previewSizes = physical
            .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)
            ?.toList()
            .orEmpty()
        require(previewSizes.isNotEmpty()) { "Physical camera 5 has no SurfaceTexture output sizes" }
        val chosenSize = choosePreviewSize(previewSizes)

        val results = JSONArray()
        var attempted = 0
        var configured = 0
        var rejected = 0
        var skippedNotSessionKey = 0
        var controlFailed = 0

        for ((index, spec) in CANDIDATES.withIndex()) {
            runOnUiThread {
                status.text = "v0.69 test \${index + 1}/\${CANDIDATES.size}: \${spec.shortName}"
            }

            val item = JSONObject()
                .put("index", index)
                .put("shortName", spec.shortName)
                .put("name", spec.keyName)
                .put("source", spec.source)
                .put("v065ResolvedRepresentation", spec.type.label)
                .put("candidateValue", jsonValue(spec.value()))
                .put("physicalRequestPresent", requestByName.containsKey(spec.keyName))
                .put("physicalSessionPresent", spec.keyName in sessionNames)
                .put("candidateAttempted", false)

            if (!requestByName.containsKey(spec.keyName)) {
                item.put("classification", "SKIPPED_NOT_IN_PHYSICAL5_REQUEST_KEYS")
                results.put(item)
                checkpoint.put("candidateInFlight", false)
                    .put("nextCandidateIndex", index + 1)
                    .put("lastStage", "SKIPPED_NOT_REQUEST_KEY")
                updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                writeCheckpoint(checkpoint)
                continue
            }
            if (spec.keyName !in sessionNames) {
                skippedNotSessionKey++
                item.put("classification", "SKIPPED_NOT_IN_PHYSICAL5_SESSION_KEYS")
                results.put(item)
                checkpoint.put("candidateInFlight", false)
                    .put("nextCandidateIndex", index + 1)
                    .put("lastStage", "SKIPPED_NOT_SESSION_KEY")
                updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                writeCheckpoint(checkpoint)
                continue
            }

            @Suppress("UNCHECKED_CAST")
            val key = requestByName[spec.keyName] as CaptureRequest.Key<Any>
            checkpoint.put("lastStage", "BEFORE_CAMERA_OPEN")
            writeCheckpoint(checkpoint)
            val opened = openLogicalCamera(cm)
            val device = opened.device
            if (device == null) {
                item.put("cameraOpenCompleted", false)
                    .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                    .put("classification", "CAMERA_OPEN_FAILED")
                results.put(item)
                checkpoint.put("candidateInFlight", false)
                    .put("nextCandidateIndex", index + 1)
                    .put("lastStage", "CAMERA_OPEN_FAILED")
                updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                writeCheckpoint(checkpoint)
                continue
            }
            item.put("cameraOpenCompleted", true)
            checkpoint.put("lastStage", "CAMERA_OPENED")
            writeCheckpoint(checkpoint)

            try {
                checkpoint.put("lastStage", "BEFORE_CONTROL_SESSION")
                writeCheckpoint(checkpoint)
                val control = configureSession(device, chosenSize, null, null, "CONTROL_NO_VENDOR_PARAMETER")
                item.put("control", control.toJson())
                checkpoint.put("lastStage", "CONTROL_SESSION_RETURNED")
                writeCheckpoint(checkpoint)
                if (!control.configured) {
                    controlFailed++
                    item.put("classification", "CONTROL_SESSION_FAILED__CANDIDATE_NOT_ATTEMPTED")
                    results.put(item)
                    checkpoint.put("candidateInFlight", false)
                        .put("nextCandidateIndex", index + 1)
                        .put("lastStage", "CONTROL_SESSION_FAILED")
                    updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                    writeCheckpoint(checkpoint)
                    continue
                }

                attempted++
                item.put("candidateAttempted", true)
                checkpoint.put("lastStage", "BEFORE_CANDIDATE_SESSION")
                updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                writeCheckpoint(checkpoint)
                val candidate = configureSession(device, chosenSize, key, spec.value(), spec.shortName)
                item.put("candidate", candidate.toJson())
                checkpoint.put("lastStage", "CANDIDATE_SESSION_RETURNED")
                writeCheckpoint(checkpoint)
                if (candidate.configured) {
                    configured++
                    item.put("classification", "HAL_SESSION_CONFIGURED__NO_CAPTURE")
                } else {
                    rejected++
                    item.put("classification", "HAL_SESSION_REJECTED_OR_FAILED__NO_CAPTURE")
                }
                results.put(item)
                checkpoint.put("candidateInFlight", false)
                    .put("nextCandidateIndex", index + 1)
                    .put("lastStage", "CANDIDATE_RECORDED")
                updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
                writeCheckpoint(checkpoint)
            } finally {
                device.close()
                opened.closedLatch?.await(2, TimeUnit.SECONDS)
                opened.executor?.shutdown()
                opened.executor?.awaitTermination(2, TimeUnit.SECONDS)
                opened.executor?.shutdownNow()
            }
            Thread.sleep(250L)
        }

        checkpoint
            .put("candidateInFlight", false)
            .put("nextCandidateIndex", CANDIDATES.size)
            .put("lastStage", "MATRIX_COMPLETE")
            .put("classification", "PHYSICAL5_HAL_SESSION_ACCEPTANCE_MATRIX_CHECKPOINTED__NO_CAPTURE")
        updateCheckpointCounts(checkpoint, attempted, configured, rejected, skippedNotSessionKey, controlFailed)
        writeCheckpoint(checkpoint)

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("physicalId5ListedByLogicalCharacteristics", logical.physicalCameraIds.contains(PHYSICAL_ID))
            .put("previewSurface", JSONObject()
                .put("class", "android.graphics.SurfaceTexture")
                .put("width", chosenSize.width)
                .put("height", chosenSize.height)
                .put("physicalCameraBinding", PHYSICAL_ID))
            .put("candidateCount", CANDIDATES.size)
            .put("candidateAttemptedCount", attempted)
            .put("halSessionConfiguredCount", configured)
            .put("halSessionRejectedOrFailedCount", rejected)
            .put("skippedNotPhysicalSessionKeyCount", skippedNotSessionKey)
            .put("controlFailedCount", controlFailed)
            .put("results", results)
            .put("cameraPermissionGranted", true)
            .put("oneCandidatePerSession", true)
            .put("matchedControlPerCandidate", true)
            .put("freshCameraOpenPerCandidate", true)
            .put("cameraDeviceCallbackExecutorKeptAliveUntilDeviceClosed", true)
            .put("checkpointedBeforeEachHalTouch", true)
            .put("captureSubmittedByTruthRaw", false)
            .put("repeatingRequestSubmittedByTruthRaw", false)
            .put("imageReaderCreatedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("classification", "PHYSICAL5_HAL_SESSION_ACCEPTANCE_MATRIX_CHECKPOINTED__NO_CAPTURE")
            .put("boundary", BOUNDARY)
    }

    private data class SessionAttempt(
        val label: String,
        val parameterAttached: Boolean,
        val localSetCompleted: Boolean,
        val localReadback: Any?,
        val createCallCompleted: Boolean,
        val configured: Boolean,
        val configureFailed: Boolean,
        val closedObserved: Boolean,
        val errorClass: String?,
        val errorMessage: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("label", label)
            .put("sessionParameterAttached", parameterAttached)
            .put("localSetCompleted", localSetCompleted)
            .put("localReadback", jsonValue(localReadback))
            .put("createCaptureSessionCallCompleted", createCallCompleted)
            .put("onConfigured", configured)
            .put("onConfigureFailed", configureFailed)
            .put("onClosedObserved", closedObserved)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
            .put("captureSubmitted", false)
            .put("repeatingRequestSubmitted", false)
            .put("imageBufferAccessed", false)
    }

    private fun configureSession(
        device: CameraDevice,
        size: Size,
        key: CaptureRequest.Key<Any>?,
        value: Any?,
        label: String,
    ): SessionAttempt {
        val executor = Executors.newSingleThreadExecutor()
        val done = CountDownLatch(1)
        val closed = CountDownLatch(1)
        val texture = SurfaceTexture(false)
        texture.setDefaultBufferSize(size.width, size.height)
        val surface = Surface(texture)

        var configured = false
        var failed = false
        var createCompleted = false
        var errorClass: String? = null
        var errorMessage: String? = null
        var localSetCompleted = key == null
        var localReadback: Any? = null

        try {
            val output = OutputConfiguration(surface).apply { setPhysicalCameraId(PHYSICAL_ID) }
            val callback = object : CameraCaptureSession.StateCallback() {
                override fun onConfigured(session: CameraCaptureSession) {
                    configured = true
                    done.countDown()
                    session.close()
                }
                override fun onConfigureFailed(session: CameraCaptureSession) {
                    failed = true
                    done.countDown()
                    session.close()
                }
                override fun onClosed(session: CameraCaptureSession) {
                    closed.countDown()
                }
            }

            val config = SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
                executor,
                callback
            )

            if (key != null) {
                require(value != null)
                val builder = device.createCaptureRequest(
                    CameraDevice.TEMPLATE_STILL_CAPTURE,
                    setOf(PHYSICAL_ID)
                )
                builder.setPhysicalCameraKey(key, value, PHYSICAL_ID)
                localReadback = builder.getPhysicalCameraKey(key, PHYSICAL_ID)
                localSetCompleted = true
                config.setSessionParameters(builder.build())
            }

            runCatching { device.createCaptureSession(config) }
                .onSuccess { createCompleted = true }
                .onFailure { e ->
                    errorClass = e.javaClass.name
                    errorMessage = e.message
                    done.countDown()
                }

            if (!done.await(8, TimeUnit.SECONDS)) {
                errorClass = "java.util.concurrent.TimeoutException"
                errorMessage = "No onConfigured/onConfigureFailed callback within 8 seconds"
            }
            if (configured || failed) closed.await(2, TimeUnit.SECONDS)

            return SessionAttempt(
                label, key != null, localSetCompleted, localReadback, createCompleted,
                configured, failed, closed.count == 0L, errorClass, errorMessage
            )
        } catch (e: Throwable) {
            return SessionAttempt(
                label, key != null, localSetCompleted, localReadback, createCompleted,
                false, failed, closed.count == 0L, e.javaClass.name, e.message
            )
        } finally {
            runCatching { surface.release() }
            runCatching { texture.release() }
            executor.shutdownNow()
        }
    }

    private data class OpenResult(
        val device: CameraDevice?,
        val executor: java.util.concurrent.ExecutorService?,
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
                    error = "CAMERA_DISCONNECTED_BEFORE_SESSION_TEST"
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
            if (!openedLatch.await(5, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        }

        if (device == null) {
            executor.shutdownNow()
            return OpenResult(null, null, null, error)
        }
        return OpenResult(device, executor, closedLatch, error)
    }

    private fun choosePreviewSize(sizes: List<Size>): Size {
        for (p in listOf(Size(1920, 1080), Size(1280, 720), Size(640, 480))) {
            sizes.firstOrNull { it.width == p.width && it.height == p.height }?.let { return it }
        }
        return sizes.minByOrNull { it.width.toLong() * it.height.toLong() }!!
    }

    private fun updateCheckpointCounts(
        checkpoint: JSONObject,
        attempted: Int,
        configured: Int,
        rejected: Int,
        skippedNotSessionKey: Int,
        controlFailed: Int,
    ) {
        checkpoint
            .put("candidateAttemptedCount", attempted)
            .put("halSessionConfiguredCount", configured)
            .put("halSessionRejectedOrFailedCount", rejected)
            .put("skippedNotPhysicalSessionKeyCount", skippedNotSessionKey)
            .put("controlFailedCount", controlFailed)
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

    override fun onResume() {
        super.onResume()
        if (::saveButton.isInitialized && ::status.isInitialized) refreshStatus()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.69 report."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.69 report bestaat maar kon niet worden gelezen."
            return
        }
        status.text = buildString {
            append("candidates=").append(report.optInt("candidateCount", 0)).append('\n')
            append("attempted=").append(report.optInt("candidateAttemptedCount", 0)).append('\n')
            append("configured=").append(report.optInt("halSessionConfiguredCount", 0)).append('\n')
            append("rejected/failed=").append(report.optInt("halSessionRejectedOrFailedCount", 0)).append('\n')
            append("skipped non-session=").append(report.optInt("skippedNotPhysicalSessionKeyCount", 0)).append('\n')
            append("control failures=").append(report.optInt("controlFailedCount", 0)).append('\n')
            append("capture=false · repeating=false · pixels=false").append('\n')
            append(report.optString("classification", "?"))
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
            status.text = "v0.69 JSON opgeslagen."
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

    private enum class ValueType(val label: String) {
        INT("java.lang.Integer"),
        INT_ARRAY("int[]"),
        BYTE_ARRAY("byte[]"),
    }

    private data class CandidateSpec(
        val shortName: String,
        val keyName: String,
        val type: ValueType,
        val source: String,
    ) {
        fun value(): Any = when (type) {
            ValueType.INT -> 1
            ValueType.INT_ARRAY -> intArrayOf(1)
            ValueType.BYTE_ARRAY -> byteArrayOf(1)
        }
    }

    companion object {
        private const val SCHEMA = "truthraw.physical5-session-acceptance-matrix.v0.69"
        private const val AUTHORITY = "CAMERA2_HAL_SESSION_ACCEPTANCE_MATRIX_NO_CAPTURE"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_SESSION_ACCEPTANCE_MATRIX_v069.json"
        private const val REQUEST_CAMERA_PERMISSION = 66962
        private const val REQUEST_SAVE_JSON = 66963
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val BOUNDARY =
            "SESSION_CONFIGURATION_SUCCESS_ESTABLISHES_HAL_SESSION_ACCEPTANCE_IN_EACH_EXACT_ISOLATED_CONTEXT_ONLY; IT_DOES_NOT_PROVE_VENDOR_SEMANTICS, ACTIVE_PROCESSING_EFFECT, OEM_HIGH_PIXEL_ROUTE_IDENTITY, DIRECT_CFA_200MP, SENSOR_TOPOLOGY, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH"

        private val CANDIDATES = listOf(
            CandidateSpec("extendedSceneMode", "android.control.extendedSceneMode", ValueType.INT, "ANDROID_STANDARD_V065"),
            CandidateSpec("sensorPixelMode", "android.sensor.pixelMode", ValueType.INT, "ANDROID_STANDARD_V065"),
            CandidateSpec("MasterFilmSensorType", "com.hihonor.capture.metadata.MasterFilmSensorType", ValueType.INT_ARRAY, "HONOR_V065"),
            CandidateSpec("aoRunningMode", "com.hihonor.capture.metadata.aoRunningMode", ValueType.INT_ARRAY, "HONOR_V065"),
            CandidateSpec("videoDynamicFrameRate", "com.hihonor.capture.metadata.videoDynamicFrameRate", ValueType.INT_ARRAY, "HONOR_V065"),
            CandidateSpec("EnableVSR", "org.codeaurora.qcamera3.sessionParameters.EnableVSR", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("ExtendedMaxZoom", "org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("enableQLL", "org.codeaurora.qcamera3.sessionParameters.enableQLL", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("inSensorZoomEnable", "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable", ValueType.BYTE_ARRAY, "QTI_V065"),
            CandidateSpec("EnableHDRDCGMode", "org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("EnableOfflineHALZSL", "org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("EnableAICameraHSR", "org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("AICameraMode", "org.codeaurora.qcamera3.sessionParameters.AICameraMode", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("SnapshotHDRMode", "org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode", ValueType.INT_ARRAY, "QTI_V065"),
            CandidateSpec("HDRVividEnable", "com.hihonor.capture.metadata.HDRVividEnable", ValueType.BYTE_ARRAY, "HONOR_V065"),
            CandidateSpec("cameraSceneMode", "com.hihonor.capture.metadata.cameraSceneMode", ValueType.INT_ARRAY, "HONOR_V065"),
            CandidateSpec("extStreamSize", "com.hihonor.capture.metadata.extStreamSize", ValueType.INT_ARRAY, "HONOR_V065"),
            CandidateSpec("teleconverterEnable", "com.hihonor.capture.metadata.teleconverterEnable", ValueType.BYTE_ARRAY, "HONOR_V065"),
            CandidateSpec("thirdPartyCamera", "com.hihonor.capture.metadata.thirdPartyCamera", ValueType.BYTE_ARRAY, "HONOR_V065"),
        )

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

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

/**
 * v0.67 narrow physical-5 HAL/session acceptance probe.
 *
 * Candidate under test:
 *   com.hihonor.capture.metadata.aoRunningMode = intArrayOf(1)
 *
 * Evidence basis:
 * - v0.61: key is present on physical-5 session/request surfaces
 * - v0.64: physical-targeted builder is valid
 * - v0.65: local Camera2 marshaling resolves this key as int[]
 *
 * This probe first creates a control session with the same physical-5 Surface and no vendor
 * session parameter. Only if that configures successfully does it create a second session with
 * the single candidate session parameter attached.
 *
 * No capture request is submitted, no repeating request is started, and no ImageReader/pixel
 * access occurs.
 */
class Physical5AoRunningSessionAcceptanceActivity : Activity() {
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

        body.addView(label("TruthRaw v0.67 · aoRunningMode session acceptance", 21f, true))
        body.addView(label(
            "Nauwe HAL/session-test op physical camera 5. Eerst een identieke control-session zonder vendorwaarde; " +
                "daarna alleen aoRunningMode=intArrayOf(1). Geen capture of pixeltoegang.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Test control + MasterFilm session") { runProbe() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Een succesvolle candidate-session bewijst HAL/session-acceptatie in deze context, niet de semantische betekenis van de vendor-key en niet 200MP/RAW.",
            11f, false, Color.rgb(155, 165, 180)
        ))
        body.addView(space(10))
        status = label("Nog geen v0.67 report.", 10f, false)
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

    private fun runProbe() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            status.text = "v0.67 vraagt CAMERA-toestemming voor de session-acceptance test."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeProbe()
    }

    private fun executeProbe() {
        status.text = "v0.67 configureert eerst control-session…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V067_FATAL_ERROR")
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

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_CAMERA_PERMISSION) return
        if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            executeProbe()
        } else {
            status.text = "CAMERA-toestemming niet verleend; niets getest."
        }
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)

        val physicalRequestKeys = physical.availableCaptureRequestKeys.orEmpty()
        val physicalSessionKeys = physical.availableSessionKeys.orEmpty()
        val key = physicalRequestKeys.firstOrNull { it.name == TARGET_KEY }
            ?: error("Target key is not advertised by physical-5 capture-request surface")

        val previewSizes = physical
            .get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
            ?.getOutputSizes(SurfaceTexture::class.java)
            ?.toList()
            .orEmpty()

        require(previewSizes.isNotEmpty()) { "Physical camera 5 has no SurfaceTexture output sizes" }
        val chosenSize = choosePreviewSize(previewSizes)

        val out = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("physicalId5ListedByLogicalCharacteristics", logical.physicalCameraIds.contains(PHYSICAL_ID))
            .put("targetKey", TARGET_KEY)
            .put("targetKeyPhysicalRequestPresent", physicalRequestKeys.any { it.name == TARGET_KEY })
            .put("targetKeyPhysicalSessionPresent", physicalSessionKeys.any { it.name == TARGET_KEY })
            .put("resolvedJavaRepresentation", "int[]")
            .put("candidateValue", JSONArray(listOf(1)))
            .put("typeEvidence", "V065_SINGLE_JAVA_REPRESENTATION_ACCEPTED_INT_ARRAY")
            .put("previewSurface", JSONObject()
                .put("class", "android.graphics.SurfaceTexture")
                .put("width", chosenSize.width)
                .put("height", chosenSize.height)
                .put("physicalCameraBinding", PHYSICAL_ID)
                .put("imageReaderUsed", false))
            .put("cameraPermissionGranted", true)
            .put("sessionParameterKeyCount", 1)
            .put("captureSubmittedByTruthRaw", false)
            .put("repeatingRequestSubmittedByTruthRaw", false)
            .put("imageReaderCreatedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)

        val opened = openLogicalCamera(cm)
        val device = opened.device
        if (device == null) {
            return out
                .put("cameraOpenCompleted", false)
                .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                .put("classification", "V067_CAMERA_OPEN_FAILED")
                .put("boundary", BOUNDARY)
        }
        out.put("cameraOpenCompleted", true)

        try {
            val control = configureSession(
                device = device,
                size = chosenSize,
                sessionParameterKey = null,
                sessionParameterValue = null,
                label = "CONTROL_NO_VENDOR_PARAMETER"
            )
            out.put("control", control.toJson())

            if (!control.configured) {
                return out
                    .put("candidateAttempted", false)
                    .put("candidateSkipReason", "CONTROL_SESSION_DID_NOT_CONFIGURE")
                    .put("classification", "V067_CONTROL_SESSION_FAILED__CANDIDATE_NOT_ATTEMPTED")
                    .put("boundary", BOUNDARY)
            }

            @Suppress("UNCHECKED_CAST")
            val typedKey = key as CaptureRequest.Key<Any>
            val candidate = configureSession(
                device = device,
                size = chosenSize,
                sessionParameterKey = typedKey,
                sessionParameterValue = intArrayOf(1),
                label = "AO_RUNNING_MODE_INT_ARRAY_1"
            )
            out.put("candidateAttempted", true)
            out.put("candidate", candidate.toJson())

            return out
                .put("classification",
                    if (candidate.configured)
                        "AORUNNINGMODE_INT_ARRAY_1__HAL_SESSION_CONFIGURED__NO_CAPTURE"
                    else
                        "AORUNNINGMODE_INT_ARRAY_1__HAL_SESSION_REJECTED_OR_FAILED__NO_CAPTURE")
                .put("boundary", BOUNDARY)
        } finally {
            device.close()
        }
    }

    private data class SessionAttempt(
        val label: String,
        val sessionParameterAttached: Boolean,
        val localSetCompleted: Boolean,
        val localReadback: Any?,
        val createCallCompleted: Boolean,
        val configured: Boolean,
        val configureFailedCallback: Boolean,
        val closedCallbackObserved: Boolean,
        val errorClass: String?,
        val errorMessage: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("label", label)
            .put("sessionParameterAttached", sessionParameterAttached)
            .put("localSetCompleted", localSetCompleted)
            .put("localReadback", jsonValueStatic(localReadback))
            .put("createCaptureSessionCallCompleted", createCallCompleted)
            .put("onConfigured", configured)
            .put("onConfigureFailed", configureFailedCallback)
            .put("onClosedObserved", closedCallbackObserved)
            .put("errorClass", errorClass ?: JSONObject.NULL)
            .put("errorMessage", errorMessage ?: JSONObject.NULL)
            .put("captureSubmitted", false)
            .put("repeatingRequestSubmitted", false)
            .put("imageBufferAccessed", false)
    }

    private fun configureSession(
        device: CameraDevice,
        size: Size,
        sessionParameterKey: CaptureRequest.Key<Any>?,
        sessionParameterValue: Any?,
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
        var localSetCompleted = sessionParameterKey == null
        var localReadback: Any? = null

        try {
            val output = OutputConfiguration(surface).apply {
                setPhysicalCameraId(PHYSICAL_ID)
            }

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

            if (sessionParameterKey != null) {
                require(sessionParameterValue != null)
                val builder = device.createCaptureRequest(
                    CameraDevice.TEMPLATE_STILL_CAPTURE,
                    setOf(PHYSICAL_ID)
                )
                builder.setPhysicalCameraKey(
                    sessionParameterKey,
                    sessionParameterValue,
                    PHYSICAL_ID
                )
                localReadback = builder.getPhysicalCameraKey(sessionParameterKey, PHYSICAL_ID)
                localSetCompleted = true
                config.setSessionParameters(builder.build())
            }

            runCatching {
                device.createCaptureSession(config)
            }.onSuccess {
                createCompleted = true
            }.onFailure { e ->
                errorClass = e.javaClass.name
                errorMessage = e.message
                done.countDown()
            }

            if (!done.await(8, TimeUnit.SECONDS)) {
                errorClass = "java.util.concurrent.TimeoutException"
                errorMessage = "No onConfigured/onConfigureFailed callback within 8 seconds"
            }
            if (configured || failed) {
                closed.await(2, TimeUnit.SECONDS)
            }

            return SessionAttempt(
                label = label,
                sessionParameterAttached = sessionParameterKey != null,
                localSetCompleted = localSetCompleted,
                localReadback = localReadback,
                createCallCompleted = createCompleted,
                configured = configured,
                configureFailedCallback = failed,
                closedCallbackObserved = closed.count == 0L,
                errorClass = errorClass,
                errorMessage = errorMessage,
            )
        } catch (e: Throwable) {
            return SessionAttempt(
                label = label,
                sessionParameterAttached = sessionParameterKey != null,
                localSetCompleted = localSetCompleted,
                localReadback = localReadback,
                createCallCompleted = createCompleted,
                configured = false,
                configureFailedCallback = failed,
                closedCallbackObserved = closed.count == 0L,
                errorClass = e.javaClass.name,
                errorMessage = e.message,
            )
        } finally {
            runCatching { surface.release() }
            runCatching { texture.release() }
            executor.shutdownNow()
        }
    }

    private fun choosePreviewSize(sizes: List<Size>): Size {
        val preferred = listOf(Size(1920, 1080), Size(1280, 720), Size(640, 480))
        for (p in preferred) {
            sizes.firstOrNull { it.width == p.width && it.height == p.height }?.let { return it }
        }
        return sizes.minByOrNull { it.width.toLong() * it.height.toLong() }!!
    }

    private data class OpenResult(val device: CameraDevice?, val error: String?)

    private fun openLogicalCamera(cm: CameraManager): OpenResult {
        val latch = CountDownLatch(1)
        val executor = Executors.newSingleThreadExecutor()
        var device: CameraDevice? = null
        var error: String? = null

        try {
            cm.openCamera(LOGICAL_ID, executor, object : CameraDevice.StateCallback() {
                override fun onOpened(camera: CameraDevice) {
                    device = camera
                    latch.countDown()
                }

                override fun onDisconnected(camera: CameraDevice) {
                    error = "CAMERA_DISCONNECTED_BEFORE_SESSION_TEST"
                    camera.close()
                    latch.countDown()
                }

                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_$code"
                    camera.close()
                    latch.countDown()
                }
            })
            if (!latch.await(5, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (e: Throwable) {
            error = e.javaClass.name + ": " + e.message
        } finally {
            executor.shutdownNow()
        }
        return OpenResult(device, error)
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.67 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.67 report bestaat maar kon niet worden gelezen."
            return
        }

        val control = report.optJSONObject("control")
        val candidate = report.optJSONObject("candidate")
        status.text = buildString {
            append("control configured=").append(control?.optBoolean("onConfigured", false) ?: false).append('\n')
            append("candidate attempted=").append(report.optBoolean("candidateAttempted", false)).append('\n')
            append("candidate configured=").append(candidate?.optBoolean("onConfigured", false) ?: false).append('\n')
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
            status.text = "v0.67 JSON opgeslagen."
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
        private const val SCHEMA = "truthraw.physical5-aorunning-session-acceptance.v0.67"
        private const val AUTHORITY = "CAMERA2_HAL_SESSION_ACCEPTANCE_NO_CAPTURE"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_AORUNNING_SESSION_ACCEPTANCE_v067.json"
        private const val REQUEST_CAMERA_PERMISSION = 66762
        private const val REQUEST_SAVE_JSON = 66763
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val TARGET_KEY = "com.hihonor.capture.metadata.aoRunningMode"

        private const val BOUNDARY =
            "SESSION_CONFIGURATION_SUCCESS_ESTABLISHES_HAL_SESSION_ACCEPTANCE_IN_THIS_EXACT_CONTEXT_ONLY; IT_DOES_NOT_PROVE_VENDOR_SEMANTICS, ACTIVE_PROCESSING_EFFECT, OEM_HIGH_PIXEL_ROUTE_IDENTITY, DIRECT_CFA_200MP, SENSOR_TOPOLOGY, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH"

        private fun jsonValueStatic(value: Any?): Any = when (value) {
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

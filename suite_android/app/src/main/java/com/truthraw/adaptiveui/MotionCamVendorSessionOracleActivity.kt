package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.os.Bundle
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
 * v0.61 MotionCam-discovered vendor/session-key runtime oracle.
 *
 * Read-only by contract:
 * - no vendor key write
 * - no capture session
 * - no capture submission
 * - no ImageReader / pixel access
 *
 * The camera is opened only to create a disposable TEMPLATE_STILL_CAPTURE builder so existing
 * request defaults can be read where Camera2 exposes them.
 */
class MotionCamVendorSessionOracleActivity : Activity() {
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
        body.addView(label("TruthRaw v0.61 · vendor/session oracle", 21f, true))
        body.addView(label(
            "MotionCam-discovered keys · logical 0 versus physical 5 · read-only. " +
                "Geen vendor write, geen capture session, geen capture.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))
        body.addView(button("1 · Lees runtime key surface") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        status = label("Nog geen v0.61 report.", 10f, false)
        body.addView(status)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(12, 14, 18))
            addView(body, ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            ))
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
        }
    }

    private fun runOracle() {
        status.text = "v0.61 leest Camera2 key surfaces…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { error ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V061_FATAL_ERROR")
                    .put("errorClass", error.javaClass.name)
                    .put("errorMessage", error.message ?: JSONObject.NULL)
                    .put("vendorRequestWrittenByTruthRaw", false)
                    .put("sessionCreatedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
            }
            reportFile().writeText(report.toString(2))
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)

        val logicalSession = logical.availableSessionKeys.orEmpty()
        val physicalSession = physical.availableSessionKeys.orEmpty()
        val logicalRequest = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequest = physical.availableCaptureRequestKeys.orEmpty()

        val ls = logicalSession.map { it.name }.toSet()
        val ps = physicalSession.map { it.name }.toSet()
        val lr = logicalRequest.map { it.name }.toSet()
        val pr = physicalRequest.map { it.name }.toSet()

        val priority = JSONArray()
        for (spec in PRIORITY_KEYS) {
            priority.put(JSONObject()
                .put("name", spec.name)
                .put("source", spec.source)
                .put("logicalSessionPresent", spec.name in ls)
                .put("physicalSessionPresent", spec.name in ps)
                .put("logicalRequestPresent", spec.name in lr)
                .put("physicalRequestPresent", spec.name in pr)
                .put("logicalTemplateDefault", JSONObject.NULL)
                .put("physicalTemplateDefault", JSONObject.NULL)
                .put("defaultReadAttempted", false))
        }

        val out = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("discoveryProvenance", "RUNTIME_CAMERA2_SESSION_KEY_DISCOVERY_VIA_MOTIONCAM_UI")
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("logicalSessionKeys", keyNames(logicalSession))
            .put("physicalSessionKeys", keyNames(physicalSession))
            .put("logicalCaptureRequestKeys", keyNames(logicalRequest))
            .put("physicalCaptureRequestKeys", keyNames(physicalRequest))
            .put("priorityKeys", priority)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("sessionCreatedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("packageIdentitySpoofedByTruthRaw", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)

        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            return out
                .put("cameraPermissionGranted", false)
                .put("templateBuilderCreated", false)
                .put("classification", "KEY_SURFACE_COMPLETE__DEFAULT_READ_SKIPPED_NO_CAMERA_PERMISSION")
                .put("boundary", BOUNDARY)
        }
        out.put("cameraPermissionGranted", true)

        val opened = openLogicalCamera(cm)
        val device = opened.device
        if (device == null) {
            return out
                .put("templateBuilderCreated", false)
                .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                .put("classification", "KEY_SURFACE_COMPLETE__TEMPLATE_DEFAULT_READ_UNAVAILABLE")
                .put("boundary", BOUNDARY)
        }

        try {
            val builder = device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            out.put("templateBuilderCreated", true)
            out.put("cameraOpenedOnlyForDisposableTemplateRead", true)

            for (i in 0 until priority.length()) {
                val item = priority.getJSONObject(i)
                val name = item.getString("name")
                item.put("defaultReadAttempted", true)

                val lk = logicalRequest.firstOrNull { it.name == name }
                if (lk != null) {
                    @Suppress("UNCHECKED_CAST")
                    val key = lk as CaptureRequest.Key<Any>
                    runCatching { builder.get(key) }
                        .onSuccess { value ->
                            item.put("logicalDefaultReadCompleted", true)
                            item.put("logicalDefaultRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            item.put("logicalTemplateDefault", jsonValue(value))
                        }
                        .onFailure { error ->
                            item.put("logicalDefaultReadCompleted", false)
                            item.put("logicalDefaultReadError", errorJson(error))
                        }
                } else {
                    item.put("logicalDefaultReadCompleted", false)
                    item.put("logicalDefaultReadError", "NOT_IN_LOGICAL_AVAILABLE_CAPTURE_REQUEST_KEYS")
                }

                val pk = physicalRequest.firstOrNull { it.name == name }
                if (pk != null) {
                    @Suppress("UNCHECKED_CAST")
                    val key = pk as CaptureRequest.Key<Any>
                    runCatching { builder.getPhysicalCameraKey(key, PHYSICAL_ID) }
                        .onSuccess { value ->
                            item.put("physicalDefaultReadCompleted", true)
                            item.put("physicalDefaultRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            item.put("physicalTemplateDefault", jsonValue(value))
                        }
                        .onFailure { error ->
                            item.put("physicalDefaultReadCompleted", false)
                            item.put("physicalDefaultReadError", errorJson(error))
                        }
                } else {
                    item.put("physicalDefaultReadCompleted", false)
                    item.put("physicalDefaultReadError", "NOT_IN_PHYSICAL_AVAILABLE_CAPTURE_REQUEST_KEYS")
                }
            }
        } finally {
            device.close()
        }

        return out
            .put("classification", "READ_ONLY_MOTIONCAM_VENDOR_SESSION_SURFACE_AND_TEMPLATE_DEFAULTS")
            .put("boundary", BOUNDARY)
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
                    error = "CAMERA_DISCONNECTED_BEFORE_TEMPLATE_READ"
                    camera.close()
                    latch.countDown()
                }

                override fun onError(camera: CameraDevice, code: Int) {
                    error = "CAMERA_OPEN_ERROR_" + code
                    camera.close()
                    latch.countDown()
                }
            })
            if (!latch.await(5, TimeUnit.SECONDS)) error = "CAMERA_OPEN_TIMEOUT"
        } catch (t: Throwable) {
            error = t.javaClass.name + ": " + t.message
        } finally {
            executor.shutdownNow()
        }
        return OpenResult(device, error)
    }

    private fun keyNames(keys: List<CaptureRequest.Key<*>>): JSONArray =
        JSONArray(keys.map { it.name }.sorted())

    private fun errorJson(t: Throwable): JSONObject = JSONObject()
        .put("class", t.javaClass.name)
        .put("message", t.message ?: JSONObject.NULL)

    private fun jsonValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is Byte -> value.toInt() and 0xff
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.map { jsonValue(it) })
        is Collection<*> -> JSONArray(value.map { jsonValue(it) })
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.61 report."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.61 report bestaat maar kon niet als JSON worden gelezen."
            return
        }
        val keys = report.optJSONArray("priorityKeys")
        var ls = 0
        var ps = 0
        if (keys != null) {
            for (i in 0 until keys.length()) {
                val k = keys.optJSONObject(i) ?: continue
                if (k.optBoolean("logicalSessionPresent")) ls++
                if (k.optBoolean("physicalSessionPresent")) ps++
            }
        }
        status.text = "priorityKeys=" + (keys?.length() ?: 0) +
            "\\nlogical0 session hits=" + ls +
            "\\nphysical5 session hits=" + ps +
            "\\ntemplateBuilder=" + report.optBoolean("templateBuilderCreated", false) +
            "\\nvendorWrite=false · session=false · capture=false" +
            "\\n" + report.optString("classification", "?")
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
            status.text = "v0.61 JSON opgeslagen."
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

    private data class KeySpec(val name: String, val source: String)

    companion object {
        private const val SCHEMA = "truthraw.motioncam-vendor-session-oracle.v0.61"
        private const val AUTHORITY = "CAMERA2_SESSION_REQUEST_SURFACE_READ_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_MOTIONCAM_VENDOR_SESSION_ORACLE_v061.json"
        private const val REQUEST_SAVE_JSON = 66161
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"
        private const val BOUNDARY =
            "SESSION_REQUEST_KEY_PRESENCE_AND_TEMPLATE_DEFAULTS_ARE_RUNTIME_ROUTING_CONFIGURATION_EVIDENCE_ONLY; THEY_DO_NOT_PROVE_VENDOR_SEMANTICS, SENSOR_TOPOLOGY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OR_CALIBRATION_TRUTH"

        private val PRIORITY_KEYS = listOf(
            KeySpec("org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode", "MOTIONCAM_UI_QTI"),
            KeySpec("org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL", "MOTIONCAM_UI_QTI"),
            KeySpec("org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR", "MOTIONCAM_UI_QTI"),
            KeySpec("org.codeaurora.qcamera3.sessionParameters.AICameraMode", "MOTIONCAM_UI_QTI"),
            KeySpec("org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode", "MOTIONCAM_UI_QTI"),
            KeySpec("com.hihonor.capture.metadata.MasterFilmSensorType", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.HDRVividEnable", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.aoRunningMode", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.cameraSceneMode", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.extStreamSize", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.teleconverterEnable", "MOTIONCAM_UI_HONOR"),
            KeySpec("com.hihonor.capture.metadata.thirdPartyCamera", "MOTIONCAM_UI_HONOR"),
        )
    }
}

package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.os.Build
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
 * v0.64 permission-aware corrected physical-camera template-default oracle.
 *
 * v0.61 used CameraDevice.createCaptureRequest(TEMPLATE_STILL_CAPTURE) and then
 * getPhysicalCameraKey(..., "5"). Android rejects that because the builder was not created
 * with physical camera IDs. v0.64 uses the API-28 overload
 * createCaptureRequest(template, setOf("5")) and remains read-only:
 * no request values are written, no session is created, no request is submitted.
 */
class Physical5TemplateDefaultsOracleActivity : Activity() {
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
        body.addView(label("TruthRaw v0.64 · Physical-5 template defaults", 21f, true))
        body.addView(label(
            "Corrigeert de v0.61 physical-default read en vraagt CAMERA-toestemming wanneer nodig. " +
                "Daarna wordt de request-builder expliciet voor physical camera 5 gemaakt. Geen vendor writes, geen session, geen capture.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Lees physical-5 defaults") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        status = label("Nog geen v0.64 report.", 10f, false)
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
            status.text = "v0.64 vraagt CAMERA-toestemming voor alleen de read-only template-builder."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeOracle()
    }

    private fun executeOracle() {
        status.text = "v0.64 opent logical 0 alleen voor read-only request templates…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V064_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("vendorRequestWrittenByTruthRaw", false)
                    .put("sessionCreatedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
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
            status.text = "CAMERA-toestemming verleend; v0.64 start nu automatisch."
            executeOracle()
        } else {
            status.text = "CAMERA-toestemming niet verleend; er is niets geopend of vastgelegd."
        }
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)

        val logicalRequestKeys = logical.availableCaptureRequestKeys.orEmpty()
        val physicalRequestKeys = physical.availableCaptureRequestKeys.orEmpty()

        val out = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("logicalPhysicalCameraIds",
                JSONArray(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) logical.physicalCameraIds.toList() else emptyList()))
            .put("physicalId5ListedByLogicalCharacteristics",
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && logical.physicalCameraIds.contains(PHYSICAL_ID))
            .put("logicalCaptureRequestKeys", JSONArray(logicalRequestKeys.map { it.name }.sorted()))
            .put("physicalCaptureRequestKeys", JSONArray(physicalRequestKeys.map { it.name }.sorted()))
            .put("targets", JSONArray())
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("sessionCreatedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)

        val logicalNames = logicalRequestKeys.map { it.name }.toSet()
        val physicalNames = physicalRequestKeys.map { it.name }.toSet()

        val targets = out.getJSONArray("targets")
        for (name in TARGET_KEYS) {
            targets.put(JSONObject()
                .put("name", name)
                .put("logicalRequestPresent", name in logicalNames)
                .put("physicalRequestPresent", name in physicalNames)
                .put("logicalTemplateDefault", JSONObject.NULL)
                .put("physicalTemplateDefault", JSONObject.NULL))
        }

        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            return out
                .put("cameraPermissionGranted", false)
                .put("logicalBuilderCreated", false)
                .put("physicalTargetedBuilderCreated", false)
                .put("classification", "KEY_SURFACE_COMPLETE__DEFAULT_READ_SKIPPED_NO_CAMERA_PERMISSION")
                .put("boundary", BOUNDARY)
        }
        out.put("cameraPermissionGranted", true)

        val opened = openLogicalCamera(cm)
        val device = opened.device
        if (device == null) {
            return out
                .put("logicalBuilderCreated", false)
                .put("physicalTargetedBuilderCreated", false)
                .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                .put("classification", "KEY_SURFACE_COMPLETE__CAMERA_OPEN_FAILED")
                .put("boundary", BOUNDARY)
        }

        try {
            val logicalBuilder = runCatching {
                device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
            }.getOrNull()
            out.put("logicalBuilderCreated", logicalBuilder != null)

            val physicalBuilderResult = runCatching {
                device.createCaptureRequest(
                    CameraDevice.TEMPLATE_STILL_CAPTURE,
                    setOf(PHYSICAL_ID)
                )
            }
            val physicalBuilder = physicalBuilderResult.getOrNull()
            out.put("physicalTargetedBuilderCreated", physicalBuilder != null)
            physicalBuilderResult.exceptionOrNull()?.let {
                out.put("physicalTargetedBuilderCreateError", errorJson(it))
            }

            for (i in 0 until targets.length()) {
                val entry = targets.getJSONObject(i)
                val name = entry.getString("name")

                val lk = logicalRequestKeys.firstOrNull { it.name == name }
                if (logicalBuilder != null && lk != null) {
                    @Suppress("UNCHECKED_CAST")
                    val key = lk as CaptureRequest.Key<Any>
                    runCatching { logicalBuilder.get(key) }
                        .onSuccess { value ->
                            entry.put("logicalDefaultReadCompleted", true)
                            entry.put("logicalDefaultRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            entry.put("logicalTemplateDefault", jsonValue(value))
                        }
                        .onFailure { e ->
                            entry.put("logicalDefaultReadCompleted", false)
                            entry.put("logicalDefaultReadError", errorJson(e))
                        }
                } else {
                    entry.put("logicalDefaultReadCompleted", false)
                    entry.put("logicalDefaultReadError",
                        if (lk == null) "NOT_IN_LOGICAL_AVAILABLE_CAPTURE_REQUEST_KEYS" else "LOGICAL_BUILDER_UNAVAILABLE")
                }

                val pk = physicalRequestKeys.firstOrNull { it.name == name }
                if (physicalBuilder != null && pk != null) {
                    @Suppress("UNCHECKED_CAST")
                    val key = pk as CaptureRequest.Key<Any>
                    runCatching { physicalBuilder.getPhysicalCameraKey(key, PHYSICAL_ID) }
                        .onSuccess { value ->
                            entry.put("physicalDefaultReadCompleted", true)
                            entry.put("physicalDefaultRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            entry.put("physicalTemplateDefault", jsonValue(value))
                        }
                        .onFailure { e ->
                            entry.put("physicalDefaultReadCompleted", false)
                            entry.put("physicalDefaultReadError", errorJson(e))
                        }
                } else {
                    entry.put("physicalDefaultReadCompleted", false)
                    entry.put("physicalDefaultReadError",
                        if (pk == null) "NOT_IN_PHYSICAL_AVAILABLE_CAPTURE_REQUEST_KEYS" else "PHYSICAL_TARGETED_BUILDER_UNAVAILABLE")
                }
            }
        } finally {
            device.close()
        }

        return out
            .put("cameraOpenedOnlyForTemplateReads", true)
            .put("classification", "READ_ONLY_PHYSICAL5_TARGETED_TEMPLATE_DEFAULTS")
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

    private fun errorJson(e: Throwable): JSONObject = JSONObject()
        .put("class", e.javaClass.name)
        .put("message", e.message ?: JSONObject.NULL)

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
            status.text = "Nog geen v0.64 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.64 report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val targets = report.optJSONArray("targets")
        var physicalReads = 0
        if (targets != null) {
            for (i in 0 until targets.length()) {
                if (targets.optJSONObject(i)?.optBoolean("physicalDefaultReadCompleted") == true) physicalReads++
            }
        }

        status.text = buildString {
            append("logical lists physical5=")
                .append(report.optBoolean("physicalId5ListedByLogicalCharacteristics", false)).append('\n')
            append("physical-targeted builder=")
                .append(report.optBoolean("physicalTargetedBuilderCreated", false)).append('\n')
            append("physical defaults read=").append(physicalReads).append('/').append(targets?.length() ?: 0).append('\n')
            append("vendorWrite=false · session=false · capture=false").append('\n')
            append("classification=").append(report.optString("classification", "?"))
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
            status.text = "v0.64 JSON opgeslagen."
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
        private const val SCHEMA = "truthraw.physical5-template-defaults-oracle.v0.64"
        private const val AUTHORITY = "CAMERA2_REQUEST_TEMPLATE_READ_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_TEMPLATE_DEFAULTS_ORACLE_v064.json"
        private const val REQUEST_CAMERA_PERMISSION = 66462\n        private const val REQUEST_SAVE_JSON = 66463
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private val TARGET_KEYS = listOf(
            "android.control.extendedSceneMode",
            "android.sensor.pixelMode",
            "com.hihonor.capture.metadata.MasterFilmSensorType",
            "com.hihonor.capture.metadata.aoRunningMode",
            "com.hihonor.capture.metadata.mmiLaserEyeSafeMode",
            "com.hihonor.capture.metadata.videoDynamicFrameRate",
            "org.codeaurora.qcamera3.sessionParameters.EnableVSR",
            "org.codeaurora.qcamera3.sessionParameters.ExtendedMaxZoom",
            "org.codeaurora.qcamera3.sessionParameters.enableQLL",
            "org.codeaurora.qcamera3.sessionParameters.inSensorZoomEnable",
            "org.codeaurora.qcamera3.sessionParameters.EnableHDRDCGMode",
            "org.codeaurora.qcamera3.sessionParameters.EnableOfflineHALZSL",
            "org.codeaurora.qcamera3.sessionParameters.EnableAICameraHSR",
            "org.codeaurora.qcamera3.sessionParameters.AICameraMode",
            "org.codeaurora.qcamera3.sessionParameters.SnapshotHDRMode",
            "com.hihonor.capture.metadata.HDRVividEnable",
            "com.hihonor.capture.metadata.cameraSceneMode",
            "com.hihonor.capture.metadata.extStreamSize",
            "com.hihonor.capture.metadata.teleconverterEnable",
            "com.hihonor.capture.metadata.thirdPartyCamera"
        )

        private const val BOUNDARY =
            "REQUEST_TEMPLATE_DEFAULTS_ARE_RUNTIME_CONFIGURATION_EVIDENCE_ONLY; THEY_DO_NOT_PROVE_VENDOR_SEMANTICS, SENSOR_TOPOLOGY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OEM_ROUTE_IDENTITY, OR_CALIBRATION_TRUTH"
    }
}

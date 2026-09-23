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
 * v0.65 local type/acceptance oracle for physical camera 5 request keys.
 *
 * It creates a physical-targeted TEMPLATE_STILL_CAPTURE builder for logical 0 + physical 5,
 * then tries candidate Java value representations on a fresh local builder per candidate.
 *
 * No CaptureSession is created and no CaptureRequest is submitted to HAL.
 * Therefore this can resolve Camera2 marshaling/Java value acceptance only, not vendor semantics.
 */
class Physical5LocalTypeOracleActivity : Activity() {
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
        body.addView(label("TruthRaw v0.65 · Physical-5 local type oracle", 21f, true))
        body.addView(label(
            "Test alleen lokale Camera2 request-marshaling voor physical camera 5. " +
                "Elke kandidaat krijgt een verse request-builder; geen session en niets wordt naar HAL verstuurd.",
            12f, false, Color.rgb(190, 198, 210)
        ))
        body.addView(space(10))
        body.addView(button("1 · Resolve local key types") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        status = label("Nog geen v0.65 report.", 10f, false)
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
            status.text = "v0.65 vraagt CAMERA-toestemming om alleen een lokale request-builder te maken."
            requestPermissions(arrayOf(Manifest.permission.CAMERA), REQUEST_CAMERA_PERMISSION)
            return
        }
        executeOracle()
    }

    private fun executeOracle() {
        status.text = "v0.65 resolveert lokale Camera2 value-types…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "V065_FATAL_ERROR")
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
                    .put("sessionCreatedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("vendorModifiedRequestSubmittedToHal", false)
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
            executeOracle()
        } else {
            status.text = "CAMERA-toestemming niet verleend; er is niets getest."
        }
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val logical = cm.getCameraCharacteristics(LOGICAL_ID)
        val physical = cm.getCameraCharacteristics(PHYSICAL_ID)
        val physicalKeys = physical.availableCaptureRequestKeys.orEmpty()
        val physicalByName = physicalKeys.associateBy { it.name }

        val out = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("logicalCameraId", LOGICAL_ID)
            .put("physicalCameraId", PHYSICAL_ID)
            .put("physicalId5ListedByLogicalCharacteristics", logical.physicalCameraIds.contains(PHYSICAL_ID))
            .put("targets", JSONArray())
            .put("cameraPermissionGranted", true)
            .put("localRequestMetadataMutatedForTypeTesting", true)
            .put("sessionCreatedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("vendorModifiedRequestSubmittedToHal", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)

        val opened = openLogicalCamera(cm)
        val device = opened.device
        if (device == null) {
            return out
                .put("cameraOpenError", opened.error ?: JSONObject.NULL)
                .put("classification", "PHYSICAL5_LOCAL_TYPE_ORACLE_CAMERA_OPEN_FAILED")
                .put("boundary", BOUNDARY)
        }

        try {
            for (name in TARGET_KEYS) {
                val target = JSONObject()
                    .put("name", name)
                    .put("physicalRequestPresent", physicalByName.containsKey(name))
                    .put("candidateTests", JSONArray())

                val keyAny = physicalByName[name]
                if (keyAny == null) {
                    target.put("classification", "KEY_NOT_ADVERTISED_BY_PHYSICAL5")
                    out.getJSONArray("targets").put(target)
                    continue
                }

                @Suppress("UNCHECKED_CAST")
                val key = keyAny as CaptureRequest.Key<Any>
                val accepted = mutableListOf<String>()

                for (candidate in CANDIDATES) {
                    val one = JSONObject()
                        .put("candidateName", candidate.name)
                        .put("candidateRuntimeClass", candidate.value.javaClass.name)

                    val builderResult = runCatching {
                        device.createCaptureRequest(
                            CameraDevice.TEMPLATE_STILL_CAPTURE,
                            setOf(PHYSICAL_ID)
                        )
                    }
                    val builder = builderResult.getOrNull()
                    if (builder == null) {
                        one.put("builderCreated", false)
                        one.put("builderError", errorJson(builderResult.exceptionOrNull() ?: RuntimeException("unknown builder error")))
                    } else {
                        one.put("builderCreated", true)
                        runCatching {
                            builder.setPhysicalCameraKey(key, candidate.value, PHYSICAL_ID)
                            builder.getPhysicalCameraKey(key, PHYSICAL_ID)
                        }.onSuccess { readback ->
                            one.put("setAccepted", true)
                            one.put("readbackRuntimeClass", readback?.javaClass?.name ?: JSONObject.NULL)
                            one.put("readback", jsonValue(readback))
                            accepted += candidate.name
                        }.onFailure { e ->
                            one.put("setAccepted", false)
                            one.put("error", errorJson(e))
                        }
                    }

                    target.getJSONArray("candidateTests").put(one)
                }

                target.put("acceptedCandidateNames", JSONArray(accepted))
                target.put("acceptedCandidateCount", accepted.size)
                target.put("classification", when (accepted.size) {
                    0 -> "NO_TESTED_JAVA_REPRESENTATION_ACCEPTED"
                    1 -> "SINGLE_JAVA_REPRESENTATION_ACCEPTED"
                    else -> "MULTIPLE_JAVA_REPRESENTATIONS_ACCEPTED"
                })
                out.getJSONArray("targets").put(target)
            }
        } finally {
            device.close()
        }

        return out
            .put("cameraOpenedOnlyForLocalRequestTypeTests", true)
            .put("classification", "PHYSICAL5_LOCAL_REQUEST_TYPE_ACCEPTANCE__NO_SESSION_NO_HAL_SUBMISSION")
            .put("boundary", BOUNDARY)
    }

    private data class Candidate(val name: String, val value: Any)

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
                    error = "CAMERA_DISCONNECTED_BEFORE_LOCAL_TYPE_TEST"
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
        is Int -> value
        is Long -> value
        is Float -> value.toDouble()
        is Double -> value
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.65 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.65 report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val targets = report.optJSONArray("targets")
        var single = 0
        var unresolved = 0
        if (targets != null) {
            for (i in 0 until targets.length()) {
                when (targets.optJSONObject(i)?.optString("classification")) {
                    "SINGLE_JAVA_REPRESENTATION_ACCEPTED" -> single++
                    "NO_TESTED_JAVA_REPRESENTATION_ACCEPTED" -> unresolved++
                }
            }
        }

        status.text = buildString {
            append("targets=").append(targets?.length() ?: 0).append('\n')
            append("single-type resolved=").append(single).append('\n')
            append("unresolved=").append(unresolved).append('\n')
            append("session=false · capture=false · HAL submission=false").append('\n')
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
            status.text = "v0.65 JSON opgeslagen."
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
        private const val SCHEMA = "truthraw.physical5-local-type-oracle.v0.65"
        private const val AUTHORITY = "CAMERA2_LOCAL_REQUEST_MARSHALING_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_PHYSICAL5_LOCAL_TYPE_ORACLE_v065.json"
        private const val REQUEST_CAMERA_PERMISSION = 66562
        private const val REQUEST_SAVE_JSON = 66563
        private const val LOGICAL_ID = "0"
        private const val PHYSICAL_ID = "5"

        private val TARGET_KEYS = listOf(
            "android.control.extendedSceneMode",
            "android.sensor.pixelMode",
            "com.hihonor.capture.metadata.MasterFilmSensorType",
            "com.hihonor.capture.metadata.aoRunningMode",
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

        private val CANDIDATES = listOf(
            Candidate("INT_1", 1),
            Candidate("INT_ARRAY_1", intArrayOf(1)),
            Candidate("BYTE_1", 1.toByte()),
            Candidate("BYTE_ARRAY_1", byteArrayOf(1)),
            Candidate("LONG_1", 1L),
            Candidate("LONG_ARRAY_1", longArrayOf(1L)),
            Candidate("FLOAT_1", 1f),
            Candidate("FLOAT_ARRAY_1", floatArrayOf(1f)),
            Candidate("DOUBLE_1", 1.0),
            Candidate("DOUBLE_ARRAY_1", doubleArrayOf(1.0))
        )

        private const val BOUNDARY =
            "LOCAL_REQUEST_VALUE_ACCEPTANCE_ESTABLISHES_CAMERA2_MARSHALING_COMPATIBILITY_ONLY; IT_DOES_NOT_PROVE_VENDOR_SEMANTICS, HAL_ACCEPTANCE, SENSOR_TOPOLOGY, DIRECT_CFA_200MP, ADC_BIT_DEPTH, OEM_ROUTE_IDENTITY, OR_CALIBRATION_TRUTH"
    }
}

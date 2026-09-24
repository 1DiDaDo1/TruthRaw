package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.params.OutputConfiguration
import android.hardware.camera2.params.SessionConfiguration
import android.os.Build
import android.os.Bundle
import android.util.Size
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

/**
 * v0.51 CameraDeviceSetup session-support oracle.
 *
 * Public API, read-only query:
 * - does not open CameraDevice
 * - does not create a real CameraCaptureSession
 * - does not instantiate ImageReader
 * - does not access pixels
 *
 * It tests only evidence-based stream candidates already observed in TruthRaw Camera2
 * characteristics and the Android-17 RAW14 format value.
 */
class CameraDeviceSetupRaw14SessionOracleActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
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

        body.addView(label("TruthRaw v0.51 · CameraDeviceSetup RAW14 session oracle", 21f, true))
        body.addView(label(
            "Vraagt Android alleen of een kleine, vooraf gemotiveerde set hypothetische RAW-sessies ondersteund wordt. " +
                "Er wordt geen camera geopend en er wordt geen echte capturesessie of ImageReader gemaakt.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Query RAW session support") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "Camera 5 kandidaten: 4080×3072, 8160×6144 en 16320×12288. " +
                "RAW_SENSOR/RAW10 dienen als controles; RAW14 gebruikt runtime format value 44. " +
                "Voor max-res kandidaten wordt ook de reeds historisch gebruikte physical SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION variant getest.",
            11f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.51 report.", 10f, false)
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
        status.text = "v0.51 voert read-only CameraDeviceSetup queries uit…"
        saveButton.isEnabled = false

        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("cameraOpenedByTruthRaw", false)
                    .put("captureSessionCreatedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("errorClass", e.javaClass.name)
                    .put("errorMessage", e.message ?: JSONObject.NULL)
            }
            reportFile().writeText(report.toString(2))
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private fun buildReport(): JSONObject {
        val cm = getSystemService(CameraManager::class.java)
        val publicIds = cm.cameraIdList.toList()
        val logical0Chars = cm.getCameraCharacteristics("0")
        val physicalIds = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            logical0Chars.physicalCameraIds
        } else {
            emptySet()
        }

        val raw14 = resolveRaw14()
        val setupSupported = JSONObject()
        for (id in publicIds) {
            setupSupported.put(id, runCatching { cm.isCameraDeviceSetupSupported(id) }
                .fold(
                    onSuccess = { JSONObject().put("completed", true).put("supported", it) },
                    onFailure = { JSONObject().put("completed", false).put("error", errorJson(it)) },
                ))
        }

        val queries = JSONArray()

        if ("0" in publicIds) {
            val setupResult = runCatching { cm.getCameraDeviceSetup("0") }
            setupResult.onSuccess { setup ->
                for (candidate in candidates(raw14.value)) {
                    queries.put(queryOne(setup, physicalIds, candidate))
                }
            }.onFailure { e ->
                queries.put(JSONObject()
                    .put("scope", "LOGICAL_0_SETUP_CREATION")
                    .put("completed", false)
                    .put("error", errorJson(e)))
            }
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("captureEvidenceGranted", false)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSessionCreatedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageReaderCreatedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("raw14Runtime", raw14.toJson())
            .put("publicCameraIds", JSONArray(publicIds))
            .put("logical0PhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameraDeviceSetupSupport", setupSupported)
            .put("candidateSelection", JSONObject()
                .put("authority", "EVIDENCE_BASED_PREDECLARED_CANDIDATES_ONLY")
                .put("camera2Source", "v0.47/v0.48 advertised RAW_SENSOR/RAW10 geometries")
                .put("physicalMaxPixelModeSource", "v0.37/v0.38 established request topology control")
                .put("unknownSizeEnumerationAttempted", false)
                .put("unknownFormatEnumerationAttempted", false))
            .put("queries", queries)
            .put("classification", "CAMERA_DEVICE_SETUP_RAW_SESSION_SUPPORT_ORACLE__NO_CAMERA_OPEN_NO_SESSION_NO_CAPTURE")
            .put("boundary",
                "SESSION_SUPPORT_QUERY_IS_CONFIGURATION_CAPABILITY_EVIDENCE_ONLY; IT DOES_NOT PROVE_CAPTURE_SUCCESS, PAYLOAD_POPULATION, NATIVE_ADC_TOPOLOGY, DIRECT_CFA_200MP, OR OEM_ROUTE_IDENTITY")
    }

    private data class Candidate(
        val physicalId: String,
        val formatName: String,
        val formatValue: Int,
        val width: Int,
        val height: Int,
        val parameterMode: ParameterMode,
        val rationale: String,
    )

    private enum class ParameterMode {
        NONE,
        PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
    }

    private data class RuntimeIntConstant(
        val available: Boolean,
        val value: Int?,
        val error: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("name", "android.graphics.ImageFormat.RAW14")
            .put("available", available)
            .put("value", value ?: JSONObject.NULL)
            .put("error", error ?: JSONObject.NULL)
    }

    private fun resolveRaw14(): RuntimeIntConstant =
        runCatching {
            RuntimeIntConstant(true, ImageFormat::class.java.getField("RAW14").getInt(null), null)
        }.getOrElse {
            RuntimeIntConstant(false, null, "${it.javaClass.name}: ${it.message}")
        }

    private fun candidates(raw14Value: Int?): List<Candidate> {
        val out = mutableListOf<Candidate>()

        fun add(
            physicalId: String,
            name: String,
            format: Int,
            w: Int,
            h: Int,
            mode: ParameterMode,
            rationale: String,
        ) {
            out += Candidate(physicalId, name, format, w, h, mode, rationale)
        }

        // Physical 5: exact standard/max/high-res geometries measured by v0.47/v0.48.
        for ((name, format) in listOf("RAW_SENSOR" to ImageFormat.RAW_SENSOR, "RAW10" to ImageFormat.RAW10)) {
            add("5", name, format, 4080, 3072, ParameterMode.NONE, "CAM5_ADVERTISED_DEFAULT_CONTROL")
            add("5", name, format, 8160, 6144, ParameterMode.NONE, "CAM5_ADVERTISED_MAX_RES_CONTROL_WITHOUT_PIXEL_MODE")
            add("5", name, format, 8160, 6144, ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, "CAM5_ADVERTISED_MAX_RES_CONTROL_WITH_PHYSICAL_MAX_PIXEL_MODE")
            add("5", name, format, 16320, 12288, ParameterMode.NONE, "CAM5_ADVERTISED_HIGH_RES_CONTROL_WITHOUT_PIXEL_MODE")
            add("5", name, format, 16320, 12288, ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, "CAM5_ADVERTISED_HIGH_RES_CONTROL_WITH_PHYSICAL_MAX_PIXEL_MODE")
        }

        if (raw14Value != null) {
            add("5", "RAW14", raw14Value, 4080, 3072, ParameterMode.NONE, "CAM5_RAW14_AT_DEFAULT_GEOMETRY")
            add("5", "RAW14", raw14Value, 8160, 6144, ParameterMode.NONE, "CAM5_RAW14_AT_MAX_GEOMETRY_WITHOUT_PIXEL_MODE")
            add("5", "RAW14", raw14Value, 8160, 6144, ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, "CAM5_RAW14_AT_MAX_GEOMETRY_WITH_PHYSICAL_MAX_PIXEL_MODE")
            add("5", "RAW14", raw14Value, 16320, 12288, ParameterMode.NONE, "CAM5_RAW14_AT_HIGH_RES_GEOMETRY_WITHOUT_PIXEL_MODE")
            add("5", "RAW14", raw14Value, 16320, 12288, ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, "CAM5_RAW14_AT_HIGH_RES_GEOMETRY_WITH_PHYSICAL_MAX_PIXEL_MODE")

            // Physical 2 and 4 serve as narrow cross-camera controls using observed geometries only.
            add("2", "RAW14", raw14Value, 4096, 3072, ParameterMode.NONE, "CAM2_RAW14_AT_DEFAULT_GEOMETRY")
            add("2", "RAW14", raw14Value, 8192, 6144, ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION, "CAM2_RAW14_AT_MAX_GEOMETRY_WITH_PHYSICAL_MAX_PIXEL_MODE")
            add("4", "RAW14", raw14Value, 4032, 3024, ParameterMode.NONE, "CAM4_RAW14_AT_DEFAULT_GEOMETRY")
        }

        return out
    }

    private fun queryOne(
        setup: CameraDevice.CameraDeviceSetup,
        physicalIds: Set<String>,
        c: Candidate,
    ): JSONObject {
        val out = JSONObject()
            .put("logicalCameraId", setup.id)
            .put("physicalCameraId", c.physicalId)
            .put("physicalIdDisclosedByLogical0", c.physicalId in physicalIds)
            .put("formatName", c.formatName)
            .put("formatValue", c.formatValue)
            .put("width", c.width)
            .put("height", c.height)
            .put("pixels", c.width.toLong() * c.height.toLong())
            .put("parameterMode", c.parameterMode.name)
            .put("rationale", c.rationale)

        if (c.physicalId !in physicalIds) {
            return out
                .put("queryAttempted", false)
                .put("skipReason", "PHYSICAL_ID_NOT_DISCLOSED_BY_LOGICAL_0")
        }

        val output = runCatching {
            OutputConfiguration(c.formatValue, Size(c.width, c.height)).also {
                it.setPhysicalCameraId(c.physicalId)
            }
        }.getOrElse { e ->
            return out
                .put("queryAttempted", false)
                .put("outputConfigurationError", errorJson(e))
        }

        val config = runCatching {
            SessionConfiguration(
                SessionConfiguration.SESSION_REGULAR,
                listOf(output),
            )
        }.getOrElse { e ->
            return out
                .put("queryAttempted", false)
                .put("sessionConfigurationError", errorJson(e))
        }

        if (c.parameterMode == ParameterMode.PHYSICAL_SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION) {
            runCatching {
                val builder = setup.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)
                builder.setPhysicalCameraKey(
                    CaptureRequest.SENSOR_PIXEL_MODE,
                    CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION,
                    c.physicalId,
                )
                val request = builder.build()
                config.setSessionParameters(request)
                out.put("sessionParametersAttached", true)
                out.put("physicalSensorPixelModeRequested", CameraMetadata.SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION)
                out.put(
                    "physicalSensorPixelModeReadback",
                    builder.getPhysicalCameraKey(CaptureRequest.SENSOR_PIXEL_MODE, c.physicalId)
                        ?: JSONObject.NULL,
                )
            }.onFailure { e ->
                out.put("sessionParametersAttached", false)
                out.put("sessionParameterError", errorJson(e))
            }
        } else {
            out.put("sessionParametersAttached", false)
        }

        out.put("queryAttempted", true)

        runCatching { setup.isSessionConfigurationSupported(config) }
            .onSuccess { supported ->
                out.put("isSessionConfigurationSupportedCompleted", true)
                out.put("supported", supported)

                if (supported) {
                    runCatching { setup.getSessionCharacteristics(config) }
                        .onSuccess { sc ->
                            out.put("sessionCharacteristicsCompleted", true)
                            out.put("sessionCharacteristicsKeyCount", sc.keys.size)
                            out.put(
                                "sessionActiveArray",
                                sc.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)?.toString()
                                    ?: JSONObject.NULL,
                            )
                            out.put(
                                "sessionMaxDigitalZoom",
                                sc.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM)
                                    ?: JSONObject.NULL,
                            )
                        }
                        .onFailure { e ->
                            out.put("sessionCharacteristicsCompleted", false)
                            out.put("sessionCharacteristicsError", errorJson(e))
                        }
                }
            }
            .onFailure { e ->
                out.put("isSessionConfigurationSupportedCompleted", false)
                out.put("supportQueryError", errorJson(e))
            }

        return out
    }

    private fun errorJson(e: Throwable): JSONObject =
        JSONObject()
            .put("class", e.javaClass.name)
            .put("message", e.message ?: JSONObject.NULL)
            .put("causeClass", e.cause?.javaClass?.name ?: JSONObject.NULL)
            .put("causeMessage", e.cause?.message ?: JSONObject.NULL)

    private fun refreshStatus() {
        val f = reportFile()
        saveButton.isEnabled = f.exists() && f.length() > 0L
        if (!f.exists()) {
            status.text = "Nog geen v0.51 report."
            return
        }

        val report = runCatching { JSONObject(f.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.51 report bestaat maar kon niet worden gelezen."
            return
        }

        var yes = 0
        var no = 0
        var err = 0
        val qs = report.optJSONArray("queries")
        if (qs != null) {
            for (i in 0 until qs.length()) {
                val q = qs.optJSONObject(i) ?: continue
                when {
                    q.optBoolean("isSessionConfigurationSupportedCompleted", false) &&
                        q.optBoolean("supported", false) -> yes++
                    q.optBoolean("isSessionConfigurationSupportedCompleted", false) -> no++
                    q.optBoolean("queryAttempted", false) -> err++
                }
            }
        }

        status.text = buildString {
            append("supported=").append(yes)
                .append(" · unsupported=").append(no)
                .append(" · queryErrors=").append(err).append('\n')
            append("cameraOpenedByTruthRaw=").append(report.optBoolean("cameraOpenedByTruthRaw", true)).append('\n')
            append("captureSessionCreatedByTruthRaw=").append(report.optBoolean("captureSessionCreatedByTruthRaw", true)).append('\n')
            append("authority=").append(report.optString("authority", "?"))
        }
    }

    @Suppress("DEPRECATION")
    private fun saveReport() {
        val source = reportFile()
        if (!source.exists()) return
        startActivityForResult(
            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "application/json"
                putExtra(Intent.EXTRA_TITLE, source.name)
            },
            REQUEST_SAVE_JSON,
        )
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
            status.text = "v0.51 JSON opgeslagen · setup-query only · geen camera-open."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File = File(filesDir, REPORT_FILENAME)

    private fun button(text: String, action: () -> Unit): Button =
        Button(this).apply {
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
        private const val SCHEMA = "truthraw.camera-device-setup-raw14-session-oracle.v0.51"
        private const val AUTHORITY = "CAMERA2_CAMERA_DEVICE_SETUP_SESSION_QUERY_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_CAMERA_DEVICE_SETUP_RAW14_SESSION_ORACLE_v051.json"
        private const val REQUEST_SAVE_JSON = 65151
    }
}

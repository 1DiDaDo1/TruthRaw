package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Build
import android.os.Bundle
import android.util.Range
import android.util.Rational
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
import kotlin.math.abs

/**
 * v0.46 read-only Honor capability route oracle.
 *
 * This experiment does not open a camera and does not submit a request. It reads only
 * CameraCharacteristics already exposed by CameraManager for public logical IDs and the
 * physical IDs disclosed by those logical cameras.
 *
 * Target vendor-characteristic names are selected from static analysis of the exact
 * user-supplied Honor Camera APK. APK-derived names and control-flow hints remain
 * STATIC_APK_TARGET_SELECTION_ONLY; runtime characteristic values remain
 * CAMERA2_CHARACTERISTICS_OBSERVATION_ONLY.
 */
class HonorCapabilityRouteOracleActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
        refreshStatus()
    }

    override fun onResume() {
        super.onResume()
        refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }

        body.addView(label("TruthRaw v0.46 · Honor capability route oracle", 22f, true))
        body.addView(label(
            "Leest alleen CameraCharacteristics. Geen camera-open, geen CaptureRequest, geen ImageReader, " +
                "geen Honor Binder en geen vendor write. Doel: de runtimewaarden achter Honors high-pixel routekeys " +
                "vastleggen op jouw Android 16 / SDK 36 toestel.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Lees v0.46 capability oracle") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "Gerichte keys: physicalCameraScene, sceneCameraIdCapability, cameraIdCustomInfo, " +
                "needOpenPhysicalCamera en ultraResolutionSwitchSupportedSize. De ruwe arrays worden opgeslagen; " +
                "groeperingen uit APK-control-flow worden apart als interpretatiehulp gemarkeerd.",
            11f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.46 report.", 10f, false)
        body.addView(status)

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(12, 14, 18))
            addView(
                body,
                ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
        }
    }

    private fun runOracle() {
        status.text = "v0.46 leest CameraCharacteristics…"
        saveButton.isEnabled = false

        Thread {
            val report = runCatching { buildReport() }
                .getOrElse { error ->
                    JSONObject()
                        .put("schema", SCHEMA)
                        .put("createdAtUtc", Instant.now().toString())
                        .put("authority", "CAMERA2_CHARACTERISTICS_OBSERVATION_ONLY")
                        .put("captureEvidenceGranted", false)
                        .put("calibrationAuthorityGranted", false)
                        .put("scientificMasterModified", false)
                        .put("cameraOpenedByTruthRaw", false)
                        .put("captureSubmittedByTruthRaw", false)
                        .put("vendorRequestWrittenByTruthRaw", false)
                        .put("errorClass", error.javaClass.name)
                        .put("errorMessage", error.message ?: JSONObject.NULL)
                }

            reportFile().writeText(report.toString(2))
            runOnUiThread { refreshStatus() }
        }.start()
    }

    private fun buildReport(): JSONObject {
        val cameraManager = getSystemService(CameraManager::class.java)
        val publicIds = cameraManager.cameraIdList.toList()
        val physicalIds = linkedSetOf<String>()

        for (id in publicIds) {
            runCatching {
                val c = cameraManager.getCameraCharacteristics(id)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    physicalIds += c.physicalCameraIds
                }
            }
        }

        val ids = linkedSetOf<String>().apply {
            addAll(publicIds)
            addAll(physicalIds)
        }

        val cameras = JSONArray()
        val teleCandidates = JSONArray()

        for (id in ids) {
            val entry = JSONObject()
                .put("cameraId", id)
                .put("publicCameraId", publicIds.contains(id))
                .put("disclosedPhysicalId", physicalIds.contains(id))

            runCatching {
                val c = cameraManager.getCameraCharacteristics(id)
                val standard = JSONObject()
                    .put("lensFacing", c.get(CameraCharacteristics.LENS_FACING) ?: JSONObject.NULL)
                    .put("focalLengthsMm", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)))
                    .put("apertures", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)))
                    .put("physicalCameraIds", if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) JSONArray(c.physicalCameraIds.toList()) else JSONArray())

                entry.put("standard", standard)

                val vendor = JSONObject()
                for (name in TARGET_VENDOR_KEYS) {
                    val key = c.keys.firstOrNull { it.name == name }
                    if (key == null) {
                        vendor.put(name, JSONObject()
                            .put("present", false)
                            .put("value", JSONObject.NULL))
                    } else {
                        @Suppress("UNCHECKED_CAST")
                        val value = c.get(key as CameraCharacteristics.Key<Any>)
                        val target = JSONObject()
                            .put("present", true)
                            .put("runtimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            .put("value", jsonValue(value))

                        val ints = value as? IntArray
                        when (name) {
                            KEY_PHYSICAL_CAMERA_SCENE -> {
                                target.put("containsUiScene53UltraHighPixel", ints?.contains(53) ?: false)
                                target.put("containsUiScene87LivePhotoHighPixel", ints?.contains(87) ?: false)
                                target.put("containsUiScene110UltraResolution", ints?.contains(110) ?: false)
                            }
                            KEY_SCENE_CAMERA_ID_CAPABILITY -> {
                                target.put("pairGroups", groupedInts(ints, 2))
                            }
                            KEY_CAMERA_ID_CUSTOM_INFO -> {
                                target.put("groupsOf10", groupedInts(ints, 10))
                            }
                            KEY_NEED_OPEN_PHYSICAL_CAMERA -> {
                                target.put("groupsOf4", groupedInts(ints, 4))
                            }
                            KEY_ULTRA_RES_SWITCH_SIZE -> {
                                target.put("sizePairs", sizePairs(ints))
                            }
                        }
                        vendor.put(name, target)
                    }
                }
                entry.put("targetVendorCharacteristics", vendor)

                val focals = c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
                if (focals != null && focals.any { abs(it - TELE_FOCAL_MM) <= TELE_FOCAL_TOLERANCE_MM }) {
                    teleCandidates.put(JSONObject()
                        .put("cameraId", id)
                        .put("focalLengthsMm", jsonValue(focals))
                        .put("matchBasis", "STANDARD_CAMERA2_CHARACTERISTICS_FOCAL_LENGTH_NEAR_22_48MM"))
                }
            }.onFailure { error ->
                entry.put("readError", JSONObject()
                    .put("class", error.javaClass.name)
                    .put("message", error.message ?: JSONObject.NULL))
            }

            cameras.put(entry)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_CHARACTERISTICS_OBSERVATION_ONLY")
            .put("captureEvidenceGranted", false)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("brand", Build.BRAND)
                .put("model", Build.MODEL)
                .put("device", Build.DEVICE)
                .put("product", Build.PRODUCT)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion)
                .put("interpretation", "DEVICE_RUNTIME_ANDROID_VERSION_IS_AUTHORITATIVE; APK_TARGET_SDK_IS_NOT_RUNTIME_VERSION"))
            .put("staticApkTargetSelection", JSONObject()
                .put("authority", "STATIC_APK_TARGET_SELECTION_ONLY")
                .put("sourceApkSha256", HONOR_CAMERA_APK_SHA256)
                .put("sourceApkLabel", "user-supplied Honor Camera beta APK")
                .put("uiSceneUltraHighPixel", 53)
                .put("uiSceneLivePhotoHighPixel", 87)
                .put("uiSceneUltraResolution", 110)
                .put("featureValue200MTargetMode", "UltraHighPixelMode")
                .put("featureValue50MTargetMode", "UltraResolutionMode")
                .put("processorInternalRawMfPipeline", "pipeline4rawmfultrahighpixelcap.json")
                .put("processorInternalRawMfSceneModes", JSONArray(listOf(23, 24, 32, 33)))
                .put("uiSceneAndProcessorSceneNamespacesMustNotBeEquated", true)
                .put("staticApkSemanticsAreNotCaptureEvidence", true))
            .put("targetVendorKeyNames", JSONArray(TARGET_VENDOR_KEYS))
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("teleCandidatesByStandardFocalLength", teleCandidates)
            .put("cameras", cameras)
            .put("classification", "READ_ONLY_VENDOR_CHARACTERISTIC_ROUTE_ORACLE__NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary", "RUNTIME_CHARACTERISTIC_VALUES_CAN_DESCRIBE_EXPOSED_CAPABILITY_STATE_BUT_DO_NOT_PROVE_OEM_CAPTURE_EXECUTION_OR_DIRECT_CFA_200MP")
    }

    private fun groupedInts(values: IntArray?, width: Int): JSONArray {
        val out = JSONArray()
        if (values == null || width <= 0) return out
        var i = 0
        while (i < values.size) {
            val group = JSONArray()
            for (j in 0 until width) {
                if (i + j < values.size) group.put(values[i + j])
            }
            out.put(group)
            i += width
        }
        return out
    }

    private fun sizePairs(values: IntArray?): JSONArray {
        val out = JSONArray()
        if (values == null) return out
        var i = 0
        while (i + 1 < values.size) {
            val w = values[i]
            val h = values[i + 1]
            out.put(JSONObject()
                .put("width", w)
                .put("height", h)
                .put("pixels", w.toLong() * h.toLong()))
            i += 2
        }
        return out
    }

    private fun jsonValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ByteArray -> JSONArray(value.map { it.toInt() })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.map { jsonValue(it) })
        is Collection<*> -> JSONArray(value.map { jsonValue(it) })
        is Size -> JSONObject().put("width", value.width).put("height", value.height)
        is android.graphics.Rect -> JSONObject()
            .put("left", value.left).put("top", value.top)
            .put("right", value.right).put("bottom", value.bottom)
        is Range<*> -> JSONObject().put("lower", value.lower.toString()).put("upper", value.upper.toString())
        is Rational -> value.toString()
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.46 report. Runtime verwacht: Android 16 / SDK 36; de app registreert dit opnieuw uit Build.VERSION."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.46 report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val device = report.optJSONObject("device")
        val cameras = report.optJSONArray("cameras")
        val tele = report.optJSONArray("teleCandidatesByStandardFocalLength")
        status.text = buildString {
            append("schema=").append(report.optString("schema", "?")).append('\n')
            append("runtime=").append(device?.optString("release", "?"))
                .append(" / SDK ").append(device?.optInt("sdkInt", -1)).append('\n')
            append("cameraEntries=").append(cameras?.length() ?: 0)
                .append(" · tele22.48Candidates=").append(tele?.length() ?: 0).append('\n')
            append("cameraOpenedByTruthRaw=").append(report.optBoolean("cameraOpenedByTruthRaw", true)).append('\n')
            append("captureSubmittedByTruthRaw=").append(report.optBoolean("captureSubmittedByTruthRaw", true)).append('\n')
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
            status.text = "v0.46 JSON opgeslagen · CameraCharacteristics-only · geen camera-open/capture."
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

    private fun label(
        text: String,
        size: Float,
        bold: Boolean,
        color: Int = Color.WHITE,
    ): TextView =
        TextView(this).apply {
            this.text = text
            textSize = size
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun space(height: Int): View =
        View(this).apply {
            layoutParams = LinearLayout.LayoutParams(1, dp(height))
        }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val REQUEST_SAVE_JSON = 64646
        private const val REPORT_FILENAME = "TRUTHRAW_HONOR_CAPABILITY_ROUTE_ORACLE_v046.json"
        private const val SCHEMA = "truthraw.honor-capability-route-oracle.v0.46"
        private const val HONOR_CAMERA_APK_SHA256 = "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"
        private const val TELE_FOCAL_MM = 22.48f
        private const val TELE_FOCAL_TOLERANCE_MM = 0.05f

        private const val KEY_PHYSICAL_CAMERA_SCENE =
            "com.hihonor.device.capabilities.physicalCameraScene"
        private const val KEY_SCENE_CAMERA_ID_CAPABILITY =
            "com.hihonor.device.capabilities.sceneCameraIdCapability"
        private const val KEY_CAMERA_ID_CUSTOM_INFO =
            "com.hihonor.device.capabilities.cameraIdCustomInfo"
        private const val KEY_NEED_OPEN_PHYSICAL_CAMERA =
            "com.hihonor.device.capabilities.needOpenPhysicalCamera"
        private const val KEY_ULTRA_RES_SWITCH_SIZE =
            "com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize"

        private val TARGET_VENDOR_KEYS = listOf(
            KEY_PHYSICAL_CAMERA_SCENE,
            KEY_SCENE_CAMERA_ID_CAPABILITY,
            KEY_CAMERA_ID_CUSTOM_INFO,
            KEY_NEED_OPEN_PHYSICAL_CAMERA,
            KEY_ULTRA_RES_SWITCH_SIZE,
        )
    }
}

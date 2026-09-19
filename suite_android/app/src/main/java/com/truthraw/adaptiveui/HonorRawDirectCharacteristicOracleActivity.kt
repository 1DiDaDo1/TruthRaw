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

/**
 * v0.50 read-only direct Honor vendor-characteristic oracle.
 *
 * Static analysis of the exact .452 and .706 Honor Camera APKs reconstructed
 * specific vendor characteristic names plus their Java/native value types.
 *
 * v0.46 only checked whether selected names were present in CameraCharacteristics.keys.
 * v0.50 tests the narrower hypothesis that an exact name/type key may still be directly
 * readable with CameraCharacteristics.get(key) even when omitted from the enumerable list.
 *
 * No camera open, CaptureRequest, ImageReader, Honor Binder or vendor write occurs.
 */
class HonorRawDirectCharacteristicOracleActivity : Activity() {
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

        body.addView(label("TruthRaw v0.50 · Honor RAW direct characteristic oracle", 22f, true))
        body.addView(label(
            "Construeert alleen exact bekende Honor CameraCharacteristics-keys met de statisch afgeleide types " +
                "en leest ze rechtstreeks. Geen camera-open, CaptureRequest, ImageReader, Binder of vendor write.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Lees directe Honor RAW characteristics") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "De test houdt apart bij: zichtbaar in characteristics.keys, key-constructie, directe get(), null/waarde, " +
                "exception en runtime type. Een waarde is vendor-capability evidence, geen capture- of sensorbewijs.",
            11f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.50 report.", 10f, false)
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
        status.text = "v0.50 leest characteristics…"
        saveButton.isEnabled = false

        Thread {
            val report = runCatching { buildReport() }.getOrElse { error ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", "CAMERA2_VENDOR_CHARACTERISTIC_OBSERVATION_ONLY")
                    .put("captureEvidenceGranted", false)
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
        val cm = getSystemService(CameraManager::class.java)
        val publicIds = cm.cameraIdList.toList()
        val physicalIds = linkedSetOf<String>()

        for (id in publicIds) {
            runCatching {
                val c = cm.getCameraCharacteristics(id)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    physicalIds += c.physicalCameraIds
                }
            }
        }

        val allIds = linkedSetOf<String>().apply {
            addAll(publicIds)
            addAll(physicalIds)
        }

        val cameras = JSONArray()
        for (id in allIds) {
            val out = JSONObject()
                .put("cameraId", id)
                .put("publicCameraId", publicIds.contains(id))
                .put("disclosedPhysicalId", physicalIds.contains(id))

            runCatching {
                val c = cm.getCameraCharacteristics(id)
                val visibleNames = c.keys.map { it.name }.toSet()

                out.put("standard", JSONObject()
                    .put("focalLengthsMm", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)))
                    .put("apertures", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)))
                    .put("physicalCameraIds", JSONArray(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) c.physicalCameraIds.toList() else emptyList()))
                    .put("enumerableCharacteristicKeyCount", visibleNames.size))

                val results = JSONArray()
                for (spec in TARGET_KEYS) {
                    results.put(readDirect(c, visibleNames, spec))
                }
                out.put("directVendorCharacteristicReads", results)
            }.onFailure { error ->
                out.put("cameraReadError", JSONObject()
                    .put("class", error.javaClass.name)
                    .put("message", error.message ?: JSONObject.NULL))
            }

            cameras.put(out)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_VENDOR_CHARACTERISTIC_OBSERVATION_ONLY")
            .put("captureEvidenceGranted", false)
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("cameraOpenedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("extensionSessionCreatedByTruthRaw", false)
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
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("staticApkBasis", JSONObject()
                .put("authority", "STATIC_APK_TARGET_SELECTION_ONLY")
                .put("android16HonorCameraVersion", "171.0.10.452")
                .put("android16HonorCameraSha256", "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52")
                .put("android17HonorCameraVersion", "171.0.10.706")
                .put("android17HonorCameraSha256", "bbc6312e0289d01a51dbe45efc519e56715e6250521227df81cf64a633b8f027")
                .put("rawNamedCapabilitySetChangedBetweenApks", false)
                .put("directGetHypothesis", "EXACT_NAME_AND_TYPE_MAY_BE_GETTABLE_EVEN_IF_OMITTED_FROM_ENUMERABLE_CHARACTERISTICS_KEYS"))
            .put("targetKeySpecs", JSONArray(TARGET_KEYS.map { it.toJson() }))
            .put("systemPropertyProbe", readRawSaveSystemProperty())
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameras", cameras)
            .put("classification", "DIRECT_HONOR_VENDOR_CHARACTERISTIC_GET_ORACLE__NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary", "DIRECT_VENDOR_CHARACTERISTIC_VALUES_ARE_CAPABILITY_METADATA_ONLY__NEVER_CAPTURE_DIRECT_CFA_SENSOR_TOPOLOGY_OR_CALIBRATION_EVIDENCE")
    }

    private enum class ValueType {
        BYTE,
        INT32,
        INT_ARRAY,
    }

    private data class KeySpec(
        val name: String,
        val valueType: ValueType,
        val family: String,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("name", name)
            .put("valueType", valueType.name)
            .put("family", family)
    }

    @Suppress("UNCHECKED_CAST")
    private fun keyFor(spec: KeySpec): CameraCharacteristics.Key<Any> {
        val clazz: Class<*> = when (spec.valueType) {
            ValueType.BYTE -> Byte::class.javaPrimitiveType!!
            ValueType.INT32 -> Int::class.javaPrimitiveType!!
            ValueType.INT_ARRAY -> IntArray::class.java
        }
        return CameraCharacteristics.Key(spec.name, clazz as Class<Any>)
    }

    private fun readDirect(
        c: CameraCharacteristics,
        visibleNames: Set<String>,
        spec: KeySpec,
    ): JSONObject {
        val out = JSONObject()
            .put("name", spec.name)
            .put("valueType", spec.valueType.name)
            .put("family", spec.family)
            .put("enumeratedInCharacteristicsKeys", visibleNames.contains(spec.name))

        val key = runCatching { keyFor(spec) }
            .onFailure { e ->
                out.put("keyConstructed", false)
                out.put("keyConstructionError", "${e.javaClass.name}: ${e.message}")
            }
            .getOrNull()

        if (key == null) return out
        out.put("keyConstructed", true)

        runCatching { c.get(key) }
            .onSuccess { value ->
                out.put("directGetCompleted", true)
                out.put("valueIsNull", value == null)
                out.put("runtimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                out.put("value", jsonValue(value))

                if (value is IntArray) {
                    when (spec.name) {
                        KEY_HW_RAW_STREAM_CONFIGS -> out.put("interpretedGroupsOf4", groupedInts(value, 4))
                        KEY_RAW_SENSOR_RESOLUTION,
                        KEY_RAW_CAPTURE_SIZE,
                        KEY_ULTRA_RES_SWITCH_SIZE -> out.put("interpretedSizePairs", sizePairs(value))
                        KEY_SCENE_CAMERA_ID_CAPABILITY -> out.put("interpretedPairs", groupedInts(value, 2))
                        KEY_CAMERA_ID_CUSTOM_INFO -> out.put("interpretedGroupsOf10", groupedInts(value, 10))
                        KEY_NEED_OPEN_PHYSICAL_CAMERA -> out.put("interpretedGroupsOf4", groupedInts(value, 4))
                    }
                }
            }
            .onFailure { e ->
                out.put("directGetCompleted", false)
                out.put("directGetErrorClass", e.javaClass.name)
                out.put("directGetErrorMessage", e.message ?: JSONObject.NULL)
            }

        return out
    }

    private fun readRawSaveSystemProperty(): JSONObject {
        val out = JSONObject()
            .put("propertyName", RAW_SAVE_PROPERTY)
            .put("authority", "READ_ONLY_SYSTEM_PROPERTY_OBSERVATION_IF_PLATFORM_ALLOWS")
            .put("hiddenApiBypassAttempted", false)

        runCatching {
            val clazz = Class.forName("android.os.SystemProperties")
            val get = clazz.getMethod("get", String::class.java, String::class.java)
            get.invoke(null, RAW_SAVE_PROPERTY, "__TRUTHRAW_ABSENT__") as String
        }.onSuccess { value ->
            out.put("reflectionSucceeded", true)
            out.put("value", value)
            out.put("wasDefaultSentinel", value == "__TRUTHRAW_ABSENT__")
        }.onFailure { e ->
            out.put("reflectionSucceeded", false)
            out.put("errorClass", e.javaClass.name)
            out.put("errorMessage", e.cause?.message ?: e.message ?: JSONObject.NULL)
        }

        return out
    }

    private fun groupedInts(values: IntArray, width: Int): JSONArray {
        val out = JSONArray()
        if (width <= 0) return out
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

    private fun sizePairs(values: IntArray): JSONArray {
        val out = JSONArray()
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
            status.text = "Nog geen v0.50 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.50 report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        var nonNull = 0
        var exceptions = 0
        val cameras = report.optJSONArray("cameras")
        if (cameras != null) {
            for (i in 0 until cameras.length()) {
                val reads = cameras.getJSONObject(i).optJSONArray("directVendorCharacteristicReads") ?: continue
                for (j in 0 until reads.length()) {
                    val r = reads.getJSONObject(j)
                    if (r.optBoolean("directGetCompleted", false) && !r.optBoolean("valueIsNull", true)) nonNull++
                    if (r.has("directGetErrorClass")) exceptions++
                }
            }
        }

        status.text = buildString {
            append("runtime=").append(report.optJSONObject("device")?.optString("release", "?"))
                .append(" / SDK ").append(report.optJSONObject("device")?.optInt("sdkInt", -1)).append('\n')
            append("nonNullDirectValues=").append(nonNull).append('\n')
            append("directGetExceptions=").append(exceptions).append('\n')
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
            status.text = "v0.50 JSON opgeslagen · direct characteristics-only · geen capture."
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
        private const val REQUEST_SAVE_JSON = 65050
        private const val REPORT_FILENAME = "TRUTHRAW_HONOR_RAW_DIRECT_CHARACTERISTIC_ORACLE_v050.json"
        private const val SCHEMA = "truthraw.honor-raw-direct-characteristic-oracle.v0.50"

        private const val RAW_SAVE_PROPERTY = "msc.camera.rawphoto.save"

        private const val KEY_RAW_IMG_SUPPORTED =
            "com.hihonor.device.capabilities.rawImgSupported"
        private const val KEY_HW_RAW_STREAM_CONFIGS =
            "com.hihonor.device.capabilities.hwCaptureRawStreamConfigurations"
        private const val KEY_HW_PRO_RAW_CAPTURE_MODE =
            "com.hihonor.device.capabilities.hwProfessionalRawCaptureMode"
        private const val KEY_TELE_RAW_LOGICAL_ID =
            "com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID"
        private const val KEY_RAW_CAPTURE_SIZE =
            "com.hihonor.device.capabilities.rawCaptureSize"
        private const val KEY_RAW_BOKEH =
            "com.hihonor.device.capabilities.rawForBokehSupported"
        private const val KEY_RAW_SENSOR_RESOLUTION =
            "com.hihonor.device.capabilities.rawSensorResolution"
        private const val KEY_RAW_ZOOM =
            "com.hihonor.device.capabilities.rawZoomSupported"
        private const val KEY_OFFLINE_RAW_SCENE =
            "com.hihonor.device.capabilities.supportOfflineRawSceneMode"

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

        private val TARGET_KEYS = listOf(
            KeySpec(KEY_RAW_IMG_SUPPORTED, ValueType.BYTE, "RAW"),
            KeySpec(KEY_HW_RAW_STREAM_CONFIGS, ValueType.INT_ARRAY, "RAW"),
            KeySpec(KEY_HW_PRO_RAW_CAPTURE_MODE, ValueType.BYTE, "RAW"),
            KeySpec(KEY_TELE_RAW_LOGICAL_ID, ValueType.INT32, "RAW"),
            KeySpec(KEY_RAW_CAPTURE_SIZE, ValueType.INT_ARRAY, "RAW"),
            KeySpec(KEY_RAW_BOKEH, ValueType.BYTE, "RAW"),
            KeySpec(KEY_RAW_SENSOR_RESOLUTION, ValueType.INT_ARRAY, "RAW"),
            KeySpec(KEY_RAW_ZOOM, ValueType.BYTE, "RAW"),
            KeySpec(KEY_OFFLINE_RAW_SCENE, ValueType.INT_ARRAY, "RAW"),
            KeySpec(KEY_PHYSICAL_CAMERA_SCENE, ValueType.INT_ARRAY, "ROUTING"),
            KeySpec(KEY_SCENE_CAMERA_ID_CAPABILITY, ValueType.INT_ARRAY, "ROUTING"),
            KeySpec(KEY_CAMERA_ID_CUSTOM_INFO, ValueType.INT_ARRAY, "ROUTING"),
            KeySpec(KEY_NEED_OPEN_PHYSICAL_CAMERA, ValueType.INT_ARRAY, "ROUTING"),
            KeySpec(KEY_ULTRA_RES_SWITCH_SIZE, ValueType.INT_ARRAY, "ROUTING"),
        )
    }
}

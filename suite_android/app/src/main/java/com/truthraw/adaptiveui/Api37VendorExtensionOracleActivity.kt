package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraExtensionCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.os.Build
import android.os.Bundle
import android.util.Size
import android.view.SurfaceView
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
import java.lang.reflect.InvocationTargetException
import java.time.Instant

/**
 * v0.62 Android-17/API-37 vendor-defined Camera Extension oracle.
 *
 * Read-only by contract:
 * - no CameraDevice open
 * - no CameraExtensionSession
 * - no CaptureRequest submission
 * - no ImageReader / pixel access
 * - no vendor request writes
 *
 * compileSdk remains 35. API-37 isExtensionSupported(int) is invoked reflectively on Android 17.
 * This lets TruthRaw query device-specific extension IDs that are not required to appear in
 * getSupportedExtensions().
 */
class Api37VendorExtensionOracleActivity : Activity() {
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

        body.addView(label("TruthRaw v0.62 · Android 17 vendor extensions", 21f, true))
        body.addView(label(
            "Read-only API-37 oracle. Vraagt known + device-specific extension IDs op zonder camera-open, " +
                "extension session of capture. Gevonden modes worden daarna op keys en outputmaten geïnventariseerd.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))
        body.addView(button("1 · Scan Android 17 extensions") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Scanbereik extension ID 0..255. Een positieve ID is capability-evidence; geen bewijs van RAW, 200MP, remosaic of OEM shutter-route.",
            11f, false, Color.rgb(155, 165, 180),
        ))
        body.addView(space(10))
        status = label("Nog geen v0.62 report.", 10f, false)
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
        status.text = "v0.62 scant extension IDs read-only…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { error ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("classification", "API37_VENDOR_EXTENSION_ORACLE_FATAL_ERROR")
                    .put("errorClass", error.javaClass.name)
                    .put("errorMessage", error.message ?: JSONObject.NULL)
                    .put("cameraOpenedByTruthRaw", false)
                    .put("extensionSessionCreatedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("vendorRequestWrittenByTruthRaw", false)
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
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    physicalIds += cm.getCameraCharacteristics(id).physicalCameraIds
                }
            }
        }

        val allIds = linkedSetOf<String>().apply {
            addAll(publicIds)
            addAll(physicalIds)
        }

        val cameras = JSONArray()
        for (id in allIds) {
            val camera = JSONObject()
                .put("cameraId", id)
                .put("publicCameraId", id in publicIds)
                .put("disclosedPhysicalId", id in physicalIds)

            runCatching {
                val chars = cm.getCameraCharacteristics(id)
                camera.put("focalLengthsMm", jsonValue(chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)))
                camera.put("apertures", jsonValue(chars.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)))
                camera.put("physicalCameraIds", JSONArray(
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) chars.physicalCameraIds.toList()
                    else emptyList()
                ))
                camera.put("extensions", inspectCameraExtensions(cm, id))
            }.onFailure { error ->
                camera.put("readError", errorJson(error))
            }

            cameras.put(camera)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("runtime", JSONObject()
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("manufacturer", Build.MANUFACTURER)
                .put("brand", Build.BRAND)
                .put("model", Build.MODEL)
                .put("device", Build.DEVICE)
                .put("product", Build.PRODUCT)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawCompileSdk", 35)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("scanRange", JSONObject()
                .put("minExtensionId", SCAN_MIN)
                .put("maxExtensionId", SCAN_MAX)
                .put("count", SCAN_MAX - SCAN_MIN + 1)
                .put("boundedScan", true))
            .put("knownPublicIds", knownPublicIdsJson())
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameras", cameras)
            .put("cameraOpenedByTruthRaw", false)
            .put("extensionSessionCreatedByTruthRaw", false)
            .put("captureSubmittedByTruthRaw", false)
            .put("imageBufferAccessedByTruthRaw", false)
            .put("vendorRequestWrittenByTruthRaw", false)
            .put("honorBinderInvokedByTruthRaw", false)
            .put("scientificMasterModified", false)
            .put("calibrationAuthorityGranted", false)
            .put("physicalFrameCount", 0)
            .put("independentEvidenceCount", 0)
            .put("classification", "API37_VENDOR_DEFINED_EXTENSION_CAPABILITY_ORACLE__NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary", BOUNDARY)
    }

    private fun inspectCameraExtensions(cm: CameraManager, cameraId: String): JSONObject {
        val ext = cm.getCameraExtensionCharacteristics(cameraId)
        val out = JSONObject()
            .put("cameraId", cameraId)
            .put("queriedWithoutCreatingExtensionSession", true)

        val legacyList = runCatching { ext.supportedExtensions.toList() }
        legacyList.onSuccess { out.put("getSupportedExtensions", JSONArray(it)) }
            .onFailure { out.put("getSupportedExtensionsError", errorJson(it)) }

        val supportMethod = runCatching {
            ext.javaClass.getMethod("isExtensionSupported", Integer.TYPE)
        }.getOrNull()

        out.put("api37IsExtensionSupportedMethodPresent", supportMethod != null)
        if (supportMethod == null) {
            return out
                .put("scanPerformed", false)
                .put("classification", "API37_IS_EXTENSION_SUPPORTED_METHOD_NOT_PRESENT")
        }

        val checks = JSONArray()
        val supportedIds = mutableListOf<Int>()
        var falseCount = 0
        var errorCount = 0

        for (extensionId in SCAN_MIN..SCAN_MAX) {
            val one = JSONObject()
                .put("extensionId", extensionId)
                .put("knownPublicName", publicName(extensionId) ?: JSONObject.NULL)
                .put("listedByGetSupportedExtensions",
                    legacyList.getOrNull()?.contains(extensionId) == true)

            val supported = runCatching {
                supportMethod.invoke(ext, extensionId) as Boolean
            }.fold(
                onSuccess = { value ->
                    one.put("queryCompleted", true)
                    one.put("supported", value)
                    if (value) supportedIds += extensionId else falseCount++
                    value
                },
                onFailure = { error ->
                    one.put("queryCompleted", false)
                    one.put("supported", JSONObject.NULL)
                    one.put("error", errorJson(unwrapReflection(error)))
                    errorCount++
                    false
                },
            )

            if (supported) {
                one.put("details", describeSupportedExtension(ext, extensionId))
            }
            checks.put(one)
        }

        val vendorSpecific = supportedIds.filter { it !in KNOWN_PUBLIC_IDS }

        return out
            .put("scanPerformed", true)
            .put("checks", checks)
            .put("supportedIds", JSONArray(supportedIds))
            .put("vendorSpecificSupportedIds", JSONArray(vendorSpecific))
            .put("supportedCount", supportedIds.size)
            .put("vendorSpecificSupportedCount", vendorSpecific.size)
            .put("falseCount", falseCount)
            .put("errorCount", errorCount)
            .put("classification",
                if (vendorSpecific.isEmpty())
                    "NO_VENDOR_SPECIFIC_EXTENSION_ID_FOUND_IN_BOUNDED_0_255_SCAN"
                else
                    "VENDOR_SPECIFIC_EXTENSION_ID_FOUND_IN_BOUNDED_0_255_SCAN")
    }

    private fun describeSupportedExtension(
        ext: CameraExtensionCharacteristics,
        extensionId: Int,
    ): JSONObject {
        val out = JSONObject()
            .put("extensionId", extensionId)
            .put("knownPublicName", publicName(extensionId) ?: JSONObject.NULL)
            .put("deviceSpecificById", extensionId !in KNOWN_PUBLIC_IDS)

        runCatching { ext.getKeys(extensionId).toList() }
            .onSuccess { keys ->
                val arr = JSONArray()
                for (key in keys.sortedBy { it.name }) {
                    val item = JSONObject().put("name", key.name)
                    runCatching { getExtensionCharacteristicValue(ext, extensionId, key) }
                        .onSuccess { value ->
                            item.put("valueReadCompleted", true)
                            item.put("runtimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            item.put("value", jsonValue(value))
                        }
                        .onFailure { error ->
                            item.put("valueReadCompleted", false)
                            item.put("valueReadError", errorJson(error))
                        }
                    arr.put(item)
                }
                out.put("extensionCharacteristicKeys", arr)
                out.put("extensionCharacteristicKeyCount", keys.size)
            }
            .onFailure { out.put("extensionCharacteristicKeysError", errorJson(it)) }

        runCatching { ext.getAvailableCaptureRequestKeys(extensionId).toList() }
            .onSuccess { keys ->
                out.put("captureRequestKeys", JSONArray(keys.map { it.name }.sorted()))
                out.put("captureRequestKeyCount", keys.size)
            }
            .onFailure { out.put("captureRequestKeysError", errorJson(it)) }

        runCatching { ext.getAvailableCaptureResultKeys(extensionId).toList() }
            .onSuccess { keys ->
                out.put("captureResultKeys", JSONArray(keys.map { it.name }.sorted()))
                out.put("captureResultKeyCount", keys.size)
            }
            .onFailure { out.put("captureResultKeysError", errorJson(it)) }

        val formatSizes = JSONObject()
        for (spec in CAPTURE_FORMATS) {
            val item = JSONObject()
                .put("formatName", spec.name)
                .put("formatValue", spec.value)
            runCatching { ext.getExtensionSupportedSizes(extensionId, spec.value).toList() }
                .onSuccess { sizes ->
                    item.put("sizes", sizeArray(sizes))
                    item.put("sizeCount", sizes.size)
                    item.put("contains8160x6144", sizes.any { it.width == 8160 && it.height == 6144 })
                    item.put("contains16320x12288", sizes.any { it.width == 16320 && it.height == 12288 })
                }
                .onFailure { item.put("error", errorJson(it)) }
            formatSizes.put(spec.name, item)
        }
        out.put("captureFormats", formatSizes)

        runCatching { ext.getExtensionSupportedSizes(extensionId, SurfaceTexture::class.java).toList() }
            .onSuccess { out.put("surfaceTextureSizes", sizeArray(it)).put("surfaceTextureSizeCount", it.size) }
            .onFailure { out.put("surfaceTextureSizesError", errorJson(it)) }

        runCatching { ext.getExtensionSupportedSizes(extensionId, SurfaceView::class.java).toList() }
            .onSuccess { out.put("surfaceViewSizes", sizeArray(it)).put("surfaceViewSizeCount", it.size) }
            .onFailure { out.put("surfaceViewSizesError", errorJson(it)) }

        runCatching { ext.isPostviewAvailable(extensionId) }
            .onSuccess { out.put("postviewAvailable", it) }
            .onFailure { out.put("postviewAvailableError", errorJson(it)) }

        runCatching { ext.isCaptureProcessProgressAvailable(extensionId) }
            .onSuccess { out.put("captureProcessProgressAvailable", it) }
            .onFailure { out.put("captureProcessProgressAvailableError", errorJson(it)) }

        return out
    }

    @Suppress("UNCHECKED_CAST")
    private fun getExtensionCharacteristicValue(
        ext: CameraExtensionCharacteristics,
        extensionId: Int,
        key: CameraCharacteristics.Key<*>,
    ): Any? = ext.get(extensionId, key as CameraCharacteristics.Key<Any>)

    private fun knownPublicIdsJson(): JSONArray = JSONArray().apply {
        for (id in KNOWN_PUBLIC_IDS) {
            put(JSONObject().put("extensionId", id).put("name", publicName(id)))
        }
    }

    private fun publicName(id: Int): String? = when (id) {
        CameraExtensionCharacteristics.EXTENSION_AUTOMATIC -> "AUTOMATIC"
        CameraExtensionCharacteristics.EXTENSION_FACE_RETOUCH -> "FACE_RETOUCH"
        CameraExtensionCharacteristics.EXTENSION_BOKEH -> "BOKEH"
        CameraExtensionCharacteristics.EXTENSION_HDR -> "HDR"
        CameraExtensionCharacteristics.EXTENSION_NIGHT -> "NIGHT"
        else -> null
    }

    private fun unwrapReflection(error: Throwable): Throwable =
        if (error is InvocationTargetException && error.cause != null) error.cause!! else error

    private fun sizeArray(sizes: List<Size>): JSONArray = JSONArray().apply {
        for (s in sizes.sortedWith(compareBy<Size>(
            { it.width.toLong() * it.height.toLong() },
            { it.width },
            { it.height },
        ))) {
            put(JSONObject()
                .put("width", s.width)
                .put("height", s.height)
                .put("pixels", s.width.toLong() * s.height.toLong()))
        }
    }

    private fun errorJson(error: Throwable): JSONObject = JSONObject()
        .put("class", error.javaClass.name)
        .put("message", error.message ?: JSONObject.NULL)
        .put("causeClass", error.cause?.javaClass?.name ?: JSONObject.NULL)
        .put("causeMessage", error.cause?.message ?: JSONObject.NULL)

    private fun jsonValue(value: Any?): Any = when (value) {
        null -> JSONObject.NULL
        is IntArray -> JSONArray(value.toList())
        is LongArray -> JSONArray(value.toList())
        is FloatArray -> JSONArray(value.map { it.toDouble() })
        is DoubleArray -> JSONArray(value.toList())
        is ByteArray -> JSONArray(value.map { it.toInt() and 0xff })
        is ShortArray -> JSONArray(value.map { it.toInt() })
        is BooleanArray -> JSONArray(value.toList())
        is Array<*> -> JSONArray(value.map { jsonValue(it) })
        is Collection<*> -> JSONArray(value.map { jsonValue(it) })
        is Size -> JSONObject().put("width", value.width).put("height", value.height)
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.62 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.62 report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        var supported = 0
        var vendor = 0
        val cams = report.optJSONArray("cameras")
        if (cams != null) {
            for (i in 0 until cams.length()) {
                val e = cams.optJSONObject(i)?.optJSONObject("extensions") ?: continue
                supported += e.optInt("supportedCount", 0)
                vendor += e.optInt("vendorSpecificSupportedCount", 0)
            }
        }

        status.text = buildString {
            append("runtime SDK=").append(report.optJSONObject("runtime")?.optInt("sdkInt", -1)).append('\n')
            append("cameraEntries=").append(cams?.length() ?: 0).append('\n')
            append("supported extension hits=").append(supported).append('\n')
            append("vendor-specific hits=").append(vendor).append('\n')
            append("cameraOpened=false · extensionSession=false · capture=false").append('\n')
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
            status.text = "v0.62 JSON opgeslagen · extension capability-only."
        }.onFailure {
            status.text = "Opslaan faalde: " + it.javaClass.simpleName + ": " + it.message
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

    private data class FormatSpec(val name: String, val value: Int)

    companion object {
        private const val SCHEMA = "truthraw.api37-vendor-extension-oracle.v0.62"
        private const val AUTHORITY = "CAMERA2_EXTENSION_CHARACTERISTICS_READ_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_API37_VENDOR_EXTENSION_ORACLE_v062.json"
        private const val REQUEST_SAVE_JSON = 66262
        private const val SCAN_MIN = 0
        private const val SCAN_MAX = 255

        private val KNOWN_PUBLIC_IDS = setOf(
            CameraExtensionCharacteristics.EXTENSION_AUTOMATIC,
            CameraExtensionCharacteristics.EXTENSION_FACE_RETOUCH,
            CameraExtensionCharacteristics.EXTENSION_BOKEH,
            CameraExtensionCharacteristics.EXTENSION_HDR,
            CameraExtensionCharacteristics.EXTENSION_NIGHT,
        )

        private val CAPTURE_FORMATS = listOf(
            FormatSpec("JPEG", ImageFormat.JPEG),
            FormatSpec("YUV_420_888", ImageFormat.YUV_420_888),
            FormatSpec("JPEG_R", ImageFormat.JPEG_R),
            FormatSpec("YCBCR_P010", ImageFormat.YCBCR_P010),
            FormatSpec("DEPTH_JPEG", ImageFormat.DEPTH_JPEG),
        )

        private const val BOUNDARY =
            "EXTENSION_SUPPORT_AND_ADVERTISED_OUTPUTS_ARE_CAPABILITY_EVIDENCE_ONLY; THEY_DO_NOT_PROVE_RAW_ACCESS, OEM_SHUTTER_ROUTE, NATIVE_ADC_TOPOLOGY, DIRECT_CFA_200MP, REMOSAIC_IDENTITY, OR_CALIBRATION_TRUTH"
    }
}

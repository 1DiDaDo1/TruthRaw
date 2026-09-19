package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.params.StreamConfigurationMap
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
import java.io.FileInputStream
import java.security.MessageDigest
import java.time.Instant

/**
 * v0.55 current Android-17 + installed HONOR Camera baseline.
 *
 * Read-only baseline only:
 * - no CameraDevice open
 * - no CaptureRequest
 * - no ImageReader
 * - no image/pixel access
 * - no Honor Binder
 * - no vendor request writes
 * - no extension session
 *
 * The report deliberately fingerprints the current Android build, the actually
 * installed com.hihonor.camera package (including APK/split hashes where readable),
 * and the public Camera2/RAW capability surface. It grants no capture or
 * calibration authority.
 */
class CurrentAndroid17HonorCameraBaselineActivity : Activity() {
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
        refreshStatus()
    }

    private fun buildUi(): View {
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(12), dp(16), dp(20))
            setBackgroundColor(Color.rgb(12, 14, 18))
        }

        body.addView(label("TruthRaw v0.55 · huidige Android 17 + HONOR Camera baseline", 22f, true))
        body.addView(label(
            "Nieuwe read-only nulmeting na v0.53/v0.54. Leest de huidige Android-build, " +
                "de werkelijk geïnstalleerde HONOR Camera package-identiteit en Camera2/RAW-capabilities. " +
                "Opent geen camera en maakt geen capture.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))
        body.addView(button("1 · Maak huidige baseline") { runBaseline() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)
        body.addView(space(10))
        body.addView(label(
            "Doel: exact vastleggen wat er NU op Android 17 is voordat we opnieuw een actieve Camera-5-route " +
                "of OEM Pro RAW-test uitvoeren. Verschillen met v0.47/v0.48/v0.53/v0.54 blijven observaties " +
                "tot een gecontroleerde vervolgmeting ze verklaart.",
            11f, false, Color.rgb(155, 165, 180),
        ))
        body.addView(space(10))
        status = label("Nog geen v0.55 baseline.", 10f, false)
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

    private fun runBaseline() {
        status.text = "v0.55 leest huidige Android/HONOR/Camera2-baseline…"
        saveButton.isEnabled = false
        Thread {
            val report = runCatching { buildReport() }.getOrElse { error ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("cameraOpenedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("imageBufferAccessedByTruthRaw", false)
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

        val raw14Runtime = resolveRaw14Runtime()
        val formats = mutableListOf(
            FormatSpec("RAW_SENSOR", ImageFormat.RAW_SENSOR),
            FormatSpec("RAW10", ImageFormat.RAW10),
            FormatSpec("RAW12", ImageFormat.RAW12),
        )
        raw14Runtime.value?.let { formats += FormatSpec("RAW14", it) }

        val cameras = JSONArray()
        for (id in allIds) {
            val out = JSONObject()
                .put("cameraId", id)
                .put("publicCameraId", publicIds.contains(id))
                .put("disclosedPhysicalId", physicalIds.contains(id))

            runCatching {
                val c = cm.getCameraCharacteristics(id)
                out.put("standard", JSONObject()
                    .put("focalLengthsMm", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)))
                    .put("apertures", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)))
                    .put("capabilities", jsonValue(c.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)))
                    .put("physicalCameraIds", JSONArray(if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) c.physicalCameraIds.toList() else emptyList()))
                    .put("sensorPixelModeRequestKeyPresent", c.availableCaptureRequestKeys.any { it.name == "android.sensor.pixelMode" })
                    .put("pixelArraySize", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE)))
                    .put("activeArraySize", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE)))
                    .put("preCorrectionActiveArraySize", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE)))
                    .put("pixelArraySizeMaximumResolution", jsonValue(runCatching { c.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE_MAXIMUM_RESOLUTION) }.getOrNull()))
                    .put("activeArraySizeMaximumResolution", jsonValue(runCatching { c.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION) }.getOrNull()))
                    .put("preCorrectionActiveArraySizeMaximumResolution", jsonValue(runCatching { c.get(CameraCharacteristics.SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE_MAXIMUM_RESOLUTION) }.getOrNull()))
                    .put("binningFactor", jsonValue(runCatching { c.get(CameraCharacteristics.SENSOR_INFO_BINNING_FACTOR) }.getOrNull()))
                    .put("colorFilterArrangement", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT)))
                    .put("whiteLevel", jsonValue(c.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL)))
                    .put("blackLevelPattern", jsonValue(c.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN)))
                )

                val defaultMap = c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
                val maxMap = runCatching {
                    c.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP_MAXIMUM_RESOLUTION)
                }.getOrNull()

                out.put("defaultStreamMap", describeMap(defaultMap, formats))
                out.put("maximumResolutionStreamMap", describeMap(maxMap, formats))
                out.put("publicSurface", JSONObject()
                    .put("characteristicKeyNames", sortedNames(c.keys.map { it.name }))
                    .put("captureRequestKeyNames", sortedNames(c.availableCaptureRequestKeys.map { it.name }))
                    .put("captureResultKeyNames", sortedNames(c.availableCaptureResultKeys.map { it.name }))
                    .put("vendorKeyNamesAreSemanticAuthority", false))
                out.put("extensions", describeExtensions(cm, id))
            }.onFailure { e ->
                out.put("readError", JSONObject()
                    .put("class", e.javaClass.name)
                    .put("message", e.message ?: JSONObject.NULL))
            }

            cameras.put(out)
        }

        val ownInfo = packageManager.getPackageInfo(packageName, 0)
        val ownApp = ownInfo.applicationInfo

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
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
            .put("baselinePurpose", "CURRENT_ANDROID17_HONOR_CAMERA_AND_CAMERA2_CAPABILITY_ZERO_MEASUREMENT")
            .put("comparisonAnchors", JSONArray(listOf(
                "v0.47 Android17/API37 RAW14 + extension oracle",
                "v0.48 targetSdk37 retarget control",
                "v0.53 Android17 replay of v0.14 Camera-5 route",
                "v0.54 HONOR Pro RAW/DNG exported-container fingerprint",
            )))
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("brand", Build.BRAND)
                .put("model", Build.MODEL)
                .put("device", Build.DEVICE)
                .put("product", Build.PRODUCT)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("incremental", Build.VERSION.INCREMENTAL)
                .put("securityPatch", Build.VERSION.SECURITY_PATCH)
                .put("buildId", Build.ID)
                .put("display", Build.DISPLAY)
                .put("fingerprint", Build.FINGERPRINT)
                .put("buildType", Build.TYPE)
                .put("buildTags", Build.TAGS)
                .put("buildTimeMs", Build.TIME))
            .put("truthRawApp", JSONObject()
                .put("packageName", packageName)
                .put("versionName", ownInfo.versionName ?: JSONObject.NULL)
                .put("longVersionCode", ownInfo.longVersionCode)
                .put("targetSdkVersion", ownApp?.targetSdkVersion ?: JSONObject.NULL)
                .put("minSdkVersion", ownApp?.minSdkVersion ?: JSONObject.NULL)
                .put("compileSdkVersion", ownApp?.compileSdkVersion ?: JSONObject.NULL))
            .put("honorCameraPackage", honorCameraPackageSnapshot())
            .put("raw14Runtime", raw14Runtime.toJson())
            .put("formatInventory", JSONArray(formats.map {
                JSONObject().put("name", it.name).put("value", it.value)
            }))
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameras", cameras)
            .put("classification", "CURRENT_ANDROID17_HONOR_CAMERA_BASELINE__READ_ONLY_NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary",
                "PACKAGE_HASHES_AND_CAMERA2_CHARACTERISTICS_ARE SOFTWARE/CAPABILITY OBSERVATIONS ONLY; " +
                    "THEY DO NOT PROVE A SUCCESSFUL CAPTURE, OEM PRIVILEGED ROUTE, RAW14 DELIVERY, " +
                    "NATIVE ADC TOPOLOGY, DIRECT_CFA_200MP, OR CALIBRATION AUTHORITY")
    }

    @Suppress("DEPRECATION")
    private fun honorCameraPackageSnapshot(): JSONObject {
        return runCatching {
            val info = packageManager.getPackageInfo(HONOR_CAMERA_PACKAGE, 0)
            val app = info.applicationInfo
            val archivePaths = buildList {
                app?.sourceDir?.let { add(it) }
                app?.splitSourceDirs?.let { addAll(it) }
            }

            JSONObject()
                .put("installed", true)
                .put("packageName", info.packageName)
                .put("versionName", info.versionName ?: JSONObject.NULL)
                .put("longVersionCode", info.longVersionCode)
                .put("firstInstallTime", info.firstInstallTime)
                .put("lastUpdateTime", info.lastUpdateTime)
                .put("targetSdkVersion", app?.targetSdkVersion ?: JSONObject.NULL)
                .put("minSdkVersion", app?.minSdkVersion ?: JSONObject.NULL)
                .put("compileSdkVersion", app?.compileSdkVersion ?: JSONObject.NULL)
                .put("systemApp", app?.let { (it.flags and android.content.pm.ApplicationInfo.FLAG_SYSTEM) != 0 } ?: false)
                .put("updatedSystemApp", app?.let { (it.flags and android.content.pm.ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0 } ?: false)
                .put("packageArchives", JSONArray().apply {
                    for (path in archivePaths) {
                        put(hashArchive(path))
                    }
                })
        }.getOrElse { e ->
            JSONObject()
                .put("installed", false)
                .put("error", "${e.javaClass.name}: ${e.message}")
        }
    }

    private fun hashArchive(path: String): JSONObject {
        val file = File(path)
        val out = JSONObject()
            .put("fileName", file.name)
            .put("bytes", if (file.exists()) file.length() else JSONObject.NULL)

        return runCatching {
            val md = MessageDigest.getInstance("SHA-256")
            FileInputStream(file).use { input ->
                val buffer = ByteArray(1024 * 1024)
                while (true) {
                    val n = input.read(buffer)
                    if (n <= 0) break
                    md.update(buffer, 0, n)
                }
            }
            out.put("sha256", md.digest().joinToString("") { "%02x".format(it) })
                .put("readable", true)
        }.getOrElse { e ->
            out.put("readable", false)
                .put("sha256", JSONObject.NULL)
                .put("error", "${e.javaClass.name}: ${e.message}")
        }
    }

    private data class FormatSpec(val name: String, val value: Int)

    private data class RuntimeIntConstant(
        val name: String,
        val available: Boolean,
        val value: Int?,
        val error: String?,
    ) {
        fun toJson(): JSONObject = JSONObject()
            .put("name", name)
            .put("available", available)
            .put("value", value ?: JSONObject.NULL)
            .put("error", error ?: JSONObject.NULL)
    }

    private fun resolveRaw14Runtime(): RuntimeIntConstant =
        runCatching {
            val value = ImageFormat::class.java.getField("RAW14").getInt(null)
            RuntimeIntConstant("android.graphics.ImageFormat.RAW14", true, value, null)
        }.getOrElse {
            RuntimeIntConstant(
                "android.graphics.ImageFormat.RAW14",
                false,
                null,
                "${it.javaClass.name}: ${it.message}",
            )
        }

    private fun describeMap(map: StreamConfigurationMap?, formats: List<FormatSpec>): JSONObject {
        if (map == null) return JSONObject().put("present", false)

        val obj = JSONObject()
            .put("present", true)
            .put("outputFormats", jsonValue(runCatching { map.outputFormats }.getOrNull()))

        val perFormat = JSONObject()
        for (format in formats) {
            val entry = JSONObject()
            val normal = runCatching { map.getOutputSizes(format.value)?.toList() ?: emptyList() }
            val high = runCatching { map.getHighResolutionOutputSizes(format.value)?.toList() ?: emptyList() }

            entry.put("formatValue", format.value)
            normal.onSuccess { sizes ->
                entry.put("outputSizes", sizeArray(sizes))
                entry.put("outputSizeCount", sizes.size)
            }.onFailure { e ->
                entry.put("outputSizesError", "${e.javaClass.name}: ${e.message}")
            }
            high.onSuccess { sizes ->
                entry.put("highResolutionOutputSizes", sizeArray(sizes))
                entry.put("highResolutionOutputSizeCount", sizes.size)
            }.onFailure { e ->
                entry.put("highResolutionOutputSizesError", "${e.javaClass.name}: ${e.message}")
            }

            perFormat.put(format.name, entry)
        }
        obj.put("formats", perFormat)
        return obj
    }

    private fun describeExtensions(cm: CameraManager, cameraId: String): JSONObject {
        val out = JSONObject()
            .put("queriedWithoutCreatingExtensionSession", true)
            .put("knownPublicExtensionIdsQueried", JSONArray(listOf(0, 1, 2, 3, 4)))
            .put("unknownVendorExtensionIdEnumerationAttempted", false)

        return runCatching {
            val ext = cm.getCameraExtensionCharacteristics(cameraId)
            val supportedList = runCatching { ext.supportedExtensions }.getOrElse { emptyList() }
            out.put("getSupportedExtensions", JSONArray(supportedList))

            val method = runCatching {
                ext.javaClass.getMethod("isExtensionSupported", Integer.TYPE)
            }.getOrNull()

            out.put("api37IsExtensionSupportedMethodPresent", method != null)
            val checks = JSONArray()
            if (method != null) {
                for (id in listOf(0, 1, 2, 3, 4)) {
                    val one = JSONObject().put("extensionId", id)
                    runCatching { method.invoke(ext, id) as Boolean }
                        .onSuccess { one.put("supported", it) }
                        .onFailure { e ->
                            one.put("error", "${e.javaClass.name}: ${e.cause?.message ?: e.message}")
                        }
                    checks.put(one)
                }
            }
            out.put("isExtensionSupportedKnownPublicIds", checks)
            out
        }.getOrElse { e ->
            out.put("readError", "${e.javaClass.name}: ${e.message}")
        }
    }

    private fun sortedNames(names: List<String>): JSONArray =
        JSONArray(names.distinct().sorted())

    private fun sizeArray(sizes: List<Size>): JSONArray = JSONArray().apply {
        for (s in sizes.sortedWith(compareBy<Size>({ it.width.toLong() * it.height.toLong() }, { it.width }, { it.height }))) {
            put(JSONObject()
                .put("width", s.width)
                .put("height", s.height)
                .put("pixels", s.width.toLong() * s.height.toLong()))
        }
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
        is Number, is Boolean, is String -> value
        else -> value.toString()
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.55 baseline."
            return
        }
        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.55 baseline bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val device = report.optJSONObject("device")
        val honor = report.optJSONObject("honorCameraPackage")
        val raw14 = report.optJSONObject("raw14Runtime")
        val cams = report.optJSONArray("cameras")
        status.text = buildString {
            append("runtime=").append(device?.optString("release", "?"))
                .append(" / SDK ").append(device?.optInt("sdkInt", -1)).append('\n')
            append("build=").append(device?.optString("display", "?")).append('\n')
            append("HONOR Camera=").append(honor?.optString("versionName", "?"))
                .append(" (").append(honor?.optLong("longVersionCode", -1L)).append(")").append('\n')
            append("RAW14 runtime constant=").append(raw14?.optBoolean("available", false))
                .append(" value=").append(raw14?.opt("value")).append('\n')
            append("cameraEntries=").append(cams?.length() ?: 0).append('\n')
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
            status.text = "v0.55 JSON opgeslagen · read-only baseline · geen capture."
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
        private const val REQUEST_SAVE_JSON = 65555
        private const val REPORT_FILENAME = "TRUTHRAW_ANDROID17_HONOR_CAMERA_BASELINE_v055.json"
        private const val SCHEMA = "truthraw.android17-honor-camera-baseline.v0.55"
        private const val AUTHORITY = "ANDROID17_HONOR_CAMERA_PACKAGE_AND_CAMERA2_CHARACTERISTICS_BASELINE_ONLY"
        private const val HONOR_CAMERA_PACKAGE = "com.hihonor.camera"
    }
}

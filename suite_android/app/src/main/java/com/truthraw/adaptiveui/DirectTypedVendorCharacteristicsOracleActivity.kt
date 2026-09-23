package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
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
import java.security.MessageDigest
import java.time.Instant

/**
 * v0.61 expanded typed vendor-characteristics oracle.
 *
 * Read only. No CameraDevice, CaptureRequest, ImageReader, Honor Binder, package spoofing,
 * vendor write, or extension session.
 *
 * The exact names/types below are selected from static analysis of the exact Honor Camera
 * 171.0.10.452 and 171.0.10.706 APKs. APK-derived typing is target-selection evidence only.
 * Runtime values/errors are CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY.
 */
class DirectTypedVendorCharacteristicsOracleActivity : Activity() {
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

        body.addView(label("TruthRaw v0.50 · direct typed vendor-key oracle", 22f, true))
        body.addView(label(
            "Leest CameraCharacteristics zonder camera-open. v0.61 herhaalt de v0.50 keys en breidt ze uit met exact uit de " +
                "Honor Camera .452/.706 bytecode herleide RAW-, high-pixel- en remosaic-keys. Iedere key wordt " +
                "read-only opgevraagd en daarna twee keer herlezen om runtime-stabiliteit te controleren.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Lees uitgebreide RAW/remosaic vendor keys") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "Dit is geen bypass en geen capture. Mogelijke uitkomsten per key: waarde, null, constructiefout, " +
                "IllegalArgumentException, SecurityException of andere frameworkfout. Iedere uitkomst wordt bewaard.",
            11f, false, Color.rgb(155, 165, 180),
        ))

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
        status.text = "v0.61 leest uitsluitend CameraCharacteristics…"
        saveButton.isEnabled = false

        Thread {
            val report = runCatching { buildReport() }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
                    .put("captureEvidenceGranted", false)
                    .put("cameraOpenedByTruthRaw", false)
                    .put("captureSubmittedByTruthRaw", false)
                    .put("vendorRequestWrittenByTruthRaw", false)
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

        val cameraReports = JSONArray()
        for (id in allIds) {
            val out = JSONObject()
                .put("cameraId", id)
                .put("publicCameraId", id in publicIds)
                .put("disclosedPhysicalId", id in physicalIds)

            runCatching {
                val c = cm.getCameraCharacteristics(id)
                val enumeratedNames = c.keys.map { it.name }.toSet()

                out.put("standard", JSONObject()
                    .put("focalLengthsMm", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)))
                    .put("apertures", jsonValue(c.get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)))
                    .put("physicalCameraIds", JSONArray(
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) c.physicalCameraIds.toList()
                        else emptyList()
                    ))
                    .put("enumeratedCharacteristicKeyCount", c.keys.size))

                val reads = JSONArray()
                for (spec in KEY_SPECS) {
                    reads.put(readOne(c, enumeratedNames, spec))
                }
                out.put("directTypedReads", reads)
            }.onFailure { e ->
                out.put("cameraCharacteristicsReadError", errorJson(e))
            }

            cameraReports.put(out)
        }

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
            .put("packageIdentitySpoofedByTruthRaw", false)
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
            .put("staticApkTypeOracle", JSONObject()
                .put("authority", "STATIC_APK_SOFTWARE_ROUTE_EVIDENCE_ONLY")
                .put("android16HonorCamera", JSONObject()
                    .put("version", "171.0.10.452")
                    .put("sha256", APK_452_SHA256))
                .put("android17HonorCamera", JSONObject()
                    .put("version", "171.0.10.706")
                    .put("sha256", APK_706_SHA256))
                .put("typingObservedInBothComparedApks", true)
                .put("keySpecs", keySpecsJson())
                .put("staticTypingDoesNotGrantRuntimeAccess", true))
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameras", cameraReports)
            .put("classification", "DIRECT_TYPED_VENDOR_CHARACTERISTICS_READ_ONLY_ORACLE__NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary",
                "A_RETURNED_VALUE_IS_RUNTIME_CHARACTERISTICS_EVIDENCE_ONLY; IT DOES_NOT PROVE OEM_SHUTTER_ROUTE, ACTIVE_PHYSICAL_ID_FOR_A_CAPTURE, NATIVE_ADC_TOPOLOGY, DIRECT_CFA_200MP, OR CALIBRATION_TRUTH")
    }

    private fun readOne(
        c: CameraCharacteristics,
        enumeratedNames: Set<String>,
        spec: KeySpec,
    ): JSONObject {
        val out = JSONObject()
            .put("name", spec.name)
            .put("staticType", spec.typeLabel)
            .put("listedInCameraCharacteristicsKeys", spec.name in enumeratedNames)

        val key = runCatching { constructKey(spec) }
            .onFailure { e ->
                out.put("keyConstructed", false)
                out.put("constructionError", errorJson(e))
            }
            .getOrNull()

        if (key == null) return out

        out.put("keyConstructed", true)

        var firstCanonical: String? = null
        runCatching { c.get(key) }
            .onSuccess { value ->
                val encoded = jsonValue(value)
                firstCanonical = if (value == null) "<NULL>" else encoded.toString()
                out.put("directGetCompleted", true)
                out.put("valueIsNull", value == null)
                out.put("valueRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                out.put("value", encoded)
                out.put("valueSha256", sha256Text(firstCanonical!!))
            }
            .onFailure { e ->
                out.put("directGetCompleted", false)
                out.put("lookupError", errorJson(e))
            }

        val repeats = JSONArray()
        var repeatsStable = out.optBoolean("directGetCompleted", false)
        if (repeatsStable) {
            repeat(2) { index ->
                val one = JSONObject().put("ordinal", index + 2)
                runCatching { c.get(key) }
                    .onSuccess { value ->
                        val encoded = jsonValue(value)
                        val canonical = if (value == null) "<NULL>" else encoded.toString()
                        val same = canonical == firstCanonical
                        if (!same) repeatsStable = false
                        one.put("completed", true)
                            .put("valueIsNull", value == null)
                            .put("valueRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                            .put("sameAsFirst", same)
                            .put("valueSha256", sha256Text(canonical))
                    }
                    .onFailure { e ->
                        repeatsStable = false
                        one.put("completed", false)
                            .put("error", errorJson(e))
                    }
                repeats.put(one)
            }
        }
        out.put("repeatReads", repeats)
        out.put("repeatReadStable", repeatsStable)

        return out
    }

    @Suppress("UNCHECKED_CAST")
    private fun constructKey(spec: KeySpec): CameraCharacteristics.Key<Any> {
        val clazz: Class<Any> = when (spec.kind) {
            KeyKind.INT_ARRAY -> IntArray::class.java as Class<Any>
            KeyKind.BYTE_SCALAR -> java.lang.Byte.TYPE as Class<Any>
            KeyKind.INT_SCALAR -> java.lang.Integer.TYPE as Class<Any>
        }
        return CameraCharacteristics.Key(spec.name, clazz)
    }

    private fun keySpecsJson(): JSONArray = JSONArray().apply {
        for (spec in KEY_SPECS) {
            put(JSONObject()
                .put("name", spec.name)
                .put("javaType", spec.typeLabel))
        }
    }

    private fun errorJson(e: Throwable): JSONObject =
        JSONObject()
            .put("class", e.javaClass.name)
            .put("message", e.message ?: JSONObject.NULL)
            .put("causeClass", e.cause?.javaClass?.name ?: JSONObject.NULL)
            .put("causeMessage", e.cause?.message ?: JSONObject.NULL)

    private fun sha256Text(value: String): String =
        MessageDigest.getInstance("SHA-256")
            .digest(value.toByteArray(Charsets.UTF_8))
            .joinToString("") { "%02x".format(it) }

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
        is Byte -> value.toInt() and 0xff
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

        var values = 0
        var nulls = 0
        var errors = 0
        val cameras = report.optJSONArray("cameras")
        if (cameras != null) {
            for (i in 0 until cameras.length()) {
                val reads = cameras.optJSONObject(i)?.optJSONArray("directTypedReads") ?: continue
                for (j in 0 until reads.length()) {
                    val r = reads.optJSONObject(j) ?: continue
                    when {
                        !r.optBoolean("directGetCompleted", false) -> errors++
                        r.optBoolean("valueIsNull", true) -> nulls++
                        else -> values++
                    }
                }
            }
        }

        status.text = buildString {
            append("runtime=").append(report.optJSONObject("device")?.optString("release", "?"))
                .append(" / SDK ").append(report.optJSONObject("device")?.optInt("sdkInt", -1)).append('\n')
            append("nonNullValues=").append(values)
                .append(" · nulls=").append(nulls)
                .append(" · lookup/constructionErrors=").append(errors).append('\n')
            append("cameraOpenedByTruthRaw=").append(report.optBoolean("cameraOpenedByTruthRaw", true)).append('\n')
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
            status.text = "v0.61 JSON opgeslagen · characteristics read-only."
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

    private enum class KeyKind { INT_ARRAY, BYTE_SCALAR, INT_SCALAR }

    private data class KeySpec(
        val name: String,
        val kind: KeyKind,
        val typeLabel: String,
    )

    companion object {
        private const val SCHEMA = "truthraw.expanded-vendor-characteristics-oracle.v0.61"
        private const val AUTHORITY = "CAMERA2_CHARACTERISTICS_DIRECT_TYPED_VENDOR_KEY_READ_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_EXPANDED_VENDOR_CHARACTERISTICS_ORACLE_v061.json"
        private const val REQUEST_SAVE_JSON = 65061
        private const val APK_452_SHA256 = "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"
        private const val APK_706_SHA256 = "bbc6312e0289d01a51dbe45efc519e56715e6250521227df81cf64a633b8f027"

        private val KEY_SPECS = listOf(
            // v0.50 control keys
            KeySpec("com.hihonor.device.capabilities.physicalCameraScene", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.rawSensorResolution", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.sceneCameraIdCapability", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.cameraIdCustomInfo", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.needOpenPhysicalCamera", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.teleSupport", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.ultraHighPixelMonoSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.rawZoomSupported", KeyKind.BYTE_SCALAR, "byte"),

            // Exact .452/.706 bytecode types: RAW/high-pixel/remosaic expansion.
            KeySpec("com.hihonor.device.capabilities.rawImgSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.rawCaptureSize", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.rawForBokehSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.supportOfflineRawSceneMode", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.hwCaptureRawStreamConfigurations", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.hwProfessionalRawCaptureMode", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID", KeyKind.INT_SCALAR, "int"),
            KeySpec("com.hihonor.device.capabilities.remosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.softRemosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.frontSensorRemosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.sensorRemosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.rearSensorZoomRemosaicSupported", KeyKind.INT_SCALAR, "int"),
            KeySpec("com.hihonor.device.capabilities.subSensorRemosaic", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.remosaicFlashSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.ultraHighPixel", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.highPixelAlgoSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.customIdWithA200", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.brightnessThresholdWithA200", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.highPixelLivePhotoSupported", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.highPixelLivePhotoResolution", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.isUltraHighPixelSupportBeauty", KeyKind.INT_SCALAR, "int"),
            KeySpec("com.hihonor.device.capabilities.ultraHighPixelWatermarkSupported", KeyKind.INT_SCALAR, "int")
        )
    }
}

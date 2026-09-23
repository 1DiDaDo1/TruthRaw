package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Build
import android.os.Bundle
import android.os.SystemClock
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
 * v0.61 read-only HONOR RAW/remosaic vendor-characteristics oracle.
 *
 * No CameraDevice, CaptureRequest, ImageReader, ServiceHost/Binder call, package spoofing,
 * vendor write or extension session.
 *
 * Exact vendor-key names/types are recovered from the supplied HONOR Camera 171.0.10.452
 * bytecode. Runtime values remain characteristics evidence only.
 */
class RawRemosaicVendorCharacteristicsOracleActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveButton: Button
    private var snapshotIndex = 0

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

        body.addView(label("TruthRaw v0.61 · HONOR RAW/remosaic oracle", 22f, true))
        body.addView(label(
            "Read-only CameraCharacteristics. Deze versie breidt v0.50 uit met exact getypeerde RAW-, " +
                "tele-RAW-, remosaic- en high-pixel capability keys uit de .452 OEM-bytecode. " +
                "Geen camera wordt geopend.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Lees RAW/remosaic characteristics") { runOracle() })
        saveButton = button("2 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "Belangrijk: scene 66 is in de .452-code gekoppeld aan ProPhoto RAW wanneer physicalCameraScene " +
                "die scene ondersteunt. Een returned vendor value blijft runtime-characteristics evidence, " +
                "geen capture-, ADC- of 200MP-bewijs.",
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
        snapshotIndex += 1
        status.text = "v0.61 leest uitsluitend CameraCharacteristics…"
        saveButton.isEnabled = false

        Thread {
            val report = runCatching { buildReport(snapshotIndex) }.getOrElse { e ->
                JSONObject()
                    .put("schema", SCHEMA)
                    .put("createdAtUtc", Instant.now().toString())
                    .put("authority", AUTHORITY)
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

    private fun buildReport(index: Int): JSONObject {
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
                out.put("derivedReadOnlySummary", deriveSummary(reads))
            }.onFailure { e ->
                out.put("cameraCharacteristicsReadError", errorJson(e))
            }

            cameraReports.put(out)
        }

        return JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", AUTHORITY)
            .put("snapshotIndex", index)
            .put("elapsedRealtimeNanos", SystemClock.elapsedRealtimeNanos())
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
                .put("honorCameraVersion", "171.0.10.452")
                .put("honorCameraSha256", APK_452_SHA256)
                .put("scene66SoftwareMeaning", "PROPHOTO_RAW_WHEN_PHYSICAL_CAMERA_SCENE_SUPPORTS_66_AND_RAW_IS_OPEN")
                .put("scene65SoftwareMeaning", "PROPHOTO_JPEGL_WHEN_PHYSICAL_CAMERA_SCENE_SUPPORTS_65_AND_JPEGL_IS_OPEN")
                .put("keySpecs", keySpecsJson())
                .put("staticTypingDoesNotGrantRuntimeAccess", true))
            .put("publicCameraIds", JSONArray(publicIds))
            .put("disclosedPhysicalIds", JSONArray(physicalIds.toList()))
            .put("cameras", cameraReports)
            .put("classification", "HONOR_RAW_REMOSAIC_DIRECT_TYPED_CHARACTERISTICS_READ_ONLY_ORACLE__NO_CAMERA_OPEN_NO_CAPTURE")
            .put("boundary",
                "RUNTIME_VENDOR_CHARACTERISTIC_VALUES_ARE_ROUTE_CAPABILITY_EVIDENCE_ONLY; THEY_DO_NOT_PROVE_CAPTURE_SUCCESS, OEM_BUFFER_OWNERSHIP, NATIVE_ADC_TOPOLOGY, DIRECT_CFA_200MP, OR CALIBRATION_TRUTH")
    }

    private fun deriveSummary(reads: JSONArray): JSONObject {
        var scene66 = false
        var scene65 = false
        var nonNull = 0
        val returnedNames = JSONArray()

        for (i in 0 until reads.length()) {
            val r = reads.optJSONObject(i) ?: continue
            if (!r.optBoolean("directGetCompleted", false) || r.optBoolean("valueIsNull", true)) continue
            nonNull += 1
            returnedNames.put(r.optString("name"))
            if (r.optString("name") == "com.hihonor.device.capabilities.physicalCameraScene") {
                val a = r.optJSONArray("value")
                if (a != null) {
                    for (j in 0 until a.length()) {
                        if (a.optInt(j, Int.MIN_VALUE) == 66) scene66 = true
                        if (a.optInt(j, Int.MIN_VALUE) == 65) scene65 = true
                    }
                }
            }
        }

        return JSONObject()
            .put("nonNullVendorValues", nonNull)
            .put("returnedKeyNames", returnedNames)
            .put("physicalCameraSceneContains66", scene66)
            .put("scene66StaticSoftwareBinding", if (scene66) "PROPHOTO_RAW_ROUTE_CAPABILITY_PRESENT" else "NOT_OBSERVED")
            .put("physicalCameraSceneContains65", scene65)
            .put("scene65StaticSoftwareBinding", if (scene65) "PROPHOTO_JPEGL_ROUTE_CAPABILITY_PRESENT" else "NOT_OBSERVED")
            .put("derivedSummaryIsNotCaptureEvidence", true)
    }

    private fun readOne(
        c: CameraCharacteristics,
        enumeratedNames: Set<String>,
        spec: KeySpec,
    ): JSONObject {
        val out = JSONObject()
            .put("name", spec.name)
            .put("staticType", spec.typeLabel)
            .put("staticSource", spec.staticSource)
            .put("listedInCameraCharacteristicsKeys", spec.name in enumeratedNames)

        val key = runCatching { constructKey(spec) }
            .onFailure { e ->
                out.put("keyConstructed", false)
                out.put("constructionError", errorJson(e))
            }
            .getOrNull()

        if (key == null) return out
        out.put("keyConstructed", true)

        runCatching { c.get(key) }
            .onSuccess { value ->
                out.put("directGetCompleted", true)
                out.put("valueIsNull", value == null)
                out.put("valueRuntimeClass", value?.javaClass?.name ?: JSONObject.NULL)
                out.put("value", jsonValue(value))
                if (spec.name.endsWith("cameraIdCustomInfo") && value is IntArray) {
                    out.put("recordWidthFrom452Parser", 10)
                    out.put("records10", chunkIntArray(value, 10))
                }
            }
            .onFailure { e ->
                out.put("directGetCompleted", false)
                out.put("lookupError", errorJson(e))
            }

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

    private fun chunkIntArray(values: IntArray, width: Int): JSONArray = JSONArray().apply {
        var start = 0
        while (start < values.size) {
            val row = JSONArray()
            val end = minOf(values.size, start + width)
            for (i in start until end) row.put(values[i])
            put(row)
            start = end
        }
    }

    private fun keySpecsJson(): JSONArray = JSONArray().apply {
        for (spec in KEY_SPECS) {
            put(JSONObject()
                .put("name", spec.name)
                .put("javaType", spec.typeLabel)
                .put("staticSource", spec.staticSource))
        }
    }

    private fun errorJson(e: Throwable): JSONObject =
        JSONObject()
            .put("class", e.javaClass.name)
            .put("message", e.message ?: JSONObject.NULL)
            .put("causeClass", e.cause?.javaClass?.name ?: JSONObject.NULL)
            .put("causeMessage", e.cause?.message ?: JSONObject.NULL)

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
        var scene66Cameras = 0
        val cameras = report.optJSONArray("cameras")
        if (cameras != null) {
            for (i in 0 until cameras.length()) {
                val c = cameras.optJSONObject(i) ?: continue
                if (c.optJSONObject("derivedReadOnlySummary")?.optBoolean("physicalCameraSceneContains66", false) == true) {
                    scene66Cameras += 1
                }
                val reads = c.optJSONArray("directTypedReads") ?: continue
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
                .append(" · errors=").append(errors).append('\n')
            append("physicalCameraScene contains 66 on ").append(scene66Cameras).append(" camera id(s)").append('\n')
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
            status.text = "v0.61 JSON opgeslagen · read-only characteristics."
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
        val staticSource: String = "HONOR_CAMERA_171.0.10.452_BYTECODE",
    )

    companion object {
        private const val SCHEMA = "truthraw.honor-raw-remosaic-vendor-characteristics-oracle.v0.61"
        private const val AUTHORITY = "CAMERA2_CHARACTERISTICS_RAW_REMOSAIC_DIRECT_TYPED_READ_ONLY"
        private const val REPORT_FILENAME = "TRUTHRAW_HONOR_RAW_REMOSAIC_VENDOR_CHARACTERISTICS_ORACLE_v061.json"
        private const val REQUEST_SAVE_JSON = 65061
        private const val APK_452_SHA256 = "3985d9ce23ca4e47cdd34231723b8006f033753c6a3f611685c5c8fffd457d52"

        private val KEY_SPECS = listOf(
            KeySpec("com.hihonor.device.capabilities.physicalCameraScene", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.sceneCameraIdCapability", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.cameraIdCustomInfo", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.rawSensorResolution", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.rawCaptureSize", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.hwCaptureRawStreamConfigurations", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.supportOfflineRawSceneMode", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.ultraResolutionSwitchSupportedSize", KeyKind.INT_ARRAY, "int[]"),
            KeySpec("com.hihonor.device.capabilities.rawImgSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.hwProfessionalRawCaptureMode", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.rawForBokehSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.rawZoomSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.jpeglZoomSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.teleSupport", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.remosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.sensorRemosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.frontSensorRemosaicSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.ultraHighPixelMonoSupported", KeyKind.BYTE_SCALAR, "byte"),
            KeySpec("com.hihonor.device.capabilities.professionalTeleRawLogicalCameraID", KeyKind.INT_SCALAR, "int"),
            KeySpec("com.hihonor.device.capabilities.rearSensorZoomRemosaicSupported", KeyKind.INT_SCALAR, "int"),
        )
    }
}

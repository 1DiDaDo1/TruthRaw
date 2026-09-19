package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.ImageFormat
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
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
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.security.MessageDigest
import java.time.Instant

/**
 * v0.52 Android-17 Camera-5 payload delta companion.
 *
 * Capture is delegated unchanged to the already established FotoGraaf200MpStagedActivity
 * (v0.11 capture gate). This activity itself never opens a camera. After the capture activity
 * seals the app-visible RAW_SENSOR buffer and evidence JSON, v0.52 reads those files read-only
 * and runs the existing raster audit + payload geometry decoder.
 */
class Android17Camera5PayloadDeltaActivity : Activity() {
    private lateinit var status: TextView
    private lateinit var saveJsonButton: Button
    private lateinit var saveCandidateButton: Button
    private lateinit var savePreviewButton: Button

    private var reportFileRef: File? = null
    private var candidateFileRef: File? = null
    private var previewFileRef: File? = null

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

        body.addView(label("TruthRaw v0.52 · Android 17 Camera-5 payload delta", 21f, true))
        body.addView(label(
            "Herhaal eerst exact de bewezen 16320×12288 RAW_SENSOR capture-route. " +
                "Daarna analyseert v0.52 het verzegelde RAW-bestand read-only en vergelijkt de payloadtopologie " +
                "met de Android-16 v0.19/v0.20 referentie.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("1 · Open bewezen Camera-5 16320×12288 capture") {
            startActivity(Intent(this, FotoGraaf200MpStagedActivity::class.java))
        })
        body.addView(button("2 · Analyseer nieuwste verzegelde capture") { analyzeLatest() })

        saveJsonButton = button("3 · Delta JSON opslaan") {
            saveFile(reportFileRef, "application/json", REQUEST_SAVE_JSON)
        }.apply { isEnabled = false }
        body.addView(saveJsonButton)

        saveCandidateButton = button("Optioneel · exact kandidaat-payload opslaan") {
            saveFile(candidateFileRef, "application/octet-stream", REQUEST_SAVE_CANDIDATE)
        }.apply { isEnabled = false }
        body.addView(saveCandidateButton)

        savePreviewButton = button("Optioneel · appearance-only diagnosepreview opslaan") {
            saveFile(previewFileRef, "image/png", REQUEST_SAVE_PREVIEW)
        }.apply { isEnabled = false }
        body.addView(savePreviewButton)

        body.addView(space(10))
        body.addView(label(
            "Android-16 referentie: envelope 16320×12288 / 401,080,320 bytes; populated prefix 25,067,520 bytes; " +
                "exacte standaard-RAW kandidaat 4080×3072. Een match betekent stabiliteit van de app-visible HAL-payloadtopologie, " +
                "niet native sensor- of Direct-CFA-bewijs.",
            11f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.52 delta-report.", 10f, false)
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

    private fun analyzeLatest() {
        status.text = "Nieuwste verzegelde Camera-5 capture zoeken en read-only auditen…"
        saveJsonButton.isEnabled = false
        saveCandidateButton.isEnabled = false
        savePreviewButton.isEnabled = false

        Thread {
            val result = runCatching { buildDeltaReport() }
            runOnUiThread {
                result.onSuccess { report ->
                    status.text = summarize(report)
                    saveJsonButton.isEnabled = reportFileRef?.exists() == true
                    saveCandidateButton.isEnabled = candidateFileRef?.exists() == true
                    savePreviewButton.isEnabled = previewFileRef?.exists() == true
                }.onFailure { e ->
                    status.text = "v0.52 analyse geblokkeerd: ${e.javaClass.simpleName}: ${e.message}"
                }
            }
        }.start()
    }

    private fun buildDeltaReport(): JSONObject {
        val evidenceFile = cacheDir.listFiles()
            ?.filter { it.isFile && it.name.contains("_CAM5_200MP_EVIDENCE_v011.json") }
            ?.maxByOrNull { it.lastModified() }
            ?: error("Geen v0.11 Camera-5 evidence JSON in app-cache. Voer eerst stap 1 en de capture uit.")

        val evidence = JSONObject(evidenceFile.readText())
        val rawObj = evidence.getJSONObject("rawPayload")
        val rawName = rawObj.getString("file")
        val expectedRawSha = rawObj.getString("sha256")
        val rawFile = File(cacheDir, rawName)

        require(rawFile.exists()) { "Verzegelde RAW source ontbreekt in cache: $rawName" }
        require(rawFile.length() == rawObj.getLong("bytes")) {
            "RAW source byte count wijkt af van evidence JSON"
        }

        val observedSha = sha256(rawFile)
        require(observedSha.equals(expectedRawSha, ignoreCase = true)) {
            "RAW source SHA-256 wijkt af van sealed evidence: observed=$observedSha expected=$expectedRawSha"
        }

        val manager = getSystemService(CameraManager::class.java)
        val physical5 = manager.getCameraCharacteristics(PHYSICAL_ID)
        val standardMap = physical5.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)
        val standardRawSizes = runCatching {
            standardMap?.getOutputSizes(ImageFormat.RAW_SENSOR)?.toList().orEmpty()
        }.getOrElse { emptyList() }

        val stamp = System.currentTimeMillis()
        val candidate = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_ANDROID17_v052_candidate.rawpayload")
        val preview = File(cacheDir, "TRUTHRAW_${stamp}_CAM5_ANDROID17_v052_preview.png")

        val audit = RawSensorRasterAudit.audit(
            file = rawFile,
            width = TARGET_W,
            height = TARGET_H,
            pixelBytes = 2,
            expectedSealedSha256 = expectedRawSha,
        )

        val decoder = RawPayloadGeometryDecoder.decode(
            source = rawFile,
            rasterAudit = audit,
            declaredWidth = TARGET_W,
            declaredHeight = TARGET_H,
            advertisedStandardRawSizes = standardRawSizes,
            candidatePayloadFile = candidate,
            diagnosticPreviewFile = preview,
        )

        val payloadBytes = decoder.optLong("payloadBytes", -1L)
        val selected = decoder.optJSONObject("selectedCandidate")
        val selectedW = selected?.optInt("width", -1) ?: -1
        val selectedH = selected?.optInt("height", -1) ?: -1

        val sameEnvelope = rawFile.length() == ANDROID16_ENVELOPE_BYTES
        val samePayloadBytes = payloadBytes == ANDROID16_POPULATED_PREFIX_BYTES
        val sameCandidateGeometry =
            selectedW == ANDROID16_CANDIDATE_W && selectedH == ANDROID16_CANDIDATE_H

        val classification = when {
            sameEnvelope && samePayloadBytes && sameCandidateGeometry ->
                "ANDROID17_REPLICATES_ANDROID16_CAM5_ENVELOPE_AND_4080x3072_POPULATED_PREFIX_TOPOLOGY"
            sameEnvelope ->
                "ANDROID17_CAM5_ENVELOPE_SIZE_STABLE_BUT_PAYLOAD_TOPOLOGY_CHANGED_OR_UNRESOLVED"
            else ->
                "ANDROID17_CAM5_APP_VISIBLE_RAW_ENVELOPE_CHANGED"
        }

        val report = JSONObject()
            .put("schema", SCHEMA)
            .put("createdAtUtc", Instant.now().toString())
            .put("authority", "CAMERA2_SEALED_RAW_PAYLOAD_DELTA_AUDIT")
            .put("calibrationAuthorityGranted", false)
            .put("scientificMasterModified", false)
            .put("sourceEvidenceImmutable", true)
            .put("sourceRawOpenedForWriting", false)
            .put("analysisCreatesNewCaptureEvidence", false)
            .put("device", JSONObject()
                .put("manufacturer", Build.MANUFACTURER)
                .put("model", Build.MODEL)
                .put("sdkInt", Build.VERSION.SDK_INT)
                .put("release", Build.VERSION.RELEASE)
                .put("fingerprint", Build.FINGERPRINT)
                .put("truthRawTargetSdk", applicationInfo.targetSdkVersion))
            .put("captureEvidence", JSONObject()
                .put("file", evidenceFile.name)
                .put("sha256", sha256(evidenceFile))
                .put("authority", evidence.optString("authority", JSONObject.NULL.toString()))
                .put("requestTopology", evidence.optJSONObject("requestTopology") ?: JSONObject.NULL)
                .put("captureRoute", evidence.optJSONObject("captureRoute") ?: JSONObject.NULL)
                .put("captureResult", evidence.optJSONObject("captureResult") ?: JSONObject.NULL)
                .put("rawPayload", rawObj))
            .put("sealedSourceVerification", JSONObject()
                .put("file", rawFile.name)
                .put("bytes", rawFile.length())
                .put("expectedSha256", expectedRawSha)
                .put("observedSha256", observedSha)
                .put("sha256IdentityPass", true))
            .put("android16Reference", JSONObject()
                .put("declaredWidth", TARGET_W)
                .put("declaredHeight", TARGET_H)
                .put("envelopeBytes", ANDROID16_ENVELOPE_BYTES)
                .put("populatedPrefixBytes", ANDROID16_POPULATED_PREFIX_BYTES)
                .put("candidateWidth", ANDROID16_CANDIDATE_W)
                .put("candidateHeight", ANDROID16_CANDIDATE_H)
                .put("referenceAuthority", "PRIOR_DEVICE_CAPTURE_PAYLOAD_OBSERVATION"))
            .put("currentAdvertisedStandardRawSizes", sizesJson(standardRawSizes))
            .put("rasterAudit", audit)
            .put("payloadGeometryDecoder", decoder)
            .put("delta", JSONObject()
                .put("sameDeclaredEnvelopeBytesAsAndroid16", sameEnvelope)
                .put("samePopulatedPrefixBytesAsAndroid16", samePayloadBytes)
                .put("sameSelected4080x3072CandidateAsAndroid16", sameCandidateGeometry)
                .put("currentPayloadBytes", payloadBytes)
                .put("currentSelectedCandidateWidth", selectedW)
                .put("currentSelectedCandidateHeight", selectedH))
            .put("classification", classification)
            .put("boundary",
                "APP_VISIBLE_CAMERA2_RAW_SENSOR_PAYLOAD_TOPOLOGY_ONLY__NO_UNTOUCHED_ADC_NATIVE_SENSOR_GEOMETRY_OR_DIRECT_CFA_200MP_PROMOTION")

        val reportFile = File(cacheDir, "TRUTHRAW_${stamp}_ANDROID17_CAM5_PAYLOAD_DELTA_v052.json")
        reportFile.writeText(report.toString(2))

        reportFileRef = reportFile
        candidateFileRef = candidate.takeIf { it.exists() }
        previewFileRef = preview.takeIf { it.exists() }
        return report
    }

    private fun sizesJson(sizes: List<Size>): org.json.JSONArray =
        org.json.JSONArray().apply {
            sizes.sortedBy { it.width.toLong() * it.height.toLong() }.forEach {
                put(JSONObject()
                    .put("width", it.width)
                    .put("height", it.height)
                    .put("bytesU16", it.width.toLong() * it.height.toLong() * 2L))
            }
        }

    private fun summarize(report: JSONObject): String {
        val delta = report.getJSONObject("delta")
        return buildString {
            append("v0.52 analyse PASS\n")
            append("classification=").append(report.getString("classification")).append('\n')
            append("sourceSHA identity=true\n")
            append("payloadBytes=").append(delta.optLong("currentPayloadBytes", -1)).append('\n')
            append("candidate=")
                .append(delta.optInt("currentSelectedCandidateWidth", -1))
                .append("×")
                .append(delta.optInt("currentSelectedCandidateHeight", -1)).append('\n')
            append("sameAndroid16Envelope=").append(delta.optBoolean("sameDeclaredEnvelopeBytesAsAndroid16")).append('\n')
            append("sameAndroid16Payload=").append(delta.optBoolean("samePopulatedPrefixBytesAsAndroid16")).append('\n')
            append("same4080x3072Candidate=").append(delta.optBoolean("sameSelected4080x3072CandidateAsAndroid16"))
        }
    }

    private fun refreshStatus() {
        val latest = cacheDir.listFiles()
            ?.filter { it.isFile && it.name.contains("_ANDROID17_CAM5_PAYLOAD_DELTA_v052.json") }
            ?.maxByOrNull { it.lastModified() }

        if (latest == null) {
            status.text = "Nog geen v0.52 delta-report. Voer eerst de capture uit."
            saveJsonButton.isEnabled = false
            saveCandidateButton.isEnabled = false
            savePreviewButton.isEnabled = false
            return
        }

        reportFileRef = latest
        val report = runCatching { JSONObject(latest.readText()) }.getOrNull()
        if (report != null) {
            status.text = summarize(report)
            val decoder = report.optJSONObject("payloadGeometryDecoder")
            val candidateName = decoder?.optJSONObject("candidatePayload")?.optString("file", "")
            val previewName = decoder?.optJSONObject("diagnosticPreview")?.optString("file", "")
            candidateFileRef = candidateName?.takeIf { it.isNotBlank() }?.let { File(cacheDir, it) }?.takeIf { it.exists() }
            previewFileRef = previewName?.takeIf { it.isNotBlank() }?.let { File(cacheDir, it) }?.takeIf { it.exists() }
        }
        saveJsonButton.isEnabled = reportFileRef?.exists() == true
        saveCandidateButton.isEnabled = candidateFileRef?.exists() == true
        savePreviewButton.isEnabled = previewFileRef?.exists() == true
    }

    private fun sha256(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        FileInputStream(file).use { input ->
            val buffer = ByteArray(1024 * 1024)
            while (true) {
                val n = input.read(buffer)
                if (n <= 0) break
                md.update(buffer, 0, n)
            }
        }
        return md.digest().joinToString("") { "%02x".format(it.toInt() and 0xff) }
    }

    @Suppress("DEPRECATION")
    private fun saveFile(file: File?, mime: String, requestCode: Int) {
        if (file == null || !file.exists()) return
        startActivityForResult(
            Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = mime
                putExtra(Intent.EXTRA_TITLE, file.name)
            },
            requestCode,
        )
    }

    @Deprecated("Document export bridge")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        val source = when (requestCode) {
            REQUEST_SAVE_JSON -> reportFileRef
            REQUEST_SAVE_CANDIDATE -> candidateFileRef
            REQUEST_SAVE_PREVIEW -> previewFileRef
            else -> null
        } ?: return

        runCatching {
            contentResolver.openOutputStream(uri)?.use { out ->
                source.inputStream().use { input -> input.copyTo(out) }
            } ?: error("Geen output stream")
        }.onSuccess {
            status.text = "${source.name} opgeslagen; bron-authority ongewijzigd."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

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
        private const val SCHEMA = "truthraw.android17-camera5-payload-delta.v0.52"
        private const val PHYSICAL_ID = "5"
        private const val TARGET_W = 16320
        private const val TARGET_H = 12288
        private const val ANDROID16_ENVELOPE_BYTES = 401_080_320L
        private const val ANDROID16_POPULATED_PREFIX_BYTES = 25_067_520L
        private const val ANDROID16_CANDIDATE_W = 4080
        private const val ANDROID16_CANDIDATE_H = 3072
        private const val REQUEST_SAVE_JSON = 65201
        private const val REQUEST_SAVE_CANDIDATE = 65202
        private const val REQUEST_SAVE_PREVIEW = 65203
    }
}

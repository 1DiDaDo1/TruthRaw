package com.truthraw.adaptiveui

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONObject
import java.io.File

class PassiveHonorProRawDngFingerprintActivity : Activity() {
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

        body.addView(label("TruthRaw v0.54 · Honor Android-17 Pro RAW/DNG fingerprint", 21f, true))
        body.addView(label(
            "Passieve exported-output test. TruthRaw opent geen camera. Na een door jou gemaakte Honor Pro RAW/DNG " +
                "leest v0.54 alleen TIFF/DNG-containermetadata via een read-only file descriptor; pixeldata wordt niet gedecodeerd.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("0 · Geef MediaStore + notificatie toestemming") { requestNeededPermissions() })

        body.addView(space(8))
        body.addView(label("Kies per run precies één Honor Pro RAW-opname:", 12f, true))

        body.addView(button("1A · Start PRO RAW MAIN observer") {
            startRun(PassiveHonorProRawDngFingerprintService.PROFILE_PRO_MAIN_RAW)
        })
        body.addView(button("1B · Start PRO RAW TELE observer") {
            startRun(PassiveHonorProRawDngFingerprintService.PROFILE_PRO_TELE_RAW)
        })
        body.addView(button("2 · Ga naar Honor Camera") { moveTaskToBack(false) })

        body.addView(space(8))
        body.addView(label(
            "Honor Camera: kies PRO en zet RAW/DNG aan. MAIN: gebruik 1×. TELE: gebruik 3.7× als Honor PRO dat toelaat. " +
                "Maak exact één RAW-opname, wacht tot opslaan klaar is en kom daarna terug.",
            11f, true, Color.rgb(220, 225, 234),
        ))

        body.addView(button("3 · Markeer SHUTTER (optioneel, user timing)") {
            serviceAction(PassiveHonorProRawDngFingerprintService.ACTION_MARK_SHUTTER)
        })
        body.addView(button("4 · Terug uit Honor Camera + snapshot") {
            serviceAction(PassiveHonorProRawDngFingerprintService.ACTION_MARK_RETURNED)
            status.postDelayed({ refreshStatus() }, 700)
        })
        body.addView(button("5 · Extra MediaStore snapshot") {
            serviceAction(PassiveHonorProRawDngFingerprintService.ACTION_SNAPSHOT)
            status.postDelayed({ refreshStatus() }, 700)
        })
        body.addView(button("6 · Stop observer") {
            serviceAction(PassiveHonorProRawDngFingerprintService.ACTION_STOP)
            status.postDelayed({ refreshStatus() }, 1600)
        })

        saveButton = button("7 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(10))
        body.addView(label(
            "BitsPerSample=14 is een geëxporteerde 14-bit containerclaim. BitsPerSample=16 met WhiteLevel=16383 kan 14 effectieve " +
                "codebits in een 16-bit container betekenen. Geen van beide wordt zonder payloadanalyse gepromoveerd tot untouched ADC/native CFA.",
            10f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.54 report.", 10f, false)
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

    private fun requestNeededPermissions() {
        val needed = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.READ_MEDIA_IMAGES) != PackageManager.PERMISSION_GRANTED
        ) needed += Manifest.permission.READ_MEDIA_IMAGES
        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) needed += Manifest.permission.POST_NOTIFICATIONS

        if (needed.isEmpty()) status.text = "Benodigde toestemmingen zijn al verleend."
        else requestPermissions(needed.toTypedArray(), REQUEST_PERMISSIONS)
    }

    private fun startRun(profile: String) {
        startForegroundService(
            Intent(this, PassiveHonorProRawDngFingerprintService::class.java)
                .setAction(PassiveHonorProRawDngFingerprintService.ACTION_START)
                .putExtra(PassiveHonorProRawDngFingerprintService.EXTRA_RUN_PROFILE, profile)
        )
        status.postDelayed({ refreshStatus() }, 700)
    }

    private fun serviceAction(action: String) {
        startService(Intent(this, PassiveHonorProRawDngFingerprintService::class.java).setAction(action))
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text = "Nog geen v0.54 report."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "v0.54 report bestaat maar kon niet worden gelezen."
            return
        }

        var fingerprints = 0
        var success = 0
        var latestName: String? = null
        var latestRawSummary: String? = null
        val events = report.optJSONArray("events")
        if (events != null) {
            for (i in 0 until events.length()) {
                val e = events.optJSONObject(i) ?: continue
                if (e.optString("type") != "EXPORTED_DNG_CONTAINER_FINGERPRINT") continue
                fingerprints++
                val p = e.optJSONObject("payload") ?: continue
                if (p.optBoolean("success", false)) success++
                latestName = p.optString("displayName", latestName ?: "")
                val raws = p.optJSONObject("container")?.optJSONArray("rawCfaIfdCandidates")
                if (raws != null && raws.length() > 0) {
                    val r = raws.optJSONObject(0)
                    latestRawSummary = "RAW IFD: ${r?.opt("imageWidth")}×${r?.opt("imageLength")} · Bits=${r?.opt("bitsPerSample")} · White=${r?.opt("whiteLevel")} · Compression=${r?.opt("compression")}"
                }
            }
        }

        status.text = buildString {
            append("profile=").append(report.optString("runProfile", "?")).append('\n')
            append("DNG fingerprints=").append(fingerprints).append(" · success=").append(success).append('\n')
            append("latest=").append(latestName ?: "none").append('\n')
            if (latestRawSummary != null) append(latestRawSummary).append('\n')
            append("cameraOpenedByTruthRaw=false · imageSamplesDecoded=false · pixelPayloadBytesRead=false")
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
            status.text = "v0.54 JSON opgeslagen · exported DNG container metadata only."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, PassiveHonorProRawDngFingerprintService.REPORT_FILENAME)

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
        private const val REQUEST_PERMISSIONS = 65401
        private const val REQUEST_SAVE_JSON = 65402
    }
}

package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
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

class HonorOutputConfigCallbackProbeActivity : Activity() {
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

        body.addView(label("TruthRaw v0.41 · Honor output-config callback probe", 22f, true))
        body.addView(label(
            "Eén grens per run. Deze build bindt de Honor CameraAccessoriseService en probeert uitsluitend " +
                "registerOutputConfigCallback (transaction 3). Geen Surface, geen camera-open, geen capture, " +
                "geen andere callbackregistratie, geen allowlist-bypass.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))

        body.addView(button("1 · Start v0.41 observer") {
            startForegroundService(Intent(this, HonorOutputConfigCallbackProbeService::class.java))
            status.postDelayed({ refreshStatus() }, 500)
        })

        body.addView(button("2 · Probe output-config callback") {
            startService(
                Intent(this, HonorOutputConfigCallbackProbeService::class.java)
                    .setAction(HonorOutputConfigCallbackProbeService.ACTION_PROBE_OUTPUT_CONFIG),
            )
            status.postDelayed({ refreshStatus() }, 1000)
        })

        body.addView(button("3 · Markeer en open Honor Camera handmatig") {
            mark(
                "USER_WILL_OPEN_HONOR_CAMERA_MANUALLY",
                "Only useful if callback registration succeeded; TruthRaw does not launch or own the camera.",
            )
            moveTaskToBack(false)
        })

        body.addView(button("Markeer · 200MP nu geselecteerd") {
            mark(
                "USER_SELECTED_200MP",
                "Manual user marker only; does not assign semantics to any Honor callback string.",
            )
        })

        body.addView(button("Markeer · foto nu gemaakt") {
            mark(
                "USER_SHUTTER_NOW",
                "Manual user marker only; TruthRaw did not submit the capture.",
            )
        })

        saveButton = button("JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(button("Stop v0.41 observer") {
            startService(
                Intent(this, HonorOutputConfigCallbackProbeService::class.java)
                    .setAction(HonorOutputConfigCallbackProbeService.ACTION_STOP),
            )
            status.postDelayed({ refreshStatus() }, 600)
        })

        body.addView(space(12))
        status = label("Nog geen v0.41 report.", 10f, false)
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

    private fun mark(label: String, note: String) {
        startService(
            Intent(this, HonorOutputConfigCallbackProbeService::class.java)
                .setAction(HonorOutputConfigCallbackProbeService.ACTION_MARK)
                .putExtra(HonorOutputConfigCallbackProbeService.EXTRA_LABEL, label)
                .putExtra(HonorOutputConfigCallbackProbeService.EXTRA_NOTE, note),
        )
    }

    private fun refreshStatus() {
        val f = reportFile()
        saveButton.isEnabled = f.exists() && f.length() > 0L
        if (!f.exists()) {
            status.text =
                "Start eerst v0.41. Probe daarna exact één keer de output-config callback."
            return
        }

        val report = runCatching { JSONObject(f.readText()) }.getOrNull()
        if (report == null) {
            status.text = "Report bestaat maar JSON kon niet worden gelezen."
            return
        }

        val events = report.optJSONArray("events")
        val count = events?.length() ?: 0
        val last = if (count > 0) events?.optJSONObject(count - 1) else null
        val registrationResult = (0 until count)
            .mapNotNull { events?.optJSONObject(it) }
            .lastOrNull { it.optString("type") == "HONOR_OUTPUT_CONFIG_REGISTRATION_RESULT" }
            ?.optJSONObject("payload")

        status.text = buildString {
            append("Report: ").append(f.name).append('\n')
            append("events=").append(count).append(" · bytes=").append(f.length()).append('\n')
            append("registrationAttempted=")
                .append(report.optBoolean("registrationAttempted", false))
                .append(" · callbackRegistered=")
                .append(report.optBoolean("callbackRegistered", false))
                .append('\n')
            if (registrationResult != null) {
                append("classification=")
                    .append(registrationResult.optString("classification", "?"))
                    .append('\n')
                append("exception=")
                    .append(registrationResult.opt("exceptionClass"))
                    .append(" · ")
                    .append(registrationResult.opt("exceptionMessage"))
                    .append('\n')
            }
            append("lastEvent=").append(last?.optString("type", "none")).append('\n')
            append("cameraOpenedByTruthRaw=false · previewSurfaceProvided=false · captureSubmitted=false")
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
            status.text =
                "v0.41 JSON opgeslagen · software-route observation only · geen capture evidence."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, HonorOutputConfigCallbackProbeService.REPORT_FILENAME)

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
        private const val REQUEST_SAVE_JSON = 64141
    }
}

package com.truthraw.adaptiveui

import android.app.Activity
import android.content.ComponentName
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONObject
import java.io.File

class HonorPassiveRouteObserverActivity : Activity() {
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

        body.addView(label("TruthRaw v0.40 · Passive Honor Route Observer", 23f, true))
        body.addView(label(
            "Android-16 route observer. TruthRaw opent geen camera, submit geen capture, leest geen beeldbuffer " +
                "en schrijft geen vendor request. Start de observer en open daarna de Honor Camera handmatig of via één van de modusknoppen.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))

        body.addView(button("1 · Start passieve observer") {
            val i = Intent(this, HonorPassiveRouteObserverService::class.java)
            startForegroundService(i)
            status.postDelayed({ refreshStatus() }, 500)
        })

        body.addView(button("2 · Markeer: ik open Honor Camera handmatig") {
            mark("USER_WILL_OPEN_HONOR_CAMERA_MANUALLY", "No camera intent launched by TruthRaw.")
            moveTaskToBack(false)
        })

        body.addView(button("Honor · Ultra High Pixel openen") {
            mark("TRUTHRAW_LAUNCH_INTENT", ACTION_ULTRA_HIGH_PIXEL)
            launchHonorMode(ACTION_ULTRA_HIGH_PIXEL)
        })

        body.addView(button("Honor · Pro Photo openen") {
            mark("TRUTHRAW_LAUNCH_INTENT", ACTION_PRO_PHOTO)
            launchHonorMode(ACTION_PRO_PHOTO)
        })

        body.addView(button("Honor · Super Tele Photo openen") {
            mark("TRUTHRAW_LAUNCH_INTENT", ACTION_SUPER_TELE_PHOTO)
            launchHonorMode(ACTION_SUPER_TELE_PHOTO)
        })

        body.addView(button("Optioneel · CameraAccessoriseService handshake") {
            startService(
                Intent(this, HonorPassiveRouteObserverService::class.java)
                    .setAction(HonorPassiveRouteObserverService.ACTION_PROBE_ACCESSORY),
            )
            status.postDelayed({ refreshStatus() }, 800)
        })

        body.addView(button("Markeer · capture/mode nu gewijzigd") {
            mark("USER_MANUAL_MODE_OR_CAPTURE_MARK", "User explicitly marked a manual Honor Camera state change.")
        })

        saveButton = button("JSON opslaan") {
            saveReport()
        }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(button("Stop observer") {
            startService(
                Intent(this, HonorPassiveRouteObserverService::class.java)
                    .setAction(HonorPassiveRouteObserverService.ACTION_STOP),
            )
            status.postDelayed({ refreshStatus() }, 500)
        })

        body.addView(space(10))
        status = label("Nog geen report.", 10f, false, Color.WHITE)
        body.addView(status)

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            addView(
                body,
                ScrollView.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                ),
            )
            setBackgroundColor(Color.rgb(12, 14, 18))
        }
        scroll.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
            insets
        }
        return scroll
    }

    private fun launchHonorMode(action: String) {
        val intent = Intent(action).apply {
            component = ComponentName(
                HonorPassiveRouteObserverService.HONOR_PACKAGE,
                "com.hihonor.camera.StoreShow",
            )
            addCategory(Intent.CATEGORY_DEFAULT)
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        }

        val resolved = intent.resolveActivity(packageManager)
        if (resolved == null) {
            mark("HONOR_MODE_INTENT_UNRESOLVED", action)
            status.text = "Honor mode-intent niet resolveerbaar: $action"
            return
        }

        runCatching { startActivity(intent) }
            .onFailure {
                mark("HONOR_MODE_INTENT_FAILED", "${it.javaClass.simpleName}: ${it.message}")
                status.text = "Honor mode start faalde: ${it.javaClass.simpleName}: ${it.message}"
            }
    }

    private fun mark(label: String, note: String) {
        startService(
            Intent(this, HonorPassiveRouteObserverService::class.java)
                .setAction(HonorPassiveRouteObserverService.ACTION_MARK)
                .putExtra(HonorPassiveRouteObserverService.EXTRA_LABEL, label)
                .putExtra(HonorPassiveRouteObserverService.EXTRA_NOTE, note),
        )
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L
        if (!file.exists()) {
            status.text =
                "Nog geen v0.40 report. Start eerst de observer. Daarna mag je de Honor Camera volledig handmatig openen."
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "Report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val events = report.optJSONArray("events")
        val count = events?.length() ?: 0
        val last = if (count > 0) events?.optJSONObject(count - 1) else null
        val device = report.optJSONObject("device")
        val honor = report.optJSONObject("honorCameraPackage")

        status.text = buildString {
            append("Report: ").append(file.name).append('\n')
            append("bytes=").append(file.length()).append(" · events=").append(count).append('\n')
            append("device SDK=").append(device?.optInt("sdkInt", -1))
                .append(" release=").append(device?.optString("release", "?")).append('\n')
            append("Honor installed=").append(honor?.optBoolean("installed", false))
                .append(" version=").append(honor?.optString("versionName", "?"))
                .append(" targetSdk=").append(honor?.opt("targetSdkVersion"))
                .append(" minSdk=").append(honor?.opt("minSdkVersion"))
                .append(" compileSdk=").append(honor?.opt("compileSdkVersion")).append('\n')
            append("lastEvent=").append(last?.optString("type", "none")).append('\n')
            append("TruthRaw camera opened=false · capture submitted=false · image buffer accessed=false")
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
            status.text = "v0.40 JSON opgeslagen. Authority blijft PASSIVE_ROUTE_OBSERVATION_ONLY."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, HonorPassiveRouteObserverService.REPORT_FILENAME)

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
        View(this).apply {
            layoutParams = LinearLayout.LayoutParams(1, dp(height))
        }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object {
        private const val ACTION_ULTRA_HIGH_PIXEL =
            "com.hihonor.camera.store.show.BACK_ULTRA_HIGH_PIXEL"
        private const val ACTION_PRO_PHOTO =
            "com.hihonor.camera.store.show.BACK_PRO_PHOTO"
        private const val ACTION_SUPER_TELE_PHOTO =
            "com.hihonor.camera.store.show.SUPER_TELE_PHOTO"
        private const val REQUEST_SAVE_JSON = 64040
    }
}

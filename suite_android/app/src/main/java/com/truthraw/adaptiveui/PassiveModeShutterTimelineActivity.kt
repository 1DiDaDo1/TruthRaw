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

class PassiveModeShutterTimelineActivity : Activity() {
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

        body.addView(label("TruthRaw v0.43 · Passive mode/shutter timeline", 22f, true))
        body.addView(label(
            "Replicatie van v0.42 met de werkelijk zichtbare Honor-modi: PHOTO, PRO en HI-RES. " +
                "HI-RES is een kandidaatlabel, niet vooraf gelijkgesteld aan 200 MP. Honor blijft camera-eigenaar.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))

        body.addView(button("0 · Geef MediaStore + notificatie toestemming") {
            requestNeededPermissions()
        })

        body.addView(label(
            "De notificatie bevat drie markerknoppen zodat Honor Camera open kan blijven: MAIN STABLE, " +
                "MODE SELECTED en SHUTTER PRESSED. MODE SELECTED wordt gekoppeld aan het gekozen runprofiel.",
            10f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(8))
        body.addView(label("Kies precies één runprofiel per verse Honor-start:", 12f, true))

        body.addView(button("1A · Start PHOTO-controle") {
            startRun(PassiveModeShutterTimelineService.PROFILE_NORMAL)
        })

        body.addView(button("1B · Start PRO-controle") {
            startRun(PassiveModeShutterTimelineService.PROFILE_PRO)
        })

        body.addView(button("1C · Start HI-RES kandidaat") {
            startRun(PassiveModeShutterTimelineService.PROFILE_HIRES)
        })

        body.addView(button("2 · Markeer en ga naar Honor Camera") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_MARK_LAUNCH, "activity")
            moveTaskToBack(false)
        })

        body.addView(space(8))
        body.addView(label(
            "Fallback-markers hieronder zijn alleen bedoeld als de notificatie-actions niet zichtbaar zijn. " +
                "Gebruik bij voorkeur de notificatie terwijl Honor Camera open blijft.",
            10f, false, Color.rgb(150, 160, 174),
        ))

        body.addView(button("Fallback · MAIN STABLE") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_MARK_MAIN_STABLE, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · MODE SELECTED") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_MARK_MODE_SELECTED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · SHUTTER PRESSED") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_MARK_SHUTTER_PRESSED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("3 · Markeer terug uit Honor Camera") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_MARK_RETURNED, "activity")
            status.postDelayed({ refreshStatus() }, 300)
        })

        body.addView(button("4 · MediaStore snapshot nu") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_SNAPSHOT_MEDIA, "activity")
            status.postDelayed({ refreshStatus() }, 500)
        })

        body.addView(button("5 · Stop v0.43 observer") {
            serviceAction(PassiveModeShutterTimelineService.ACTION_STOP, "activity")
            status.postDelayed({ refreshStatus() }, 1600)
        })

        saveButton = button("6 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(12))
        body.addView(label(
            "PHOTO: fresh Honor-data → start 1A → Honor openen → MAIN STABLE → PHOTO actief laten → MODE SELECTED → " +
                "één foto → SHUTTER PRESSED → terug → snapshot → stop → JSON.\n\n" +
                "PRO: opnieuw fresh Honor-data → start 1B → Honor openen → MAIN STABLE → PRO kiezen → MODE SELECTED → " +
                "één foto → SHUTTER PRESSED → terug → snapshot → stop → JSON.\n\n" +
                "HI-RES: opnieuw fresh Honor-data → start 1C → Honor openen → MAIN STABLE → HI-RES kiezen → MODE SELECTED → " +
                "één foto → SHUTTER PRESSED → terug → snapshot → stop → JSON. Pas de gemeten outputgeometrie bepaalt of HI-RES " +
                "overeenkomt met de eerdere 16320×12288-outputklasse.",
            11f, true, Color.rgb(220, 225, 234),
        ))

        body.addView(space(8))
        body.addView(label(
            "Na ieder MediaStore-change leest v0.43 dezelfde metadata-row opnieuw op 0, 25, 100, 250 en 1000 ms. " +
                "Er worden geen afbeeldingsbytes of EXIF gelezen.",
            10f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.43 report.", 10f, false)
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

    private fun requestNeededPermissions() {
        val needed = mutableListOf<String>()

        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.READ_MEDIA_IMAGES) != PackageManager.PERMISSION_GRANTED
        ) {
            needed += Manifest.permission.READ_MEDIA_IMAGES
        }

        if (Build.VERSION.SDK_INT >= 33 &&
            checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
        ) {
            needed += Manifest.permission.POST_NOTIFICATIONS
        }

        if (needed.isEmpty()) {
            status.text = "MediaStore- en notificatie-toegang zijn al verleend."
            return
        }

        requestPermissions(needed.toTypedArray(), REQUEST_PERMISSIONS)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != REQUEST_PERMISSIONS) return

        val result = permissions.indices.joinToString("\n") { index ->
            val granted = grantResults.getOrNull(index) == PackageManager.PERMISSION_GRANTED
            "${permissions[index]}=$granted"
        }
        status.text = result
    }

    private fun startRun(profile: String) {
        val intent = Intent(this, PassiveModeShutterTimelineService::class.java)
            .setAction(PassiveModeShutterTimelineService.ACTION_START)
            .putExtra(PassiveModeShutterTimelineService.EXTRA_RUN_PROFILE, profile)

        startForegroundService(intent)
        status.postDelayed({ refreshStatus() }, 700)
    }

    private fun serviceAction(action: String, source: String) {
        startService(
            Intent(this, PassiveModeShutterTimelineService::class.java)
                .setAction(action)
                .putExtra(PassiveModeShutterTimelineService.EXTRA_MARK_SOURCE, source),
        )
    }

    private fun refreshStatus() {
        val file = reportFile()
        saveButton.isEnabled = file.exists() && file.length() > 0L

        val mediaGranted =
            Build.VERSION.SDK_INT < 33 ||
                checkSelfPermission(Manifest.permission.READ_MEDIA_IMAGES) == PackageManager.PERMISSION_GRANTED

        val notificationsGranted =
            Build.VERSION.SDK_INT < 33 ||
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED

        if (!file.exists()) {
            status.text = buildString {
                append("Nog geen v0.43 report.\n")
                append("READ_MEDIA_IMAGES=").append(mediaGranted).append('\n')
                append("POST_NOTIFICATIONS=").append(notificationsGranted)
            }
            return
        }

        val report = runCatching { JSONObject(file.readText()) }.getOrNull()
        if (report == null) {
            status.text = "Report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val events = report.optJSONArray("events")
        val count = events?.length() ?: 0

        var mainMarkers = 0
        var modeSelectedMarkers = 0
        var shutterMarkers = 0
        var mediaChanges = 0
        var delayedSnapshots = 0
        var cameraUnavailable = 0
        var latestGeometry: String? = null

        for (i in 0 until count) {
            val e = events?.optJSONObject(i) ?: continue
            when (e.optString("type")) {
                "USER_MARK" -> {
                    when (e.optJSONObject("payload")?.optString("label")) {
                        PassiveModeShutterTimelineService.MARK_MAIN_STABLE -> mainMarkers++
                        PassiveModeShutterTimelineService.MARK_PHOTO_SELECTED,
                        PassiveModeShutterTimelineService.MARK_PRO_SELECTED,
                        PassiveModeShutterTimelineService.MARK_HIRES_SELECTED,
                        PassiveModeShutterTimelineService.MARK_MODE_SELECTED_UNSPECIFIED -> modeSelectedMarkers++
                        PassiveModeShutterTimelineService.MARK_SHUTTER_PRESSED -> shutterMarkers++
                    }
                }

                "MEDIASTORE_CHANGE" -> mediaChanges++
                "MEDIASTORE_DELAYED_METADATA_SNAPSHOT" -> {
                    delayedSnapshots++
                    val rows = e.optJSONObject("payload")
                        ?.optJSONObject("query")
                        ?.optJSONArray("rows")
                    val row = if (rows != null && rows.length() > 0) rows.optJSONObject(0) else null
                    if (row != null) {
                        val w = row.optInt("width", -1)
                        val h = row.optInt("height", -1)
                        if (w > 0 && h > 0) latestGeometry = "${w}×${h}"
                    }
                }
                "CAMERA_UNAVAILABLE" -> cameraUnavailable++
            }
        }

        val last = if (count > 0) events?.optJSONObject(count - 1) else null

        status.text = buildString {
            append("runProfile=").append(report.optString("runProfile", "?")).append('\n')
            append("events=").append(count)
                .append(" · mediaChanges=").append(mediaChanges)
                .append(" · delayedSnapshots=").append(delayedSnapshots)
                .append('\n')
            append("MAIN=").append(mainMarkers)
                .append(" · MODE=").append(modeSelectedMarkers)
                .append(" · SHUTTER=").append(shutterMarkers)
                .append(" · cameraUnavailable=").append(cameraUnavailable)
                .append('\n')
            append("latestGeometry=").append(latestGeometry ?: "none").append('\n')
            append("lastEvent=").append(last?.optString("type", "none")).append('\n')
            append("media=").append(mediaGranted)
                .append(" · notifications=").append(notificationsGranted)
                .append('\n')
            append("cameraOpenedByTruthRaw=false · pixelRead=false · EXIF=false · Honor Binder=false")
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
                "v0.43 JSON opgeslagen · markers + systeemmetadata only · geen foto-pixels gelezen."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, PassiveModeShutterTimelineService.REPORT_FILENAME)

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
        private const val REQUEST_PERMISSIONS = 64300
        private const val REQUEST_SAVE_JSON = 64343
    }
}

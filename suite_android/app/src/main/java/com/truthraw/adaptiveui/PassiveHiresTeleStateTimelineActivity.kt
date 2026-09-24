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

class PassiveHiresTeleStateTimelineActivity : Activity() {
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

        body.addView(label("TruthRaw v0.44 · HI-RES / tele state anchors", 22f, true))
        body.addView(label(
            "Gebouwd uit de v0.43-uitkomst: PHOTO is de main-baseline zonder modewissel; PRO krijgt een expliciete switch-marker; " +
                "HI-RES krijgt twee afzonderlijke UI-state markers: HIRES MAIN en TELE / 200MP UI. " +
                "Geen van deze markers bewijst een fysiek camera-ID of native sensorresolutie.",
            12f, false, Color.rgb(190, 198, 210),
        ))

        body.addView(space(10))
        body.addView(button("0 · Geef MediaStore + notificatie toestemming") {
            requestNeededPermissions()
        })

        body.addView(label(
            "Screenshots mogen gewoon tussendoor gemaakt worden. v0.44 classificeert alleen hun MediaStore-metadata als " +
                "SCREENSHOT_UI_ANCHOR_CANDIDATE; het opent of leest de screenshot-afbeelding niet.",
            10f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(8))
        body.addView(label("Kies één sessie:", 12f, true))

        body.addView(button("1A · PHOTO main-baseline") {
            startRun(PassiveHiresTeleStateTimelineService.PROFILE_PHOTO)
        })

        body.addView(button("1B · PRO switch-sessie") {
            startRun(PassiveHiresTeleStateTimelineService.PROFILE_PRO)
        })

        body.addView(button("1C · HI-RES main → tele/200MP UI") {
            startRun(PassiveHiresTeleStateTimelineService.PROFILE_HIRES_TELE)
        })

        body.addView(button("2 · Markeer en ga naar Honor Camera") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_LAUNCH, "activity")
            moveTaskToBack(false)
        })

        body.addView(space(8))
        body.addView(label(
            "Gebruik bij voorkeur de knoppen in de TruthRaw-notificatie terwijl Honor Camera open blijft. " +
                "De notificatie past zich aan het gekozen sessietype aan.",
            10f, false, Color.rgb(150, 160, 174),
        ))

        body.addView(button("Fallback · MAIN STABLE") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_MAIN_STABLE, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · PHOTO MAIN bevestigd") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_PHOTO_CONFIRMED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · PRO geselecteerd") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_PRO_SELECTED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · HIRES MAIN bevestigd") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_HIRES_MAIN_CONFIRMED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · TELE / 200MP UI geselecteerd") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_TELE_200MP_UI_SELECTED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("Fallback · SHUTTER PRESSED") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_SHUTTER_PRESSED, "activity_fallback")
            status.postDelayed({ refreshStatus() }, 250)
        })

        body.addView(button("3 · Markeer terug uit Honor Camera") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_MARK_RETURNED, "activity")
            status.postDelayed({ refreshStatus() }, 300)
        })

        body.addView(button("4 · MediaStore snapshot nu") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_SNAPSHOT_MEDIA, "activity")
            status.postDelayed({ refreshStatus() }, 500)
        })

        body.addView(button("5 · Stop v0.44 observer") {
            serviceAction(PassiveHiresTeleStateTimelineService.ACTION_STOP, "activity")
            status.postDelayed({ refreshStatus() }, 1600)
        })

        saveButton = button("6 · JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(space(12))
        body.addView(label(
            "PHOTO: open Honor in de normale main/PHOTO-toestand → tik MAIN/PHOTO in de notificatie → eventueel screenshot → " +
                "maak foto → tik SHUTTER → terug → snapshot → stop → JSON. Er is hier geen modewissel nodig.\n\n" +
                "PRO: open main → MAIN STABLE → schakel naar PRO → PRO SELECTED → eventueel screenshot → foto → SHUTTER.\n\n" +
                "HI-RES: ga naar HI-RES terwijl hij nog op main staat → HIRES MAIN → eventueel screenshot → maak desgewenst één main-HIRES-foto → " +
                "SHUTTER → schakel binnen HI-RES handmatig naar tele / de zichtbare 200MP-UI-toestand → TELE / 200MP UI → eventueel screenshot → " +
                "maak foto → SHUTTER. Je hoeft niet te haasten: de marker blijft een menselijke tijdsanker, geen hardware-shuttertimestamp.",
            11f, true, Color.rgb(220, 225, 234),
        ))

        body.addView(space(8))
        body.addView(label(
            "Na iedere MediaStore-change meet v0.44 dezelfde row op 0, 25, 100, 250 en 1000 ms en bewaart expliciet " +
                "same-row metadata-overgangen, bijvoorbeeld 6144×8192 → 16320×12288. Ook wordt de laatst bekende menselijke state-marker " +
                "bij ieder change-event vastgelegd.",
            10f, false, Color.rgb(155, 165, 180),
        ))

        body.addView(space(10))
        status = label("Nog geen v0.44 report.", 10f, false)
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

        status.text = permissions.indices.joinToString("\n") { index ->
            val granted = grantResults.getOrNull(index) == PackageManager.PERMISSION_GRANTED
            "${permissions[index]}=$granted"
        }
    }

    private fun startRun(profile: String) {
        startForegroundService(
            Intent(this, PassiveHiresTeleStateTimelineService::class.java)
                .setAction(PassiveHiresTeleStateTimelineService.ACTION_START)
                .putExtra(PassiveHiresTeleStateTimelineService.EXTRA_RUN_PROFILE, profile),
        )
        status.postDelayed({ refreshStatus() }, 700)
    }

    private fun serviceAction(action: String, source: String) {
        startService(
            Intent(this, PassiveHiresTeleStateTimelineService::class.java)
                .setAction(action)
                .putExtra(PassiveHiresTeleStateTimelineService.EXTRA_MARK_SOURCE, source),
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
                append("Nog geen v0.44 report.\n")
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
        var shutterMarkers = 0
        var mediaChanges = 0
        var cameraUnavailable = 0
        var rowTransitions = 0
        var screenshotCandidates = 0
        var honorOutputs = 0
        var latestGeometry: String? = null
        var latestMarker: String? = null

        for (i in 0 until count) {
            val event = events?.optJSONObject(i) ?: continue
            when (event.optString("type")) {
                "USER_MARK" -> {
                    val label = event.optJSONObject("payload")?.optString("label")
                    if (!label.isNullOrBlank()) latestMarker = label
                    if (label == PassiveHiresTeleStateTimelineService.MARK_SHUTTER_PRESSED) shutterMarkers++
                }
                "MEDIASTORE_CHANGE" -> mediaChanges++
                "CAMERA_UNAVAILABLE" -> cameraUnavailable++
                "MEDIASTORE_DELAYED_METADATA_SNAPSHOT",
                "USER_REQUESTED_MEDIA_SNAPSHOT",
                "MEDIASTORE_BASELINE_SNAPSHOT" -> {
                    val rows = event.optJSONObject("payload")
                        ?.optJSONObject("query")
                        ?.optJSONArray("rows") ?: continue
                    for (r in 0 until rows.length()) {
                        val row = rows.optJSONObject(r) ?: continue
                        when (row.optString("observationClass")) {
                            "SCREENSHOT_UI_ANCHOR_CANDIDATE" -> screenshotCandidates++
                            "HONOR_CAMERA_SYSTEM_VISIBLE_OUTPUT_CANDIDATE" -> honorOutputs++
                        }
                        if (row.optBoolean("metadataStateChangedSincePreviousObservation", false)) {
                            rowTransitions++
                        }
                        val w = row.optLong("width", -1L)
                        val h = row.optLong("height", -1L)
                        if (w > 0L && h > 0L) latestGeometry = "${w}×${h}"
                    }
                }
            }
        }

        val last = if (count > 0) events?.optJSONObject(count - 1) else null
        status.text = buildString {
            append("runProfile=").append(report.optString("runProfile", "?")).append('\n')
            append("events=").append(count)
                .append(" · mediaChanges=").append(mediaChanges)
                .append(" · cameraUnavailable=").append(cameraUnavailable)
                .append('\n')
            append("shutterMarkers=").append(shutterMarkers)
                .append(" · rowTransitions=").append(rowTransitions)
                .append('\n')
            append("screenshotCandidates=").append(screenshotCandidates)
                .append(" · honorOutputObservations=").append(honorOutputs)
                .append('\n')
            append("latestMarker=").append(latestMarker ?: "none").append('\n')
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
                "v0.44 JSON opgeslagen · UI-state markers + systeemmetadata only · geen foto-/screenshotpixels gelezen."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, PassiveHiresTeleStateTimelineService.REPORT_FILENAME)

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
        private const val REQUEST_PERMISSIONS = 64400
        private const val REQUEST_SAVE_JSON = 64444
    }
}

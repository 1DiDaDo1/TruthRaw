package com.truthraw.adaptiveui

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

class PassiveMediaStoreCameraTimelineActivity : Activity() {
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

        body.addView(label("TruthRaw v0.42 · Passive MediaStore + Camera timeline", 22f, true))
        body.addView(label(
            "Honor blijft volledig eigenaar van de camera. TruthRaw observeert alleen CameraManager-status en " +
                "MediaStore-database metadata. Geen JPEG/RAW-pixels, geen EXIF-decode, geen Honor Binder-callback.",
            12f, false, Color.rgb(190, 198, 210),
        ))
        body.addView(space(10))

        body.addView(button("0 · Geef MediaStore foto-metadata toegang") {
            requestMediaPermission()
        })

        body.addView(label(
            "Android noemt dit READ_MEDIA_IMAGES. Die permissie zou technisch ook bestandslezing mogelijk maken, " +
                "maar v0.42 bevat bewust géén openInputStream/image decode/EXIF-pad. Zonder volledige toegang blijft " +
                "de observer bruikbaar, maar row-metadata van nieuwe Honor-foto's kan door Android worden afgeschermd.",
            10f, false, Color.rgb(150, 160, 174),
        ))

        body.addView(button("1 · Start v0.42 observer") {
            startForegroundService(Intent(this, PassiveMediaStoreCameraTimelineService::class.java))
            status.postDelayed({ refreshStatus() }, 600)
        })

        body.addView(button("2 · Markeer en open Honor Camera handmatig") {
            mark(
                "USER_WILL_OPEN_HONOR_CAMERA_MANUALLY",
                "TruthRaw moved to background only; user opens Honor Camera manually.",
            )
            moveTaskToBack(false)
        })

        body.addView(button("Markeer · terug uit Honor Camera") {
            mark(
                "USER_RETURNED_FROM_HONOR_CAMERA",
                "User returned to TruthRaw after manual Honor Camera operation.",
            )
            status.postDelayed({ refreshStatus() }, 300)
        })

        body.addView(button("MediaStore snapshot nu") {
            startService(
                Intent(this, PassiveMediaStoreCameraTimelineService::class.java)
                    .setAction(PassiveMediaStoreCameraTimelineService.ACTION_SNAPSHOT_MEDIA),
            )
            status.postDelayed({ refreshStatus() }, 500)
        })

        saveButton = button("JSON opslaan") { saveReport() }.apply { isEnabled = false }
        body.addView(saveButton)

        body.addView(button("Stop v0.42 observer") {
            startService(
                Intent(this, PassiveMediaStoreCameraTimelineService::class.java)
                    .setAction(PassiveMediaStoreCameraTimelineService.ACTION_STOP),
            )
            status.postDelayed({ refreshStatus() }, 700)
        })

        body.addView(space(12))
        body.addView(label(
            "Aanbevolen eerste run: start observer → ga naar Honor Camera → laat standaard main-preview even staan → " +
                "schakel handmatig naar 200 MP → wacht 2–3 s → maak één foto → wacht 3–5 s → sluit/verlaten Honor Camera → " +
                "keer terug → MediaStore snapshot → stop → JSON opslaan.",
            11f, true, Color.rgb(220, 225, 234),
        ))
        body.addView(space(8))

        status = label("Nog geen v0.42 report.", 10f, false)
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

    private fun requestMediaPermission() {
        if (Build.VERSION.SDK_INT < 33) {
            status.text = "Android < 13: READ_MEDIA_IMAGES is hier niet van toepassing."
            return
        }
        if (checkSelfPermission(android.Manifest.permission.READ_MEDIA_IMAGES) == PackageManager.PERMISSION_GRANTED) {
            status.text = "READ_MEDIA_IMAGES is al verleend."
            return
        }
        requestPermissions(
            arrayOf(android.Manifest.permission.READ_MEDIA_IMAGES),
            REQUEST_MEDIA_PERMISSION,
        )
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_MEDIA_PERMISSION) {
            val granted = grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED
            status.text =
                if (granted) "READ_MEDIA_IMAGES verleend · v0.42 blijft metadata-only by design."
                else "READ_MEDIA_IMAGES niet volledig verleend · observer draait fail-soft; metadata-query kan beperkt zijn."
        }
    }

    private fun mark(label: String, note: String) {
        startService(
            Intent(this, PassiveMediaStoreCameraTimelineService::class.java)
                .setAction(PassiveMediaStoreCameraTimelineService.ACTION_MARK)
                .putExtra(PassiveMediaStoreCameraTimelineService.EXTRA_LABEL, label)
                .putExtra(PassiveMediaStoreCameraTimelineService.EXTRA_NOTE, note),
        )
    }

    private fun refreshStatus() {
        val f = reportFile()
        saveButton.isEnabled = f.exists() && f.length() > 0L

        val fullImageAccess =
            Build.VERSION.SDK_INT < 33 ||
                checkSelfPermission(android.Manifest.permission.READ_MEDIA_IMAGES) ==
                PackageManager.PERMISSION_GRANTED
        val selectedOnly =
            Build.VERSION.SDK_INT >= 34 &&
                checkSelfPermission(android.Manifest.permission.READ_MEDIA_VISUAL_USER_SELECTED) ==
                PackageManager.PERMISSION_GRANTED

        if (!f.exists()) {
            status.text = buildString {
                append("Nog geen v0.42 report.\n")
                append("READ_MEDIA_IMAGES=").append(fullImageAccess).append('\n')
                if (Build.VERSION.SDK_INT >= 34) {
                    append("READ_MEDIA_VISUAL_USER_SELECTED=").append(selectedOnly).append('\n')
                }
                append("Start daarna de observer.")
            }
            return
        }

        val report = runCatching { JSONObject(f.readText()) }.getOrNull()
        if (report == null) {
            status.text = "Report bestaat maar kon niet als JSON worden gelezen."
            return
        }

        val events = report.optJSONArray("events")
        val count = events?.length() ?: 0
        val last = if (count > 0) events?.optJSONObject(count - 1) else null

        var mediaChanges = 0
        var metadataRows = 0
        var cameraUnavailable = 0
        for (i in 0 until count) {
            val e = events?.optJSONObject(i) ?: continue
            when (e.optString("type")) {
                "MEDIASTORE_CHANGE" -> mediaChanges++
                "CAMERA_UNAVAILABLE" -> cameraUnavailable++
                "MEDIASTORE_CHANGE_METADATA_SNAPSHOT",
                "MEDIASTORE_BASELINE_SNAPSHOT",
                "USER_REQUESTED_MEDIA_SNAPSHOT" -> {
                    val rows = e.optJSONObject("payload")
                        ?.optJSONObject("query")
                        ?.optInt("rowCount", 0) ?: 0
                    metadataRows += rows
                }
            }
        }

        status.text = buildString {
            append("Report: ").append(f.name).append('\n')
            append("events=").append(count)
                .append(" · MediaStore changes=").append(mediaChanges)
                .append(" · metadata rows=").append(metadataRows)
                .append(" · camera unavailable events=").append(cameraUnavailable)
                .append('\n')
            append("READ_MEDIA_IMAGES=").append(fullImageAccess)
                .append(" · selectedOnly=").append(selectedOnly).append('\n')
            append("lastEvent=").append(last?.optString("type", "none")).append('\n')
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
                "v0.42 JSON opgeslagen · alleen systeemzichtbare metadata/events, geen foto-pixels gelezen."
        }.onFailure {
            status.text = "Opslaan faalde: ${it.javaClass.simpleName}: ${it.message}"
        }
    }

    private fun reportFile(): File =
        File(filesDir, PassiveMediaStoreCameraTimelineService.REPORT_FILENAME)

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
        private const val REQUEST_MEDIA_PERMISSION = 64200
        private const val REQUEST_SAVE_JSON = 64242
    }
}

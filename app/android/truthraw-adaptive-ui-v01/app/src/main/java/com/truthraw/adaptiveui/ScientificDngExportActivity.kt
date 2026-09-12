package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.database.Cursor
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView

class ScientificDngExportActivity : Activity() {
    private var selectedSource: Uri? = null
    private var selectedName: String = "Geen RAW geselecteerd"
    private var pendingSourceForSave: Uri? = null
    private var exporting = false
    private var statusText: String =
        "Kies een ondersteunde Honor/MotionCam DNG. De output is een float32 XYZ-D50 LinearRaw compatibility projection van de finalized Scientific Master, nooit nieuw Direct-CFA sensorbewijs."

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        render()
    }

    @Suppress("DEPRECATION")
    private fun chooseRaw() {
        if (exporting) return
        startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }, REQUEST_OPEN_RAW)
    }

    @Suppress("DEPRECATION")
    private fun chooseDestination() {
        if (exporting) return
        val source = selectedSource ?: run {
            statusText = "Kies eerst een bron-DNG."
            render()
            return
        }
        val stem = selectedName.substringBeforeLast('.', selectedName)
        pendingSourceForSave = source
        startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_TRUTHRAW_SCIENTIFIC_LINEAR.dng")
        }, REQUEST_SAVE_DNG)
    }

    @Deprecated("Dependency-light research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_OPEN_RAW) {
            if (resultCode != RESULT_OK || data?.data == null) {
                statusText = "RAW-keuze geannuleerd."
                render()
                return
            }
            val uri = data.data!!
            try {
                contentResolver.takePersistableUriPermission(
                    uri,
                    data.flags and Intent.FLAG_GRANT_READ_URI_PERMISSION,
                )
            } catch (_: Exception) {
                // The active grant is still usable if a provider does not support persistence.
            }
            selectedSource = uri
            selectedName = displayName(uri) ?: "source.dng"
            statusText =
                "Bron geselecteerd. Druk 'Scientific Linear DNG opslaan'. TruthRaw verzegelt de bron opnieuw, haalt de finalized gate, reconstrueert de Scientific Master streaming en commit alleen bij exact dezelfde master-SHA-256."
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_DNG) {
            val source = pendingSourceForSave
            pendingSourceForSave = null
            if (resultCode != RESULT_OK || data?.data == null) {
                statusText = "DNG-export geannuleerd."
                render()
                return
            }
            if (source == null || source != selectedSource) {
                statusText = "Export fail-closed geblokkeerd: bron veranderde tijdens de doelkeuze."
                render()
                return
            }
            startExport(source, data.data!!)
        }
    }

    private fun startExport(source: Uri, destination: Uri) {
        if (exporting) return
        exporting = true
        statusText =
            "Bezig: source seal → dual-illuminant color binding → Scientific Master/TruthRange/Backplane finalization → float32 master replay + exact digest gate → private staged DNG → gekozen doelbestand. Dit kan meerdere minuten duren."
        render()

        Thread({
            val result = ScientificDngProjectionExporter.export(
                contentResolver,
                cacheDir,
                source,
                destination,
            )
            runOnUiThread {
                exporting = false
                statusText = result.fold(
                    onSuccess = { out ->
                        "DNG opgeslagen: ${out.width}×${out.height}, ${formatBytes(out.bytesWritten)}, float32 LinearRaw. " +
                            "pixels=${out.projectedPixels}, tiles=${out.tilesWritten}, negatieve componenten=${out.negativeComponents}, >1 componenten=${out.overOneComponents}. " +
                            "master identity verified=${out.scientificMasterIdentityVerified}. Rol: LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION — representation-only; geen Direct-CFA evidence en geen FULL_PHYSICAL kleurclaim."
                    },
                    onFailure = { error ->
                        "DNG-export fail-closed gestopt: ${error.message ?: error.javaClass.simpleName}"
                    },
                )
                render()
            }
        }, "truthraw-scientific-linear-dng-export").start()
    }

    private fun render() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20), dp(24), dp(20), dp(24))
            setBackgroundColor(Color.rgb(15, 17, 20))
        }
        val scroll = ScrollView(this)
        val body = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }

        body.addView(text("TruthRaw Scientific DNG", 26f, true))
        body.addView(text("Finalized Scientific Master → float32 LinearRaw", 13f, false, muted = true))
        body.addView(space(20))
        body.addView(text(selectedName, 16f, true))
        body.addView(space(12))
        body.addView(button("RAW/DNG kiezen") { chooseRaw() }.apply { isEnabled = !exporting })
        body.addView(space(8))
        body.addView(button("Scientific Linear DNG opslaan") { chooseDestination() }.apply {
            isEnabled = selectedSource != null && !exporting
        })
        body.addView(space(18))

        if (exporting) {
            body.addView(LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                addView(ProgressBar(this@ScientificDngExportActivity).apply { isIndeterminate = true },
                    LinearLayout.LayoutParams(dp(42), dp(42)).apply { marginEnd = dp(12) })
                addView(text("Scientific Master wordt gevalideerd en geëxporteerd…", 14f, true))
            })
            body.addView(space(12))
        }

        body.addView(text(statusText, 13f, false, muted = true))
        body.addView(space(18))
        body.addView(text(
            "De DNG is downstream representatie. De sealed Direct-CFA bron, Scientific Master, TruthRange zero-line, Technical Backplane, frame/evidence=1/1 en kleurautoriteit worden door export niet opgewaardeerd of herschreven.",
            12f,
            false,
            muted = true,
        ))

        scroll.addView(body)
        root.addView(scroll, LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f))
        setContentView(root)
    }

    private fun displayName(uri: Uri): String? {
        var cursor: Cursor? = null
        return try {
            cursor = contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)
            if (cursor != null && cursor.moveToFirst()) cursor.getString(0) else null
        } catch (_: Exception) {
            null
        } finally {
            cursor?.close()
        }
    }

    private fun text(value: String, size: Float, bold: Boolean, muted: Boolean = false): TextView =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(if (muted) Color.rgb(178, 183, 191) else Color.rgb(245, 246, 248))
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }

    private fun button(value: String, action: () -> Unit): Button = Button(this).apply {
        text = value
        isAllCaps = false
        setOnClickListener { action() }
    }

    private fun space(height: Int): View = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.2f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.2f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.2f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_OPEN_RAW = 5201
        private const val REQUEST_SAVE_DNG = 5202
    }
}

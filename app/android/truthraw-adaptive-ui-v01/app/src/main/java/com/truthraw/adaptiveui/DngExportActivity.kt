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

class DngExportActivity : Activity() {
    private var selectedSource: Uri? = null
    private var selectedName: String = "Geen RAW geselecteerd"
    private var statusText: String =
        "Kies een ondersteunde MotionCam/Honor DNG. De export schrijft een reconstructed LinearRaw compatibility projection; nooit nieuw sensorbewijs."
    private var exporting = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        render()
    }

    @Suppress("DEPRECATION")
    private fun chooseRaw() {
        if (exporting) return
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_OPEN_RAW)
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
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_linear_scientific_projection.dng")
        }
        pendingSourceForSave = source
        startActivityForResult(intent, REQUEST_SAVE_DNG)
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
                val takeFlags = data.flags and
                    (Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
                contentResolver.takePersistableUriPermission(uri, takeFlags and Intent.FLAG_GRANT_READ_URI_PERMISSION)
            } catch (_: Exception) {
                // The current granted URI remains usable even when a provider does not support persistence.
            }
            selectedSource = uri
            selectedName = displayName(uri) ?: "source.dng"
            statusText =
                "Bron geselecteerd. Druk 'Linear DNG opslaan'. TruthRaw valideert de bron opnieuw, haalt de finalized gate en vergelijkt de exportreplay met de Scientific-Master-digest vóór er pixels worden geschreven."
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
                statusText = "DNG-export geblokkeerd: bron veranderde tijdens de doelkeuze."
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
            "Bezig: source seal → dual-illuminant color binding → Scientific Master/TruthRange/Backplane finalization → exact master-digest replay → LinearRaw DNG projection. Dit kan enkele minuten duren."
        render()

        Thread({
            val result = LinearDngProjectionExporter.export(contentResolver, source, destination)
            runOnUiThread {
                exporting = false
                statusText = result.fold(
                    onSuccess = { exported ->
                        val scale = "%.6f".format(exported.projectionScale)
                        "DNG opgeslagen: ${exported.width}×${exported.height}, ${formatBytes(exported.bytesWritten)}, ${exported.bitDepth}-bit LinearRaw. " +
                            "Scientific samples=${exported.scientificSamples}; projectionScale=$scale; low-clamp=${exported.clampedLowSamples}; high-clamp=${exported.clampedHighSamples}. " +
                            "Rol: RECONSTRUCTED_LINEAR_DNG_PROJECTION — geen Direct-CFA evidence en geen FULL_PHYSICAL kleurclaim."
                    },
                    onFailure = { error ->
                        "DNG-export fail-closed gestopt: ${error.message ?: error.javaClass.simpleName}"
                    },
                )
                render()
            }
        }, "truthraw-linear-dng-export").start()
    }

    private fun render() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20), dp(24), dp(20), dp(24))
            setBackgroundColor(Color.rgb(15, 17, 20))
        }
        val scroll = ScrollView(this)
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }

        body.addView(text("TruthRaw DNG Export", 26f, true))
        body.addView(text("Finalized Scientific Master → LinearRaw compatibility projection", 13f, false, muted = true))
        body.addView(space(20))
        body.addView(text(selectedName, 16f, true))
        body.addView(space(12))
        body.addView(button("RAW/DNG kiezen") { chooseRaw() })
        body.addView(space(8))
        body.addView(button("Linear DNG opslaan") { chooseDestination() }.apply {
            isEnabled = selectedSource != null && !exporting
        })
        body.addView(space(18))

        if (exporting) {
            body.addView(LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                addView(ProgressBar(this@DngExportActivity).apply { isIndeterminate = true },
                    LinearLayout.LayoutParams(dp(42), dp(42)).apply { marginEnd = dp(12) })
                addView(text("TruthRaw reconstrueert en schrijft…", 14f, true))
            })
            body.addView(space(12))
        }

        body.addView(text(statusText, 13f, false, muted = true))
        body.addView(space(18))
        body.addView(text(
            "Wetenschappelijke grens: de DNG is een downstream representatie. De sealed bron, Scientific Master, zero-line, Backplane en evidence-count worden door export niet gewijzigd.",
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
        private const val REQUEST_OPEN_RAW = 5101
        private const val REQUEST_SAVE_DNG = 5102
        private var pendingSourceForSave: Uri? = null
    }
}

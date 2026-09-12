package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.net.Uri
import android.os.Bundle
import android.os.SystemClock
import android.provider.OpenableColumns
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.Chronometer
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView

class DngExportActivity : Activity() {
    private var pendingRole: ScientificDngRole? = null
    private var sourceUri: Uri? = null
    private var sourceName: String? = null
    private var statusText: String = "Kies eerst een exporttype. Daarna kies je de bron-RAW en de opslaglocatie."
    private var busy = false
    private var startedElapsed: Long? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        render()
    }

    @Suppress("DEPRECATION")
    private fun chooseSource(role: ScientificDngRole) {
        if (busy) return
        pendingRole = role
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_SOURCE)
    }

    @Suppress("DEPRECATION")
    private fun chooseDestination() {
        val role = pendingRole ?: return
        val uri = sourceUri ?: return
        val stem = (sourceName ?: "truthraw_source").substringBeforeLast('.', sourceName ?: "truthraw_source")
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/x-adobe-dng"
            putExtra(Intent.EXTRA_TITLE, "${stem}_${role.suffix}")
        }
        // Keep uri live in state; only the target is selected here.
        if (uri.scheme != null) startActivityForResult(intent, REQUEST_DESTINATION)
    }

    @Deprecated("Platform result bridge is intentionally dependency-light in this research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_SOURCE) {
            if (resultCode != RESULT_OK || data?.data == null) {
                statusText = "Bronselectie geannuleerd."
                render()
                return
            }
            val uri = data.data!!
            try {
                val takeFlags = data.flags and
                    (Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
                contentResolver.takePersistableUriPermission(uri, takeFlags and Intent.FLAG_GRANT_READ_URI_PERMISSION)
            } catch (_: SecurityException) {
                // The current grant is still valid for this activity/run.
            }
            sourceUri = uri
            sourceName = displayName(uri)
            statusText = "Bron gekozen: ${sourceName ?: uri.lastPathSegment ?: "RAW"}. Kies nu de doelbestandslocatie."
            render()
            chooseDestination()
            return
        }

        if (requestCode == REQUEST_DESTINATION) {
            if (resultCode != RESULT_OK || data?.data == null) {
                statusText = "DNG-opslag geannuleerd."
                render()
                return
            }
            val role = pendingRole
            val source = sourceUri
            if (role == null || source == null) {
                statusText = "DNG-export geblokkeerd: bron of projectierol ontbreekt."
                render()
                return
            }
            val destination = data.data!!
            busy = true
            startedElapsed = SystemClock.elapsedRealtime()
            statusText = "Finalized Phase-2 lineage + Scientific Master worden gecontroleerd en DNG wordt tile-by-tile geschreven…"
            render()

            Thread({
                val result = ScientificDngExporter.export(contentResolver, source, destination, role)
                if (result is ScientificDngExportResult.Failed) {
                    try {
                        contentResolver.delete(destination, null, null)
                    } catch (_: Exception) {
                        // Best effort: never report a failed partial output as a valid TruthRaw DNG.
                    }
                }
                runOnUiThread {
                    busy = false
                    startedElapsed = null
                    statusText = when (result) {
                        is ScientificDngExportResult.Success -> {
                            val m = result.metrics
                            buildString {
                                append("DNG opgeslagen · ${m.role.title}\n")
                                append("${m.width}×${m.height} · ${formatBytes(m.bytesWritten.toLong())} · ${m.tileCount} exporttiles\n")
                                append("Scientific Master identity match=${m.scientificMasterIdentityMatched} · Phase-2 lineage=${m.finalizedLineageValidated}\n")
                                append("frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · full master materialized=${m.fullScientificMasterMaterialized}\n")
                                append("source-bound color=${m.sourceMetadataBoundColor} · independent physical color=${m.independentPhysicalColor}\n")
                                append("bronreads tijdens exportpipeline: ${m.sourceTileReadCalls} tiles · ${formatBytes(m.rawPayloadBytesRead.toLong())} RAW-payload")
                            }
                        }
                        is ScientificDngExportResult.Failed -> "DNG-export fail-closed: ${result.reason}"
                    }
                    render()
                }
            }, "truthraw-dng-export").start()
        }
    }

    private fun displayName(uri: Uri): String? {
        return try {
            contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
                if (cursor.moveToFirst()) cursor.getString(0) else null
            }
        } catch (_: Exception) {
            null
        }
    }

    private fun render() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(18), dp(18), dp(18))
            setBackgroundColor(Color.rgb(15, 17, 20))
        }
        val scroll = ScrollView(this)
        val body = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            addView(text("TruthRaw DNG Export", 24f, true))
            addView(text(
                "Downstream compatibility projection · Scientific Master blijft apart · reconstructed CFA is geen gemeten sensorbewijs.",
                12f,
                false,
            ))
            addView(space(16))
            addView(button("Linear DNG maken") { chooseSource(ScientificDngRole.LINEAR_RAW_COMPATIBILITY) })
            addView(space(8))
            addView(button("Reconstructed CFA DNG maken") { chooseSource(ScientificDngRole.RECONSTRUCTED_CFA_COMPATIBILITY) })
            addView(space(16))
            if (busy) {
                addView(LinearLayout(this@DngExportActivity).apply {
                    orientation = LinearLayout.HORIZONTAL
                    gravity = Gravity.CENTER_VERTICAL
                    addView(ProgressBar(this@DngExportActivity).apply { isIndeterminate = true },
                        LinearLayout.LayoutParams(dp(36), dp(36)).apply { marginEnd = dp(10) })
                    addView(LinearLayout(this@DngExportActivity).apply {
                        orientation = LinearLayout.VERTICAL
                        addView(text("Export bezig…", 15f, true))
                        startedElapsed?.let { started ->
                            addView(Chronometer(this@DngExportActivity).apply {
                                base = started
                                setTextColor(Color.LTGRAY)
                                textSize = 12f
                                format = "Looptijd %s"
                                start()
                            })
                        }
                    })
                })
                addView(space(10))
            }
            addView(text(statusText, 13f, false))
            addView(space(14))
            addView(text(
                "Linear DNG = 16-bit camera-native reconstructed RGB (LinearRaw).\n" +
                    "CFA DNG = 16-bit remosaic van die reconstructie volgens het bron-CFA-patroon.\n" +
                    "Beide zijn begrensde compatibility projections; negatieve en >1 Scientific-Master waarden blijven alleen in de Scientific Master zelf.",
                11f,
                false,
            ))
        }
        scroll.addView(body, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        root.addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        setContentView(root)
    }

    private fun text(value: String, size: Float, bold: Boolean): TextView = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(if (bold) Color.WHITE else Color.LTGRAY)
        if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
    }

    private fun button(value: String, action: () -> Unit): Button = Button(this).apply {
        text = value
        isAllCaps = false
        isEnabled = !busy
        setOnClickListener { action() }
    }

    private fun space(height: Int): TextView = TextView(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_SOURCE = 4301
        private const val REQUEST_DESTINATION = 4302
    }
}

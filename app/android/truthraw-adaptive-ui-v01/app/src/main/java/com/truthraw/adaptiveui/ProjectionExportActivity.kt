package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.os.SystemClock
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.Chronometer
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView

class ProjectionExportActivity : Activity() {
    private var selectedJob: RawJob? = null
    private var pendingKind: ProjectionExportKind? = null
    private var exportRunning = false
    private var statusText: String = "Kies eerst één originele RAW/DNG."
    private var startedElapsedMs: Long? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        render()
    }

    @Suppress("DEPRECATION")
    private fun chooseRaw() {
        if (exportRunning) return
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_OPEN_RAW)
    }

    @Suppress("DEPRECATION")
    private fun chooseDestination(kind: ProjectionExportKind) {
        val job = selectedJob ?: return
        if (exportRunning) return
        pendingKind = kind
        statusText = when (kind) {
            ProjectionExportKind.LINEAR_DNG_16 ->
                "Linear DNG doel kiezen · camera-native reconstructed RGB compatibility projection."
            ProjectionExportKind.CFA_DNG_16 ->
                "CFA DNG doel kiezen · normalized Stage-2 reconstructed projection, niet gemeten sensor-counts."
            ProjectionExportKind.SCIENTIFIC_RAWSENSOR_F32 ->
                "Scientific .rawsensor doel kiezen · exact float32 camera-native Scientific Master RGB."
        }
        render()
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = kind.mimeType
            putExtra(Intent.EXTRA_TITLE, stem + kind.suffix)
        }
        startActivityForResult(intent, REQUEST_SAVE_PROJECTION)
    }

    @Deprecated("Dependency-light research activity uses the platform result bridge")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_OPEN_RAW) {
            if (resultCode != RESULT_OK || data?.data == null) {
                statusText = "RAW-selectie geannuleerd."
                render()
                return
            }
            selectedJob = RawIngress.readHandlesOnly(
                contentResolver,
                listOf(data.data!!),
                data.flags,
            ).firstOrNull()
            pendingKind = null
            statusText = selectedJob?.let {
                "Geselecteerd: ${it.source.displayName}. Kies nu bewust één projection-output."
            } ?: "RAW kon niet als documenthandle worden geopend."
            render()
            return
        }

        if (requestCode != REQUEST_SAVE_PROJECTION) return
        val job = selectedJob
        val kind = pendingKind
        pendingKind = null
        if (resultCode != RESULT_OK || data?.data == null) {
            statusText = "Projection-export geannuleerd."
            render()
            return
        }
        if (job == null || kind == null) {
            statusText = "Export geblokkeerd: bron of projectierol veranderde tijdens de dialoog."
            render()
            return
        }

        exportRunning = true
        startedElapsedMs = SystemClock.elapsedRealtime()
        statusText = "Export rekent Scientific Master opnieuw uit, verifieert de digest en schrijft bounded strips…"
        render()
        val destination = data.data!!
        Thread({
            val outcome = ProjectionExporter.export(contentResolver, job, destination, kind)
            runOnUiThread {
                exportRunning = false
                startedElapsedMs = null
                statusText = outcome.fold(
                    onSuccess = { m ->
                        val role = when (kind) {
                            ProjectionExportKind.LINEAR_DNG_16 -> "Linear DNG compatibility projection"
                            ProjectionExportKind.CFA_DNG_16 -> "CFA reconstructed projection"
                            ProjectionExportKind.SCIENTIFIC_RAWSENSOR_F32 -> "Scientific Master .rawsensor"
                        }
                        "$role opgeslagen · ${formatBytes(m.bytesWritten)} · " +
                            "master-digest verified=${m.scientificMasterDigestVerified} · " +
                            "tiles=${m.tilesProcessed} · frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · " +
                            "clamp <0=${m.negativeSamplesClamped}, >1=${m.overOneSamplesClamped}."
                    },
                    onFailure = { error ->
                        "Export fail-closed gestopt: ${error.message ?: error.javaClass.simpleName}. " +
                            "Bij een native failure wordt het doelbestand terug naar 0 bytes getrunceerd."
                    },
                )
                render()
            }
        }, "truthraw-projection-export").start()
    }

    private fun render() {
        val scroll = ScrollView(this)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(18), dp(18), dp(24))
            setBackgroundColor(Color.rgb(15, 17, 20))
        }
        root.addView(text("TruthRaw Export", 25f, true))
        root.addView(text(
            "Downstream projections van exact dezelfde finalized source/Scientific-Master lineage. " +
                "Export creëert geen extra evidence en promoveert source-metadata-kleur niet naar FULL_PHYSICAL.",
            13f,
            false,
        ))
        root.addView(space())
        root.addView(button(if (selectedJob == null) "RAW/DNG kiezen" else "Andere RAW/DNG kiezen") { chooseRaw() })
        selectedJob?.let { job ->
            root.addView(text(job.source.displayName, 16f, true))
            job.source.declaredSizeBytes?.let { root.addView(text("Bron ${formatBytes(it)}", 12f, false)) }
            root.addView(space())

            root.addView(text("Aanbevolen voor Lightroom", 15f, true))
            root.addView(text(
                "Linear DNG: 16-bit camera-native reconstructed RGB. Geen remosaic→demosaic-cyclus.",
                12f,
                false,
            ))
            root.addView(button("Linear DNG opslaan") { chooseDestination(ProjectionExportKind.LINEAR_DNG_16) })

            root.addView(text("RAW-achtige compatibility test", 15f, true))
            root.addView(text(
                "CFA DNG: normalized Stage-2 Bayer-projectie. Dit is reconstructed/corrected representation, niet de originele gemeten fotosite-counts.",
                12f,
                false,
            ))
            root.addView(button("CFA DNG opslaan") { chooseDestination(ProjectionExportKind.CFA_DNG_16) })

            root.addView(text("Exact TruthRaw wetenschappelijk payload", 15f, true))
            root.addView(text(
                ".rawsensor: float32 camera-native reconstructed Scientific Master RGB met source/master lineage-header. Niet bedoeld als Lightroom-container.",
                12f,
                false,
            ))
            root.addView(button("Scientific .rawsensor opslaan") {
                chooseDestination(ProjectionExportKind.SCIENTIFIC_RAWSENSOR_F32)
            })
        }

        root.addView(space())
        if (exportRunning) {
            root.addView(LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                gravity = Gravity.CENTER_VERTICAL
                addView(ProgressBar(this@ProjectionExportActivity).apply { isIndeterminate = true },
                    LinearLayout.LayoutParams(dp(38), dp(38)).apply { marginEnd = dp(10) })
                addView(LinearLayout(this@ProjectionExportActivity).apply {
                    orientation = LinearLayout.VERTICAL
                    addView(text("Export bezig…", 14f, true))
                    startedElapsedMs?.let { started ->
                        addView(Chronometer(this@ProjectionExportActivity).apply {
                            base = started
                            textSize = 12f
                            setTextColor(Color.rgb(172, 178, 187))
                            format = "Looptijd %s"
                            start()
                        })
                    }
                })
            })
        }
        root.addView(text(statusText, 12f, false))
        root.addView(space())
        root.addView(text(
            "Authority-grens: measured source blijft immutable evidence; DNG-uitvoer is projection; " +
                ".rawsensor is een private Scientific-Master-serialisatie. Geen van deze exports verandert de Backplane.",
            11f,
            false,
        ))
        scroll.addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        setContentView(scroll)
    }

    private fun button(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        isAllCaps = false
        isEnabled = !exportRunning
        setOnClickListener { action() }
        layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(8)
        }
    }

    private fun text(value: String, size: Float, bold: Boolean): TextView = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(Color.rgb(240, 242, 246))
        if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        setPadding(0, dp(4), 0, dp(4))
    }

    private fun space() = android.widget.Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(10))
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_OPEN_RAW = 4201
        private const val REQUEST_SAVE_PROJECTION = 4202
    }
}

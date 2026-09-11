package com.truthraw.adaptiveui

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.SystemClock
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.Chronometer
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.Space
import android.widget.TextView
import java.io.IOException

class MainActivity : Activity() {
    private var session = BatchSession()
    private var activeJobId: String? = null
    private var previewState: TilePreviewUiState = TilePreviewUiState.Idle
    private var previewGeneration: Long = 0
    private var loadingStartedAtElapsedMs: Long? = null
    private var pendingJpegJobId: String? = null
    private var jpegStatus: String? = null
    private var empiricalAudit: EmpiricalRunAudit? = null
    private var pendingEmpiricalJobId: String? = null
    private var pendingEmpiricalJson: String? = null
    private var empiricalStatus: String? = null

    private enum class LayoutTier { COMPACT, MEDIUM, EXPANDED }

    private data class Palette(
        val background: Int,
        val surface: Int,
        val surfaceAlt: Int,
        val text: Int,
        val textMuted: Int,
        val accent: Int,
    )

    private val palette: Palette
        get() {
            val dark = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
                Configuration.UI_MODE_NIGHT_YES
            return if (dark) {
                Palette(
                    background = Color.rgb(15, 17, 20),
                    surface = Color.rgb(27, 30, 35),
                    surfaceAlt = Color.rgb(36, 40, 46),
                    text = Color.rgb(245, 246, 248),
                    textMuted = Color.rgb(172, 178, 187),
                    accent = Color.rgb(128, 157, 255),
                )
            } else {
                Palette(
                    background = Color.rgb(246, 247, 249),
                    surface = Color.WHITE,
                    surfaceAlt = Color.rgb(235, 238, 243),
                    text = Color.rgb(24, 27, 31),
                    textMuted = Color.rgb(92, 99, 109),
                    accent = Color.rgb(54, 88, 200),
                )
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        render()
    }

    override fun onDestroy() {
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        super.onDestroy()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        render()
    }

    @Suppress("DEPRECATION")
    private fun launchRawPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_OPEN_RAW)
    }

    @Suppress("DEPRECATION")
    private fun launchJpegExport(job: RawJob) {
        val ready = previewState as? TilePreviewUiState.Ready ?: return
        if (ready.jobId != job.id) return
        pendingJpegJobId = job.id
        jpegStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "image/jpeg"
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_finalized_scientific_preview.jpg")
        }
        startActivityForResult(intent, REQUEST_SAVE_JPEG)
    }

    @Suppress("DEPRECATION")
    private fun launchEmpiricalExport(job: RawJob) {
        val audit = empiricalAudit ?: return
        if (job.id != activeJobId) return
        pendingEmpiricalJobId = job.id
        pendingEmpiricalJson = EmpiricalReportEncoder.toJson(this, job, previewState, audit)
        empiricalStatus = null
        val stem = job.source.displayName.substringBeforeLast('.', job.source.displayName)
        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(Intent.EXTRA_TITLE, "${stem}_truthraw_honor_empirical_v0_1.json")
        }
        startActivityForResult(intent, REQUEST_SAVE_EMPIRICAL_JSON)
    }

    @Deprecated("Platform result bridge is intentionally dependency-light in this research prototype")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_SAVE_JPEG) {
            val expectedJob = pendingJpegJobId
            pendingJpegJobId = null
            if (resultCode != RESULT_OK || data?.data == null) {
                jpegStatus = "JPEG-export geannuleerd."
                render()
                return
            }
            val ready = previewState as? TilePreviewUiState.Ready
            if (expectedJob == null || ready == null || ready.jobId != expectedJob) {
                jpegStatus = "JPEG-export geblokkeerd: actieve preview veranderde tijdens de bestandsdialoog."
                render()
                return
            }
            jpegStatus = try {
                val stream = contentResolver.openOutputStream(data.data!!, "w")
                    ?: throw IOException("Documentprovider gaf geen outputstream.")
                stream.use { PortablePreviewEncoder.encodeJpeg(ready.bitmap, it) }
                "JPEG opgeslagen · sRGB-projectie van finalized Scientific Preview · geen Scientific Master/evidence."
            } catch (error: Exception) {
                "JPEG-export faalde: ${error.message ?: error.javaClass.simpleName}"
            }
            render()
            return
        }

        if (requestCode == REQUEST_SAVE_EMPIRICAL_JSON) {
            val expectedJob = pendingEmpiricalJobId
            val report = pendingEmpiricalJson
            pendingEmpiricalJobId = null
            pendingEmpiricalJson = null
            if (resultCode != RESULT_OK || data?.data == null) {
                empiricalStatus = "Empirical JSON-export geannuleerd."
                render()
                return
            }
            if (expectedJob == null || expectedJob != activeJobId || report == null) {
                empiricalStatus = "Empirical JSON-export geblokkeerd: actieve run veranderde tijdens de bestandsdialoog."
                render()
                return
            }
            empiricalStatus = try {
                val stream = contentResolver.openOutputStream(data.data!!, "w")
                    ?: throw IOException("Documentprovider gaf geen outputstream.")
                stream.bufferedWriter(Charsets.UTF_8).use { it.write(report) }
                "Empirical JSON opgeslagen · meetlaag only · verandert geen Scientific Master/authority."
            } catch (error: Exception) {
                "Empirical JSON-export faalde: ${error.message ?: error.javaClass.simpleName}"
            }
            render()
            return
        }

        if (requestCode != REQUEST_OPEN_RAW || resultCode != RESULT_OK || data == null) return

        val uris = buildList {
            data.data?.let(::add)
            val clip: ClipData? = data.clipData
            if (clip != null) {
                for (index in 0 until clip.itemCount) add(clip.getItemAt(index).uri)
            }
        }

        val jobs = RawIngress.readHandlesOnly(contentResolver, uris, data.flags)
        session = session.withJobs(jobs)
        val first = session.jobs.firstOrNull()
        jpegStatus = null
        empiricalStatus = null
        empiricalAudit = null
        if (first == null) {
            activeJobId = null
            loadingStartedAtElapsedMs = null
            previewState = TilePreviewUiState.Idle
            render()
        } else {
            selectJob(first)
        }
    }

    private fun selectJob(job: RawJob) {
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        ++previewGeneration
        activeJobId = job.id
        loadingStartedAtElapsedMs = null
        previewState = TilePreviewUiState.Idle
        jpegStatus = null
        empiricalStatus = null
        empiricalAudit = null
        pendingJpegJobId = null
        pendingEmpiricalJobId = null
        pendingEmpiricalJson = null
        render()
    }

    private fun requestPreview(job: RawJob) {
        (previewState as? TilePreviewUiState.Ready)?.bitmap?.recycle()
        activeJobId = job.id
        jpegStatus = null
        empiricalStatus = null
        empiricalAudit = null
        pendingJpegJobId = null
        pendingEmpiricalJobId = null
        pendingEmpiricalJson = null
        val generation = ++previewGeneration
        loadingStartedAtElapsedMs = SystemClock.elapsedRealtime()
        previewState = TilePreviewUiState.Loading(job.id)
        val frameSampler = UiFramePacingSampler().also { it.start() }
        render()
        Thread({
            val result = EmpiricalPreviewRunner.run(this@MainActivity, contentResolver, job)
            runOnUiThread {
                val pacing = frameSampler.stop()
                if (generation != previewGeneration || activeJobId != job.id) {
                    (result.state as? TilePreviewUiState.Ready)?.bitmap?.recycle()
                    return@runOnUiThread
                }
                loadingStartedAtElapsedMs = null
                previewState = result.state
                empiricalAudit = result.audit.copy(framePacing = pacing)
                render()
            }
        }, "truthraw-preview-${job.id.take(8)}").start()
    }

    private fun render() {
        val tier = currentLayoutTier()
        val root = vertical().apply {
            setBackgroundColor(palette.background)
            setPadding(dp(12), 0, dp(12), dp(12))
        }

        root.addView(topBar(tier))
        root.addView(
            when (tier) {
                LayoutTier.COMPACT -> compactLayout()
                LayoutTier.MEDIUM -> mediumLayout()
                LayoutTier.EXPANDED -> expandedLayout()
            },
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f),
        )

        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(dp(12) + bars.left, bars.top, dp(12) + bars.right, dp(12) + bars.bottom)
            insets
        }

        setContentView(root)
    }

    private fun topBar(tier: LayoutTier): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(4), dp(8), dp(4), dp(8))

        addView(vertical().apply {
            addView(label("TruthRaw", 22f, bold = true))
            addView(label("${tier.name.lowercase().replaceFirstChar { it.uppercase() }} layout · ${session.selectedCount} RAW geselecteerd", 12f, muted = true))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        addView(actionButton("RAW kiezen") { launchRawPicker() })
    }

    private fun compactLayout(): View = vertical().apply {
        addView(previewPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        addView(routePane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        if (session.jobs.isNotEmpty()) {
            addView(jobStrip(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(132)))
        }
    }

    private fun mediumLayout(): View = horizontal().apply {
        addView(vertical().apply {
            addView(jobListPane(), LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
            addView(routePane())
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.38f).apply { marginEnd = dp(8) })

        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.62f))
    }

    private fun expandedLayout(): View = horizontal().apply {
        addView(jobListPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f).apply { marginEnd = dp(8) })
        addView(previewPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.50f).apply { marginEnd = dp(8) })
        addView(toolsPane(), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.MATCH_PARENT, 0.25f))
    }

    private fun previewPane(): View = card().apply {
        val active = session.jobs.firstOrNull { it.id == activeJobId } ?: session.jobs.firstOrNull()
        if (active == null) {
            gravity = Gravity.CENTER
            addView(label("Selecteer één of meerdere RAW-bestanden", 20f, bold = true).apply { gravity = Gravity.CENTER })
            addView(space(10))
            addView(label(
                "De ingang bewaart alleen documenthandles en metadata. RAW-sensorwaarden worden pas tile-voor-tile opgevraagd.",
                13f,
                muted = true,
            ).apply { gravity = Gravity.CENTER })
            return@apply
        }

        addView(label(active.source.displayName, 16f, bold = true))
        addView(label(
            "Finalized Scientific Preview · Scientific Master/TruthRange/Backplane-lineage vereist vóór vrijgave",
            11f,
            muted = true,
        ))
        addView(space(8))

        when (val state = previewState) {
            TilePreviewUiState.Idle -> {
                addView(label(
                    "RAW is geselecteerd en nog niet verwerkt. Start hieronder bewust de empirical + finalized TruthRaw-route.",
                    13f,
                    muted = true,
                ))
                addView(space(8))
                addView(actionButton("Start TruthRaw") { requestPreview(active) })
            }
            is TilePreviewUiState.Loading -> {
                addView(horizontal().apply {
                    gravity = Gravity.CENTER_VERTICAL
                    addView(ProgressBar(this@MainActivity).apply { isIndeterminate = true }, LinearLayout.LayoutParams(dp(36), dp(36)).apply {
                        marginEnd = dp(10)
                    })
                    addView(vertical().apply {
                        addView(label("Bezig met verwerken…", 15f, bold = true))
                        loadingStartedAtElapsedMs?.let { started ->
                            addView(Chronometer(this@MainActivity).apply {
                                base = started
                                textSize = 12f
                                setTextColor(palette.textMuted)
                                format = "Looptijd %s"
                                start()
                            })
                        }
                    })
                })
                addView(space(8))
                addView(label(
                    "Empirical pre-probe → onveranderde finalized route → empirical post-probe · SHA-256, DNG-profiel, RSS, latency, thermiek en framepacing worden gemeten.",
                    13f,
                    muted = true,
                ))
                addView(label(
                    "De route kan op een echte volledige RAW merkbaar rekenen. Zolang de looptijd doorloopt en Android de app niet als fout beëindigt, is de worker actief.",
                    11f,
                    muted = true,
                ))
            }
            is TilePreviewUiState.Failed -> {
                addView(label("Preview fail-closed geblokkeerd", 14f, bold = true))
                addView(label(state.reason, 12f, muted = true))
                addView(space(6))
                addView(actionButton("Opnieuw proberen") { requestPreview(active) })
            }
            is TilePreviewUiState.Ready -> {
                val image = ImageView(this@MainActivity).apply {
                    setImageBitmap(state.bitmap)
                    adjustViewBounds = true
                    scaleType = ImageView.ScaleType.FIT_CENTER
                    contentDescription = "Finalized TruthRaw Scientific Preview voor ${active.source.displayName}"
                }
                addView(image, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
                addView(space(6))
                val m = state.metrics
                addView(label(
                    "${m.previewAuthority.name} · scientific release=${m.scientificPreviewReleaseAllowed} · stronger physical-color claim=${m.scientificClaimAllowed}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "bron ${m.sourceWidth}×${m.sourceHeight} · source resident ≤ ${formatBytes(m.sourceResidentUpperBoundBytes.toLong())} · logical resident ≤ ${formatBytes(m.logicalResidentUpperBoundBytes.toLong())}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "RAW gelezen ${formatBytes(m.rawPayloadBytesRead.toLong())} · tile passes ${m.tilesProcessedPass1}/${m.tilesProcessedPass2} · fullRawMaterialized=${m.fullRawMaterialized}",
                    10f,
                    muted = true,
                ))
                addView(label(
                    "frame/evidence=${m.physicalFrameCount}/${m.independentEvidenceCount} · ForwardMatrix=${m.usedForwardMatrix} · CameraCalibration toegepast=${m.cameraCalibrationApplied}",
                    10f,
                    muted = true,
                ))
                addView(space(6))
                addView(actionButton("JPEG preview opslaan") { launchJpegExport(active) })
                jpegStatus?.let { addView(label(it, 10f, muted = true)) }
            }
        }

        empiricalAudit?.let { audit ->
            addView(space(8))
            addView(label("Honor/MotionCam empirical v0.1", 13f, bold = true))
            val probe = audit.preProbe
            val shaShort = probe.sourceSha256?.let { if (it.length > 16) "${it.take(16)}…" else it } ?: "onbekend"
            addView(label(
                "source SHA=$shaShort · stabiel=${audit.sourceStableAcrossHarness} · DNG-kleur=${probe.metadataForm} · probe status=${probe.statusCode}",
                10f,
                muted = true,
            ))
            addView(label(
                "pipeline=${"%.1f".format(audit.runtime.pipelineWallMs)} ms · worker CPU=${"%.1f".format(audit.runtime.workerCpuMs)} ms · PSS piek=${formatBytes(audit.runtime.pssPeakKb.toLong() * 1024L)}",
                10f,
                muted = true,
            ))
            val pacing = audit.framePacing
            addView(label(
                "thermal ${thermalLabel(audit.runtime.thermalStart)}→${thermalLabel(audit.runtime.thermalEnd)} (piek ${thermalLabel(audit.runtime.thermalPeak)}) · UI p95=${pacing?.p95Ms?.let { "%.1f ms".format(it) } ?: "n/a"}",
                10f,
                muted = true,
            ))
            addView(label(
                "Meetlaag only: deze waarden sturen geen reconstructie, kleurmatrix, TruthRange, zero-line of scientific authority.",
                10f,
                muted = true,
            ))
            addView(space(5))
            addView(actionButton("Empirical JSON opslaan") { launchEmpiricalExport(active) })
            empiricalStatus?.let { addView(label(it, 10f, muted = true)) }
        }
    }

    private fun routePane(): View = card().apply {
        addView(label("Uitkomst", 16f, bold = true))
        addView(space(6))

        when (session.jobs.size) {
            0 -> addView(label("Kies eerst RAW-bestanden.", 13f, muted = true))
            1 -> {
                addView(routeButton("1 upload → 1 uitkomst", InputRoute.SINGLE_ONE_OUTPUT))
                addView(routeButton("1 upload → meerdere uitkomsten", InputRoute.SINGLE_MULTIPLE_OUTPUTS))
            }
            else -> {
                addView(routeButton("Afzonderlijk verwerken", InputRoute.BATCH_INDEPENDENT))
                addView(routeButton("Verbeterde foto", InputRoute.MULTI_CAPTURE_ENHANCED))
                addView(routeButton("HDR", InputRoute.MULTI_CAPTURE_HDR))
                addView(label(
                    "Fusion/HDR is nog alleen een expliciete kandidaatroute. Frames worden niet automatisch als gezamenlijk bewijs behandeld.",
                    11f,
                    muted = true,
                ))
            }
        }
    }

    private fun toolsPane(): View = vertical().apply {
        addView(routePane())
        addView(space(8))
        addView(card().apply {
            addView(label("Kamers", 16f, bold = true))
            addView(label("Natural", 13f))
            addView(label("Detail", 13f))
            addView(label("Illumination", 13f))
            addView(label("Export", 13f))
            addView(space(8))
            addView(label("UI-keuzes veranderen geen scientific authority, zero-line of scene-ISO-contract.", 11f, muted = true))
        })
    }

    private fun jobListPane(): View = card().apply {
        addView(label("Ingang", 16f, bold = true))
        addView(label("${session.selectedCount} onafhankelijke bronhandle(s)", 12f, muted = true))
        addView(space(6))
        val scroll = ScrollView(this@MainActivity)
        scroll.addView(vertical().apply {
            if (session.jobs.isEmpty()) {
                addView(label("Nog geen RAW geselecteerd.", 13f, muted = true))
            } else {
                session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
            }
        })
        addView(scroll, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
    }

    private fun jobStrip(): View = ScrollView(this).apply {
        addView(vertical().apply {
            session.jobs.forEachIndexed { index, job -> addView(jobRow(index, job)) }
        })
    }

    private fun jobRow(index: Int, job: RawJob): View = vertical().apply {
        setPadding(dp(10), dp(8), dp(10), dp(8))
        background = rounded(palette.surfaceAlt, 12f)
        addView(label("${if (job.id == activeJobId) "▶ " else ""}${index + 1}. ${job.source.displayName}", 13f, bold = true))
        val size = job.source.declaredSizeBytes?.let { " · ${formatBytes(it)}" } ?: ""
        addView(label("${job.state.name.lowercase()}$size", 11f, muted = true))
        addView(label("lineage: afzonderlijk totdat expliciete fusion-validatie bestaat", 10f, muted = true))
        setOnClickListener { selectJob(job) }
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(6)
        }
    }

    private fun routeButton(text: String, route: InputRoute): View = actionButton(
        if (session.route == route) "✓ $text" else text,
    ) {
        session = session.copy(route = route)
        render()
    }.also {
        it.layoutParams = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            bottomMargin = dp(5)
        }
    }

    private fun card(): LinearLayout = vertical().apply {
        setPadding(dp(16), dp(16), dp(16), dp(16))
        background = rounded(palette.surface, 18f)
    }

    private fun vertical(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }

    private fun label(text: String, sizeSp: Float, bold: Boolean = false, muted: Boolean = false): TextView =
        TextView(this).apply {
            this.text = text
            textSize = sizeSp
            setTextColor(if (muted) palette.textMuted else palette.text)
            if (bold) setTypeface(typeface, Typeface.BOLD)
        }

    private fun actionButton(text: String, action: () -> Unit): Button = Button(this).apply {
        this.text = text
        isAllCaps = false
        setTextColor(palette.text)
        background = rounded(palette.surfaceAlt, 14f)
        setOnClickListener { action() }
    }

    private fun rounded(color: Int, radiusDp: Float): GradientDrawable = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(radiusDp).toFloat()
    }

    private fun space(heightDp: Int): View = Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(heightDp))
    }

    private fun currentLayoutTier(): LayoutTier {
        val widthPx = windowManager.currentWindowMetrics.bounds.width()
        val widthDp = widthPx / resources.displayMetrics.density
        return when {
            widthDp >= 840f -> LayoutTier.EXPANDED
            widthDp >= 600f -> LayoutTier.MEDIUM
            else -> LayoutTier.COMPACT
        }
    }

    private fun thermalLabel(status: Int): String = when (status) {
        0 -> "NONE"
        1 -> "LIGHT"
        2 -> "MODERATE"
        3 -> "SEVERE"
        4 -> "CRITICAL"
        5 -> "EMERGENCY"
        6 -> "SHUTDOWN"
        else -> "UNKNOWN($status)"
    }

    private fun formatBytes(bytes: Long): String = when {
        bytes >= 1024L * 1024L * 1024L -> "%.1f GB".format(bytes / (1024.0 * 1024.0 * 1024.0))
        bytes >= 1024L * 1024L -> "%.1f MB".format(bytes / (1024.0 * 1024.0))
        bytes >= 1024L -> "%.1f KB".format(bytes / 1024.0)
        else -> "$bytes B"
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
    private fun dp(value: Float): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_OPEN_RAW = 4101
        private const val REQUEST_SAVE_JPEG = 4102
        private const val REQUEST_SAVE_EMPIRICAL_JSON = 4103
    }
}

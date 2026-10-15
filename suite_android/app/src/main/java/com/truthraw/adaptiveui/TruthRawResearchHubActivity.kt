package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import org.json.JSONArray
import org.json.JSONObject

class TruthRawResearchHubActivity : Activity() {
    private val bg = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val ink = DrawVisualTheme.INK
    private val muted = DrawVisualTheme.MUTED
    private val blue = DrawVisualTheme.BLUE

    private var pendingGlobalJson: String? = null
    private var pendingSourceCapabilityDiagnosticJson: String? = null
    private var statusMessage: String? = null
    private var statusPhase: TruthRawResearchStatusVisualV01.Phase? = null
    private var statusStartedAtMs: Long = 0L
    private var statusFinishedAtMs: Long? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)
        render()
    }

    private fun render() {
        val root = vertical().apply {
            setBackgroundColor(bg)
            setPadding(dp(18), dp(12), dp(18), dp(24))
            addView(header())
            addView(space(16))

            addView(card("Onderzoek & JSON").apply {
                addView(body(
                    "Centrale ingang voor de Free World onderzoekslaag. Normale fotografie blijft via Bestand of Camera werken; deze pagina is voor multi-observation, relation-records en machine-readable onderzoeksexports. Geïmporteerde relation-records worden canonical gededupliceerd en mogen alleen kandidaten voeden wanneer hun source-SHA’s aan de actuele observatieset zijn gebonden.",
                    12.5f,
                ))
                addView(space(10))
                addView(action("Open Multi-observation / JSON werkbank") {
                    openWorkbench(false, false)
                })
                addView(space(8))
                addView(action("Open RAW/DNG direct in onderzoekswerkbank") {
                    openWorkbench(true, false)
                })
                addView(space(8))
                addView(action("Importeer relation-based Calibration Observation Records") {
                    openWorkbench(false, true)
                })
            })

            addView(space(12))
            addView(card("Source Capability · real-device diagnostic").apply {
                addView(body(
                    "Exporteert uitsluitend de reeds opgeslagen source_capability_real_device_diagnostic_v0_1 uit de volledige UniversalSourceProfile(s). Geen nieuwe parser, bronhash, ledger, pixelread, Scientific Master-mutatie of promotion. Als de huidige profielen nog van een oudere APK zijn, blijft deze export fail-closed en moet Universele Analyse één keer opnieuw worden uitgevoerd.",
                    12f,
                ))
                addView(space(8))
                addView(action("Exporteer Source Capability Real-Device Diagnostic v0.1 · JSON") {
                    exportSourceCapabilityRealDeviceDiagnostic()
                })
            })

            addView(space(12))
            addView(card("Globale Research Snapshot · JSON").apply {
                addView(body(
                    "Exporteert één bron-onafhankelijke kaart van routes, geïmplementeerde candidate-runtimes, promotion-gates en permanente wetenschappelijke grenzen. Dit JSON-bestand bevat géén fotometing en is nooit calibratiebewijs.",
                    12f,
                ))
                addView(space(8))
                addView(action("Exporteer Global Research Snapshot v0.1 · JSON") {
                    statusStartedAtMs = System.currentTimeMillis()
                    statusFinishedAtMs = null
                    statusPhase = TruthRawResearchStatusVisualV01.Phase.RUNNING
                    statusMessage = "Global Research Snapshot voorbereiden"
                    pendingGlobalJson =
                        GlobalResearchSnapshotV01.build().toString(2) + "\n"
                    render()
                    val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                        addCategory(Intent.CATEGORY_OPENABLE)
                        type = "application/json"
                        putExtra(
                            Intent.EXTRA_TITLE,
                            "DRAW_GLOBAL_RESEARCH_SNAPSHOT_v0_1.json",
                        )
                    }
                    @Suppress("DEPRECATION")
                    startActivityForResult(
                        intent,
                        REQUEST_SAVE_GLOBAL_RESEARCH_JSON,
                    )
                })
                val phase = statusPhase
                val message = statusMessage
                if (phase != null && message != null) {
                    addView(space(9))
                    addView(
                        TruthRawResearchStatusVisualV01.build(
                            context = this@TruthRawResearchHubActivity,
                            message = message,
                            phase = phase,
                            startedAtWallMs = statusStartedAtMs,
                            finishedAtWallMs = statusFinishedAtMs,
                        ),
                    )
                }
            })

            addView(space(12))
            addView(card("Welke JSON gebruik je wanneer?").apply {
                addView(step(
                    "1 · Eén bron begrijpen",
                    "Na Universele Analyse: Source Capability Real-Device Diagnostic, Observation Optical Field Chart en Universal Observation & Calibration Atlas. De diagnostic is de directe read-only toestelcontrole van de bestaande Source Capability runtime-envelope.",
                ))
                addView(step(
                    "2 · Meerdere bronnen vergelijken",
                    "Field Response Repeatability en Observation–World Field Separation. Voor herhaalde observaties; geen lensprofiel of correctiepromotie.",
                ))
                addView(step(
                    "3 · Hele Free World-sessiestatus",
                    "Free World Observation Geometry Foundation. Dit is de uitgebreide per-session onderzoeks-export met processing- én physical-evidence lineage, record-session binding, geometry, atlas, uncertainty en candidate-runtimes.",
                ))
                addView(step(
                    "4 · Projectarchitectuur zonder foto",
                    "Global Research Snapshot. Gebruik dit wanneer je alleen wilt zien wat D.RAW kan en welke promotion-gates nog dichtstaan.",
                ))
            })

            addView(space(12))
            addView(card("Teststatus blijft zichtbaar").apply {
                addView(body(
                    "Onderzoek/tests behouden het bestaande punt + timer-principe: groen bij normaal lopend of gereed, rood bij fout, live Looptijd tijdens uitvoering en daarna Gereed in / Gestopt na. Deze status verandert nooit scientific authority.",
                    12f,
                ))
            })

            addView(space(12))
            addView(action("Wat is geïmplementeerd en hoe gebruik ik het?") {
                startActivity(
                    Intent(
                        this@TruthRawResearchHubActivity,
                        TruthRawImplementationGuideActivity::class.java,
                    ),
                )
            })
            addView(space(8))
            addView(action("Universele camera openen") {
                startActivity(
                    Intent(
                        this@TruthRawResearchHubActivity,
                        UniversalPhysicalCaptureActivity::class.java,
                    ),
                )
            })
            addView(space(18))
            addView(
                DrawVisualTheme.brandFooter(
                    this@TruthRawResearchHubActivity,
                    72,
                ),
            )
        }

        setContentView(
            ScrollView(this).apply {
                isFillViewport = true
                setBackgroundColor(bg)
                setOnApplyWindowInsetsListener { view, insets ->
                    val bars = insets.getInsets(WindowInsets.Type.systemBars())
                    view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                    insets
                }
                addView(root)
            },
        )
    }

    private fun buildSourceCapabilityRealDeviceDiagnosticExport(): JSONObject? {
        val session =
            ResearchWorkbenchSessionStoreV01.load(
                filesDir,
            ) ?: return null
        if (session.jobs.isEmpty()) return null

        val profiles =
            ResearchUniversalProfileStoreV01.load(
                filesDir = filesDir,
                session = session,
            )
        val diagnostics = JSONArray()

        for (job in session.jobs) {
            val profile = profiles[job.id] ?: continue
            val diagnostic =
                profile.optJSONObject(
                    "source_capability_real_device_diagnostic_v0_1",
                ) ?: continue

            diagnostics.put(
                JSONObject()
                    .put("job_id", job.id)
                    .put("display_name", job.source.displayName)
                    .put(
                        "source_sha256",
                        profile.optString("source_sha256", "UNKNOWN"),
                    )
                    .put(
                        "profile_cache_generation",
                        profile.optString(
                            "profile_cache_generation",
                            "UNKNOWN",
                        ),
                    )
                    .put(
                        "diagnostic",
                        JSONObject(diagnostic.toString()),
                    ),
            )
        }

        if (diagnostics.length() == 0) return null

        return JSONObject()
            .put(
                "schema",
                "D.RAW/SourceCapabilityRealDeviceDiagnosticExport/0.1",
            )
            .put("status", "READ_ONLY_REAL_DEVICE_DIAGNOSTIC_EXPORT")
            .put("test_build_marker", "SOURCE_CAPABILITY_DIAGNOSTIC_EXPORT_V01")
            .put("diagnostic_count", diagnostics.length())
            .put("diagnostics", diagnostics)
            .put("source_values_modified", false)
            .put("creates_new_evidence", false)
            .put("scientific_writeback_allowed", false)
    }

    private fun exportSourceCapabilityRealDeviceDiagnostic() {
        statusStartedAtMs = System.currentTimeMillis()
        statusFinishedAtMs = null
        statusPhase = TruthRawResearchStatusVisualV01.Phase.RUNNING
        statusMessage = "Source Capability real-device diagnostic voorbereiden"

        val report = buildSourceCapabilityRealDeviceDiagnosticExport()
        if (report == null) {
            pendingSourceCapabilityDiagnosticJson = null
            statusFinishedAtMs = System.currentTimeMillis()
            statusPhase = TruthRawResearchStatusVisualV01.Phase.ERROR
            statusMessage =
                "Geen actuele diagnostic gevonden · open de onderzoekswerkbank en voer ‘Analyseer alle geselecteerde bronnen universeel’ één keer opnieuw uit met deze APK."
            render()
            return
        }

        pendingSourceCapabilityDiagnosticJson = report.toString(2) + "\n"
        render()

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "application/json"
            putExtra(
                Intent.EXTRA_TITLE,
                "DRAW_SOURCE_CAPABILITY_REAL_DEVICE_DIAGNOSTIC_v0_1.json",
            )
        }
        @Suppress("DEPRECATION")
        startActivityForResult(
            intent,
            REQUEST_SAVE_SOURCE_CAPABILITY_DIAGNOSTIC_JSON,
        )
    }

    private fun openWorkbench(
        autoRawPicker: Boolean,
        autoRelationPicker: Boolean,
    ) {
        startActivity(
            Intent(this, MainActivity::class.java).apply {
                flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                putExtra(MainActivity.EXTRA_OPEN_RESEARCH_WORKBENCH, true)
                if (autoRawPicker) {
                    putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                }
                if (autoRelationPicker) {
                    putExtra(
                        MainActivity.EXTRA_AUTO_OPEN_CALIBRATION_RECORD_PICKER,
                        true,
                    )
                }
            },
        )
    }

    @Deprecated("Legacy activity result retained for project API compatibility.")
    override fun onActivityResult(
        requestCode: Int,
        resultCode: Int,
        data: Intent?,
    ) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_SAVE_SOURCE_CAPABILITY_DIAGNOSTIC_JSON) {
            statusFinishedAtMs = System.currentTimeMillis()

            if (resultCode != RESULT_OK) {
                pendingSourceCapabilityDiagnosticJson = null
                statusPhase = TruthRawResearchStatusVisualV01.Phase.CANCELLED
                statusMessage = "Source Capability diagnostic opslaan geannuleerd"
                render()
                return
            }

            val uri = data?.data
            val json = pendingSourceCapabilityDiagnosticJson
            pendingSourceCapabilityDiagnosticJson = null

            if (uri == null || json == null) {
                statusPhase = TruthRawResearchStatusVisualV01.Phase.ERROR
                statusMessage = "Source Capability diagnostic kon niet worden opgeslagen"
                render()
                return
            }

            val error =
                runCatching {
                    contentResolver.openOutputStream(uri, "wt")
                        ?.bufferedWriter()
                        ?.use { it.write(json) }
                        ?: error("Geen outputstream")
                }.exceptionOrNull()

            if (error == null) {
                statusPhase = TruthRawResearchStatusVisualV01.Phase.SUCCESS
                statusMessage =
                    "Source Capability Real-Device Diagnostic v0.1 opgeslagen · read-only · writeback=false"
            } else {
                statusPhase = TruthRawResearchStatusVisualV01.Phase.ERROR
                statusMessage =
                    "Opslaan faalde: " +
                        (error.message ?: error.javaClass.simpleName)
            }
            render()
            return
        }

        if (requestCode != REQUEST_SAVE_GLOBAL_RESEARCH_JSON) return

        statusFinishedAtMs = System.currentTimeMillis()

        if (resultCode != RESULT_OK) {
            pendingGlobalJson = null
            statusPhase = TruthRawResearchStatusVisualV01.Phase.CANCELLED
            statusMessage = "Global Research Snapshot opslaan geannuleerd"
            render()
            return
        }

        val uri = data?.data
        val json = pendingGlobalJson
        pendingGlobalJson = null

        if (uri == null || json == null) {
            statusPhase = TruthRawResearchStatusVisualV01.Phase.ERROR
            statusMessage = "Global Research Snapshot kon niet worden opgeslagen"
            render()
            return
        }

        val error =
            runCatching {
                contentResolver.openOutputStream(uri, "wt")
                    ?.bufferedWriter()
                    ?.use { it.write(json) }
                    ?: error("Geen outputstream")
            }.exceptionOrNull()

        if (error == null) {
            statusPhase = TruthRawResearchStatusVisualV01.Phase.SUCCESS
            statusMessage = "Global Research Snapshot v0.1 opgeslagen"
        } else {
            statusPhase = TruthRawResearchStatusVisualV01.Phase.ERROR
            statusMessage =
                "Opslaan faalde: " +
                    (error.message ?: error.javaClass.simpleName)
        }
        render()
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(
            TextView(this@TruthRawResearchHubActivity).apply {
                text = "‹"
                textSize = 36f
                setTextColor(ink)
                gravity = Gravity.CENTER
                setOnClickListener { finish() }
            },
            LinearLayout.LayoutParams(dp(48), dp(48)).apply {
                marginEnd = dp(8)
            },
        )
        addView(
            vertical().apply {
                addView(title("D.RAW · Research & JSON", 24f))
                addView(
                    body(
                        "Free World onderzoek · centrale praktische ingang",
                        11.5f,
                    ),
                )
            },
            LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f,
            ),
        )
    }

    private fun step(label: String, explanation: String): View =
        vertical().apply {
            addView(title(label, 13.5f))
            addView(space(3))
            addView(body(explanation, 11.3f))
            addView(space(9))
        }

    private fun card(heading: String): LinearLayout =
        vertical().apply {
            setPadding(dp(15), dp(15), dp(15), dp(15))
            background = rounded(surface, DrawVisualTheme.BORDER, 18f)
            addView(title(heading, 17f))
            addView(space(7))
        }

    private fun action(label: String, onClick: () -> Unit): View =
        TextView(this).apply {
            text = label
            textSize = 14f
            setTextColor(ink)
            gravity = Gravity.CENTER
            setPadding(dp(14), dp(13), dp(14), dp(13))
            background = rounded(DrawVisualTheme.PAPER_BLUE, blue, 15f)
            setOnClickListener { onClick() }
        }

    private fun rounded(
        fill: Int,
        stroke: Int,
        radius: Float,
    ): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = radius * resources.displayMetrics.density
            setColor(fill)
            setStroke(dp(1), stroke)
        }

    private fun vertical() =
        LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
        }

    private fun horizontal() =
        LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
        }

    private fun space(height: Int) =
        View(this).apply {
            layoutParams = LinearLayout.LayoutParams(1, dp(height))
        }

    private fun title(value: String, size: Float) =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(ink)
            setTypeface(typeface, Typeface.BOLD)
        }

    private fun body(value: String, size: Float) =
        TextView(this).apply {
            text = value
            textSize = size
            setTextColor(muted)
            setLineSpacing(0f, 1.12f)
        }

    private fun dp(value: Int): Int =
        (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        private const val REQUEST_SAVE_GLOBAL_RESEARCH_JSON = 4701
        private const val REQUEST_SAVE_SOURCE_CAPABILITY_DIAGNOSTIC_JSON = 4702
    }
}
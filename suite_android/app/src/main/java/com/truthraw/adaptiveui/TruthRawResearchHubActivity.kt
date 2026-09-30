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

class TruthRawResearchHubActivity : Activity() {
    private val bg = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val ink = DrawVisualTheme.INK
    private val muted = DrawVisualTheme.MUTED
    private val blue = DrawVisualTheme.BLUE

    private var pendingGlobalJson: String? = null
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
                    "Na Universele Analyse: Observation Optical Field Chart en Universal Observation & Calibration Atlas. Voor source-bound geometry, CFA-field en losse authority-assen.",
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
    }
}

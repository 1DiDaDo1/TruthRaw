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

class TruthRawImplementationGuideActivity : Activity() {
    private val bg = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val ink = DrawVisualTheme.INK
    private val muted = DrawVisualTheme.MUTED
    private val teal = DrawVisualTheme.TEAL

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(bg)
            setPadding(dp(18), dp(12), dp(18), dp(24))
            addView(header())
            addView(space(16))

            addView(card("Zo gebruik je D.RAW normaal").apply {
                addView(body(
                    "1. De app opent in D.RAW Workspace / Vrije Raster-weergave.\n" +
                        "2. Kies of behoud PURE, ADVANCED of PRO.\n" +
                        "3. Open Bestand of Universele camera.\n" +
                        "4. De bron wordt verzegeld en gaat door Universal Intake.\n" +
                        "5. Scientific Master blijft de gedeelde evidence-bound kern.\n" +
                        "6. Route-specifieke view/projectie/export komt pas daarna.",
                    12.3f,
                ))
            })

            addView(space(12))
            addView(card("Workspace / Vrije Raster-weergave v0.1").apply {
                addView(body(
                    "De Workspace is de nieuwe centrale UI-laag, niet een nieuwe scientific pipeline. RAW/DNG blijft via MainActivity / Universal Intake lopen. Het lokale vrije-rastercanvas accepteert alleen een reeds gerenderd presentatie-raster en biedt pan, pinch-zoom, Fit en 1:1 als VIEW-transforms. Die transforms zijn PRESENTATION_ONLY: ze veranderen geen bron-SHA, geen Scientific Master, geen authority en maken nooit nieuwe MEASURED samples. Full-resolution projectie/export blijft in de bestaande D.RAW werkbank totdat een expliciete bestaande-runtime bridge aan het canvas is gebonden.",
                    12f,
                ))
                addView(space(8))
                addView(body(
                    "De pipelinekaart en Evidence/Authority Inspector in v0.1 zijn authority-neutrale uitleg zolang geen actieve bronruntime eraan is gebonden. UNKNOWN mag niet door UI-status in groen/certainty veranderen.",
                    11.5f,
                ))
            })

            addView(space(12))
            addView(card("PURE / ADVANCED / PRO").apply {
                addView(item(
                    "PURE",
                    "Evidence-constrained Scientific View en wetenschappelijke projecties. Geen appearance/restoration schrijft terug.",
                ))
                addView(item(
                    "ADVANCED",
                    "Lichtheid, display-HDR, kleurvolheid, detail en restoration als downstream Appearance/Restoration View.",
                ))
                addView(item(
                    "PRO",
                    "Dezelfde scientific core plus Open Scene, provenance, professionele exports en research-candidates. Meer gereedschap, niet meer evidence.",
                ))
            })

            addView(space(12))
            addView(card("Universele camera").apply {
                addView(body(
                    "Ultra-wide, wide/main en tele zijn acquisitie/UI-rollen. Camera2, camera-ID en focal length zijn transport/provenance. Normale capture verzegelt eerst RAW_SENSOR en maakt daarna een afgeleide DNG voor dezelfde Universal Intake als een bestand. D.RAW bewaart nu beide machine-readable: de fysieke RAW_SENSOR-root en de processing-DNG-root, zonder van de DNG een tweede fysieke opname te maken. De speciale 4K→200MP-route blijft apart.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("Universele analyse · achterkant + voorkant").apply {
                addView(body(
                    "De achterkant levert bytes, CFA, metadata, sample-geometrie en measured support. De voorkant levert deterministische zichtbare structuur/edges/proporties als appearance-derived bescherming. De voorkant mag de achterkant nooit herschrijven.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("Multi-observation & Calibration Atlas").apply {
                addView(body(
                    "Gebruik ‘Analyseer alle geselecteerde bronnen universeel’ voor één of meer observaties. Eén bron kan al een Observation Optical Field Chart en Calibration Atlas opleveren. ≥3 geschikte onafhankelijke field charts zijn nodig voor de bestaande repeatability-audit. Calibration Observation Records zijn optioneel extra relationeel bewijs en nooit een normale ingangseis. Records krijgen een canonical SHA-256-identiteit, worden aan de actuele bron-SHA’s gebonden en een RAW_SENSOR plus zijn afgeleide DNG mogen niet als twee onafhankelijke observaties meetellen.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("Nieuwe candidate-runtimes").apply {
                addView(body(
                    "Radiometric response/OECF · component-separated noise · sparse-CFA repeated-observation noise · NPS · world-vs-sensor field separation · SFR/MTF/PSF optical support · NPS+optics joint candidate · noise-aware inverse optics · multi-illuminant colour relation · colour-covariance J·C·Jᵀ transport · temporal/stop-motion relation · 3D/depth/visibility candidates · world-space residuals · uncertainty-weighted reconstruction · typed promotion-state · gated scientific denoise · perceptual noise visibility.",
                    11.8f,
                ))
                addView(space(8))
                addView(body(
                    "Status: IMPLEMENTED CANDIDATE / NOT PROMOTED. Candidate-uitvoer mag Scientific Master niet wijzigen totdat de afzonderlijke fysieke validation/promotion-gate bewezen is.",
                    11.8f,
                ))
            })

            addView(space(12))
            addView(card("Welke onderzoeksroute kies je?").apply {
                addView(item(
                    "Eén foto / één bron",
                    "Gebruik Calibration Atlas, Optical Field Chart, N2/holdout audits en backside/frontside diagnostics.",
                ))
                addView(item(
                    "Herhaalde foto's van dezelfde wereld",
                    "Gebruik Multi-observation, Field Response Repeatability, world-vs-sensor scheiding en later world-space residuals.",
                ))
                addView(item(
                    "Stop-motion / tijd",
                    "Gebruik expliciete onafhankelijke captures. Temporal candidates mogen sequence/readout modelleren; een synthetisch tussenframe wordt nooit extra fysiek bewijs.",
                ))
                addView(item(
                    "Kleur / licht / optica",
                    "Voeg alleen gecontroleerde relation-records toe wanneer echte target/illuminant/SFR/MTF/PSF/exposure-observaties bestaan. Geen camera- of lensprofiel is ingangseis.",
                ))
            })

            addView(space(12))
            addView(card("JSON-route in één oogopslag").apply {
                addView(item(
                    "Global Research Snapshot",
                    "Projectarchitectuur en gates zonder foto; geen measurement evidence.",
                ))
                addView(item(
                    "Universal Calibration Atlas",
                    "Eén bron met losse kleur/lichtval/optiek/tijd/restauratie authority-assen.",
                ))
                addView(item(
                    "Observation Optical Field Chart",
                    "Gemeten CFA-field support van één sealed observation.",
                ))
                addView(item(
                    "Field Response Repeatability",
                    "Vergelijkt geschikte onafhankelijke field charts; minimaal drie voor de bestaande gate.",
                ))
                addView(item(
                    "Free World Foundation",
                    "Breedste per-session onderzoeks-JSON met world geometry, uncertainty en de nieuwe candidate-runtimes.",
                ))
            })

            addView(space(12))
            addView(card("Noise reduction · praktische volgorde").apply {
                addView(body(
                    "NoiseProfile-metadata is context, geen bewijs. D.RAW begint bij measured CFA/backside support, bewaart structure-protection van de voorkant, scheidt temporal/fixed-pattern/radiometric/optical componenten waar bewijs bestaat en laat UNKNOWN residual bestaan. Kleurcovariantie wordt alleen getransporteerd wanneer echte input-covariantie aanwezig is; inverse optics vereist gecontroleerde NPS, MTF/SFR én expliciete signal-PSD. Pas na interne held-out validation, source-binding en afzonderlijke approval kan een scientific denoise-route een afgeleide RECONSTRUCTED-uitvoer toelaten; Scientific Master blijft ongewijzigd.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("CPU / GPU").apply {
                addView(body(
                    "De algemene compute-router blijft bewust CPU_REFERENCE zolang een kernel geen eigen correctness/self-test heeft. ARM64/NEON/Vulkan-detectie is capability-informatie; specifieke bewezen paden zoals TruthNegative-Vulkan en restoration-multicore mogen hun eigen gevalideerde route gebruiken. Hardwarekeuze verandert nooit authority.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("Teststatus / timer").apply {
                addView(body(
                    "Bij tests en zware analyses blijft de bestaande visuele statusregel gelden: groen punt bij normaal lopend/gereed, rood punt bij fout, live Looptijd tijdens uitvoering en een vaste eindduur na afloop. Een kleur of timer is alleen UX-status en verandert nooit authority.",
                    12f,
                ))
            })

            addView(space(12))
            addView(card("Wat blijft altijd vast?").apply {
                addView(body(
                    "MEASURED ≠ CALIBRATED_ESTIMATE ≠ RECONSTRUCTED ≠ CENSORED ≠ UNKNOWN ≠ APPEARANCE. BlackLevel ≠ Zero-Line. Outputresolutie ≠ optische resolutie. Camera/lens/vendor/RAW-identiteit mag geen scientific model key zijn. UNKNOWN wordt niet automatisch noise. Representatie mag de bron overstijgen; kennisclaims niet.",
                    12f,
                ))
            })

            addView(space(12))
            addView(action("Open D.RAW Workspace / Vrije Raster") {
                startActivity(
                    Intent(
                        this@TruthRawImplementationGuideActivity,
                        TruthRawWorkspaceActivity::class.java,
                    ).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                    },
                )
            })
            addView(space(8))
            addView(action("Open Research & JSON") {
                startActivity(
                    Intent(
                        this@TruthRawImplementationGuideActivity,
                        TruthRawResearchHubActivity::class.java,
                    ),
                )
            })
            addView(space(8))
            addView(action("Open normale D.RAW werkbank") {
                startActivity(
                    Intent(
                        this@TruthRawImplementationGuideActivity,
                        MainActivity::class.java,
                    ).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                    },
                )
            })
            addView(space(18))
            addView(
                DrawVisualTheme.brandFooter(
                    this@TruthRawImplementationGuideActivity,
                    72,
                ),
            )
        }

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(bg)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(
            TextView(this@TruthRawImplementationGuideActivity).apply {
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
                addView(title("Wat kan D.RAW nu?", 24f))
                addView(
                    body(
                        "Geïmplementeerd · praktisch gebruik · authority-grenzen",
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

    private fun item(label: String, explanation: String): View =
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
            background =
                rounded(
                    DrawVisualTheme.PAPER_MINT,
                    teal,
                    15f,
                )
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
}

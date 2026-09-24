package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
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

class TruthRawProActivity : Activity() {
    private var computeProbeView: TextView? = null

    private val bg = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val textColor = DrawVisualTheme.INK
    private val muted = DrawVisualTheme.MUTED
    private val purple = DrawVisualTheme.PURPLE

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
        }

        root.addView(header())
        root.addView(space(18))
        root.addView(card("Professionele route").apply {
            addView(body(
                "PRO gebruikt dezelfde sealed source, Scientific Master, Zero-Line, scene-scale en Dynamic Authority als PURE en ADVANCED. " +
                    "De extra controle zit in interpretatie, precision, projecties en provenance; niet in sterkere evidence.",
                12.5f,
            ))
        })

        root.addView(space(12))
        root.addView(card("Color & illumination").apply {
            addView(body(
                "White balance, observed illumination, camera→XYZ color characterization en appearance blijven gescheiden. " +
                    "Source-metadata-bound kleur wordt niet automatisch FULL_PHYSICAL.",
                12f,
            ))
        })

        root.addView(space(12))
        root.addView(card("Precision").apply {
            addView(title("F32 canonical storage · F64 compute actief", 14f))
            addView(body(
                "De opgeslagen Scientific Master-identiteit blijft exact IEEE Float32. Branch-sensitive reconstructie draait in de hoofdcode actief in Float64 en wordt daarna gecontroleerd naar Float32 canonical storage teruggebracht. " +
                    "Meer rekenprecisie wordt nooit als extra evidence gepresenteerd.",
                11.5f,
            ))
            addView(space(8))
            addView(body(
                "F64 canonical storage blijft apart geblokkeerd totdat een eigen identity/replay-contract daarvoor is gevalideerd.",
                11.5f,
            ))
        })

        root.addView(space(12))
        root.addView(card("Huidige scientific core").apply {
            addView(body(
                "Open Scene Field v0.85 · TruthNegative TN-4 local authority v0.4 · local HDR/Restoration/detail policy v0.86 · Unified Output Preview v0.1. " +
                    "De outputpreview leest de gekozen primary-route en blijft presentation-only.",
                12f,
            ))
        })

        root.addView(space(12))
        root.addView(card("RAW / DNG ingress").apply {
            addView(body(
                "DNG gebruikt de volledige admitted tile-native Scientific-Master-route. Nikon NEF heeft een beperkte measurement-only uncompressed-16 CFA decoder. " +
                    "CR3/CR2, ARW, RAF, RW2, ORF, PEF, RWL, 3FR/FFF, IIQ en andere proprietary RAW-formaten worden veilig als immutable bronhandle opgenomen en blijven fail-closed totdat hun decoder-adapter is gekoppeld.",
                11.5f,
            ))
        })

        root.addView(space(12))
        root.addView(card("Professionele exports").apply {
            addView(body(
                "PRO maakt na verwerking de uitgebreide exportwerkbank zichtbaar: PURE Float32 DNG, Restoration, Float32 TIFF, OpenEXR, " +
                    "compatibility DNG en toekomstige JPG-L/TruthPhoto-container.",
                12f,
            ))
        })

        root.addView(space(12))
        root.addView(card("Hardware acceleration · research").apply {
            addView(body(
                "Deze probe verandert geen pixels. De algemene compute-router blijft CPU_REFERENCE selecteren totdat een kernel afzonderlijk correctness + benchmark-validatie heeft. " +
                    "ARM64/NEON, Vulkan, ADPF/headroom en dynamische workers worden wel gedetecteerd/gebruikt waar hun eigen contract dat toelaat; de TruthNegative Vulkan-kernel heeft een aparte bit-exact selftest. APV blijft alleen professionele video/intermediate en vervangt de Scientific Master niet.",
                11.5f,
            ))
            addView(space(8))
            computeProbeView = body("Hardware wordt read-only geïnventariseerd…", 11f)
            addView(computeProbeView)
            addView(space(8))
            addView(action("Hardware opnieuw meten") { runComputeProbe() })
        })

        root.addView(space(12))
        root.addView(action("Advanced scene-instellingen openen") {
            startActivity(Intent(this, TruthRawAdvancedActivity::class.java))
        })

        root.addView(space(10))
        root.addView(action("Gebruik D.RAW PRO") {
            getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
                .edit()
                .putString(
                    TruthRawSuiteLauncherActivity.KEY_OUTPUT,
                    TruthRawSuiteLauncherActivity.OUTPUT_PRO,
                )
                .apply()
            finish()
        })

        root.addView(space(18))
        root.addView(DrawVisualTheme.brandFooter(this, 72))

        val scroll = ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(bg)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
        scroll.post { runComputeProbe() }
        return scroll
    }

    private fun runComputeProbe() {
        computeProbeView?.text = "Hardware wordt read-only geïnventariseerd…"
        Thread({
            val text = TruthRawComputeCapabilitiesProbe.probe().fold(
                onSuccess = { capabilities ->
                    val systemHeadroom =
                        TruthRawSystemHeadroomProbe.sample(force = true).getOrNull()
                    val appearancePlan = TruthRawComputeRouterV01.plan(
                        TruthRawComputeClass.APPEARANCE,
                        capabilities,
                        systemHeadroom,
                    )
                    val cpuPlan = TruthRawCpuSchedulingPolicyV01.plan(
                        context = this@TruthRawProActivity,
                        workload = TruthRawCpuWorkload.EXACT_SCIENTIFIC,
                        // Current 128px tile paths remain comfortably below this
                        // conservative per-worker envelope; future 200MP kernels
                        // must provide their own measured estimate.
                        bytesPerWorkerEstimate = 16L * 1024L * 1024L,
                    )
                    val apv = TruthRawApvCapabilitiesProbe.probe().getOrNull()
                    buildString {
                        append(capabilities.summary)
                        append("\n")
                        append(
                            systemHeadroom?.summary
                                ?: "ADPF resource headroom: unavailable/fail-closed",
                        )
                        append("\nCPU worker plan: ")
                        append(cpuPlan.reason)
                        append("\nCandidates: ")
                        append(appearancePlan.candidates.joinToString())
                        append("\nActief: ")
                        append(appearancePlan.selected)
                        append("\n")
                        append(appearancePlan.reason)
                        append("\n\nAPV professional video:")
                        append("\n")
                        if (apv != null) {
                            append(TruthRawApvCapabilitiesProbe.compactDetails(apv))
                        } else {
                            append("APV runtime-probe faalde fail-closed.")
                        }
                    }
                },
                onFailure = { error ->
                    "Hardware-probe fail-closed: " +
                        (error.message ?: error.javaClass.simpleName) +
                        "\nActief: CPU_REFERENCE"
                },
            )
            runOnUiThread { computeProbeView?.text = text }
        }, "truthraw-compute-probe").start()
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(TextView(this@TruthRawProActivity).apply {
            text = "‹"
            textSize = 36f
            setTextColor(textColor)
            gravity = Gravity.CENTER
            contentDescription = "Terug"
            setOnClickListener { finish() }
        }, LinearLayout.LayoutParams(dp(48), dp(48)).apply { marginEnd = dp(8) })
        addView(vertical().apply {
            addView(title("D.RAW PRO", 24f))
            addView(body("Professionele werkbank · dezelfde scientific core", 11.5f))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
    }

    private fun card(heading: String): LinearLayout = vertical().apply {
        setPadding(dp(15), dp(15), dp(15), dp(15))
        background = rounded(surface, DrawVisualTheme.PURPLE, 18f)
        addView(title(heading, 17f))
        addView(space(7))
    }

    private fun action(label: String, onClick: () -> Unit): View = TextView(this).apply {
        text = label
        textSize = 14.5f
        setTextColor(textColor)
        gravity = Gravity.CENTER
        setPadding(dp(14), dp(14), dp(14), dp(14))
        background = rounded(DrawVisualTheme.PAPER_PURPLE, purple, 16f)
        setOnClickListener { onClick() }
    }

    private fun rounded(fill: Int, stroke: Int, radius: Float): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(radius).toFloat()
            setColor(fill)
            setStroke(dp(1), stroke)
        }

    private fun vertical() = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal() = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }
    private fun title(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(textColor)
        setTypeface(typeface, Typeface.BOLD)
    }
    private fun body(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(muted)
        setLineSpacing(0f, 1.12f)
    }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
    private fun dp(value: Float): Int = (value * resources.displayMetrics.density + 0.5f).toInt()
}

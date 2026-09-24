package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.text.Spannable
import android.text.SpannableString
import android.text.style.ForegroundColorSpan
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class TruthRawSuiteLauncherActivity : Activity() {
    private val backgroundColor = DrawVisualTheme.PAPER_YELLOW
    private val surface = DrawVisualTheme.PAPER_WHITE
    private val surfaceSoft = DrawVisualTheme.PAPER_WHITE
    private val textPrimary = DrawVisualTheme.INK
    private val textMuted = DrawVisualTheme.MUTED
    private val blue = DrawVisualTheme.BLUE
    private val cyan = DrawVisualTheme.TEAL
    private val amber = DrawVisualTheme.ORANGE
    private val purple = DrawVisualTheme.PURPLE

    private val compactHeight: Boolean
        get() = resources.configuration.screenHeightDp < 900

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        DrawVisualTheme.applyWindow(this)
        setContentView(buildUi())
    }

    override fun onResume() {
        super.onResume()
        DrawVisualTheme.applyWindow(this)
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(backgroundColor)
            setPadding(dp(18), dp(12), dp(18), dp(24))
        }

        root.addView(header())
        root.addView(space(if (compactHeight) 12 else 20))
        root.addView(title("Kies route", if (compactHeight) 24f else 27f))
        root.addView(body("Alle routes vertrekken uit dezelfde verzegelde bron en Scientific Master.", if (compactHeight) 12.5f else 14f))
        root.addView(space(10))

        val selected = preferredOutput()
        root.addView(routeCard(
            titleText = "D.RAW PURE",
            subtitleText = "Direct · wetenschappelijk",
            detail = "Kortste route naar de self-binding 32-bit Float scientific projectie. Geen appearance.",
            accent = cyan,
            fill = DrawVisualTheme.PAPER_MINT,
            selected = selected == OUTPUT_PURE,
        ) { setPreferredOutput(OUTPUT_PURE) })
        root.addView(space(9))
        root.addView(routeCard(
            titleText = "D.RAW ADVANCED",
            subtitleText = "Fotografische ontwikkeling",
            detail = "Light · authority-aware HDR · Detail · Restoration. Scientific Master blijft onaangeraakt.",
            accent = amber,
            fill = DrawVisualTheme.PAPER_ORANGE,
            selected = selected == OUTPUT_ADVANCED,
        ) { setPreferredOutput(OUTPUT_ADVANCED) })
        root.addView(space(9))
        root.addView(routeCard(
            titleText = "D.RAW PRO",
            subtitleText = "Professionele werkbank",
            detail = "Color · illumination · precision · projecties · provenance en uitgebreide exports.",
            accent = purple,
            fill = DrawVisualTheme.PAPER_PURPLE,
            selected = selected == OUTPUT_PRO,
        ) { setPreferredOutput(OUTPUT_PRO) })

        if (selected == OUTPUT_ADVANCED || selected == OUTPUT_PRO) {
            root.addView(space(8))
            root.addView(action(
                if (selected == OUTPUT_ADVANCED) "Advanced instellingen" else "Open professionele werkbank",
            ) {
                startActivity(
                    Intent(
                        this,
                        if (selected == OUTPUT_ADVANCED) TruthRawAdvancedActivity::class.java
                        else TruthRawProActivity::class.java,
                    ),
                )
            })
        }

        root.addView(space(if (compactHeight) 16 else 24))
        root.addView(title("Kies invoer", if (compactHeight) 24f else 27f))
        root.addView(body("Waar komt je foto vandaan?", if (compactHeight) 13.5f else 15f))
        root.addView(space(if (compactHeight) 8 else 12))
        root.addView(horizontal().apply {
            addView(
                inputCard(
                    iconRes = R.drawable.ic_folder_truthraw,
                    titleText = "Bestand",
                    subtitleText = "Open RAW / DNG",
                    accent = blue,
                    fill = DrawVisualTheme.PAPER_BLUE,
                ) {
                    startActivity(Intent(this@TruthRawSuiteLauncherActivity, MainActivity::class.java).apply {
                        flags = Intent.FLAG_ACTIVITY_REORDER_TO_FRONT
                        putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                    })
                },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginEnd = dp(6) },
            )
            addView(
                inputCard(
                    iconRes = R.drawable.ic_camera_truthraw,
                    titleText = "Camera",
                    subtitleText = "Maak één fysieke RAW",
                    accent = cyan,
                    fill = DrawVisualTheme.PAPER_MINT,
                ) {
                    startActivity(
                        Intent(
                            this@TruthRawSuiteLauncherActivity,
                            FotoGraaf200MpStagedActivity::class.java,
                        ).apply {
                            putExtra(FotoGraaf200MpStagedActivity.EXTRA_PRODUCTION_CAMERA_ENTRY, true)
                        },
                    )
                },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginStart = dp(6) },
            )
        })

        root.addView(space(if (compactHeight) 14 else 18))
        root.addView(DrawVisualTheme.brandFooter(this, if (compactHeight) 82 else 94))

        return ScrollView(this).apply {
            isFillViewport = true
            clipToPadding = true
            setBackgroundColor(backgroundColor)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
    }

    private fun brandHero(): View = ImageView(this).apply {
        setImageResource(R.drawable.draw_opening_banner)
        contentDescription = "D.RAW — Evidence-bound computational photography and open scene reconstruction"
        adjustViewBounds = true
        scaleType = ImageView.ScaleType.FIT_CENTER
    }

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(View(this@TruthRawSuiteLauncherActivity), LinearLayout.LayoutParams(
            dp(if (compactHeight) 48 else 54), dp(if (compactHeight) 48 else 54)
        ))
        addView(vertical().apply {
            gravity = Gravity.CENTER_HORIZONTAL
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                val mark = SpannableString("D.RAW")
                mark.setSpan(ForegroundColorSpan(DrawVisualTheme.PENCIL_YELLOW), 1, 2, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE)
                text = mark
                textSize = if (compactHeight) 33f else 38f
                setTextColor(textPrimary)
                setTypeface(typeface, Typeface.BOLD)
                gravity = Gravity.CENTER
            })
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = "BEYOND THE OBVIOUS"
                textSize = 10.5f
                letterSpacing = 0.24f
                setTextColor(textPrimary)
                gravity = Gravity.CENTER
            })
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = "⚙"
            textSize = 27f
            gravity = Gravity.CENTER
            setTextColor(textPrimary)
            background = cardBackground(DrawVisualTheme.PAPER_WHITE, DrawVisualTheme.PENCIL_YELLOW, false)
            setOnClickListener { startActivity(Intent(this@TruthRawSuiteLauncherActivity, TruthRawSettingsActivity::class.java)) }
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 48 else 54), dp(if (compactHeight) 48 else 54)))
    }

    private fun routeCard(
        titleText: String,
        subtitleText: String,
        detail: String,
        accent: Int,
        fill: Int,
        selected: Boolean,
        action: () -> Unit,
    ): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(14), dp(13), dp(10), dp(13))
        background = cardBackground(fill, accent, true)
        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = if (selected) "✓" else "○"
            textSize = 25f
            gravity = Gravity.CENTER
            setTextColor(if (selected) Color.WHITE else accent)
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(if (selected) accent else Color.TRANSPARENT)
                setStroke(dp(2), accent)
            }
        }, LinearLayout.LayoutParams(dp(48), dp(48)).apply { marginEnd = dp(10) })
        addView(vertical().apply {
            addView(title(titleText, if (compactHeight) 17f else 19f))
            addView(space(2))
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = subtitleText
                textSize = if (compactHeight) 12f else 13f
                setTextColor(accent)
                setTypeface(typeface, Typeface.BOLD)
            })
            addView(space(4))
            addView(body(detail, if (compactHeight) 10.7f else 11.8f))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        addView(PencilScribbleView(this@TruthRawSuiteLauncherActivity, accent),
            LinearLayout.LayoutParams(dp(54), dp(54)).apply { marginStart = dp(6) })
        setOnClickListener { action() }
    }

    private fun inputCard(
        iconRes: Int,
        titleText: String,
        subtitleText: String,
        accent: Int,
        fill: Int,
        action: () -> Unit,
    ): View = vertical().apply {
        val pad = if (compactHeight) 10 else 12
        gravity = Gravity.CENTER
        setPadding(dp(pad), dp(pad), dp(pad), dp(pad))
        background = cardBackground(fill, accent, true)
        minimumHeight = dp(if (compactHeight) 122 else 142)
        addView(ImageView(this@TruthRawSuiteLauncherActivity).apply {
            setImageResource(iconRes)
            imageTintList = ColorStateList.valueOf(textPrimary)
            setPadding(dp(9), dp(9), dp(9), dp(9))
            background = GradientDrawable().apply {
                shape = GradientDrawable.OVAL
                setColor(Color.argb(32, Color.red(accent), Color.green(accent), Color.blue(accent)))
                setStroke(dp(1), accent)
            }
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 48 else 56), dp(if (compactHeight) 48 else 56)))
        addView(space(if (compactHeight) 7 else 9))
        addView(title(titleText, if (compactHeight) 15.5f else 17f).apply { gravity = Gravity.CENTER })
        addView(space(2))
        addView(body(subtitleText, if (compactHeight) 10.5f else 11.5f).apply { gravity = Gravity.CENTER })
        addView(PencilScribbleView(this@TruthRawSuiteLauncherActivity, accent),
            LinearLayout.LayoutParams(dp(52), dp(22)).apply { topMargin = dp(3) })
        setOnClickListener { action() }
    }

    private fun infoStrip(selected: String): View = vertical().apply {
        setPadding(dp(14), dp(if (compactHeight) 10 else 13), dp(14), dp(if (compactHeight) 10 else 13))
        background = cardBackground(Color.rgb(8, 21, 35), Color.rgb(39, 73, 105), false)
        val heading = when (selected) {
            OUTPUT_ADVANCED -> "ADVANCED bouwt vrij bovenop dezelfde master"
            OUTPUT_PRO -> "PRO toont de bouwtekeningen"
            else -> "PURE blijft de rechte meetbare route"
        }
        val detail = when (selected) {
            OUTPUT_ADVANCED -> "Appearance, HDR, Light, Detail en Restoration schrijven nooit terug naar de sealed source of Scientific Master."
            OUTPUT_PRO -> "F64 branch-sensitive compute, Open Scene/local authority en exports veranderen representatie en precisie, nooit de evidence-herkomst."
            else -> "Sealed source → Scientific Master → PURE-projectie. Geen tone, relight of appearance in de route."
        }
        addView(title(heading, 14f))
        addView(space(4))
        addView(body(detail, if (compactHeight) 10.8f else 11.5f))
    }

    private fun action(label: String, onClick: () -> Unit): View = TextView(this).apply {
        text = label
        textSize = 13.5f
        setTextColor(textPrimary)
        gravity = Gravity.CENTER
        setPadding(dp(13), dp(12), dp(13), dp(12))
        background = cardBackground(surfaceSoft, Color.rgb(44, 75, 111), false)
        setOnClickListener { onClick() }
    }

    private fun setPreferredOutput(mode: String) {
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_OUTPUT, mode).apply()
        setContentView(buildUi())
    }

    private fun preferredOutput(): String {
        val value = getSharedPreferences(PREFS, MODE_PRIVATE)
            .getString(KEY_OUTPUT, OUTPUT_PURE)
            ?: OUTPUT_PURE
        return when (value) {
            OUTPUT_ADVANCED, OUTPUT_PRO, OUTPUT_PURE -> value
            OUTPUT_JPG -> OUTPUT_ADVANCED
            else -> OUTPUT_PURE
        }
    }

    private fun cardBackground(fill: Int, stroke: Int, selected: Boolean): GradientDrawable =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = dp(20).toFloat()
            setColor(fill)
            setStroke(dp(if (selected) 2 else 1), stroke)
        }

    private fun vertical() = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal() = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
    private fun space(height: Int) = View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(height))
    }
    private fun title(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(textPrimary)
        setTypeface(typeface, Typeface.BOLD)
    }
    private fun body(value: String, size: Float) = TextView(this).apply {
        text = value
        textSize = size
        setTextColor(textMuted)
        setLineSpacing(0f, 1.12f)
    }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        const val PREFS = "truthraw_ui"
        const val KEY_OUTPUT = "preferred_output"
        const val OUTPUT_PURE = "PURE"
        const val OUTPUT_ADVANCED = "ADVANCED"
        const val OUTPUT_PRO = "PRO"
        const val OUTPUT_JPG = "JPG"
        const val OUTPUT_NEGATIVE = "NEGATIVE"
    }
}

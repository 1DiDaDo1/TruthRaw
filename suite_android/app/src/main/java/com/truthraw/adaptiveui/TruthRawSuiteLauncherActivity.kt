package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class TruthRawSuiteLauncherActivity : Activity() {
    private val backgroundColor = Color.rgb(5, 12, 22)
    private val surface = Color.rgb(10, 22, 37)
    private val surfaceSoft = Color.rgb(14, 29, 48)
    private val textPrimary = Color.rgb(244, 248, 255)
    private val textMuted = Color.rgb(158, 178, 205)
    private val blue = Color.rgb(63, 142, 255)
    private val cyan = Color.rgb(94, 217, 205)
    private val amber = Color.rgb(236, 176, 82)
    private val purple = Color.rgb(190, 92, 238)

    private val compactHeight: Boolean
        get() = resources.configuration.screenHeightDp < 900

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        window.statusBarColor = backgroundColor
        window.navigationBarColor = backgroundColor
        setContentView(buildUi())
    }

    override fun onResume() {
        super.onResume()
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
            titleText = "TRUTHRAW PURE",
            subtitleText = "Direct · wetenschappelijk",
            detail = "Kortste route naar de self-binding 32-bit Float scientific projectie. Geen appearance.",
            accent = cyan,
            selected = selected == OUTPUT_PURE,
        ) { setPreferredOutput(OUTPUT_PURE) })
        root.addView(space(9))
        root.addView(routeCard(
            titleText = "TRUTHRAW ADVANCED",
            subtitleText = "Fotografische ontwikkeling",
            detail = "Light · authority-aware HDR · Detail · Restoration. Scientific Master blijft onaangeraakt.",
            accent = amber,
            selected = selected == OUTPUT_ADVANCED,
        ) { setPreferredOutput(OUTPUT_ADVANCED) })
        root.addView(space(9))
        root.addView(routeCard(
            titleText = "TRUTHRAW PRO",
            subtitleText = "Professionele werkbank",
            detail = "Color · illumination · precision · projecties · provenance en uitgebreide exports.",
            accent = purple,
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
                    titleText = "Open RAW / DNG",
                    subtitleText = "Kies een bestaand RAW- of DNG-bestand.",
                    accent = blue,
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
                    titleText = "Gebruik camera",
                    subtitleText = "Maak één fysieke RAW-opname en ga via dezelfde admission.",
                    accent = blue,
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

        root.addView(space(if (compactHeight) 14 else 22))
        root.addView(infoStrip(selected))
        root.addView(space(8))
        root.addView(body("v0.83.1 · drie routes · sealed source → Scientific Master → vrije ontwikkeling", 11f).apply {
            gravity = Gravity.CENTER
        })

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

    private fun header(): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        addView(ImageView(this@TruthRawSuiteLauncherActivity).apply {
            setImageResource(R.drawable.truthraw_icon)
            contentDescription = "TruthRaw"
            scaleType = ImageView.ScaleType.CENTER_CROP
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 58 else 68), dp(if (compactHeight) 58 else 68)).apply {
            marginEnd = dp(if (compactHeight) 10 else 14)
        })

        addView(vertical().apply {
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = "TruthRaw"
                textSize = if (compactHeight) 28f else 31f
                setTextColor(textPrimary)
                setTypeface(typeface, Typeface.BOLD)
            })
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = "BEYOND THE OBVIOUS"
                textSize = 10f
                letterSpacing = 0.24f
                setTextColor(textMuted)
            })
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))

        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = "⚙"
            textSize = 28f
            gravity = Gravity.CENTER
            setTextColor(Color.rgb(202, 218, 239))
            contentDescription = "Instellingen en onderzoek"
            background = cardBackground(surfaceSoft, Color.rgb(44, 69, 99), false)
            setOnClickListener {
                startActivity(Intent(this@TruthRawSuiteLauncherActivity, TruthRawSettingsActivity::class.java))
            }
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 48 else 54), dp(if (compactHeight) 48 else 54)))
    }

    private fun routeCard(
        titleText: String,
        subtitleText: String,
        detail: String,
        accent: Int,
        selected: Boolean,
        action: () -> Unit,
    ): View = horizontal().apply {
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(15), dp(14), dp(15), dp(14))
        background = cardBackground(if (selected) Color.rgb(12, 31, 52) else surface, accent, selected)
        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = if (selected) "✓" else "○"
            textSize = 24f
            gravity = Gravity.CENTER
            setTextColor(if (selected) accent else textMuted)
        }, LinearLayout.LayoutParams(dp(38), dp(44)).apply { marginEnd = dp(8) })
        addView(vertical().apply {
            addView(title(titleText, if (compactHeight) 16f else 18f))
            addView(space(2))
            addView(body(subtitleText, if (compactHeight) 11.5f else 12.5f))
            addView(space(4))
            addView(body(detail, if (compactHeight) 10.5f else 11.5f))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        setOnClickListener { action() }
    }

    private fun inputCard(
        iconRes: Int,
        titleText: String,
        subtitleText: String,
        accent: Int,
        action: () -> Unit,
    ): View = vertical().apply {
        val pad = if (compactHeight) 11 else 14
        setPadding(dp(pad), dp(pad), dp(pad), dp(pad))
        background = cardBackground(surfaceSoft, accent, true)
        minimumHeight = dp(if (compactHeight) 150 else 174)

        addView(ImageView(this@TruthRawSuiteLauncherActivity).apply {
            setImageResource(iconRes)
            imageTintList = ColorStateList.valueOf(Color.rgb(190, 224, 255))
            setPadding(dp(8), dp(8), dp(8), dp(8))
            background = cardBackground(Color.rgb(8, 30, 56), accent, false)
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 46 else 54), dp(if (compactHeight) 46 else 54)))
        addView(space(if (compactHeight) 9 else 12))
        addView(title(titleText, if (compactHeight) 16.5f else 18f))
        addView(space(4))
        addView(body(subtitleText, if (compactHeight) 11.5f else 12.5f))
        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = "›"
            textSize = 31f
            gravity = Gravity.END
            setTextColor(accent)
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
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
            OUTPUT_PRO -> "Professionele opties veranderen precision- en exportkeuzes, niet de herkomst of evidence-authority."
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

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

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(backgroundColor)
            setPadding(dp(18), dp(12), dp(18), dp(24))
        }

        root.addView(header())
        root.addView(space(if (compactHeight) 12 else 22))
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
                        putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                    })
                },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginEnd = dp(6) },
            )
            addView(
                inputCard(
                    iconRes = R.drawable.ic_camera_truthraw,
                    titleText = "Gebruik camera",
                    subtitleText = "Maak direct een nieuwe opname via de RAW-ingang.",
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

        root.addView(space(if (compactHeight) 14 else 24))
        root.addView(title("Kies uitvoer", if (compactHeight) 24f else 27f))
        root.addView(body("Je voorkeur bepaalt welke echte exportactie na verwerking bovenaan staat.", if (compactHeight) 12.5f else 14f))
        root.addView(space(if (compactHeight) 8 else 12))

        val selected = preferredOutput()
        root.addView(horizontal().apply {
            addView(
                outputCard(
                    titleText = "JPG",
                    subtitleText = "Universeel",
                    detail = "Finalized sRGB preview",
                    accent = blue,
                    selected = selected == OUTPUT_JPG,
                    enabled = true,
                ) { setPreferredOutput(OUTPUT_JPG) },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginEnd = dp(6) },
            )
            addView(
                outputCard(
                    titleText = "JPG XL",
                    subtitleText = "Hoge kwaliteit",
                    detail = "Nog niet toegelaten",
                    accent = purple,
                    selected = false,
                    enabled = false,
                ) {},
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginStart = dp(6) },
            )
        })
        root.addView(space(if (compactHeight) 8 else 12))
        root.addView(horizontal().apply {
            addView(
                outputCard(
                    titleText = "TRUTHRAW PURE",
                    subtitleText = "Wetenschappelijk",
                    detail = "32-bit Float DNG · self-binding",
                    accent = cyan,
                    selected = selected == OUTPUT_PURE,
                    enabled = true,
                ) { setPreferredOutput(OUTPUT_PURE) },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginEnd = dp(6) },
            )
            addView(
                outputCard(
                    titleText = "TRUTHRAW ADVANCED",
                    subtitleText = "Volledige controle",
                    detail = "Natural HDR · Light · Detail · Restoration",
                    accent = amber,
                    selected = selected == OUTPUT_ADVANCED,
                    enabled = true,
                ) {
                    startActivity(
                        Intent(
                            this@TruthRawSuiteLauncherActivity,
                            TruthRawAdvancedActivity::class.java,
                        ),
                    )
                },
                LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply { marginStart = dp(6) },
            )
        })

        root.addView(space(if (compactHeight) 14 else 22))
        root.addView(infoStrip())
        root.addView(space(8))
        root.addView(body("v0.79 · uncertainty-admission · bron/backend/trace fail-closed", 11f).apply {
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
        minimumHeight = dp(if (compactHeight) 158 else 185)

        addView(ImageView(this@TruthRawSuiteLauncherActivity).apply {
            setImageResource(iconRes)
            imageTintList = ColorStateList.valueOf(Color.rgb(190, 224, 255))
            setPadding(dp(8), dp(8), dp(8), dp(8))
            background = cardBackground(Color.rgb(8, 30, 56), accent, false)
        }, LinearLayout.LayoutParams(dp(if (compactHeight) 46 else 54), dp(if (compactHeight) 46 else 54)))
        addView(space(if (compactHeight) 9 else 14))
        addView(title(titleText, if (compactHeight) 16.5f else 18f))
        addView(space(4))
        addView(body(subtitleText, if (compactHeight) 11.5f else 12.5f))
        addView(space(if (compactHeight) 6 else 10))
        addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
            text = "›"
            textSize = 31f
            gravity = Gravity.END
            setTextColor(accent)
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        setOnClickListener { action() }
    }

    private fun outputCard(
        titleText: String,
        subtitleText: String,
        detail: String,
        accent: Int,
        selected: Boolean,
        enabled: Boolean,
        action: () -> Unit,
    ): View = vertical().apply {
        val pad = if (compactHeight) 10 else 13
        setPadding(dp(pad), dp(pad), dp(pad), dp(pad))
        background = cardBackground(if (selected) Color.rgb(12, 31, 52) else surface, accent, selected)
        alpha = if (enabled) 1f else 0.56f

        val isTruthRaw = titleText.startsWith("TRUTHRAW ")
        minimumHeight = dp(
            if (compactHeight) {
                if (isTruthRaw) 132 else 124
            } else {
                if (isTruthRaw) 158 else 142
            },
        )

        addView(horizontal().apply {
            gravity = Gravity.TOP
            minimumHeight = dp(
                if (isTruthRaw) {
                    if (compactHeight) 40 else 44
                } else 30,
            )
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = if (selected) "✓" else "○"
                textSize = 20f
                gravity = Gravity.CENTER
                setTextColor(if (selected) accent else textMuted)
            }, LinearLayout.LayoutParams(dp(28), dp(30)).apply { marginEnd = dp(7) })

            val displayTitle = when (titleText) {
                "TRUTHRAW PURE" -> "TRUTHRAW\nPURE"
                "TRUTHRAW ADVANCED" -> "TRUTHRAW\nADVANCED"
                else -> titleText
            }
            addView(TextView(this@TruthRawSuiteLauncherActivity).apply {
                text = displayTitle
                textSize = if (compactHeight) {
                    if (isTruthRaw) 12.2f else 15f
                } else {
                    if (isTruthRaw) 13f else 16f
                }
                maxLines = if (isTruthRaw) 2 else 1
                minLines = if (isTruthRaw) 2 else 1
                setTextColor(textPrimary)
                setTypeface(typeface, Typeface.BOLD)
                includeFontPadding = true
                setLineSpacing(0f, 1.0f)
            }, LinearLayout.LayoutParams(
                0,
                if (isTruthRaw) dp(if (compactHeight) 40 else 44) else ViewGroup.LayoutParams.WRAP_CONTENT,
                1f,
            ))
        })
        addView(space(if (compactHeight) 4 else 7))
        addView(body(subtitleText, if (compactHeight) 11f else 12f))
        addView(space(if (compactHeight) 4 else 7))
        addView(body(detail, if (compactHeight) 10f else 10.8f).apply {
            maxLines = if (isTruthRaw) 3 else 2
        })
        if (enabled) setOnClickListener { action() }
    }

    private fun infoStrip(): View = vertical().apply {
        setPadding(dp(14), dp(if (compactHeight) 10 else 13), dp(14), dp(if (compactHeight) 10 else 13))
        background = cardBackground(Color.rgb(8, 21, 35), Color.rgb(39, 73, 105), false)
        addView(title("PURE blijft meetbaar", 14f))
        addView(space(4))
        addView(body(
            if (compactHeight) {
                "De UI verandert geen Scientific Master, Zero-Line, scene-scale, Backplane of evidence-authority."
            } else {
                "De UI verandert geen Scientific Master, Zero-Line, scene-scale, Backplane of evidence-authority. " +
                    "TN-3/Open Scene blijft beschikbaar in de wetenschappelijke verwerkingsroute."
            },
            if (compactHeight) 10.8f else 11.5f,
        ))
    }

    private fun setPreferredOutput(mode: String) {
        getSharedPreferences(PREFS, MODE_PRIVATE).edit().putString(KEY_OUTPUT, mode).apply()
        setContentView(buildUi())
    }

    private fun preferredOutput(): String {
        val value = getSharedPreferences(PREFS, MODE_PRIVATE)
            .getString(KEY_OUTPUT, OUTPUT_PURE)
            ?: OUTPUT_PURE
        return if (value == OUTPUT_NEGATIVE) OUTPUT_PURE else value
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
        const val OUTPUT_JPG = "JPG"
        const val OUTPUT_ADVANCED = "ADVANCED"
        const val OUTPUT_NEGATIVE = "NEGATIVE"
    }
}

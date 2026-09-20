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
    private val bg = Color.rgb(5, 12, 22)
    private val surface = Color.rgb(10, 22, 37)
    private val textColor = Color.rgb(244, 248, 255)
    private val muted = Color.rgb(158, 178, 205)
    private val purple = Color.rgb(190, 92, 238)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        window.statusBarColor = bg
        window.navigationBarColor = bg
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
            addView(title("F32 canonical", 14f))
            addView(body(
                "De huidige Scientific Master-identiteit blijft exact IEEE Float32. F64 mag branch-sensitive compute/reference ondersteunen, " +
                    "maar wordt niet als extra evidence gepresenteerd.",
                11.5f,
            ))
            addView(space(8))
            addView(body("F64 canonical storage blijft geblokkeerd totdat een eigen identity/replay-contract is gevalideerd.", 11.5f))
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
        root.addView(action("Advanced scene-instellingen openen") {
            startActivity(Intent(this, TruthRawAdvancedActivity::class.java))
        })

        root.addView(space(10))
        root.addView(action("Gebruik TRUTHRAW PRO") {
            getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
                .edit()
                .putString(
                    TruthRawSuiteLauncherActivity.KEY_OUTPUT,
                    TruthRawSuiteLauncherActivity.OUTPUT_PRO,
                )
                .apply()
            finish()
        })

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
        addView(TextView(this@TruthRawProActivity).apply {
            text = "‹"
            textSize = 36f
            setTextColor(textColor)
            gravity = Gravity.CENTER
            contentDescription = "Terug"
            setOnClickListener { finish() }
        }, LinearLayout.LayoutParams(dp(48), dp(48)).apply { marginEnd = dp(8) })
        addView(vertical().apply {
            addView(title("TRUTHRAW PRO", 24f))
            addView(body("Professionele werkbank · dezelfde scientific core", 11.5f))
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
    }

    private fun card(heading: String): LinearLayout = vertical().apply {
        setPadding(dp(15), dp(15), dp(15), dp(15))
        background = rounded(surface, Color.rgb(70, 51, 92), 18f)
        addView(title(heading, 17f))
        addView(space(7))
    }

    private fun action(label: String, onClick: () -> Unit): View = TextView(this).apply {
        text = label
        textSize = 14.5f
        setTextColor(textColor)
        gravity = Gravity.CENTER
        setPadding(dp(14), dp(14), dp(14), dp(14))
        background = rounded(Color.rgb(32, 22, 43), purple, 16f)
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

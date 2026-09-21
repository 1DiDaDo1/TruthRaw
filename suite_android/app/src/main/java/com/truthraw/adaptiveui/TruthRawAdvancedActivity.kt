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
import android.widget.Button
import android.widget.CheckBox
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.TextView

class TruthRawAdvancedActivity : Activity() {
    private val bg = Color.rgb(5, 12, 22)
    private val surface = Color.rgb(10, 22, 37)
    private val textColor = Color.rgb(244, 248, 255)
    private val muted = Color.rgb(158, 178, 205)
    private val amber = Color.rgb(236, 176, 82)

    private var options = TruthRawAdvancedOptions()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        window.statusBarColor = bg
        window.navigationBarColor = bg
        options = TruthRawAdvancedSettings.load(this)
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = vertical().apply {
            setBackgroundColor(bg)
            setPadding(dp(18), dp(12), dp(18), dp(24))
        }

        root.addView(horizontal().apply {
            gravity = Gravity.CENTER_VERTICAL
            addView(TextView(this@TruthRawAdvancedActivity).apply {
                text = "‹"
                textSize = 36f
                setTextColor(textColor)
                gravity = Gravity.CENTER
                setOnClickListener { finish() }
            }, LinearLayout.LayoutParams(dp(48), dp(48)).apply { marginEnd = dp(8) })
            addView(vertical().apply {
                addView(title("TRUTHRAW ADVANCED", 24f))
                addView(body("Canonical Open Scene · gedeeld met TN-3/TRR/projecties", 11.5f))
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        })

        root.addView(space(18))
        root.addView(card().apply {
            addView(title("Authority-bound scene processing", 18f))
            addView(space(8))
            addView(toggle(
                "Open-World / Scene Physics",
                "Bindt de single-frame scene aan de Open-World illumination-authority corridor. Zonder onafhankelijke geometry/material/illumination blijft een lichtaanpassing APPEARANCE_ONLY; er wordt geen inverse-square of fysieke relight verzonnen.",
                options.naturalLight,
            ) { checked ->
                options = options.copy(naturalLight = checked)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Scientific HDR / Dynamic Authority",
                "Gebruikt scene-aware HDR alleen waar de Dynamic Authority dit toelaat. CENSORED of UNKNOWN support krijgt geen verzonnen recoverable gain en schrijft nooit terug naar PURE.",
                options.naturalHdr,
            ) { checked ->
                options = options.copy(naturalHdr = checked)
                save()
            })
            addView(space(8))
            addView(slider(
                label = "Belichting",
                explanation = "Lineaire presentation-only exposure compensation. Natural Light kan daarnaast maximaal ongeveer +0,7 EV automatische correctie toevoegen als de render-mediaan aantoonbaar te donker is. Scientific Master en RAW-black/white blijven onaangeraakt.",
                minValue = -50,
                maxValue = 50,
                value = options.exposureStep,
                format = { value ->
                    val ev = value * 0.04f
                    when {
                        value == 0 -> "Neutraal · 0,00 EV"
                        value > 0 -> "Lichter · +%.2f EV".format(ev)
                        else -> "Donkerder · %.2f EV".format(ev)
                    }
                },
            ) { value ->
                options = options.copy(exposureStep = value)
                save()
            })
            addView(space(8))
            addView(slider(
                label = "Schaduwen",
                explanation = "Herstelt uitsluitend donkere toongebieden met zwartbescherming en evidence-confidence gating. Dit verandert geen sensor-blacklevel en maakt geen ontbrekende details tot gemeten informatie.",
                minValue = 0,
                maxValue = 3,
                value = options.shadowRecoveryLevel,
                format = { value ->
                    when (value) {
                        0 -> "Neutraal"
                        1 -> "Zacht"
                        2 -> "Medium"
                        else -> "Sterk"
                    }
                },
            ) { value ->
                options = options.copy(shadowRecoveryLevel = value)
                save()
            })
            addView(space(8))
            addView(slider(
                label = "Detail / scherpte",
                explanation = "Doseerbare v4.7j/v4.7k appearance. 0 houdt de neutrale detailroute; hogere waarden mengen meer support-limited structuur en output-acutance in. Dit verandert geen optische detail-authority of Scientific Master.",
                minValue = 0,
                maxValue = 100,
                value = options.detailStrength,
                format = { value ->
                    when {
                        value == 0 -> "Neutraal · 0"
                        value < 35 -> "Subtiel · $value"
                        value < 70 -> "Duidelijk · $value"
                        else -> "Sterk · $value"
                    }
                },
            ) { value ->
                options = options.copy(detailStrength = value)
                save()
            })
            addView(space(8))
            addView(slider(
                label = "Kleurvolheid",
                explanation = "Luminantie-behoudende kleurintensiteit. Positief werkt vibrance-achtig: reeds sterke kleuren krijgen minder extra chroma; negatief maakt het beeld rustiger. Geen semantische huiddetectie en geen wijziging van de brongebonden kleur-authority.",
                minValue = -50,
                maxValue = 50,
                value = options.colorFullness,
                format = { value ->
                    when {
                        value == 0 -> "Neutraal · 0"
                        value > 0 -> "Voller · +$value"
                        else -> "Rustiger · $value"
                    }
                },
            ) { value ->
                options = options.copy(colorFullness = value)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Restoration / Dynamic Authority",
                "Conservation-regel: geldig gemeten support wordt niet overgeschilderd. Alleen expliciet CENSORED verlies kan presentation-compensatie krijgen; onvoldoende steun blijft unresolved. v0.72 behoudt dezelfde canonical Open Scene/role-mask inhoud, maar maakt lange DNG/TIFF/EXR-projecties lifecycle-safe: geen startup-cleanup race, één projectie tegelijk en zichtbare foreground voortgang.",
                options.restoration,
            ) { checked ->
                options = options.copy(restoration = checked)
                save()
            })
        })

        root.addView(space(14))
        root.addView(card().apply {
            addView(title("Scientific separation", 16f))
            addView(space(6))
            addView(body(
                "Advanced consumeert dezelfde Scientific Master en Dynamic Authority als TruthNegative. Full-res Restoration werkt 1:1 op de bronrasterpositie en blijft een aparte derivative. " +
                    "CALIBRATED_ESTIMATE, RECONSTRUCTED, CENSORED en UNKNOWN blijven onderscheiden. " +
                    "Open-World, HDR en Restoration zijn downstream; Scientific Master, Zero-Line, scene-scale en Backplane worden niet teruggeschreven.",
                12f,
            ))
        })

        root.addView(space(18))
        root.addView(Button(this).apply {
            text = "Gebruik TRUTHRAW ADVANCED"
            isAllCaps = false
            textSize = 16f
            setTextColor(textColor)
            background = rounded(Color.rgb(49, 39, 22), amber, 18f)
            setOnClickListener {
                save()
                getSharedPreferences(TruthRawSuiteLauncherActivity.PREFS, MODE_PRIVATE)
                    .edit()
                    .putString(
                        TruthRawSuiteLauncherActivity.KEY_OUTPUT,
                        TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
                    )
                    .apply()
                finish()
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(58)))

        return ScrollView(this).apply {
            isFillViewport = true
            clipToPadding = true
            setBackgroundColor(bg)
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
                insets
            }
            addView(root)
        }
    }

    private fun save() = TruthRawAdvancedSettings.save(this, options)

    private fun toggle(
        label: String,
        explanation: String,
        checked: Boolean,
        onChange: (Boolean) -> Unit,
    ): View = vertical().apply {
        setPadding(dp(12), dp(12), dp(12), dp(12))
        background = rounded(Color.rgb(13, 26, 42), Color.rgb(60, 75, 92), 14f)
        addView(CheckBox(this@TruthRawAdvancedActivity).apply {
            text = label
            textSize = 15f
            setTextColor(textColor)
            isChecked = checked
            setOnCheckedChangeListener { _, value -> onChange(value) }
        })
        addView(body(explanation, 11.5f))
    }

    private fun slider(
        label: String,
        explanation: String,
        minValue: Int,
        maxValue: Int,
        value: Int,
        format: (Int) -> String,
        onChange: (Int) -> Unit,
    ): View = vertical().apply {
        require(maxValue > minValue)
        setPadding(dp(12), dp(12), dp(12), dp(12))
        background = rounded(Color.rgb(13, 26, 42), Color.rgb(60, 75, 92), 14f)

        val valueText = body(format(value.coerceIn(minValue, maxValue)), 11.5f)
        addView(horizontal().apply {
            gravity = Gravity.CENTER_VERTICAL
            addView(title(label, 15f), LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f,
            ))
            addView(valueText)
        })
        addView(space(6))
        addView(SeekBar(this@TruthRawAdvancedActivity).apply {
            max = maxValue - minValue
            progress = value.coerceIn(minValue, maxValue) - minValue
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                    val actual = minValue + progress
                    valueText.text = format(actual)
                    if (fromUser) onChange(actual)
                }
                override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                override fun onStopTrackingTouch(seekBar: SeekBar?) = Unit
            })
        }, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
        ))
        addView(body(explanation, 11.5f))
    }

    private fun card(): LinearLayout = vertical().apply {
        setPadding(dp(15), dp(15), dp(15), dp(15))
        background = rounded(surface, Color.rgb(46, 63, 82), 18f)
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

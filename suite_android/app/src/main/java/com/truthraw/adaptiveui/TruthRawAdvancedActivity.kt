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
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(
                    dp(18) + bars.left,
                    dp(12) + bars.top,
                    dp(18) + bars.right,
                    dp(24) + bars.bottom,
                )
                insets
            }
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
                addView(body("Derivative · Scientific Master blijft onaangeraakt", 11.5f))
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        })

        root.addView(space(18))
        root.addView(card().apply {
            addView(title("Natuurlijke beeldvorming", 18f))
            addView(space(8))
            addView(toggle(
                "Natural Light Balance",
                "Voorzichtige, evidence-confidence begrensde schaduw/lichtbalans. Dit is appearance, geen fysieke relight-meting.",
                options.naturalLight,
            ) { checked ->
                options = options.copy(naturalLight = checked)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Natural HDR",
                "Gebruikt de bestaande scene-aware HDR gain-state downstream van PURE. Censored highlights krijgen geen verzonnen gain.",
                options.naturalHdr,
            ) { checked ->
                options = options.copy(naturalHdr = checked)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Detail / Structure",
                "Activeert de bestaande support-limited detail appearance. Gemeten CFA-samples in PURE worden niet overschreven.",
                options.detail,
            ) { checked ->
                options = options.copy(detail = checked)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Evidence-bound Restoration",
                "Alleen presentation-compensatie op previewpixels waarvan de onderliggende CFA-sample werkelijk clipped/censored is. Onvoldoende buursteun = geen herstel.",
                options.restoration,
            ) { checked ->
                options = options.copy(restoration = checked)
                save()
            })
        })

        root.addView(space(14))
        root.addView(card().apply {
            addView(title("Authority", 16f))
            addView(space(6))
            addView(body(
                "Advanced is een afgeleide weergave. MEASURED blijft MEASURED, reconstructie blijft RECONSTRUCTED en clipping blijft CENSORED in de wetenschappelijke laag. De PURE Scientific Master, Zero-Line, scene-scale en Backplane worden niet teruggeschreven.",
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
                startActivity(Intent(this@TruthRawAdvancedActivity, MainActivity::class.java).apply {
                    putExtra(MainActivity.EXTRA_AUTO_OPEN_RAW_PICKER, true)
                })
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(58)))

        return ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(bg)
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

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
                addView(body("Full Open Scene · Scene Physics · Dynamic Authority", 11.5f))
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
            addView(toggle(
                "Structure / Detail",
                "Activeert support-limited detail appearance downstream van de authority-bound scene. Sample count of contrast wordt niet als extra optische detail-evidence behandeld.",
                options.detail,
            ) { checked ->
                options = options.copy(detail = checked)
                save()
            })
            addView(space(8))
            addView(toggle(
                "Restoration / Dynamic Authority",
                "Conservation-regel: geldig gemeten support wordt niet overgeschilderd. Alleen expliciet CENSORED verlies kan presentation-compensatie krijgen; onvoldoende steun blijft unresolved. v0.69 behoudt de transactionele full-resolution Restoration en kan een geverifieerde .trr daarna als Float32 DNG, Float32 TIFF of OpenEXR projecteren.",
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
            text = "Gebruik OPEN-WORLD ADVANCED"
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

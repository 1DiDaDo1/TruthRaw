package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.content.res.Configuration
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
import android.widget.Space
import android.widget.TextView

class OutputModeActivity : Activity() {
    private data class Palette(
        val background: Int,
        val surface: Int,
        val surfaceSelected: Int,
        val surfaceAlt: Int,
        val text: Int,
        val textMuted: Int,
        val accent: Int,
    )

    private var selectedMode = TruthRawOutputMode.TRUTHRAW_PURE
    private var colourful = false
    private var detailed = false
    private var soft = false
    private var hdr = false

    private val palette: Palette
        get() {
            val dark = resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
                Configuration.UI_MODE_NIGHT_YES
            return if (dark) {
                Palette(
                    background = Color.rgb(10, 12, 16),
                    surface = Color.rgb(24, 27, 33),
                    surfaceSelected = Color.rgb(28, 36, 52),
                    surfaceAlt = Color.rgb(34, 38, 46),
                    text = Color.rgb(246, 248, 251),
                    textMuted = Color.rgb(170, 178, 190),
                    accent = Color.rgb(73, 147, 255),
                )
            } else {
                Palette(
                    background = Color.rgb(244, 246, 249),
                    surface = Color.WHITE,
                    surfaceSelected = Color.rgb(235, 242, 255),
                    surfaceAlt = Color.rgb(232, 236, 242),
                    text = Color.rgb(21, 25, 31),
                    textMuted = Color.rgb(88, 96, 108),
                    accent = Color.rgb(37, 99, 235),
                )
            }
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        render()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        render()
    }

    private fun render() {
        val root = ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(palette.background)
        }
        val content = vertical().apply {
            setPadding(dp(18), dp(12), dp(18), dp(20))
        }
        root.addView(content, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        content.addView(label("TruthRaw", 27f, bold = true))
        content.addView(space(3))
        content.addView(label(getString(R.string.choose_output), 21f, bold = true))
        content.addView(space(5))
        content.addView(label(getString(R.string.choose_output_subtitle), 13f, muted = true))
        content.addView(space(16))

        content.addView(modeRow(TruthRawOutputMode.JPG, TruthRawOutputMode.JPG_XL))
        content.addView(space(10))
        content.addView(modeRow(TruthRawOutputMode.TRUTHRAW_PURE, TruthRawOutputMode.TRUTHRAW_ADVANCED))
        content.addView(space(14))

        content.addView(label(getString(R.string.selected_mode, getString(selectedMode.titleRes)), 13f, bold = true))
        content.addView(space(8))

        if (selectedMode.supportsAppearanceOptions) {
            content.addView(appearanceCard())
        } else {
            content.addView(infoCard(getString(R.string.pure_locked_hint)))
        }

        if (selectedMode == TruthRawOutputMode.JPG_XL && !OutputModePolicy.JPEG_XL_ENCODER_VALIDATED) {
            content.addView(space(8))
            content.addView(infoCard(getString(R.string.jxl_pending)))
        }

        content.addView(space(10))
        content.addView(infoCard(getString(R.string.certificate_hint)))
        content.addView(space(14))
        content.addView(Button(this).apply {
            text = getString(R.string.continue_to_raw)
            isAllCaps = false
            textSize = 16f
            setTypeface(typeface, Typeface.BOLD)
            setTextColor(Color.WHITE)
            background = rounded(palette.accent, 16f)
            setPadding(dp(16), dp(13), dp(16), dp(13))
            setOnClickListener { openMainProcessingUi() }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        root.setOnApplyWindowInsetsListener { view, insets ->
            val bars = insets.getInsets(WindowInsets.Type.systemBars())
            view.setPadding(bars.left, bars.top, bars.right, bars.bottom)
            insets
        }
        setContentView(root)
    }

    private fun modeRow(left: TruthRawOutputMode, right: TruthRawOutputMode): View = horizontal().apply {
        addView(modeCard(left), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
            marginEnd = dp(5)
        })
        addView(modeCard(right), LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
            marginStart = dp(5)
        })
    }

    private fun modeCard(mode: TruthRawOutputMode): View = vertical().apply {
        minimumHeight = dp(112)
        gravity = Gravity.CENTER_VERTICAL
        setPadding(dp(15), dp(14), dp(15), dp(14))
        background = modeBackground(mode == selectedMode)
        addView(label(getString(mode.titleRes), 16f, bold = true))
        addView(space(5))
        addView(label(getString(mode.subtitleRes), 12f, muted = mode != selectedMode))
        setOnClickListener {
            selectedMode = mode
            val sanitized = OutputModePolicy.selection(mode, colourful, detailed, soft, hdr)
            colourful = sanitized.appearance.colourful
            detailed = sanitized.appearance.detailed
            soft = sanitized.appearance.soft
            hdr = sanitized.appearance.hdr
            render()
        }
    }

    private fun appearanceCard(): View = vertical().apply {
        setPadding(dp(15), dp(14), dp(15), dp(14))
        background = rounded(palette.surface, 17f)
        addView(label(getString(R.string.appearance_options), 15f, bold = true))
        addView(space(3))
        addView(label(getString(R.string.appearance_options_hint), 11f, muted = true))
        addView(space(8))
        addView(optionCheckBox(R.string.option_colourful, colourful) { colourful = it })
        addView(optionCheckBox(R.string.option_detailed, detailed) { detailed = it })
        addView(optionCheckBox(R.string.option_soft, soft) { soft = it })
        addView(optionCheckBox(R.string.option_hdr, hdr) { hdr = it })
    }

    private fun optionCheckBox(labelRes: Int, checked: Boolean, onChanged: (Boolean) -> Unit): View =
        CheckBox(this).apply {
            text = getString(labelRes)
            isChecked = checked
            textSize = 14f
            setTextColor(palette.text)
            setOnCheckedChangeListener { _, value -> onChanged(value) }
        }

    private fun infoCard(text: String): View = vertical().apply {
        setPadding(dp(14), dp(12), dp(14), dp(12))
        background = rounded(palette.surfaceAlt, 15f)
        addView(label(text, 11f, muted = true))
    }

    private fun openMainProcessingUi() {
        val selection = OutputModePolicy.selection(selectedMode, colourful, detailed, soft, hdr)
        startActivity(Intent(this, MainActivity::class.java).apply {
            putExtra(EXTRA_OUTPUT_MODE, selection.mode.wireName)
            putExtra(EXTRA_COLOURFUL, selection.appearance.colourful)
            putExtra(EXTRA_DETAILED, selection.appearance.detailed)
            putExtra(EXTRA_SOFT, selection.appearance.soft)
            putExtra(EXTRA_HDR, selection.appearance.hdr)
        })
    }

    private fun modeBackground(selected: Boolean): GradientDrawable = GradientDrawable().apply {
        setColor(if (selected) palette.surfaceSelected else palette.surface)
        cornerRadius = dp(18).toFloat()
        setStroke(dp(if (selected) 2 else 1), if (selected) palette.accent else palette.surfaceAlt)
    }

    private fun rounded(color: Int, radiusDp: Int): GradientDrawable = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(radiusDp).toFloat()
    }

    private fun label(text: String, sizeSp: Float, bold: Boolean = false, muted: Boolean = false): TextView =
        TextView(this).apply {
            this.text = text
            textSize = sizeSp
            setTextColor(if (muted) palette.textMuted else palette.text)
            if (bold) setTypeface(typeface, Typeface.BOLD)
        }

    private fun vertical(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
    private fun horizontal(): LinearLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
    private fun space(heightDp: Int): View = Space(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(heightDp))
    }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density + 0.5f).toInt()

    companion object {
        const val EXTRA_OUTPUT_MODE = "truthraw.output_mode"
        const val EXTRA_COLOURFUL = "truthraw.appearance.colourful"
        const val EXTRA_DETAILED = "truthraw.appearance.detailed"
        const val EXTRA_SOFT = "truthraw.appearance.soft"
        const val EXTRA_HDR = "truthraw.appearance.hdr"
    }
}

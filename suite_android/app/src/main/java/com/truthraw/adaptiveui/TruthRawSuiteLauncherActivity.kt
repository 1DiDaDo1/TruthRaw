package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import io.truthraw.debug.MainActivity as DeviceVerificationActivity

/** Stable front door for the Main + FotoGraaf lineage. */
class TruthRawSuiteLauncherActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
    }

    private fun buildUi(): ScrollView {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.rgb(15, 17, 20))
            setPadding(dp(24), dp(20), dp(24), dp(24))
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
            )
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(dp(24) + bars.left, dp(20) + bars.top, dp(24) + bars.right, dp(24) + bars.bottom)
                insets
            }
        }
        val scroll = ScrollView(this).apply {
            isFillViewport = true
            setBackgroundColor(Color.rgb(15, 17, 20))
            addView(root)
        }

        root.addView(TextView(this).apply {
            text = "TruthRaw Suite"
            textSize = 31f
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
        })
        root.addView(TextView(this).apply {
            text = "v0.6 · preview-first + explicit Camera-5 200MP test"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "Voor 200MP hoef je geen route meer te raden: gebruik de eerste knop. Die test uitsluitend logical 0 → physical 5 → MAX RAW 16320×12288. Preview blijft gewone tele-preview; MAX wordt alleen tijdens de ene RAW-capture geactiveerd."
            textSize = 14f
            setTextColor(Color.rgb(190, 198, 209))
            setPadding(0, 0, 0, dp(18))
        })

        root.addView(actionButton("200MP TELE TEST · physical 5 · 16320×12288") {
            startActivity(Intent(this, FotoGraaf200MpTestActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("FotoGraaf camera & diagnostics") {
            startActivity(Intent(this, FotoGraafPermissionGateActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("TruthRaw processor") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("Device verification v0.3 · source + CFA") {
            startActivity(Intent(this, DeviceVerificationActivity::class.java))
        })
        root.addView(space())
        root.addView(TextView(this).apply {
            text = "200MP PASS vereist een echte 16320×12288 RAW_SENSOR Image, exact timestamp-paar, physical Camera-5 TotalCaptureResult en SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION. Een preview of geadverteerde capability alleen is geen 200MP capturebewijs."
            textSize = 12f
            setTextColor(Color.rgb(145, 153, 165))
        })
        return scroll
    }

    private fun actionButton(label: String, action: () -> Unit): Button = Button(this).apply {
        text = label
        isAllCaps = false
        textSize = 17f
        minHeight = dp(54)
        setOnClickListener { action() }
    }

    private fun space() = android.view.View(this).apply {
        layoutParams = LinearLayout.LayoutParams(1, dp(10))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}

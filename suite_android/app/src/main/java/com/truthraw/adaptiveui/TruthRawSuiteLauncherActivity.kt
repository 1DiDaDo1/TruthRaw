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
            text = "v0.7 · staged Camera-5 200MP test"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "De 200MP-test doet bij openen niets met Camera2. Daarna test je afzonderlijk: (1) capability, (2) live logical-0 preview met 3.7× request en activePhysical-telemetrie, (3) aparte physical-5 RAW 16320×12288 capture."
            textSize = 14f
            setTextColor(Color.rgb(190, 198, 209))
            setPadding(0, 0, 0, dp(18))
        })

        root.addView(actionButton("200MP TELE TEST v0.7 · staged · physical 5") {
            startActivity(Intent(this, FotoGraaf200MpStagedActivity::class.java))
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
            text = "Belangrijk: een zichtbaar live beeld bewijst alleen preview. De 200MP capture-gate vereist nog steeds een echte 16320×12288 RAW_SENSOR Image, physical Camera-5 TotalCaptureResult, exact timestamp-paar en MAXIMUM_RESOLUTION pixel mode."
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

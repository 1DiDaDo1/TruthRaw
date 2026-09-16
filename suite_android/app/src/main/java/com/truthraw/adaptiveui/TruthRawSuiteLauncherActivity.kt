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
            text = "0.1-main-plus-FotoGraaf · integrated build 2026-09-16"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "Één app met de bestaande TruthRaw processor, FotoGraaf Camera2-acquisitie en de op de HONOR bewezen v0.3 source/CFA-verifier. Wetenschappelijke referenties blijven fail-closed: verifier, preview en export schrijven niets terug naar Scientific Master of Dynamic Authority."
            textSize = 14f
            setTextColor(Color.rgb(170, 177, 188))
            setPadding(0, 0, 0, dp(22))
        })

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
            text = "Camera authority: runtime Camera2-resultaat + exact gekoppelde RAW kunnen nieuwe capture-evidence leveren. Discovery, live preview, HONOR metadata, DNG-presentatie en de verifier zelf verhogen geen bestaande wetenschappelijke authority."
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

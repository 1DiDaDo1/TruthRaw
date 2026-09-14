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
import android.widget.TextView

/** Stable front door for the Main + FotoGraaf lineage. */
class TruthRawSuiteLauncherActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        setContentView(buildUi())
    }

    private fun buildUi(): LinearLayout {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setBackgroundColor(Color.rgb(15, 17, 20))
            setPadding(dp(24), dp(20), dp(24), dp(24))
            layoutParams = ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT,
            )
            setOnApplyWindowInsetsListener { view, insets ->
                val bars = insets.getInsets(WindowInsets.Type.systemBars())
                view.setPadding(dp(24) + bars.left, dp(20) + bars.top, dp(24) + bars.right, dp(24) + bars.bottom)
                insets
            }
        }

        root.addView(TextView(this).apply {
            text = "TruthRaw Suite"
            textSize = 31f
            setTextColor(Color.WHITE)
            setTypeface(typeface, Typeface.BOLD)
        })
        root.addView(TextView(this).apply {
            text = "0.1-main-plus-FotoGraaf · camera v0.4 live"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "FotoGraaf gebruikt nu een live Camera2-sessie zodat AF/AE/OIS vóór de single-frame RAW kunnen stabiliseren. De bestaande TruthRaw processor blijft de reconstructie- en exportlaag."
            textSize = 14f
            setTextColor(Color.rgb(170, 177, 188))
            setPadding(0, 0, 0, dp(22))
        })

        root.addView(actionButton("FotoGraaf Live camera openen") {
            startActivity(Intent(this, FotoGraafPermissionGateActivity::class.java))
        })
        root.addView(space())
        root.addView(actionButton("TruthRaw processor openen") {
            startActivity(Intent(this, MainActivity::class.java))
        })
        root.addView(space())
        root.addView(TextView(this).apply {
            text = "Authority boundary: live preview en HONOR metadata veranderen geen evidence. Alleen runtime Camera2-resultaat + sealed RAW kunnen capture-evidence leveren."
            textSize = 12f
            setTextColor(Color.rgb(145, 153, 165))
        })
        return root
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

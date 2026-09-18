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
import java.io.File

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
            layoutParams = ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
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
            text = "Camera-5 research · v0.44 HI-RES main/tele state anchors + v0.43 mode/shutter replicatie + v0.42 output-differentiaal"
            textSize = 16f
            setTextColor(Color.rgb(190, 196, 205))
            setPadding(0, dp(5), 0, dp(8))
        })
        root.addView(TextView(this).apply {
            text = "De vorige staged test crashte vóór Camera2 door een verboden TextureView-background. Dat is nu exact verwijderd. De 200MP-ingang blijft eerst een minimale crash-isolatie Activity; daarna open je bewust de v0.9 staged test."
            textSize = 14f
            setTextColor(Color.rgb(190, 198, 209))
            setPadding(0, 0, 0, dp(18))
        })

        root.addView(actionButton("200MP TEST · crash-isolatie ingang") {
            startActivity(Intent(this, FotoGraaf200MpEntryActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.40 · Passieve Honor route observer") {
            startActivity(Intent(this, HonorPassiveRouteObserverActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.41 · Honor output-config callback probe") {
            startActivity(Intent(this, HonorOutputConfigCallbackProbeActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.42 · Passieve MediaStore + Camera timeline") {
            startActivity(Intent(this, PassiveMediaStoreCameraTimelineActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.43 · Passieve mode/shutter timeline") {
            startActivity(Intent(this, PassiveModeShutterTimelineActivity::class.java))
        })
        root.addView(space())

        root.addView(actionButton("v0.44 · HI-RES main → tele state anchors") {
            startActivity(Intent(this, PassiveHiresTeleStateTimelineActivity::class.java))
        })
        root.addView(space())

        if (File(filesDir, TruthRawSuiteApplication.CRASH_FILE).exists()) {
            root.addView(actionButton("LAATSTE CRASH BEKIJKEN / OPSLAAN") {
                startActivity(Intent(this, TruthRawCrashReportActivity::class.java))
            })
            root.addView(space())
        }

        root.addView(actionButton("FotoGraaf camera & diagnostics · legacy") {
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
            text = "200MP blijft fail-closed: pas een echte 16320×12288 RAW_SENSOR Image + physical Camera-5 result + timestamp identity + MAXIMUM_RESOLUTION pixel mode kan de capture-gate passeren. Legacy previewactivities zijn nog niet als TextureView-crash-fixed gepromoveerd."
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

    private fun space() = android.view.View(this).apply { layoutParams = LinearLayout.LayoutParams(1, dp(10)) }
    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}

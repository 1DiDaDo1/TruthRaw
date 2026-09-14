package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.view.Gravity
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class SuiteLauncherActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val pad = (18 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            setPadding(pad, pad, pad, pad)
        }

        root.addView(TextView(this).apply {
            text = "TruthRaw Suite"
            textSize = 24f
            gravity = Gravity.CENTER_HORIZONTAL
        })

        root.addView(TextView(this).apply {
            text = "Eén APK met de normale TruthRaw-verwerking en de aparte FotoGraaf Camera2-acquisitieruimte. De CAMERA-permissie hoort alleen bij het FotoGraaf-pad; wetenschappelijke autoriteit blijft door de bestaande C0/C1/C2- en CalibrationPack-contracten begrensd."
            textSize = 14f
            setPadding(0, pad / 2, 0, pad)
        })

        root.addView(Button(this).apply {
            text = "Open TruthRaw"
            setOnClickListener {
                startActivity(Intent(this@SuiteLauncherActivity, OutputModeActivity::class.java))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        root.addView(Button(this).apply {
            text = "Open FotoGraaf"
            setOnClickListener {
                startActivity(Intent().setClassName(
                    this@SuiteLauncherActivity,
                    "com.truthraw.fotograafcapture.MainActivity"
                ))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })

        root.addView(TextView(this).apply {
            text = "FotoGraaf registreert capture-time Camera2-evidence en één RAW_SENSOR-frame. Het pad mag geen captureSampleDomainId of gainReadoutStateId uit ISO afleiden en verleent op zichzelf geen CALIBRATED_PHYSICAL-authoriteit."
            textSize = 12f
            setPadding(0, pad, 0, 0)
        })

        setContentView(ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        })
    }
}

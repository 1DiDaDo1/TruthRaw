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
            text = "Eén APK met normale TruthRaw-verwerking en afzonderlijke FotoGraaf Camera2-onderzoekspaden. CAMERA-toegang verleent op zichzelf geen wetenschappelijke of calibratie-authoriteit."
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
            text = "FotoGraaf · scan main / wide / tele / max-res / macro / RAW14"
            setOnClickListener {
                startActivity(Intent().setClassName(
                    this@SuiteLauncherActivity,
                    "com.truthraw.fotograafcapture.HonorCapabilityProbeActivity"
                ))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })

        root.addView(Button(this).apply {
            text = "FotoGraaf · generiek Camera2"
            setOnClickListener {
                startActivity(Intent().setClassName(
                    this@SuiteLauncherActivity,
                    "com.truthraw.fotograafcapture.MainActivity"
                ))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })

        root.addView(Button(this).apply {
            text = "FotoGraaf · HONOR tele fysieke ID 5"
            setOnClickListener {
                startActivity(Intent().setClassName(
                    this@SuiteLauncherActivity,
                    "com.truthraw.fotograafcapture.HonorTeleActivity"
                ))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })

        root.addView(TextView(this).apply {
            text = "De capability probe is read-only: hij inventariseert standaard RAW, MAXIMUM_RESOLUTION RAW, close-focus/macro-controls en RAW14 wanneer het runtime-platform dat formaat kent. Een capability is nog geen capture-proof. Fysieke TotalCaptureResult + timestamp-identiteit + sealed source blijven verplicht."
            textSize = 12f
            setPadding(0, pad, 0, 0)
        })

        setContentView(ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        })
    }
}

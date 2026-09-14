package com.truthraw.fotograafcapture

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class FotoGraafLauncherActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val pad = (16 * resources.displayMetrics.density).toInt()
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(pad, pad, pad, pad)
        }
        root.addView(TextView(this).apply {
            text = "TruthRaw FotoGraaf"
            textSize = 22f
        })
        root.addView(TextView(this).apply {
            text = "Camera2 research rooms. Capability observations and capture observations remain separate from calibration authority."
            textSize = 14f
            setPadding(0, pad / 2, 0, pad)
        })
        root.addView(Button(this).apply {
            text = "Scan HONOR main / wide / tele / max-res / macro / RAW14"
            setOnClickListener {
                startActivity(Intent(this@FotoGraafLauncherActivity, HonorCapabilityProbeActivity::class.java))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        root.addView(Button(this).apply {
            text = "Generic Camera2 RAW observation"
            setOnClickListener {
                startActivity(Intent(this@FotoGraafLauncherActivity, MainActivity::class.java))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })
        root.addView(Button(this).apply {
            text = "HONOR tele · forced physical ID 5"
            setOnClickListener {
                startActivity(Intent(this@FotoGraafLauncherActivity, HonorTeleActivity::class.java))
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = pad / 2
        })
        root.addView(TextView(this).apply {
            text = "No room above may infer captureSampleDomainId or gainReadoutStateId from ISO alone. Native 200MP, macro and RAW14 remain OPEN until the device advertises the route and a physical capture proves it."
            textSize = 12f
            setPadding(0, pad, 0, 0)
        })
        setContentView(ScrollView(this).apply {
            addView(root)
            isFillViewport = true
        })
    }
}

package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView

/**
 * Crash-isolation front door for the 200MP path.
 * This activity deliberately imports/uses no Camera2 API and starts no worker thread.
 */
class FotoGraaf200MpEntryActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(18), dp(18), dp(18), dp(24))
            setBackgroundColor(Color.rgb(11, 13, 16))
        }
        root.addView(TextView(this).apply {
            text = "TruthRaw · 200MP crash-isolatie"
            textSize = 24f
            setTextColor(Color.WHITE)
        })
        root.addView(TextView(this).apply {
            text = "ENTRY PASS betekent alleen dat de Activity/UI stabiel opent. Hier is nog geen Camera2-code geladen of uitgevoerd."
            textSize = 13f
            setTextColor(Color.rgb(190, 198, 210))
            setPadding(0, dp(8), 0, dp(14))
        })
        root.addView(TextView(this).apply {
            text = "v0.15 test de volgende falsifieerbare route: Camera ID 5 direct openen, global MAXIMUM_RESOLUTION zetten, de originele RAW-buffer verzegelen en alle 16 rasterbanden onafhankelijk hashen."
            textSize = 12f
            setTextColor(Color.rgb(190, 198, 210))
            setPadding(0, 0, 0, dp(10))
        })
        root.addView(Button(this).apply {
            text = "Open v0.15 · DIRECT Camera 5 · full-raster test"
            isAllCaps = false
            setOnClickListener { startActivity(Intent(this@FotoGraaf200MpEntryActivity, FotoGraaf200MpFullRasterV015Activity::class.java)) }
        })
        root.addView(Button(this).apply {
            text = "Open historische staged 200MP route"
            isAllCaps = false
            setOnClickListener { startActivity(Intent(this@FotoGraaf200MpEntryActivity, FotoGraaf200MpStagedActivity::class.java)) }
        })
        root.addView(Button(this).apply {
            text = "Bekijk laatste crashrapport"
            isAllCaps = false
            setOnClickListener { startActivity(Intent(this@FotoGraaf200MpEntryActivity, TruthRawCrashReportActivity::class.java)) }
        })
        root.addView(TextView(this).apply {
            text = "Als de directe route faalt: maak een screenshot van de volledige tekst. Als RAW SEALED/FULL-RASTER PASS verschijnt: sla eerst RAW en JSON op."
            textSize = 12f
            setTextColor(Color.rgb(155, 164, 178))
            setPadding(0, dp(14), 0, 0)
        })
        setContentView(root)
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()
}

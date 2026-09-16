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
            text = "v0.9 fix: de bewezen Android-crash door TextureView.setBackgroundColor() is uit de staged 200MP-UI verwijderd."
            textSize = 12f
            setTextColor(Color.rgb(190, 198, 210))
            setPadding(0, 0, 0, dp(10))
        })
        root.addView(Button(this).apply {
            text = "Open staged 200MP test v0.9 · crash-fixed"
            isAllCaps = false
            setOnClickListener { startActivity(Intent(this@FotoGraaf200MpEntryActivity, FotoGraaf200MpStagedActivity::class.java)) }
        })
        root.addView(Button(this).apply {
            text = "Bekijk laatste crashrapport"
            isAllCaps = false
            setOnClickListener { startActivity(Intent(this@FotoGraaf200MpEntryActivity, TruthRawCrashReportActivity::class.java)) }
        })
        root.addView(TextView(this).apply {
            text = "Als een volgende stap toch crasht: open TruthRaw opnieuw → Laatste crashrapport → sla het txt-bestand op en upload het hier."
            textSize = 12f
            setTextColor(Color.rgb(155, 164, 178))
            setPadding(0, dp(14), 0, 0)
        })
        setContentView(root)
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()
}

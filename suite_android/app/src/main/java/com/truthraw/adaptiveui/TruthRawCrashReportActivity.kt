package com.truthraw.adaptiveui

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import java.io.File

class TruthRawCrashReportActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        DrawVisualTheme.applyWindow(this)
        val crash = File(filesDir, TruthRawSuiteApplication.CRASH_FILE)
        val text = if (crash.exists()) crash.readText() else "Geen lokaal crashrapport aanwezig."

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(24))
            setBackgroundColor(DrawVisualTheme.PAPER_YELLOW)
        }
        root.addView(TextView(this).apply {
            this.text = "D.RAW · laatste crash"
            textSize = 23f
            setTextColor(DrawVisualTheme.INK)
        })
        root.addView(Button(this).apply {
            this.text = "Crashrapport opslaan"
            isAllCaps = false
            isEnabled = crash.exists()
            setOnClickListener {
                startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
                    addCategory(Intent.CATEGORY_OPENABLE)
                    type = "text/plain"
                    putExtra(Intent.EXTRA_TITLE, "TruthRaw_last_crash.txt")
                }, REQUEST_SAVE)
            }
        })
        root.addView(TextView(this).apply {
            this.text = text
            textSize = 11f
            setTextColor(Color.rgb(220, 225, 233))
            setTextIsSelectable(true)
        })
        setContentView(ScrollView(this).apply {
            addView(root, ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        })
    }

    @Deprecated("Retained for minSdk31 document export")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_SAVE || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        val crash = File(filesDir, TruthRawSuiteApplication.CRASH_FILE)
        if (!crash.exists()) return
        contentResolver.openOutputStream(uri)?.use { out -> crash.inputStream().use { it.copyTo(out) } }
    }

    private fun dp(v: Int): Int = (v * resources.displayMetrics.density).toInt()

    companion object { private const val REQUEST_SAVE = 5801 }
}

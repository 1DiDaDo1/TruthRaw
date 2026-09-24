package com.truthraw.adaptiveui

import android.app.Activity
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView

/** Shared D.RAW presentation language. Appearance-only: no image science changes. */
object DrawVisualTheme {
    val PAPER_YELLOW: Int = Color.rgb(255, 244, 172)
    val PAPER_YELLOW_SOFT: Int = Color.rgb(255, 248, 199)
    val PAPER_WHITE: Int = Color.rgb(255, 253, 247)
    val PAPER_BLUE: Int = Color.rgb(239, 248, 255)
    val PAPER_MINT: Int = Color.rgb(239, 255, 251)
    val PAPER_ORANGE: Int = Color.rgb(255, 248, 236)
    val PAPER_PURPLE: Int = Color.rgb(252, 244, 255)
    val INK: Int = Color.rgb(5, 20, 43)
    val MUTED: Int = Color.rgb(78, 88, 108)
    val BORDER: Int = Color.rgb(218, 205, 160)
    val BLUE: Int = Color.rgb(38, 153, 232)
    val TEAL: Int = Color.rgb(11, 181, 169)
    val ORANGE: Int = Color.rgb(255, 111, 24)
    val PURPLE: Int = Color.rgb(175, 52, 224)
    val PENCIL_YELLOW: Int = Color.rgb(244, 188, 42)

    fun applyWindow(activity: Activity) {
        activity.window.statusBarColor = PAPER_YELLOW
        activity.window.navigationBarColor = PAPER_WHITE
    }

    fun rounded(activity: Activity, fill: Int, stroke: Int, radiusDp: Float = 18f, strokeDp: Int = 1) =
        GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = (radiusDp * activity.resources.displayMetrics.density + .5f)
            setColor(fill)
            setStroke((strokeDp * activity.resources.displayMetrics.density + .5f).toInt(), stroke)
        }

    fun brandFooter(activity: Activity, iconSizeDp: Int = 88): View =
        LinearLayout(activity).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            addView(ImageView(activity).apply {
                runCatching { setImageResource(R.drawable.draw_icon) }
                contentDescription = "D.RAW"
                adjustViewBounds = true
                scaleType = ImageView.ScaleType.FIT_CENTER
            }, LinearLayout.LayoutParams(dp(activity, iconSizeDp), dp(activity, iconSizeDp)))
            addView(TextView(activity).apply {
                text = "Evidence-bound computational photography and open scene reconstruction"
                textSize = 10.5f
                gravity = Gravity.CENTER
                setTextColor(MUTED)
                setTypeface(typeface, Typeface.NORMAL)
            }, LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
            ).apply { topMargin = dp(activity, 4) })
        }

    private fun dp(activity: Activity, value: Int) =
        (value * activity.resources.displayMetrics.density + .5f).toInt()
}

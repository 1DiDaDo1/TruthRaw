package com.truthraw.adaptiveui

import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.SystemClock
import android.view.View
import android.view.ViewGroup
import android.widget.Chronometer
import android.widget.LinearLayout
import android.widget.TextView

/**
 * Shared visual convention for research/tests.
 *
 * RUNNING/SUCCESS = green, ERROR = red, CANCELLED = muted.
 * RUNNING always shows a live elapsed timer. Terminal states show duration.
 * This mirrors the existing MainActivity operation status principle.
 */
object TruthRawResearchStatusVisualV01 {
    enum class Phase {
        RUNNING,
        SUCCESS,
        ERROR,
        CANCELLED,
    }

    fun build(
        context: Context,
        message: String,
        phase: Phase,
        startedAtWallMs: Long,
        finishedAtWallMs: Long? = null,
    ): View {
        fun dp(value: Int): Int =
            (value * context.resources.displayMetrics.density + 0.5f).toInt()

        fun label(value: String, size: Float): TextView =
            TextView(context).apply {
                text = value
                textSize = size
                setTextColor(DrawVisualTheme.MUTED)
            }

        return LinearLayout(context).apply {
            orientation = LinearLayout.HORIZONTAL

            val dotColor =
                when (phase) {
                    Phase.RUNNING,
                    Phase.SUCCESS -> Color.rgb(65, 196, 106)
                    Phase.ERROR -> Color.rgb(232, 73, 73)
                    Phase.CANCELLED -> DrawVisualTheme.MUTED
                }

            addView(
                View(context).apply {
                    background =
                        GradientDrawable().apply {
                            shape = GradientDrawable.OVAL
                            setColor(dotColor)
                        }
                    contentDescription =
                        when (phase) {
                            Phase.RUNNING -> "Test of analyse loopt normaal"
                            Phase.SUCCESS -> "Test of analyse gereed"
                            Phase.ERROR -> "Test of analyse gestopt met fout"
                            Phase.CANCELLED -> "Test of analyse geannuleerd"
                        }
                },
                LinearLayout.LayoutParams(dp(10), dp(10)).apply {
                    marginEnd = dp(8)
                    topMargin = dp(4)
                },
            )

            addView(
                LinearLayout(context).apply {
                    orientation = LinearLayout.VERTICAL
                    addView(label(message, 10.5f))
                    if (phase == Phase.RUNNING) {
                        addView(
                            Chronometer(context).apply {
                                val elapsedWall =
                                    (System.currentTimeMillis() - startedAtWallMs)
                                        .coerceAtLeast(0L)
                                base = SystemClock.elapsedRealtime() - elapsedWall
                                textSize = 10f
                                setTextColor(DrawVisualTheme.MUTED)
                                format = "Looptijd %s"
                                start()
                            },
                        )
                    } else {
                        val endWall =
                            finishedAtWallMs ?: System.currentTimeMillis()
                        val seconds =
                            ((endWall - startedAtWallMs).coerceAtLeast(0L)) /
                                1000L
                        val prefix =
                            when (phase) {
                                Phase.SUCCESS -> "Gereed in"
                                Phase.ERROR -> "Gestopt na"
                                Phase.CANCELLED -> "Geannuleerd na"
                                Phase.RUNNING -> "Looptijd"
                            }
                        addView(
                            label(
                                "%s %02d:%02d".format(
                                    prefix,
                                    seconds / 60L,
                                    seconds % 60L,
                                ),
                                9.5f,
                            ),
                        )
                    }
                },
                LinearLayout.LayoutParams(
                    0,
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    1f,
                ),
            )
        }
    }
}

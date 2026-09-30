package com.truthraw.adaptiveui

import android.graphics.Color
import android.os.SystemClock
import android.text.Editable
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.TextWatcher
import android.text.style.ForegroundColorSpan
import android.widget.TextView
import java.util.WeakHashMap

/**
 * Adds the shared D.RAW research/test status convention to legacy diagnostic
 * TextViews without changing their evidence/test logic.
 *
 * Existing status.text assignments remain the source of truth.
 */
object TruthRawLegacyTestStatusV01 {
    private enum class Phase {
        IDLE,
        RUNNING,
        SUCCESS,
        ERROR,
    }

    private data class State(
        var rawMessage: String = "",
        var phase: Phase = Phase.IDLE,
        var startedAtElapsedMs: Long? = null,
        var finishedAtElapsedMs: Long? = null,
        var internalUpdate: Boolean = false,
        var tickerScheduled: Boolean = false,
    )

    private val states = WeakHashMap<TextView, State>()

    fun attach(view: TextView) {
        if (states.containsKey(view)) return
        val state = State()
        states[view] = state

        view.addTextChangedListener(
            object : TextWatcher {
                override fun beforeTextChanged(
                    s: CharSequence?,
                    start: Int,
                    count: Int,
                    after: Int,
                ) = Unit

                override fun onTextChanged(
                    s: CharSequence?,
                    start: Int,
                    before: Int,
                    count: Int,
                ) = Unit

                override fun afterTextChanged(s: Editable?) {
                    if (state.internalUpdate) return
                    val raw = s?.toString().orEmpty()
                    state.rawMessage = raw
                    val next = classify(raw)
                    if (
                        next != Phase.IDLE &&
                        state.startedAtElapsedMs == null
                    ) {
                        state.startedAtElapsedMs =
                            SystemClock.elapsedRealtime()
                    }
                    state.phase = next
                    if (
                        next == Phase.SUCCESS ||
                        next == Phase.ERROR
                    ) {
                        state.finishedAtElapsedMs =
                            SystemClock.elapsedRealtime()
                    } else {
                        state.finishedAtElapsedMs = null
                    }
                    render(view, state)
                }
            },
        )

        val initial = view.text?.toString().orEmpty()
        state.rawMessage = initial
        state.phase = classify(initial)
        render(view, state)
    }

    private fun classify(message: String): Phase {
        val u = message.uppercase()
        if (
            u.contains("FAIL") ||
            u.contains("ERROR") ||
            u.contains("FAALDE") ||
            u.contains("FOUT") ||
            u.contains("BLOCKED") ||
            u.contains("GEBLOKKEERD") ||
            u.contains("ONTBREEKT") ||
            u.contains("NIET LEESBAAR") ||
            u.contains("KON NIET")
        ) {
            return Phase.ERROR
        }
        if (
            u.contains("PASS") ||
            u.contains("GEREED") ||
            u.contains("OPGESLAGEN") ||
            u.contains("ROUTE READY") ||
            u.contains("SUCCESS")
        ) {
            return Phase.SUCCESS
        }
        if (
            u.contains("NOG GEEN") ||
            u.contains("VOER EERST") ||
            u.isBlank()
        ) {
            return Phase.IDLE
        }
        return Phase.RUNNING
    }

    private fun render(
        view: TextView,
        state: State,
    ) {
        val now = SystemClock.elapsedRealtime()
        val start = state.startedAtElapsedMs
        val end = state.finishedAtElapsedMs ?: now
        val elapsedSeconds =
            if (start == null) {
                0L
            } else {
                ((end - start).coerceAtLeast(0L)) / 1000L
            }

        val dotColor =
            when (state.phase) {
                Phase.IDLE -> DrawVisualTheme.MUTED
                Phase.RUNNING,
                Phase.SUCCESS -> Color.rgb(65, 196, 106)
                Phase.ERROR -> Color.rgb(232, 73, 73)
            }
        val timer =
            when (state.phase) {
                Phase.IDLE -> "Wacht"
                Phase.RUNNING -> "Looptijd"
                Phase.SUCCESS -> "Gereed in"
                Phase.ERROR -> "Gestopt na"
            }
        val prefix =
            if (state.phase == Phase.IDLE) {
                "● $timer · "
            } else {
                "● $timer %02d:%02d · ".format(
                    elapsedSeconds / 60L,
                    elapsedSeconds % 60L,
                )
            }
        val display =
            SpannableStringBuilder(prefix + state.rawMessage)
        display.setSpan(
            ForegroundColorSpan(dotColor),
            0,
            1,
            Spanned.SPAN_EXCLUSIVE_EXCLUSIVE,
        )

        state.internalUpdate = true
        view.text = display
        state.internalUpdate = false

        if (state.phase == Phase.RUNNING && !state.tickerScheduled) {
            state.tickerScheduled = true
            view.postDelayed(
                object : Runnable {
                    override fun run() {
                        val current = states[view]
                        if (
                            current == null ||
                            current.phase != Phase.RUNNING
                        ) {
                            current?.tickerScheduled = false
                            return
                        }
                        render(view, current)
                        view.postDelayed(this, 1000L)
                    }
                },
                1000L,
            )
        }
    }
}

package com.truthraw.adaptiveui

import android.os.SystemClock

enum class TruthRawOperationPhase {
    RUNNING,
    SUCCESS,
    ERROR,
    CANCELLED,
}

data class TruthRawOperationState(
    val key: String,
    val message: String,
    val phase: TruthRawOperationPhase,
    val startedAtElapsedMs: Long,
    val finishedAtElapsedMs: Long? = null,
) {
    fun elapsedMs(nowElapsedMs: Long = SystemClock.elapsedRealtime()): Long =
        ((finishedAtElapsedMs ?: nowElapsedMs) - startedAtElapsedMs).coerceAtLeast(0L)
}

class TruthRawOperationTracker {
    private val states = linkedMapOf<String, TruthRawOperationState>()

    fun start(key: String, message: String): TruthRawOperationState {
        val state = TruthRawOperationState(
            key = key,
            message = message,
            phase = TruthRawOperationPhase.RUNNING,
            startedAtElapsedMs = SystemClock.elapsedRealtime(),
        )
        states[key] = state
        return state
    }

    fun restoreRunning(
        key: String,
        message: String,
        startedAtElapsedMs: Long,
    ): TruthRawOperationState {
        val existing = states[key]
        if (existing != null && existing.phase == TruthRawOperationPhase.RUNNING) {
            val updated = existing.copy(message = message)
            states[key] = updated
            return updated
        }
        val state = TruthRawOperationState(
            key = key,
            message = message,
            phase = TruthRawOperationPhase.RUNNING,
            startedAtElapsedMs = startedAtElapsedMs.coerceAtLeast(0L),
        )
        states[key] = state
        return state
    }

    fun success(key: String, message: String): TruthRawOperationState =
        finish(key, message, TruthRawOperationPhase.SUCCESS)

    fun error(key: String, message: String): TruthRawOperationState =
        finish(key, message, TruthRawOperationPhase.ERROR)

    fun cancel(key: String, message: String): TruthRawOperationState =
        finish(key, message, TruthRawOperationPhase.CANCELLED)

    fun get(key: String): TruthRawOperationState? = states[key]

    fun clear(key: String) {
        states.remove(key)
    }

    fun clearAll() {
        states.clear()
    }

    private fun finish(
        key: String,
        message: String,
        phase: TruthRawOperationPhase,
    ): TruthRawOperationState {
        val now = SystemClock.elapsedRealtime()
        val started = states[key]?.startedAtElapsedMs ?: now
        val state = TruthRawOperationState(
            key = key,
            message = message,
            phase = phase,
            startedAtElapsedMs = started,
            finishedAtElapsedMs = now,
        )
        states[key] = state
        return state
    }
}

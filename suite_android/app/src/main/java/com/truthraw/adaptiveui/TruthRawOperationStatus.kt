package com.truthraw.adaptiveui

import android.content.Context
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

    fun restore(
        key: String,
        message: String,
        phase: TruthRawOperationPhase,
        startedAtElapsedMs: Long,
        finishedAtElapsedMs: Long? = null,
    ): TruthRawOperationState {
        val state = TruthRawOperationState(
            key = key,
            message = message,
            phase = phase,
            startedAtElapsedMs = startedAtElapsedMs.coerceAtLeast(0L),
            finishedAtElapsedMs = finishedAtElapsedMs,
        )
        states[key] = state
        return state
    }

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


data class TruthRawPersistedOperation(
    val key: String,
    val label: String,
    val message: String,
    val phase: TruthRawOperationPhase,
    val startedAtWallMs: Long,
    val updatedAtWallMs: Long,
    val finishedAtWallMs: Long?,
) {
    val terminal: Boolean
        get() = phase != TruthRawOperationPhase.RUNNING
}

object TruthRawOperationStore {
    private const val PREFS = "truthraw_background_operations_v0_1"

    fun begin(context: Context, key: String, label: String, message: String = label) {
        val now = System.currentTimeMillis()
        context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
            .putString("label:$key", label)
            .putString("message:$key", message)
            .putString("phase:$key", TruthRawOperationPhase.RUNNING.name)
            .putLong("started:$key", now)
            .putLong("updated:$key", now)
            .remove("finished:$key")
            .apply()
    }

    fun update(
        context: Context,
        key: String,
        phase: TruthRawOperationPhase,
        message: String,
    ) {
        val app = context.applicationContext
        val prefs = app.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val now = System.currentTimeMillis()
        val editor = prefs.edit()
            .putString("message:$key", message)
            .putString("phase:$key", phase.name)
            .putLong("updated:$key", now)
        if (phase == TruthRawOperationPhase.RUNNING) {
            if (!prefs.contains("started:$key")) editor.putLong("started:$key", now)
            editor.remove("finished:$key")
        } else {
            editor.putLong("finished:$key", now)
        }
        editor.apply()
    }

    fun recoverInterruptedIfNeeded(
        context: Context,
        key: String,
        serviceRunning: Boolean,
        startupGraceMs: Long = 30_000L,
    ): TruthRawPersistedOperation? {
        val snapshot = read(context, key) ?: return null
        if (snapshot.terminal || serviceRunning) return snapshot
        val age = System.currentTimeMillis() - snapshot.updatedAtWallMs
        if (age in 0 until startupGraceMs) return snapshot
        update(
            context,
            key,
            TruthRawOperationPhase.ERROR,
            "Vorige verwerking werd onverwacht onderbroken; geen actieve Android media-processing service meer gevonden.",
        )
        return read(context, key)
    }

    fun read(context: Context, key: String): TruthRawPersistedOperation? {
        val p = context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        val phase = runCatching {
            TruthRawOperationPhase.valueOf(p.getString("phase:$key", "") ?: "")
        }.getOrNull() ?: return null
        val started = p.getLong("started:$key", 0L)
        if (started <= 0L) return null
        return TruthRawPersistedOperation(
            key = key,
            label = p.getString("label:$key", key) ?: key,
            message = p.getString("message:$key", "") ?: "",
            phase = phase,
            startedAtWallMs = started,
            updatedAtWallMs = p.getLong("updated:$key", started),
            finishedAtWallMs =
                if (p.contains("finished:$key")) p.getLong("finished:$key", 0L) else null,
        )
    }
}

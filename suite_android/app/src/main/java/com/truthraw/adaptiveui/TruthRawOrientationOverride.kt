package com.truthraw.adaptiveui

import android.content.Context

/**
 * Presentation-only orientation correction.
 *
 * This never rewrites Source Evidence, CFA coordinates, Scientific Master pixels,
 * Zero-Line, Dynamic Authority or Open Scene coordinates. It is an explicit
 * downstream coordinate transform chosen by the user for display/export.
 */
object TruthRawOrientationOverride {
    private const val PREFS = "truthraw_orientation_override_v0_1"
    private const val PREFIX = "quarter_turns:"

    fun quarterTurns(context: Context, source: RawHandle): Int =
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .getInt(key(source), 0)
            .floorMod4()

    fun rotateClockwise(context: Context, source: RawHandle): Int {
        val next = (quarterTurns(context, source) + 1).floorMod4()
        set(context, source, next)
        return next
    }

    fun rotateCounterClockwise(context: Context, source: RawHandle): Int {
        val next = (quarterTurns(context, source) + 3).floorMod4()
        set(context, source, next)
        return next
    }

    fun reset(context: Context, source: RawHandle) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit()
            .remove(key(source))
            .apply()
    }

    fun degreesClockwise(context: Context, source: RawHandle): Int =
        quarterTurns(context, source) * 90

    private fun set(context: Context, source: RawHandle, turns: Int) {
        val normalized = turns.floorMod4()
        val editor = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit()
        if (normalized == 0) editor.remove(key(source)) else editor.putInt(key(source), normalized)
        editor.apply()
    }

    private fun key(source: RawHandle): String =
        PREFIX + source.uri.toString() + "|" + source.displayName + "|" + (source.declaredSizeBytes ?: -1L)

    private fun Int.floorMod4(): Int = ((this % 4) + 4) % 4
}

package com.truthraw.adaptiveui

import android.app.ActivityManager
import android.app.ApplicationExitInfo
import android.content.Context
import org.json.JSONObject

/**
 * Android process-exit forensics used only to diagnose lifecycle/memory kills.
 * It does not affect scientific processing or authority.
 */
object AndroidExitForensicsV01 {
    const val SCHEMA = "D.RAW/AndroidExitForensics/0.1"

    fun latest(context: Context): JSONObject? {
        val manager =
            context.getSystemService(ActivityManager::class.java)
                ?: return null
        val info =
            runCatching {
                manager.getHistoricalProcessExitReasons(
                    context.packageName,
                    0,
                    5,
                ).firstOrNull()
            }.getOrNull()
                ?: return null

        val description = info.description ?: ""
        val memoryLimiterAnonSwap =
            info.reason == REASON_MEMORY_LIMITER_API_37_2 ||
                (
                    info.reason == ApplicationExitInfo.REASON_OTHER &&
                        description.contains(
                            "MemoryLimiter:AnonSwap",
                            ignoreCase = true,
                        )
                    )

        return JSONObject()
            .put("schema", SCHEMA)
            .put("timestamp_wall_ms", info.timestamp)
            .put("process_name", info.processName ?: JSONObject.NULL)
            .put("reason_code", info.reason)
            .put("reason_name", reasonName(info.reason))
            .put("description", description)
            .put("status", info.status)
            .put("importance", info.importance)
            .put("pss_kb", info.pss)
            .put("rss_kb", info.rss)
            .put("android17_memory_limiter_anon_swap", memoryLimiterAnonSwap)
            .put("changes_scientific_authority", false)
    }

    private fun reasonName(reason: Int): String =
        when (reason) {
            ApplicationExitInfo.REASON_UNKNOWN -> "UNKNOWN"
            ApplicationExitInfo.REASON_EXIT_SELF -> "EXIT_SELF"
            ApplicationExitInfo.REASON_SIGNALED -> "SIGNALED"
            ApplicationExitInfo.REASON_LOW_MEMORY -> "LOW_MEMORY"
            ApplicationExitInfo.REASON_CRASH -> "CRASH"
            ApplicationExitInfo.REASON_CRASH_NATIVE -> "CRASH_NATIVE"
            ApplicationExitInfo.REASON_ANR -> "ANR"
            ApplicationExitInfo.REASON_INITIALIZATION_FAILURE ->
                "INITIALIZATION_FAILURE"
            ApplicationExitInfo.REASON_PERMISSION_CHANGE -> "PERMISSION_CHANGE"
            ApplicationExitInfo.REASON_EXCESSIVE_RESOURCE_USAGE ->
                "EXCESSIVE_RESOURCE_USAGE"
            ApplicationExitInfo.REASON_USER_REQUESTED -> "USER_REQUESTED"
            ApplicationExitInfo.REASON_USER_STOPPED -> "USER_STOPPED"
            ApplicationExitInfo.REASON_DEPENDENCY_DIED -> "DEPENDENCY_DIED"
            ApplicationExitInfo.REASON_OTHER -> "OTHER"
            ApplicationExitInfo.REASON_FREEZER -> "FREEZER"
            ApplicationExitInfo.REASON_PACKAGE_STATE_CHANGE ->
                "PACKAGE_STATE_CHANGE"
            ApplicationExitInfo.REASON_PACKAGE_UPDATED -> "PACKAGE_UPDATED"
            REASON_MEMORY_LIMITER_API_37_2 -> "MEMORY_LIMITER"
            else -> "REASON_$reason"
        }

    // Added by the Android 17 API surface as reason code 17. Keeping the
    // numeric value here allows the diagnostics source to compile even when a
    // build machine exposes API 37 without the later 37.2 symbol stub.
    private const val REASON_MEMORY_LIMITER_API_37_2 = 17
}

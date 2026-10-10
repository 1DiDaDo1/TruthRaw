package com.truthraw.adaptiveui

/**
 * Downstream presentation/output selector only.
 *
 * This value must never be interpreted as scientific authority, sensor headroom,
 * recovered measurement, or permission to write back into Scientific Master.
 */
object PresentationHeadroomModeV01 {
    const val OFF = 0
    const val PURE_MAP_90_TO_100 = 1

    fun forPhotoOutputRoute(route: String): Int? = when (route) {
        TruthRawSuiteLauncherActivity.OUTPUT_PURE -> PURE_MAP_90_TO_100
        TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
        TruthRawSuiteLauncherActivity.OUTPUT_PRO -> OFF
        else -> null
    }

    fun isKnown(mode: Int): Boolean =
        mode == OFF || mode == PURE_MAP_90_TO_100
}

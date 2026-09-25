package com.truthraw.adaptiveui

data class DrawRouteUi(
    val displayLabel: String,
    val viewClass: String,
    val settingsActionLabel: String,
    val fileInputSubtitle: String,
    val cameraInputSubtitle: String,
    val explanation: String,
)

object DrawRouteLogic {
    fun forMode(mode: String): DrawRouteUi =
        when (mode) {
            TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED -> DrawRouteUi(
                displayLabel = "D.RAW ADVANCED · Appearance / Restoration View",
                viewClass = "APPEARANCE_VIEW + RESTORATION_VIEW",
                settingsActionLabel = "Open Appearance / Restoration",
                fileInputSubtitle = "Open RAW / DNG · Appearance View",
                cameraInputSubtitle = "1-frame RAW · Appearance View",
                explanation =
                    "Scientific/Open Scene blijft upstream. ADVANCED verandert lichtheid, kleurvolheid, HDR, detail en restauratie uitsluitend downstream en schrijft nooit terug naar Scientific Master.",
            )

            TruthRawSuiteLauncherActivity.OUTPUT_PRO -> DrawRouteUi(
                displayLabel = "D.RAW PRO · Open Scene / Light Transport",
                viewClass = "OPEN_SCENE_VIEW + LIGHT_TRANSPORT",
                settingsActionLabel = "Open Open Scene / Light Transport",
                fileInputSubtitle = "Open RAW / DNG · Open Scene",
                cameraInputSubtitle = "1-frame RAW · Open Scene",
                explanation =
                    "PRO legt Continuous Field, Deep Scene, geometry/radiometry authority, light transport, provenance en display resolve bloot. Representatie mag rijker worden; evidence-authority niet.",
            )

            else -> DrawRouteUi(
                displayLabel = "D.RAW PURE · Scientific View",
                viewClass = "SCIENTIFIC_VIEW",
                settingsActionLabel = "Scientific View actief",
                fileInputSubtitle = "Open RAW / DNG · Scientific View",
                cameraInputSubtitle = "1-frame RAW · Scientific View",
                explanation =
                    "PURE blijft evidence-constrained: sealed source → Scientific Master → Scientific View. Geen inferred scene, restoration hypothesis, counterfactual of appearance schrijft terug.",
            )
        }
}

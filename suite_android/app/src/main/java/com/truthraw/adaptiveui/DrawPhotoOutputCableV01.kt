package com.truthraw.adaptiveui

/**
 * Stable outer binding for presentation/photo outputs that are rendered from the
 * admitted D.RAW source/scientific cable rather than from a display-preview bitmap.
 *
 * This contract deliberately knows nothing about TilePreviewUiState or
 * UnifiedOutputPreviewResult. Preview is a sibling output adapter, never the
 * authority or pixel source for a full-resolution JPEG export.
 */
data class DrawPhotoOutputBindingV01(
    val sourceJobId: String,
    val sourceUri: String,
    val sourceDisplayName: String,
    val route: String,
    val routeFlags: Int,
    val userQuarterTurns: Int,
    val adapterId: String,
    val sourceAuthority: String,
    val outputAuthority: String,
    val createsNewEvidence: Boolean,
    val scientificWritebackAllowed: Boolean,
    val sourceMutationAllowed: Boolean,
    val previewRequired: Boolean,
)

sealed interface DrawPhotoOutputBindResultV01 {
    data class Ready(
        val binding: DrawPhotoOutputBindingV01,
    ) : DrawPhotoOutputBindResultV01

    data class Failed(
        val reason: String,
    ) : DrawPhotoOutputBindResultV01
}

object DrawPhotoOutputCableV01 {
    const val CONTRACT_VERSION = "DrawPhotoOutputCable/0.1"
    const val JPEG_FULL_RES_ADAPTER = "jpeg.full_resolution.native_dng.v1"

    const val SOURCE_AUTHORITY = "EXISTING_ADMITTED_DNG_OBSERVATION"
    const val OUTPUT_AUTHORITY = "DERIVED_PRESENTATION_OUTPUT"

    const val CREATES_NEW_EVIDENCE = false
    const val SCIENTIFIC_WRITEBACK_ALLOWED = false
    const val SOURCE_MUTATION_ALLOWED = false
    const val PREVIEW_REQUIRED = false

    fun bindFullResolutionJpeg(
        job: RawJob,
        activeJobId: String?,
        route: String,
        routeFlags: Int,
        userQuarterTurns: Int,
    ): DrawPhotoOutputBindResultV01 {
        if (activeJobId == null || activeJobId != job.id) {
            return DrawPhotoOutputBindResultV01.Failed(
                "JPG-output geblokkeerd: de actieve sealed observation veranderde.",
            )
        }
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return DrawPhotoOutputBindResultV01.Failed(
                "JPG-output vereist de actieve admitted DNG-hoofdkabel.",
            )
        }
        if (userQuarterTurns !in 0..3) {
            return DrawPhotoOutputBindResultV01.Failed(
                "JPG-output geblokkeerd: ongeldige downstream oriëntatie.",
            )
        }
        if (route !in setOf(
                TruthRawSuiteLauncherActivity.OUTPUT_PURE,
                TruthRawSuiteLauncherActivity.OUTPUT_ADVANCED,
                TruthRawSuiteLauncherActivity.OUTPUT_PRO,
            )
        ) {
            return DrawPhotoOutputBindResultV01.Failed(
                "JPG-output geblokkeerd: onbekende outputroute.",
            )
        }
        if (route == TruthRawSuiteLauncherActivity.OUTPUT_PURE && routeFlags != 0) {
            return DrawPhotoOutputBindResultV01.Failed(
                "PURE JPG-output mag geen ADVANCED/PRO appearance-flags dragen.",
            )
        }

        return DrawPhotoOutputBindResultV01.Ready(
            DrawPhotoOutputBindingV01(
                sourceJobId = job.id,
                sourceUri = job.source.uri.toString(),
                sourceDisplayName = job.source.displayName,
                route = route,
                routeFlags = routeFlags,
                userQuarterTurns = userQuarterTurns,
                adapterId = JPEG_FULL_RES_ADAPTER,
                sourceAuthority = SOURCE_AUTHORITY,
                outputAuthority = OUTPUT_AUTHORITY,
                createsNewEvidence = CREATES_NEW_EVIDENCE,
                scientificWritebackAllowed = SCIENTIFIC_WRITEBACK_ALLOWED,
                sourceMutationAllowed = SOURCE_MUTATION_ALLOWED,
                previewRequired = PREVIEW_REQUIRED,
            ),
        )
    }

    /**
     * Revalidate after Android's document picker returns. The exact source/job
     * selected when the output request was created must still be active.
     */
    fun validateCurrentSource(
        binding: DrawPhotoOutputBindingV01,
        job: RawJob?,
        activeJobId: String?,
    ): String? {
        if (job == null || activeJobId == null) {
            return "JPG-output geblokkeerd: actieve bron ontbreekt."
        }
        if (job.id != binding.sourceJobId || activeJobId != binding.sourceJobId) {
            return "JPG-output geblokkeerd: actieve bron/job veranderde."
        }
        if (job.source.uri.toString() != binding.sourceUri) {
            return "JPG-output geblokkeerd: source-URI veranderde."
        }
        if (!job.source.format.nativeProcessingReady || job.source.format.id != "DNG") {
            return "JPG-output geblokkeerd: admitted DNG-route is niet meer geldig."
        }
        if (binding.createsNewEvidence ||
            binding.scientificWritebackAllowed ||
            binding.sourceMutationAllowed ||
            binding.previewRequired
        ) {
            return "JPG-output geblokkeerd: output-cable safety-contract mismatch."
        }
        return null
    }

    /**
     * Freeze the complete downstream presentation request across Android's
     * document-picker round-trip. A route, appearance-setting or orientation
     * change invalidates the request instead of silently retargeting the output.
     *
     * This is presentation freshness only. It never creates scientific authority
     * and it never turns the UI preview into an output dependency.
     */
    fun validateCurrentOutputContext(
        binding: DrawPhotoOutputBindingV01,
        job: RawJob?,
        activeJobId: String?,
        currentRoute: String,
        currentRouteFlags: Int,
        currentQuarterTurns: Int,
    ): String? {
        validateCurrentSource(
            binding = binding,
            job = job,
            activeJobId = activeJobId,
        )?.let { return it }

        if (currentRoute != binding.route) {
            return "JPG-output geblokkeerd: uitvoerroute veranderde tijdens de bestandsdialoog."
        }
        if (currentRouteFlags != binding.routeFlags) {
            return "JPG-output geblokkeerd: appearance-instellingen veranderden tijdens de bestandsdialoog."
        }
        if (currentQuarterTurns !in 0..3 ||
            currentQuarterTurns != binding.userQuarterTurns
        ) {
            return "JPG-output geblokkeerd: downstream oriëntatie veranderde tijdens de bestandsdialoog."
        }
        return null
    }
}
